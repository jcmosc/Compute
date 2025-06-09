#include "Graph.h"

#include <CoreFoundation/CFString.h>
#include <ranges>

#include "Attribute/AttributeData/Node/IndirectNode.h"
#include "Attribute/AttributeData/Node/Node.h"
#include "Attribute/AttributeID/OffsetAttributeID.h"
#include "Attribute/AttributeType/AttributeType.h"
#include "KeyTable.h"
#include "Log/Log.h"
#include "Subgraph/Subgraph.h"
#include "Trace/AGTrace.h"
#include "TraceRecorder.h"
#include "UniqueID/AGUniqueID.h"

namespace AG {

Graph *Graph::_all_graphs = nullptr;
os_unfair_lock Graph::_all_graphs_lock = OS_UNFAIR_LOCK_INIT;

Graph::Graph()
    : _heap(nullptr, 0, 0), _interned_types(nullptr, nullptr, nullptr, nullptr, &_heap),
      _contexts_by_id(nullptr, nullptr, nullptr, nullptr, &_heap), _id(AGMakeUniqueID()) {
    _types.push_back(nullptr);

    static dispatch_once_t make_keys;
    dispatch_once_f(&make_keys, nullptr, [](void *context) { Subgraph::make_current_subgraph_key(); });

    // Prepend this graph
    all_lock();
    _next = _all_graphs;
    _previous = nullptr;
    _all_graphs = this;
    if (_next) {
        _next->_previous = this;
    }
    all_unlock();
}

Graph::~Graph() {
    foreach_trace([this](Trace &trace) {
        trace.end_trace(*this);
        trace.graph_destroyed();
    });

    all_lock();
    if (_previous) {
        _previous->_next = _next;
        if (_next) {
            _next->_previous = _previous;
        }
    } else {
        _all_graphs = _next;
        if (_next) {
            _next->_previous = nullptr;
        }
    }
    all_unlock();

    for (auto subgraph : _subgraphs) {
        subgraph->graph_destroyed();
    }

    if (_keys) {
        delete _keys;
    }
}

#pragma mark - Context

Graph::Context *Graph::primary_context() const {
    struct Info {
        Context *context;
        uint64_t context_id;
    };
    Info info = {nullptr, UINT64_MAX};
    _contexts_by_id.for_each(
        [](const uint64_t context_id, Context *const context, void *info_ref) {
            auto typed_info_ref = (std::pair<Context *, uint64_t> *)info_ref;
            if (context_id < ((Info *)info_ref)->context_id) {
                ((Info *)info_ref)->context = context;
                ((Info *)info_ref)->context_id = context_id;
            }
        },
        &info);
    return info.context;
}

#pragma mark - Subgraphs

void Graph::add_subgraph(Subgraph &subgraph) {
    auto pos = std::lower_bound(_subgraphs.begin(), _subgraphs.end(), &subgraph);
    _subgraphs.insert(pos, &subgraph);

    _num_subgraphs += 1;
    _num_subgraphs_total += 1;
}

void Graph::remove_subgraph(Subgraph &subgraph) {
    auto iter = std::remove(_subgraphs.begin(), _subgraphs.end(), &subgraph);
    _subgraphs.erase(iter);

    _num_subgraphs -= 1;
}

Graph::without_invalidating::without_invalidating(Graph *graph) {
    _graph = graph;
    _was_deferring = graph->begin_deferring_subgraph_invalidation();
}

Graph::without_invalidating::~without_invalidating() {
    if (_graph) {
        _graph->end_deferring_subgraph_invalidation(_was_deferring);
    }
}

void Graph::invalidate_subgraphs() {
    if (is_deferring_subgraph_invalidation()) {
        return;
    }

    if (_main_handler == nullptr) {
        while (!_invalidating_subgraphs.empty()) {
            auto subgraph = _invalidating_subgraphs.back();
            _invalidating_subgraphs.pop_back();

            subgraph->invalidate_now(*this);
        }
    }
}

#pragma mark - Attribute type

const AttributeType &Graph::attribute_ref(data::ptr<Node> attribute, const void *_Nullable *_Nullable ref_out) const {
    auto &type = attribute_type(attribute->type_id());
    if (ref_out) {
        *ref_out = attribute->get_self(type);
    }
    return type;
}

uint32_t Graph::intern_type(const swift::metadata *metadata, ClosureFunctionVP<const AGAttributeType *> make_type) {
    uint32_t type_id = uint32_t(reinterpret_cast<uintptr_t>(_interned_types.lookup(metadata, nullptr)));
    if (type_id) {
        return type_id;
    }

    AttributeType *type = (AttributeType *)make_type();
    type->init_body_offset();

    static bool prefetch_layouts = []() -> bool {
        char *result = getenv("AG_PREFETCH_LAYOUTS");
        if (result) {
            return atoi(result) != 0;
        }
        return false;
    }();
    if (prefetch_layouts) {
        type->prefetch_layout();
    }

    type_id = _types.size();
    if (type_id >= 0xffffff) {
        precondition_failure("overflowed max type id: %u", type_id);
    }
    _types.push_back(std::unique_ptr<AttributeType, AttributeType::deleter>(type));
    _interned_types.insert(metadata, reinterpret_cast<void *>(uintptr_t(type_id)));

    size_t self_size = type->body_metadata().vw_size();
    if (self_size >= 0x2000) {
        os_log_info(misc_log(), "large attribute self: %u bytes, %s", uint(self_size),
                    type->body_metadata().name(false));
    }

    size_t value_size = type->value_metadata().vw_size();
    if (value_size >= 0x2000) {
        os_log_info(misc_log(), "large attribute value: %u bytes, %s -> %s", uint(value_size),
                    type->body_metadata().name(false), type->value_metadata().name(false));
    }

    return type_id;
}

#pragma mark - Attributes

void Graph::did_allocate_node_value(size_t size) {
    // TODO: Not implemented
}

void Graph::did_destroy_node_value(size_t size) {
    // TODO: Not implemented
}

data::ptr<Node> Graph::add_attribute(Subgraph &subgraph, uint32_t type_id, const void *body, const void *value) {
    const AttributeType &type = attribute_type(type_id);

    const void *initial_value = nullptr;
    if (value == nullptr && type.value_metadata().vw_size() == 0) {
        if (type.flags() & AGAttributeTypeFlagsExternal) {
            initial_value = this;
        }
    } else {
        initial_value = value;
    }

    // Allocate Node + body

    void *indirect_body = nullptr;
    size_t body_size = type.body_metadata().vw_size();
    size_t alignment_mask = type.body_metadata().getValueWitnesses()->getAlignmentMask();
    if (!type.body_metadata().getValueWitnesses()->isBitwiseTakable() || body_size > 0x80) {
        indirect_body = subgraph.alloc_persistent(body_size);

        body_size = sizeof(void *);
        alignment_mask = 7;
    }

    size_t total_size = ((sizeof(Node) + alignment_mask) & ~alignment_mask) + body_size;

    data::ptr<Node> node_ptr;
    if (total_size <= 0x10) {
        node_ptr = subgraph.alloc_bytes_recycle(uint32_t(total_size), uint32_t(alignment_mask | 3)).unsafe_cast<Node>();
    } else {
        node_ptr = (data::ptr<Node>)subgraph.alloc_bytes(uint32_t(total_size), uint32_t(alignment_mask | 3))
                       .unsafe_cast<Node>();
    }

    bool main_thread = type.flags() & AGAttributeTypeFlagsMainThread;
    *node_ptr = Node(type_id, main_thread);
    node_ptr->set_main_ref(true);

    if (type_id >= 0x100000) {
        precondition_failure("too many node types allocated");
    }

    void *self = (uint8_t *)node_ptr.get() + type.body_offset();
    node_ptr->set_self_initialized(true);

    if (type.value_metadata().getValueWitnesses()->isPOD() || type.flags() & AGAttributeTypeFlagsThreadSafe) {
        node_ptr->set_main_ref(false);
    } else {
        if (node_ptr->is_main_thread_only()) {
            node_ptr->set_main_ref(true);
        } else {
            node_ptr->set_main_ref(false);
        }
    }

    if (indirect_body != nullptr) {
        node_ptr->set_has_indirect_self(true);
        *(void **)self = indirect_body;
    }
    if (!type.value_metadata().getValueWitnesses()->isBitwiseTakable() || type.value_metadata().vw_size() > 0x80) {
        node_ptr->set_has_indirect_value(true);
    }

    _num_nodes += 1;
    _num_nodes_total += 1;

    // Initialize body
    if (type.body_metadata().vw_size() != 0) {
        void *self_dest = self;
        if (node_ptr->has_indirect_self()) {
            self_dest = *(void **)self_dest;
        }
        type.body_metadata().vw_initializeWithCopy((swift::opaque_value *)self_dest, (swift::opaque_value *)body);
    }

    // Initialize value
    if (initial_value != nullptr) {
        value_set_internal(node_ptr, *node_ptr.get(), initial_value, type.value_metadata());
    } else {
        node_ptr->set_dirty(true);
        node_ptr->set_pending(true);
        subgraph.add_dirty_flags(node_ptr->subgraph_flags());
    }

    subgraph.add_node(node_ptr);
    return node_ptr;
}

void Graph::update_attribute(AttributeID attribute, bool option) {
    // TODO: Not implemented
}

void Graph::update_main_refs(AttributeID attribute) {
    if (!attribute) {
        return;
    }

    auto output_edge_arrays = vector<ConstOutputEdgeArrayRef, 64, uint64_t>();

    auto update_main_ref = [this, &output_edge_arrays](const AttributeID &attribute) {
        if (!attribute) {
            return;
        }

        if (attribute.is_node()) {
            auto &node = attribute.to_node();
            auto &type =
                this->attribute_type(node.type_id()); // TODO: should AttributeType have deleted copy constructor?

            bool main_ref = false;
            if (type.value_metadata().getValueWitnesses()->isPOD() || type.flags() & AGAttributeTypeFlagsThreadSafe) {
                main_ref = false;
            } else {
                if (node.is_main_thread_only()) {
                    main_ref = true;
                } else {
                    main_ref =
                        std::any_of(node.input_edges().begin(), node.input_edges().end(), [](auto input_edge) -> bool {
                            auto resolved = input_edge.attribute.resolve(TraversalOptions::EvaluateWeakReferences);
                            if (resolved.attribute().is_node() && resolved.attribute().to_node().is_main_ref()) {
                                return true;
                            }
                        });
                }
            }
            if (node.is_main_ref() != main_ref) {
                node.set_main_ref(main_ref);
                output_edge_arrays.push_back({
                    &node.output_edges().front(),
                    node.output_edges().size(),
                });
            }
        } else if (attribute.is_indirect_node() && attribute.to_indirect_node().is_mutable()) {
            MutableIndirectNode &indirect_node = attribute.to_indirect_node().to_mutable();
            output_edge_arrays.push_back({
                &indirect_node.output_edges().front(),
                indirect_node.output_edges().size(),
            });
        }
    };

    update_main_ref(attribute);

    while (!output_edge_arrays.empty()) {
        ConstOutputEdgeArrayRef array = output_edge_arrays.back();
        output_edge_arrays.pop_back();

        for (auto output_edge : std::ranges::reverse_view(array)) {
            update_main_ref(output_edge.attribute);
        }
    }
}

#pragma mark - Value

bool Graph::value_set_internal(data::ptr<Node> node_ptr, Node &node, const void *value,
                               const swift::metadata &metadata) {
    // TODO: Not implemented
}

#pragma mark - Trace

void Graph::start_tracing(AGTraceFlags trace_flags, std::span<const char *> subsystems) {
    if (trace_flags & AGTraceFlagsEnabled && _trace_recorder == nullptr) {
        _trace_recorder = new TraceRecorder(this, trace_flags, subsystems);
        if (trace_flags & AGTraceFlagsPrepare) {
            prepare_trace(*_trace_recorder);
        }
        add_trace(_trace_recorder);

        static dispatch_once_t cleanup;
        dispatch_once(&cleanup, ^{
                          // TODO
                      });
    }
}

void Graph::stop_tracing() {
    if (_trace_recorder) {
        remove_trace(_trace_recorder->id());
        _trace_recorder = nullptr;
    }
}

void Graph::sync_tracing() {
    foreach_trace([](Trace &trace) { trace.sync_trace(); });
}

CFStringRef Graph::copy_trace_path() {
    if (_trace_recorder && _trace_recorder->trace_path()) {
        return CFStringCreateWithCString(0, _trace_recorder->trace_path(), kCFStringEncodingUTF8);
    } else {
        return nullptr;
    }
}

void Graph::prepare_trace(Trace &trace) {
    // TODO: Not implemented
}

void Graph::add_trace(Trace *_Nullable trace) {
    if (trace == nullptr) {
        return;
    }
    trace->begin_trace(*this);
    _traces.push_back(trace);
}

void Graph::remove_trace(uint64_t trace_id) {
    auto iter = std::remove_if(_traces.begin(), _traces.end(),
                               [&trace_id](auto trace) -> bool { return trace->id() == trace_id; });
    if (iter) {
        Trace *trace = *iter;
        trace->end_trace(*this);
        trace->trace_removed();
        _traces.erase(iter);
    }
}

void Graph::all_start_tracing(AGTraceFlags trace_flags, std::span<const char *> span) {
    all_lock();
    for (auto graph = _all_graphs; graph != nullptr; graph = graph->_next) {
        graph->start_tracing(trace_flags, span);
    }
    all_unlock();
}

void Graph::all_stop_tracing() {
    all_lock();
    for (auto graph = _all_graphs; graph != nullptr; graph = graph->_next) {
        graph->stop_tracing();
    }
    all_unlock();
}

void Graph::all_sync_tracing() {
    all_lock();
    for (auto graph = _all_graphs; graph != nullptr; graph = graph->_next) {
        graph->sync_tracing();
    }
    all_unlock();
}

CFStringRef Graph::all_copy_trace_path() {
    CFStringRef result = nullptr;
    all_lock();
    if (_all_graphs) {
        result = _all_graphs->copy_trace_path();
    }
    all_unlock();
    return result;
}

void Graph::trace_assertion_failure(bool all_stop_tracing, const char *format, ...) {
    char *message = nullptr;

    va_list args;
    va_start(args, format);

    bool locked = all_try_lock();
    for (auto graph = _all_graphs; graph != nullptr; graph = graph->_next) {
        graph->foreach_trace([&format, &args](Trace &trace) { trace.log_message_v(format, args); });
        if (all_stop_tracing) {
            graph->stop_tracing();
        }
    }
    if (locked) {
        all_unlock();
    }

    va_end(args);
}

#pragma mark - Keys

uint32_t Graph::intern_key(const char *key) {
    if (_keys == nullptr) {
        _keys = new Graph::KeyTable(&_heap);
    }
    const char *found = nullptr;
    uint32_t key_id = _keys->lookup(key, &found);
    if (found == nullptr) {
        key_id = _keys->insert(key);
    }
    return key_id;
}

const char *Graph::key_name(uint32_t key_id) const {
    if (_keys != nullptr && key_id < _keys->size()) {
        return _keys->get(key_id);
    }
    AG::precondition_failure("invalid string key id: %u", key_id);
}

} // namespace AG
