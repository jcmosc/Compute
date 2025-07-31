#include "Graph.h"

#include <CoreFoundation/CFString.h>
#include <mach/mach_time.h>
#include <ranges>
#include <set>

#include <Utilities/List.h>

#include "Attribute/AttributeData/Node/IndirectNode.h"
#include "Attribute/AttributeData/Node/Node.h"
#include "Attribute/AttributeID/OffsetAttributeID.h"
#include "Attribute/AttributeType/AttributeType.h"
#include "Attribute/AttributeView/AttributeView.h"
#include "ComputeCxx/AGTrace.h"
#include "ComputeCxx/AGUniqueID.h"
#include "Context.h"
#include "KeyTable.h"
#include "Log/Log.h"
#include "Subgraph/Subgraph.h"
#include "TraceRecorder.h"
#include "UpdateStack.h"

namespace AG {

Graph *Graph::_all_graphs = nullptr;
os_unfair_lock Graph::_all_graphs_lock = OS_UNFAIR_LOCK_INIT;

pthread_key_t Graph::_current_update_key = 0;

Graph::Graph()
    : _heap(nullptr, 0, 0), _interned_types(nullptr, nullptr, nullptr, nullptr, &_heap),
      _contexts_by_id(nullptr, nullptr, nullptr, nullptr, &_heap), _id(AGMakeUniqueID()) {

    static dispatch_once_t make_keys;
    dispatch_once_f(&make_keys, nullptr, [](void *context) {
        pthread_key_create(&Graph::_current_update_key, 0);
        Subgraph::make_current_subgraph_key();
    });

    _types.push_back(nullptr);

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

bool Graph::is_context_updating(uint64_t context_id) {
    for (auto update = current_update(); update != nullptr; update = update.get()->next()) {
        for (auto &frame : std::ranges::reverse_view(update.get()->frames())) {
            auto subgraph = AttributeID(frame.attribute).subgraph();
            if (subgraph && subgraph->context_id() == context_id) {
                return true;
            }
        }
    }
    return false;
}

#pragma mark - Main handler

void Graph::with_main_handler(ClosureFunctionVV<void> body, MainHandler _Nullable main_handler,
                              const void *main_handler_context) {

    auto old_main_handler = _main_handler;
    auto old_main_handler_context = _main_handler_context;

    _main_handler = main_handler;
    _main_handler_context = main_handler_context;

    body();

    _main_handler = old_main_handler;
    _main_handler_context = old_main_handler_context;
}

void Graph::call_main_handler(void *context, void (*body)(void *)) {
    assert(_main_handler);

    struct MainTrampoline {
        Graph *graph;
        pthread_t thread;
        void *context;
        void (*handler)(void *);

        static void thunk(const void *arg) {
            auto trampoline = reinterpret_cast<const MainTrampoline *>(arg);
            trampoline->handler(trampoline->context);
        };
    };

    auto current_update_thread = _current_update_thread;
    auto main_handler = _main_handler;
    auto main_handler_context = _main_handler_context;

    _current_update_thread = 0;
    _main_handler = nullptr;
    _main_handler_context = nullptr;

    MainTrampoline trampoline = {this, current_update_thread, context, body};
    main_handler(main_handler_context, MainTrampoline::thunk, &trampoline);

    _main_handler = main_handler;
    _main_handler_context = main_handler_context;
    _current_update_thread = current_update_thread;
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
        type->fetch_layout();
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

data::ptr<Node> Graph::add_attribute(Subgraph &subgraph, uint32_t type_id, const void *body, const void *value) {
    const AttributeType &type = attribute_type(type_id);

    // TODO: test this combo
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
    new (node_ptr.get()) Node(type_id, main_thread);
    node_ptr->set_main_ref(true); // setting here and below aain?

    if (type_id >= 0x100000) {
        precondition_failure("too many node types allocated");
    }

    void *self = (uint8_t *)node_ptr.get() + type.body_offset();
    node_ptr->set_self_initialized(true);

    if (type.value_metadata().getValueWitnesses()->isPOD() || type.flags() & AGAttributeTypeFlagsAsyncThread) {
        node_ptr->set_main_ref(false);
    } else {
        if (node_ptr->requires_main_thread()) {
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

data::ptr<IndirectNode> Graph::add_indirect_attribute(Subgraph &subgraph, AttributeID attribute, uint32_t offset,
                                                      std::optional<size_t> size, bool is_mutable) {
    if (subgraph.graph() != attribute.subgraph()->graph()) {
        precondition_failure("attribute references can't cross graph namespaces");
    }

    auto offset_attribute = attribute.resolve(TraversalOptions::SkipMutableReference);
    attribute = offset_attribute.attribute();
    if (__builtin_add_overflow(offset, offset_attribute.offset(), &offset) ||
        offset + offset_attribute.offset() > 0x3ffffffe) {
        precondition_failure("indirect attribute overflowed: %lu + %lu", offset, offset_attribute.offset());
    }

    if (size.has_value()) {
        auto attribute_size = attribute.size();
        if (attribute_size.has_value() && attribute_size.value() < offset + size.value()) {
            precondition_failure("invalid size for indirect attribute: %d vs %u", attribute_size.value(),
                                 offset_attribute.offset());
        }
    }

    if (is_mutable) {
        data::ptr<MutableIndirectNode> indirect_node_ptr =
            subgraph.alloc_bytes(sizeof(MutableIndirectNode), 3).unsafe_cast<MutableIndirectNode>();

        uint64_t subgraph_id = attribute && !attribute.is_nil() ? attribute.subgraph()->subgraph_id() : 0;
        auto source = WeakAttributeID(attribute, uint32_t(subgraph_id));
        bool traverses_contexts = subgraph.context_id() != attribute.subgraph()->context_id();
        new (indirect_node_ptr.get()) MutableIndirectNode(source, traverses_contexts, offset, size, source, offset);

        add_input_dependencies(AttributeID(indirect_node_ptr), attribute);
        subgraph.add_indirect(indirect_node_ptr.unsafe_cast<IndirectNode>(), true);
        return indirect_node_ptr.unsafe_cast<IndirectNode>();
    } else {
        data::ptr<IndirectNode> indirect_node_ptr =
            subgraph.alloc_bytes_recycle(sizeof(IndirectNode), 3).unsafe_cast<IndirectNode>();

        uint64_t subgraph_id = attribute && !attribute.is_nil() ? attribute.subgraph()->subgraph_id() : 0;
        auto source = WeakAttributeID(attribute, uint32_t(subgraph_id));
        bool traverses_contexts = subgraph.context_id() != attribute.subgraph()->context_id();
        new (indirect_node_ptr.get()) IndirectNode(source, traverses_contexts, offset, size);

        subgraph.add_indirect(indirect_node_ptr, &subgraph != attribute.subgraph());
        return indirect_node_ptr;
    }
}

void Graph::remove_node(data::ptr<Node> node) {
    if (node->is_updating()) {
        precondition_failure("deleting updating attribute: %u\n", node);
    }

    for (auto &input_edge : node->input_edges()) {
        this->remove_removed_input(AttributeID(node), input_edge.attribute);
    }
    for (auto output_edge : node->output_edges()) {
        this->remove_removed_output(AttributeID(node), output_edge.attribute, false);
    }

    //    if (_profile_data != nullptr) {
    //        _profile_data->remove_node(node, node->type_id());
    //    }
}

void Graph::remove_indirect_node(data::ptr<IndirectNode> indirect_node) {
    if (indirect_node->is_mutable()) {
        remove_removed_input(AttributeID(indirect_node), indirect_node->source().identifier());
        AttributeID dependency = indirect_node->to_mutable().dependency();
        if (dependency) {
            remove_removed_input(AttributeID(indirect_node), dependency);
        }
        for (auto output_edge : indirect_node->to_mutable().output_edges()) {
            remove_removed_output(AttributeID(indirect_node), output_edge.attribute, false);
        }
        return;
    }

    if (indirect_node->source().expired()) {
        return;
    }

    AttributeID source = indirect_node->source().identifier();
    while (true) {
        if (source.subgraph()->is_invalidated()) {
            break;
        }

        if (auto source_node = source.get_node()) {
            auto removed_outputs = vector<AttributeID, 8, uint64_t>();
            for (auto output_edge : source_node->output_edges()) {
                if (remove_removed_output(AttributeID(indirect_node), output_edge.attribute, false)) {
                    removed_outputs.push_back(output_edge.attribute);
                }
            }
            for (auto output : removed_outputs) {
                remove_removed_input(output, AttributeID(indirect_node));
            }
            break;
        } else if (auto source_indirect_node = source.get_indirect_node()) {
            if (source_indirect_node->is_mutable()) {
                auto removed_outputs = vector<AttributeID, 8, uint64_t>();
                for (auto output_edge : source_indirect_node->to_mutable().output_edges()) {
                    if (remove_removed_output(AttributeID(indirect_node), output_edge.attribute, false)) {
                        removed_outputs.push_back(output_edge.attribute);
                    }
                }
                for (auto output : removed_outputs) {
                    remove_removed_input(output, AttributeID(indirect_node));
                }
                break;
            } else {
                if (source_indirect_node->source().expired()) {
                    break;
                }

                source = source_indirect_node->source().identifier();
            }
        } else {
            break;
        }
    }
}

void Graph::remove_removed_input(AttributeID attribute, AttributeID input) {
    auto resolved_input =
        input.resolve(TraversalOptions::SkipMutableReference | TraversalOptions::EvaluateWeakReferences).attribute();
    if (auto input_node = resolved_input.get_node()) {
        if (!resolved_input.subgraph()->is_invalidated()) {
            remove_output_edge(input_node, attribute);
        }
    } else if (auto input_indirect_node = resolved_input.get_indirect_node()) {
        if (!resolved_input.subgraph()->is_invalidated()) {
            if (input_indirect_node->is_mutable()) {
                remove_output_edge(input_indirect_node.unsafe_cast<MutableIndirectNode>(), attribute);
            }
        }
    }
}

bool Graph::remove_removed_output(AttributeID attribute, AttributeID output, bool option) {
    if (output.subgraph()->is_invalidated()) {
        return false;
    }

    if (auto output_node = output.get_node()) {
        uint32_t index = 0;
        for (auto &input_edge : output_node->input_edges()) {
            if (input_edge.attribute.traverses(attribute, TraversalOptions::SkipMutableReference)) {
                remove_input_edge(output_node, *output_node.get(), index);
                return true;
            }
            index += 1;
        }
        return false;
    }

    if (auto output_indirect_node = output.get_indirect_node()) {
        if (output_indirect_node->source().identifier() != attribute) {

            // clear dependency
            assert(output_indirect_node->is_mutable());
            auto dependency = output_indirect_node->to_mutable().dependency();
            if (dependency && dependency == attribute) {
                foreach_trace([&output_indirect_node](Trace &trace) {
                    trace.set_dependency(output_indirect_node, AttributeID(AGAttributeNil));
                });
                output_indirect_node->to_mutable().set_dependency(AttributeID(nullptr));
                return true;
            }

            return false;
        }

        // reset source
        WeakAttributeID new_source = {AttributeID(AGAttributeNil), 0};
        uint32_t new_offset = 0;
        if (output_indirect_node->is_mutable()) {
            auto &mutable_output_indirect_node = output_indirect_node->to_mutable();
            auto initial_source = mutable_output_indirect_node.initial_source();
            if (initial_source.identifier() && !initial_source.identifier().is_nil() && !initial_source.expired()) {
                new_source = initial_source;
                new_offset = mutable_output_indirect_node.initial_offset();
            }
        }

        foreach_trace([&output_indirect_node, &new_source](Trace &trace) {
            trace.set_source(output_indirect_node, new_source.identifier());
        });

        output_indirect_node->modify(new_source, new_offset);

        if (new_source.identifier() && !new_source.identifier().is_nil() && !new_source.expired()) {
            add_input_dependencies(output, new_source.identifier());
        }

        return true;
    }

    return false;
}

uint32_t Graph::add_input(data::ptr<Node> node, AttributeID input, bool allow_nil, AGInputOptions options) {
    auto resolved_input = input.resolve(TraversalOptions::EvaluateWeakReferences).attribute();
    if (!resolved_input || resolved_input.is_nil()) {
        if (allow_nil) {
            return UINT32_MAX;
        }
        precondition_failure("reading from invalid source attribute: %u", input);
    }
    if (resolved_input == AttributeID(node)) {
        precondition_failure("cyclic edge: %u -> %u", resolved_input, node);
    }

    foreach_trace([&node, &resolved_input, &options](Trace &trace) { trace.add_edge(node, resolved_input, options); });

    auto subgraph = AttributeID(node).subgraph();
    auto context_id = subgraph ? subgraph->context_id() : 0;

    auto input_subgraph = resolved_input.subgraph();
    auto input_context_id = input_subgraph ? input_subgraph->context_id() : 0;

    if (context_id != input_context_id) {
        node->set_input_edges_traverse_contexts(true);
    }

    InputEdge new_input_edge = {
        input,
        static_cast<AGInputOptions>((options & (AGInputOptionsUnprefetched | AGInputOptionsAlwaysEnabled)) |
                                    (node->is_dirty() ? AGInputOptionsChanged : AGInputOptionsNone)),
    };

    uint32_t index = node->insert_input_edge(subgraph, new_input_edge);
    add_input_dependencies(AttributeID(node), resolved_input);

    if (node->is_updating()) {
        reset_update(node);
    }
    if (node->is_dirty()) {
        foreach_trace([&node, &index](Trace &trace) { trace.set_edge_pending(node, index, true); });
    }

    return index;
}

void Graph::remove_input(data::ptr<Node> node, uint32_t index) {
    remove_input_dependencies(AttributeID(node), node->input_edges()[index].attribute);
    remove_input_edge(node, *node.get(), index);
}

void Graph::remove_input_edge(data::ptr<Node> node_ptr, Node &node, uint32_t index) {
    foreach_trace([&node_ptr, &index](Trace &trace) { trace.remove_edge(node_ptr, index); });

    node.remove_input_edge(index);
    if (node.input_edges().size() == 0) {
        all_inputs_removed(node_ptr);
    }
    reset_update(node_ptr);
}

void Graph::remove_all_inputs(data::ptr<Node> node) {
    uint32_t input_index = node->input_edges().size();
    while (input_index > 0) {
        input_index -= 1;
        remove_input(node, input_index);
    }
    all_inputs_removed(node);
}

void Graph::all_inputs_removed(data::ptr<Node> node) {
    node->set_input_edges_traverse_contexts(false);
    node->set_needs_sort_input_edges(false);
    if (node->requires_main_thread() && !attribute_type(node->type_id()).flags() && AGAttributeTypeFlagsMainThread) {
        node->set_requires_main_thread(false);
    }
}

template <> void Graph::add_output_edge<Node>(data::ptr<Node> node, AttributeID output) {
    node->output_edges().push_back(node.page_ptr()->zone, OutputEdge(output));
}

template <> void Graph::add_output_edge<MutableIndirectNode>(data::ptr<MutableIndirectNode> node, AttributeID output) {
    node->output_edges().push_back(node.page_ptr()->zone, OutputEdge(output));
}

template <> void Graph::remove_output_edge<Node>(data::ptr<Node> node, AttributeID output) {
    auto iter = std::find_if(node->output_edges().begin(), node->output_edges().end(),
                             [&output](auto iter) -> bool { return iter.attribute == output; });
    if (iter != node->output_edges().end()) {
        node->output_edges().erase(iter);
    }

    //    if (node->outputs().empty() && node->flags().cacheable()) {
    //        AttributeID(node).subgraph()->cache_insert(node);
    //    }
}

template <>
void Graph::remove_output_edge<MutableIndirectNode>(data::ptr<MutableIndirectNode> node, AttributeID output) {
    auto iter = std::find_if(node->output_edges().begin(), node->output_edges().end(),
                             [&output](auto iter) -> bool { return iter.attribute == output; });
    if (iter != node->output_edges().end()) {
        node->output_edges().erase(iter);
    }
}

void Graph::add_input_dependencies(AttributeID attribute, AttributeID input) {
    auto resolved_input = input.resolve(TraversalOptions::SkipMutableReference).attribute();
    if (auto input_node = resolved_input.get_node()) {
        add_output_edge(input_node, attribute);
    } else if (auto input_indirect_node = resolved_input.get_indirect_node()) {
        assert(input_indirect_node->is_mutable());
        add_output_edge(input_indirect_node.unsafe_cast<MutableIndirectNode>(), attribute);
    }
    update_main_refs(attribute);
}

void Graph::remove_input_dependencies(AttributeID attribute, AttributeID input) {
    auto resolved_input = input.resolve(TraversalOptions::SkipMutableReference).attribute();
    if (auto input_node = resolved_input.get_node()) {
        remove_output_edge(input_node, attribute);
    } else if (auto input_indirect_node = resolved_input.get_indirect_node()) {
        assert(input_indirect_node->is_mutable());
        remove_output_edge(input_indirect_node.unsafe_cast<MutableIndirectNode>(), attribute);
    }
    update_main_refs(attribute);
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

        if (auto node = attribute.get_node()) {
            auto &type = this->attribute_type(node->type_id()); // TODO: should AttributeType have deleted
                                                                // copy constructor?

            bool main_ref = false;
            if (type.value_metadata().getValueWitnesses()->isPOD() || type.flags() & AGAttributeTypeFlagsAsyncThread) {
                main_ref = false;
            } else {
                if (node->requires_main_thread()) {
                    main_ref = true;
                } else {
                    main_ref = std::any_of(
                        node->input_edges().begin(), node->input_edges().end(), [](auto input_edge) -> bool {
                            auto resolved = input_edge.attribute.resolve(TraversalOptions::EvaluateWeakReferences);
                            if (auto resolved_node = resolved.attribute().get_node()) {
                                if (resolved_node->is_main_ref()) {
                                    return true;
                                }
                            }
                            return false;
                        });
                }
            }
            if (node->is_main_ref() != main_ref) {
                node->set_main_ref(main_ref);
                output_edge_arrays.push_back({
                    &node->output_edges().front(),
                    node->output_edges().size(),
                });
            }
        } else if (auto indirect_node = attribute.get_indirect_node()) {
            if (indirect_node->is_mutable()) {
                MutableIndirectNode &mutable_indirect_node = indirect_node->to_mutable();
                output_edge_arrays.push_back({
                    &mutable_indirect_node.output_edges().front(),
                    mutable_indirect_node.output_edges().size(),
                });
            }
        }
    };

    update_main_ref(attribute);

    while (!output_edge_arrays.empty()) {
        ConstOutputEdgeArrayRef array = output_edge_arrays.back();
        output_edge_arrays.pop_back();

        // TODO: test reverse view works
        // TODO: change foreach_trace to reverse_view
        for (auto output_edge : std::ranges::reverse_view(array)) {
            update_main_ref(output_edge.attribute);
        }
    }
}

uint32_t Graph::index_of_input(Node &node, InputEdge::Comparator comparator) {
    if (node.input_edges().size() > 0x100) {
        return index_of_input_slow(node, comparator);
    }
    uint32_t index = 0;
    for (auto &input_edge : node.input_edges()) {
        if (comparator.match(input_edge)) {
            return index;
        }
    }
    return UINT32_MAX;
}

uint32_t Graph::index_of_input_slow(Node &node, InputEdge::Comparator comparator) {
    node.sort_input_edges_if_needed();
    InputEdge *search_start =
        std::find_if(node.input_edges().begin(), node.input_edges().end(),
                     [&comparator](InputEdge &input_edge) { return input_edge.attribute == comparator.attribute; });
    uint32_t index = uint32_t(search_start - node.input_edges().begin());
    for (auto iter = search_start, end = node.input_edges().end(); iter != end; ++iter) {
        if (comparator.match(*iter)) {
            return index;
        }
        index += 1;
    }
    return UINT32_MAX;
}

void Graph::indirect_attribute_set(data::ptr<IndirectNode> indirect_node, AttributeID source) {
    if (!indirect_node->is_mutable()) {
        precondition_failure("not an indirect attribute: %u", indirect_node);
    }

    if (AttributeID(indirect_node).subgraph()->graph() != source.subgraph()->graph()) {
        precondition_failure("attribute references can't cross graph namespaces");
    }

    foreach_trace([&indirect_node, &source](Trace &trace) { trace.set_source(indirect_node, source); });

    OffsetAttributeID resolved_source = source.resolve(TraversalOptions::SkipMutableReference);
    source = resolved_source.attribute();
    uint32_t offset = resolved_source.offset();

    AttributeID old_source = indirect_node->source().identifier();
    if (resolved_source.attribute() == old_source) {
        if (resolved_source.offset() == indirect_node.offset()) {
            return;
        }
    } else {
        remove_input_dependencies(AttributeID(indirect_node), old_source);
    }

    indirect_node->modify(WeakAttributeID(resolved_source.attribute(), 0), resolved_source.offset());
    indirect_node->set_traverses_contexts(AttributeID(indirect_node).subgraph()->context_id() !=
                                          resolved_source.attribute().subgraph()->context_id());

    if (old_source != resolved_source.attribute()) {
        add_input_dependencies(AttributeID(indirect_node), resolved_source.attribute());
    }

    mark_changed(AttributeID(indirect_node), nullptr, 0, 0, 0);
    propagate_dirty(AttributeID(indirect_node));
}

bool Graph::indirect_attribute_reset(data::ptr<IndirectNode> indirect_node, bool non_nil) {
    if (!indirect_node->is_mutable()) {
        precondition_failure("not an indirect attribute: %u", indirect_node);
    }

    WeakAttributeID new_source = {AttributeID(AGAttributeNil), 0};
    uint32_t new_offset = 0;

    auto initial_source = indirect_node->to_mutable().initial_source();
    auto initial_offset = indirect_node->to_mutable().initial_offset();
    if (!initial_source.expired()) {
        new_source = initial_source;
        new_offset = initial_offset;
    } else {
        if (non_nil) {
            return false;
        }
    }

    AttributeID old_source_or_nil = indirect_node->source().evaluate();
    AttributeID new_source_or_nil = new_source.evaluate();

    foreach_trace(
        [&indirect_node, &new_source_or_nil](Trace &trace) { trace.set_source(indirect_node, new_source_or_nil); });

    if (old_source_or_nil != new_source_or_nil) {
        remove_input_dependencies(AttributeID(indirect_node), old_source_or_nil);
    }

    indirect_node->modify(new_source, new_offset);
    if (new_source_or_nil && !new_source_or_nil.is_nil()) {
        indirect_node->set_traverses_contexts(AttributeID(indirect_node).subgraph()->context_id() !=
                                              new_source_or_nil.subgraph()->context_id());
    }

    if (old_source_or_nil != new_source_or_nil) {
        add_input_dependencies(AttributeID(indirect_node), new_source_or_nil);
    }

    mark_changed(AttributeID(indirect_node), nullptr, 0, 0, 0);
    propagate_dirty(AttributeID(indirect_node));

    return true;
}

AttributeID Graph::indirect_attribute_dependency(data::ptr<IndirectNode> indirect_node) {
    if (!indirect_node->is_mutable()) {
        precondition_failure("not an indirect attribute: %u", indirect_node);
    }
    return indirect_node->to_mutable().dependency();
}

void Graph::indirect_attribute_set_dependency(data::ptr<IndirectNode> indirect_node, AttributeID dependency) {
    if (dependency && !dependency.is_nil()) {
        if (!dependency.is_node()) {
            precondition_failure("indirect dependencies must be attributes");
        }
        if (dependency.subgraph() != AttributeID(indirect_node).subgraph()) {
            precondition_failure("indirect dependencies must share a subgraph "
                                 "with their attribute");
        }
    } else {
        dependency = AttributeID(nullptr);
    }
    if (!indirect_node->is_mutable()) {
        precondition_failure("not an indirect attribute: %u", indirect_node);
    }

    foreach_trace([&indirect_node, &dependency](Trace &trace) { trace.set_dependency(indirect_node, dependency); });

    AttributeID old_dependency = indirect_node->to_mutable().dependency();
    if (old_dependency != dependency) {
        AttributeID indirect_attribute = AttributeID(indirect_node);
        if (old_dependency) {
            remove_output_edge(old_dependency.get_node(), indirect_attribute);
        }
        indirect_node->to_mutable().set_dependency(dependency);
        if (dependency) {
            add_output_edge(dependency.get_node(), indirect_attribute);
            if (dependency.get_node()->is_dirty()) {
                propagate_dirty(indirect_attribute);
            }
        }
    }
}

#pragma mark - Search

bool Graph::breadth_first_search(AttributeID attribute, AGSearchOptions options,
                                 ClosureFunctionAB<bool, AGAttribute> predicate) const {
    auto resolved = attribute.resolve(TraversalOptions::SkipMutableReference);
    if (!resolved.attribute() || resolved.attribute().is_nil()) {
        return false;
    }

    auto seen = std::set<AttributeID>();

    auto queue = std::deque<AttributeID>();
    queue.push_back(resolved.attribute());

    while (!queue.empty()) {
        AttributeID candidate = queue.front();
        queue.pop_front();

        if (candidate.is_nil()) {
            continue;
        }

        if (candidate.is_node() && predicate(candidate)) {
            return true;
        }

        if (options & AGSearchOptionsSearchInputs) {
            if (auto candidate_node = candidate.get_node()) {
                for (auto input_edge : candidate_node->input_edges()) {
                    auto input = input_edge.attribute.resolve(TraversalOptions::SkipMutableReference).attribute();
                    if (seen.contains(input)) {
                        continue;
                    }

                    if (options & AGSearchOptionsTraverseGraphContexts ||
                        candidate.subgraph()->context_id() == input.subgraph()->context_id()) {
                        seen.insert(input);
                        queue.push_back(input);
                    }
                }
            } else if (auto candidate_indirect_node = candidate.get_indirect_node()) {
                AttributeID source = candidate_indirect_node->source()
                                         .identifier()
                                         .resolve(TraversalOptions::SkipMutableReference)
                                         .attribute();
                if (!seen.contains(source)) {
                    if (options & AGSearchOptionsTraverseGraphContexts ||
                        candidate.subgraph()->context_id() == source.subgraph()->context_id()) {
                        seen.insert(source);
                        queue.push_back(source);
                    }
                }
            }
        }

        if (options & AGSearchOptionsSearchOutputs) {
            // outputs should never be non-mutable nodes - check!
            if (auto candidate_node = candidate.get_node()) {
                for (auto output_edge : candidate_node->output_edges()) {
                    if (seen.contains(output_edge.attribute)) {
                        continue;
                    }

                    if (options & AGSearchOptionsTraverseGraphContexts ||
                        candidate.subgraph()->context_id() == output_edge.attribute.subgraph()->context_id()) {
                        seen.insert(output_edge.attribute);
                        queue.push_back(output_edge.attribute);
                    }
                }
            } else if (auto candidate_indirect_node = candidate.get_indirect_node()) {
                assert(candidate_indirect_node->is_mutable());
                for (auto output_edge : candidate_indirect_node->to_mutable().output_edges()) {
                    if (seen.contains(output_edge.attribute)) {
                        continue;
                    }

                    if (options & AGSearchOptionsTraverseGraphContexts ||
                        candidate.subgraph()->context_id() == output_edge.attribute.subgraph()->context_id()) {
                        seen.insert(output_edge.attribute);
                        queue.push_back(output_edge.attribute);
                    }
                }
            }
        }
    }

    return false;
}

#pragma mark - Updates

bool Graph::passed_deadline() {
    if (_deadline == UINT64_MAX) {
        return false;
    }
    return passed_deadline_slow();
}

bool Graph::passed_deadline_slow() {
    if (_deadline == 0) {
        return true;
    }

    uint64_t time = mach_absolute_time();
    if (time < _deadline) {
        return false;
    }

    foreach_trace([](Trace &trace) { trace.passed_deadline(); });
    _deadline = 0;

    return true;
}

bool Graph::thread_is_updating() {
    for (auto update = current_update(); update != nullptr; update = update.get()->next()) {
        if (update.get()->graph() == this) {
            return true;
        }
    }
    return false;
}

void Graph::call_update() {
    while (_needs_update) {
        _needs_update = false;
        _contexts_by_id.for_each(
            [](uint64_t context_id, Context *graph_context, void *closure_context) { graph_context->call_update(); },
            nullptr);
    }
}

void Graph::reset_update(data::ptr<Node> node) {
    for (auto update = current_update(); update != nullptr; update = update.get()->next()) {
        for (auto &frame : update.get()->frames()) {
            if (frame.attribute == node) {
                frame.num_pushed_inputs = 0;
            }
        }
    }
}

void Graph::with_update(data::ptr<AG::Node> node, ClosureFunctionVV<void> body) {
    class scoped_update : public UpdateStack {
      public:
        scoped_update(Graph *graph, AGGraphUpdateOptions options, data::ptr<AG::Node> node)
            : UpdateStack(graph, options) {
            frames().push_back({node, node->is_pending()});
        };
        ~scoped_update() { frames().pop_back(); }
    };

    scoped_update update = scoped_update(this, AGGraphUpdateOptionsNone, node);
    body();
    // ~scoped_update
}

void Graph::without_update(ClosureFunctionVV<void> body) {
    auto old_update = current_update();
    AG::Graph::set_current_update(old_update != nullptr ? old_update.with_tag(true) : nullptr);
    body();
    AG::Graph::set_current_update(old_update);
}

void Graph::collect_stack(vector<data::ptr<Node>, 0, uint64_t> &nodes) {
    for (auto update = current_update(); update != nullptr; update = update.get()->next()) {
        for (auto &frame : std::ranges::reverse_view(update.get()->frames())) {
            nodes.push_back(frame.attribute);
        }
    }
}

Graph::UpdateStatus Graph::update_attribute(data::ptr<Node> node, AGGraphUpdateOptions options) {
    if (!(options & AGGraphUpdateOptionsInTransaction) && _needs_update) {
        if (!thread_is_updating()) {
            call_update();
        }
    }

    if (node->is_value_initialized() && !node->is_dirty()) {
        return UpdateStatus::Unchanged;
    }

    _update_count += 1;
    if (node->is_main_thread()) {
        _main_thread_update_count += 1;
    }

    UpdateStack current_update = UpdateStack(this, options);

    foreach_trace(
        [&current_update, &node, &options](Trace &trace) { trace.begin_update(current_update, node, options); });

    UpdateStatus status = UpdateStatus::Changed;
    if (current_update.push(node, *node.get(), false, !(options & AGGraphUpdateOptionsInTransaction))) {
        status = current_update.update();
        if (status == UpdateStatus::NeedsCallMainHandler) {
            std::pair<UpdateStack *, UpdateStatus> context = {&current_update, UpdateStatus::NeedsCallMainHandler};
            call_main_handler(&context, [](void *void_context) {
                auto inner_context = reinterpret_cast<std::pair<UpdateStack *, UpdateStatus> *>(void_context);
                util::tagged_ptr<UpdateStack> previous = Graph::current_update();
                inner_context->second = inner_context->first->update();
                Graph::set_current_update(previous);
            });
            status = context.second;

            _main_thread_update_count += 1;
        }
    }

    foreach_trace([&current_update, &node, &status](Trace &trace) {
        trace.end_update(current_update, node, AGGraphUpdateStatus(status));
    });

    // ~UpdateStatus called
}

void Graph::mark_changed(data::ptr<Node> node, AttributeType *_Nullable type, const void *_Nullable destination_value,
                         const void *_Nullable source_value) {
    if (!_traces.empty()) {
        mark_changed(AttributeID(node), type, destination_value, source_value, 0);
        return;
    }

    uint32_t output_index = 0;
    for (auto output_edge : node->output_edges()) {
        if (!output_edge.attribute.is_node()) {
            mark_changed(AttributeID(node), type, destination_value, source_value, output_index);
            return;
        }
        for (InputEdge &input_edge : output_edge.attribute.get_node()->input_edges()) {
            if (input_edge.attribute.resolve(TraversalOptions::None).attribute() != AttributeID(node)) {
                continue;
            }
            if (input_edge.options & AGInputOptionsChanged) {
                continue;
            }
            if (!input_edge.attribute.is_node()) {
                if (compare_edge_values(input_edge, type, destination_value, source_value)) {
                    continue;
                }
            }
            input_edge.options |= AGInputOptionsChanged;
            break;
        }
        output_index += 1;
    }

    _change_count += 1;
}

void Graph::mark_changed(AttributeID attribute, AttributeType *_Nullable type, const void *_Nullable destination_value,
                         const void *_Nullable source_value, uint32_t start_output_index) {
    if (attribute.is_nil()) {
        return;
    }

    struct Frame {
        ConstOutputEdgeArrayRef output_edges;
        AttributeID attribute;
    };

    auto heap = util::InlineHeap<0x2000>();
    auto frames = util::ForwardList<Frame>(&heap);

    ConstOutputEdgeArrayRef initial_output_edges = {};
    if (auto node = attribute.get_node()) {
        initial_output_edges = {
            &node->output_edges().front(),
            node->output_edges().size(),
        };
    } else if (auto indirect_node = attribute.get_indirect_node()) {
        // TODO: how to make sure indirect is mutable?
        assert(indirect_node->is_mutable());
        initial_output_edges = {
            &indirect_node->to_mutable().output_edges().front(),
            indirect_node->to_mutable().output_edges().size(),
        };
    } else {
        return;
    }
    frames.emplace_front(initial_output_edges, attribute);

    while (!frames.empty()) {
        auto output_edges = frames.front().output_edges;
        auto attribute = frames.front().attribute;
        frames.pop_front();

        for (auto output_edge = output_edges.begin() + start_output_index, end = output_edges.end(); output_edge != end;
             ++output_edge) {

            if (auto output_node = output_edge->attribute.get_node()) {
                auto &input_edges = output_node->input_edges();
                for (uint32_t input_index = 0, num_inputs = input_edges.size(); input_index < num_inputs;
                     ++input_index) {
                    auto &input_edge = input_edges[input_index];
                    if (input_edge.attribute.resolve(TraversalOptions::SkipMutableReference).attribute() != attribute) {
                        continue;
                    }
                    if (input_edge.options & AGInputOptionsChanged) {
                        continue;
                    }
                    if (!input_edge.attribute.is_node()) {
                        if (compare_edge_values(input_edge, type, destination_value, source_value)) {
                            continue;
                        }
                    }
                    foreach_trace([&output_node, &input_index](Trace &trace) {
                        trace.set_edge_pending(output_node, input_index, true);
                    });
                    input_edge.options |= AGInputOptionsChanged;
                }
            } else if (auto output_indirect_node = output_edge->attribute.get_indirect_node()) {
                if (output_indirect_node->to_mutable().dependency() != attribute) {
                    auto &mutable_node = output_indirect_node->to_mutable();
                    frames.emplace_front(ConstOutputEdgeArrayRef(&mutable_node.output_edges().front(),
                                                                 mutable_node.output_edges().size()),
                                         output_edge->attribute);
                }
            }
        }

        start_output_index = 0;
    }

    _change_count += 1;
}

bool Graph::compare_edge_values(InputEdge input_edge, AttributeType *type, const void *destination_value,
                                const void *source_value) {
    if (type == nullptr) {
        return false;
    }

    if (!input_edge.attribute.is_indirect_node()) {
        return false;
    }

    auto indirect_node = input_edge.attribute.get_indirect_node();
    auto optional_size = indirect_node->size();
    if (!optional_size.has_value()) {
        return false;
    }

    auto size = optional_size.value();
    if (size == 0) {
        return true;
    }

    auto offset = input_edge.attribute.resolve(TraversalOptions::None).offset();
    if (offset == 0 && type->value_metadata().vw_size() == size) {
        return false;
    }

    return type->compare_values_partial(destination_value, source_value, offset, size);
}

void Graph::compare_failed(const void *lhs, const void *rhs, size_t range_offset, size_t range_size,
                           const swift::metadata *_Nullable type) {
    auto update = current_update();
    if (update.tag() != 0 || update.get() == nullptr) {
        return;
    }

    auto graph = update.get()->graph();
    auto attribute = update.get()->frames().back().attribute;
    graph->foreach_trace([&attribute, &lhs, &rhs, &range_offset, &range_size](Trace &trace) {
        trace.compare_failed(attribute, lhs, rhs, range_offset, range_size, nullptr);
    });
}

#pragma mark - Body

void Graph::attribute_modify(data::ptr<Node> node, const swift::metadata &metadata,
                             ClosureFunctionPV<void, void *> modify, bool invalidating) {
    if (!node->is_self_initialized()) {
        precondition_failure("no self data: %u", node);
    }

    const AttributeType &type = attribute_type(node->type_id());
    if (&type.body_metadata() != &metadata) {
        precondition_failure("self type mismatch: %u", node);
    }

    foreach_trace([&node](Trace &trace) { trace.begin_modify(node); });

    void *body = node->get_self(type);
    modify(body);

    foreach_trace([&node](Trace &trace) { trace.end_modify(node); });

    if (invalidating) {
        node->set_self_modified(true);
        mark_pending(node, node.get());
    }
}

void Graph::mark_pending(data::ptr<Node> node_ptr, Node *node) {
    if (!node->is_pending()) {
        foreach_trace([&node_ptr](Trace &trace) { trace.set_pending(node_ptr, true); });
        node->set_pending(true);
    }
    if (!node->is_dirty()) {
        foreach_trace([&node_ptr](Trace &trace) { trace.set_dirty(node_ptr, true); });
        node->set_dirty(true);

        AGAttributeFlags subgraph_flags = node->subgraph_flags();
        Subgraph *subgraph = AttributeID(node_ptr).subgraph();
        if (subgraph_flags && subgraph != nullptr) {
            subgraph->add_dirty_flags(subgraph_flags);
        }

        propagate_dirty(AttributeID(node_ptr));
    }
}

#pragma mark - Value

bool Graph::value_exists(data::ptr<Node> node) { return node->is_value_initialized(); }

AGValueState Graph::value_state(AttributeID attribute) {
    auto resolved = attribute.resolve(TraversalOptions::AssertNotNil);
    if (auto node = resolved.attribute().get_node()) {
        return node->flags();
    }

    return AGValueStateNone;
}

namespace {

inline void *value_ref_checked(data::ptr<Node> node, uint32_t offset, const AttributeType &type,
                               const swift::metadata &expected_type) {
    if (offset == 0 && (&type.value_metadata() != &expected_type)) {
        precondition_failure("invalid value type for attribute: %u (saw %s, expected %s)", node,
                             type.value_metadata().name(false), expected_type.name(false));
    }
    if (!node->is_value_initialized()) {
        precondition_failure("attribute being read has no value: %u", node);
    }

    void *value = node->get_value();
    if (offset != 0) {
        value = (uint8_t *)value + (offset - 1);
    }
    return value;
}

} // namespace

bool Graph::update_attribute_checked(data::ptr<Node> node, uint32_t subgraph_id, AGGraphUpdateOptions options,
                                     AGChangedValueFlags *flags_out) {
    uint64_t page_seed_before_update = 0;
    if (subgraph_id != 0) {
        page_seed_before_update = data::table::shared().raw_page_seed(node.page_ptr());
    }

    UpdateStatus status = update_attribute(node, options);
    if (status != UpdateStatus::Unchanged) {
        if (flags_out) {
            *flags_out |= AGChangedValueFlagsChanged;
        }
    }

    // check new page seed is same as old and zone is not deleted
    if ((page_seed_before_update >> 32) & 0xff) {
        uint64_t page_seed_after_update = data::table::shared().raw_page_seed(node.page_ptr());
        if ((page_seed_after_update >> 32) & 0xff) {
            if ((page_seed_before_update & 0x7fffffff) != (page_seed_after_update & 0xffffffff)) {
                return false;
            }
        }
    }

    return true;
}

void *Graph::value_ref(AttributeID attribute, uint32_t subgraph_id, const swift::metadata &value_type,
                       AGChangedValueFlags *_Nonnull flags_out) {
    _version += 1;

    OffsetAttributeID resolved = attribute.resolve(
        TraversalOptions::UpdateDependencies | TraversalOptions::ReportIndirectionInOffset |
        (subgraph_id != 0 ? TraversalOptions::EvaluateWeakReferences : TraversalOptions::AssertNotNil));
    if (subgraph_id != 0 && (!resolved.attribute() || !resolved.attribute().is_node())) {
        return nullptr;
    }

    auto node = resolved.attribute().get_node();
    const AttributeType &type = attribute_type(node->type_id());

    if ((type.flags() & AGAttributeTypeFlagsExternal) == 0) {
        increment_transaction_count_if_needed();
        if (!update_attribute_checked(node, subgraph_id, AGGraphUpdateOptionsNone, flags_out)) {
            return nullptr;
        }
    }

    return value_ref_checked(node, resolved.offset(), type, value_type);
}

void *Graph::input_value_ref(data::ptr<AG::Node> node, AttributeID input, uint32_t subgraph_id,
                             AGInputOptions input_options, const swift::metadata &value_type,
                             AGChangedValueFlags *_Nonnull flags_out) {
    auto comparator = InputEdge::Comparator(
        input, AGInputOptionsUnprefetched | AGInputOptionsSyncMainRef | AGInputOptionsAlwaysEnabled, input_options);
    uint32_t index = index_of_input(*node.get(), comparator);

    if (index < UINT32_MAX) {
        AG::OffsetAttributeID resolved_input =
            input.resolve(AG::TraversalOptions::UpdateDependencies | AG::TraversalOptions::AssertNotNil);
        assert(resolved_input.attribute().is_node());
        auto input_node = resolved_input.attribute().get_node();

        if (input_node->is_value_initialized() && !input_node->is_dirty()) {
            InputEdge &input_edge = node->input_edges()[index];
            input_edge.options |= AGInputOptionsEnabled;

            if (input_edge.options & AGInputOptionsChanged) {
                *flags_out |= AGChangedValueFlagsChanged;
            }

            void *value = input_node->get_value();
            value = (uint8_t *)value + resolved_input.offset();

            return value;
        }
    }

    return input_value_ref_slow(node, input, subgraph_id, input_options, value_type, flags_out, index);
}

void *Graph::input_value_ref_slow(data::ptr<AG::Node> node, AttributeID input, uint32_t subgraph_id,
                                  AGInputOptions input_options, const swift::metadata &value_type,
                                  AGChangedValueFlags *_Nonnull flags_out, uint32_t index) {

    if (input_options & AGInputOptionsSyncMainRef) {
        auto comparator = InputEdge::Comparator(input, AGInputOptionsUnprefetched | AGInputOptionsAlwaysEnabled,
                                                input_options & AGInputOptionsUnprefetched);
        index = index_of_input(*node.get(), comparator);
    }

    if (index == UINT32_MAX) {
        node.assert_valid();
        if (AttributeID(node).subgraph() == nullptr || AttributeID(node).subgraph()->graph() != this) {
            precondition_failure("accessing attribute in a different namespace: %u", node);
        }
        if (!node->is_dirty()) {
            auto resolved = input.resolve(
                TraversalOptions::UpdateDependencies |
                (subgraph_id != 0 ? TraversalOptions::EvaluateWeakReferences : TraversalOptions::AssertNotNil));
            if (subgraph_id != 0 && (!resolved.attribute() || !resolved.attribute().is_node())) {
                return nullptr;
            }

            update_attribute(resolved.attribute().get_node(), AGGraphUpdateOptionsNone);
        }

        index = add_input(node, input, subgraph_id != 0, input_options & AGInputOptionsUnprefetched);
        if (index == UINT32_MAX) {
            return nullptr;
        }
    }

    InputEdge &input_edge = node->input_edges()[index];
    input_edge.options |= input_options & AGInputOptionsUnprefetched;
    input_edge.options |= AGInputOptionsEnabled;

    OffsetAttributeID resolved = input_edge.attribute.resolve(
        TraversalOptions::UpdateDependencies | TraversalOptions::ReportIndirectionInOffset |
        (subgraph_id != 0 ? TraversalOptions::EvaluateWeakReferences : TraversalOptions::AssertNotNil));
    if (subgraph_id != 0 && (!resolved.attribute() || !resolved.attribute().is_node())) {
        return nullptr;
    }

    auto input_node = resolved.attribute().get_node();
    const AttributeType &input_type = attribute_type(input_node->type_id());

    if (!input_node->is_value_initialized() || input_node->is_dirty()) {
        if (!update_attribute_checked(input_node, subgraph_id, AGGraphUpdateOptionsNone, nullptr)) {
            return nullptr;
        }
    }

    if (input_edge.options & AGInputOptionsChanged) {
        *flags_out |= AGChangedValueFlagsChanged;
    }
    if (input_options & AGInputOptionsSyncMainRef && input_node->is_main_ref() &&
        !value_type.getValueWitnesses()->isPOD()) {
        input_node->set_requires_main_thread(true);
        *flags_out |= AGChangedValueFlagsRequiresMainThread;
    }

    return value_ref_checked(input_node, resolved.offset(), input_type, value_type);
}

bool Graph::value_set(data::ptr<Node> node, const swift::metadata &value_type, const void *value) {
    if (!node->input_edges().empty() && node->is_value_initialized()) {
        precondition_failure("can only set initial value of computed attributes: %u", node);
    }

    auto update = current_update();
    if (update.tag() == 0 && update.get() != nullptr && update.get()->graph() == this &&
        (!node->output_edges().empty() || node->is_updating())) {
        precondition_failure("setting value during update: %u", node);
    }

    bool changed = value_set_internal(node, *node.get(), value, value_type);
    if (changed) {
        propagate_dirty(AttributeID(node));
    }
    return changed;
}

bool Graph::value_set_internal(data::ptr<Node> node_ptr, Node &node, const void *value,
                               const swift::metadata &metadata) {
    foreach_trace([&node_ptr, &value](Trace &trace) { trace.set_value(node_ptr, value); });

    AttributeType &type = *_types[node.type_id()];
    if (&type.value_metadata() != &metadata) {
        precondition_failure("invalid value type for attribute: %u (saw %s, expected %s)", node_ptr,
                             type.value_metadata().name(false), metadata.name(false));
    }

    if (node.is_value_initialized()) {
        void *value_dest = node.get_value();
        if (type.compare_values(value_dest, value)) {
            return false;
        }

        mark_changed(node_ptr, &type, value_dest, value);

        metadata.vw_assignWithCopy((swift::opaque_value *)value_dest, (swift::opaque_value *)value);
        return true;
    } else {
        node.allocate_value(*this, *AttributeID(node_ptr).subgraph());

        node.set_value_initialized(true);
        mark_changed(node_ptr, nullptr, nullptr, nullptr);

        void *value_dest = node.get_value();
        metadata.vw_initializeWithCopy((swift::opaque_value *)value_dest, (swift::opaque_value *)value);
        return true;
    }
}

void Graph::value_mark(data::ptr<Node> node) {
    auto update = current_update();
    if (update.tag() == 0 && update.get() != nullptr) {
        if (update.get()->graph() == this && (!node->output_edges().empty() || node->is_updating())) {
            // TODO: check output edges empty ORed with is_updating
            precondition_failure("setting value during update: %u", node);
        }
    }

    foreach_trace([&node](Trace &trace) { trace.mark_value(node); });

    const AttributeType &type = attribute_type(node->type_id());
    if (type.flags() & AGAttributeTypeFlagsExternal) {
        mark_changed(node, nullptr, nullptr, nullptr);
    } else {
        node->set_self_modified(true); // TODO: check this

        if (!node->is_dirty()) {
            foreach_trace([&node](Trace &trace) { trace.set_dirty(node, true); });
            node->set_dirty(true);
        }
        if (!node->is_pending()) {
            foreach_trace([&node](Trace &trace) { trace.set_pending(node, true); });
            node->set_pending(true);
        }
        if (node->subgraph_flags()) {
            Subgraph *subgraph = AttributeID(node).subgraph();
            subgraph->add_dirty_flags(node->subgraph_flags());
        }
    }

    propagate_dirty(AttributeID(node));
}

void Graph::value_mark_all() {
    auto update = current_update();
    if (update.tag() == 0 && update.get() != nullptr) {
        precondition_failure("invalidating all values during update");
    }

    for (auto subgraph : _subgraphs) {
        for (auto page : subgraph->pages()) {
            for (auto attribute : attribute_view(page)) {
                if (!attribute || attribute.is_nil()) {
                    break; // TODO: check if this should break out of entire
                           // loop
                }
                if (auto node = attribute.get_node()) {
                    const AttributeType &type = attribute_type(node->type_id());
                    if (!(type.flags() & AGAttributeTypeFlagsExternal)) {
                        node->set_dirty(true);
                        node->set_pending(true);
                        subgraph->add_dirty_flags(node->subgraph_flags());
                    }
                    for (auto &input_edge : node->input_edges()) {
                        input_edge.options |= AGInputOptionsChanged;
                    }
                }
            }
        }
    }
}

void Graph::propagate_dirty(AttributeID attribute) {
    if (attribute.is_nil()) {
        return;
    }

    struct Frame {
        ConstOutputEdgeArrayRef output_edges;
        NodeState state;
    };

    auto heap = util::InlineHeap<0x2000>();
    auto frames = util::ForwardList<Frame>(&heap);

    ConstOutputEdgeArrayRef initial_output_edges = {};
    NodeState initial_state = NodeState(0);
    if (auto node = attribute.get_node()) {
        initial_output_edges = {
            &node->output_edges().front(),
            node->output_edges().size(),
        };
        initial_state = node->state();
    } else if (auto indirect_node = attribute.get_indirect_node()) {
        // TODO: how to make sure indirect is mutable?
        assert(indirect_node->is_mutable());
        initial_output_edges = {
            &indirect_node->to_mutable().output_edges().front(),
            indirect_node->to_mutable().output_edges().size(),
        };
        OffsetAttributeID source = indirect_node->source().identifier().resolve(TraversalOptions::None);
        if (auto source_node = source.attribute().get_node()) {
            initial_state = source_node->state();
        }
    }
    frames.emplace_front(initial_output_edges, initial_state);

    while (!frames.empty()) {
        auto &output_edges = frames.front().output_edges;
        auto state = frames.front().state;
        frames.pop_front();

        for (auto output_edge : std::ranges::reverse_view(output_edges)) {
            AttributeID output = output_edge.attribute;

            ConstOutputEdgeArrayRef dirty_output_edges = {};
            NodeState next_state = state;

            if (auto output_node = output.get_node()) {
                if ((state & NodeState::MainThread) != (NodeState)0 && !output_node->is_main_thread()) {
                    output_node->set_main_thread(true);
                    dirty_output_edges = {
                        &output_node->output_edges().front(),
                        output_node->output_edges().size(),
                    };
                }

                next_state = state | output_node->state();

                if (!output_node->is_dirty()) {
                    foreach_trace([&output_node](Trace &trace) { trace.set_dirty(output_node, true); });

                    output_node->set_dirty(true);
                    if (auto subgraph = AttributeID(output_node).subgraph()) {
                        subgraph->add_dirty_flags(output_node->subgraph_flags());
                    }

                    dirty_output_edges = {
                        &output_node->output_edges().front(),
                        output_node->output_edges().size(),
                    };

                    if (output_node->input_edges_traverse_contexts() && !output_node->output_edges().empty()) {
                        if (auto output_subgraph = output.subgraph()) {
                            if (auto context_id = output_subgraph->context_id()) {
                                if (attribute.subgraph() == nullptr ||
                                    context_id != attribute.subgraph()->context_id()) {
                                    if (auto context = _contexts_by_id.lookup(context_id, nullptr)) {
                                        context->call_invalidation_if_needed(attribute);
                                    }
                                }
                            }
                        }
                    }
                }

            } else if (auto output_indirect_node = output.get_indirect_node()) {
                if (output_indirect_node->is_mutable()) {
                    dirty_output_edges = {
                        &output_indirect_node->to_mutable().output_edges().front(),
                        output_indirect_node->to_mutable().output_edges().size(),
                    };

                    if (output_indirect_node->traverses_contexts() &&
                        !output_indirect_node->to_mutable().output_edges().empty()) {
                        if (auto output_subgraph = output.subgraph()) {
                            if (auto context_id = output_subgraph->context_id()) {
                                if (attribute.subgraph() == nullptr ||
                                    context_id != attribute.subgraph()->context_id()) {
                                    if (auto context = _contexts_by_id.lookup(context_id, nullptr)) {
                                        context->call_invalidation_if_needed(attribute);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (!dirty_output_edges.empty()) {
                frames.emplace_front(dirty_output_edges, next_state);
            }
        }
    }

    for (auto update = current_update(); update != nullptr; update = update.get()->next()) {
        bool stop = false;
        for (auto &update_frame : update.get()->frames()) {
            if (update_frame.attribute->is_main_thread()) {
                stop = true;
                break;
            }
            update_frame.attribute->set_main_thread(true);
        }
        if (stop) {
            break;
        }
    }
}

namespace {

inline size_t find_attribute(const AttributeID *attributes, AttributeID search, uint64_t count) {
    static_assert(sizeof(wchar_t) == sizeof(AttributeID));

    const AttributeID *location = (const AttributeID *)wmemchr((const wchar_t *)attributes, search, count);
    if (location == nullptr) {
        return count;
    }
    return (location - attributes) / sizeof(AttributeID);
}

} // namespace

bool Graph::any_inputs_changed(data::ptr<Node> node, const AttributeID *exclude_attributes,
                               uint64_t exclude_attributes_count) {
    for (auto &input_edge : node->input_edges()) {
        input_edge.options |= AGInputOptionsEnabled;
        if (input_edge.options & AGInputOptionsChanged) {
            if (find_attribute(exclude_attributes, input_edge.attribute, exclude_attributes_count) ==
                exclude_attributes_count) {
                return true;
            }
        }
    }
    return false;
}

void Graph::input_value_add(data::ptr<Node> node, AttributeID input, AGInputOptions options) {
    input.validate_data_offset();

    auto comparator = InputEdge::Comparator(input, options & AGInputOptionsUnprefetched,
                                            AGInputOptionsUnprefetched | AGInputOptionsAlwaysEnabled);
    auto index = index_of_input(*node.get(), comparator);
    if (index == UINT32_MAX) {
        index = add_input(node, input, false, options & AGInputOptionsUnprefetched);
    }
    if (index >= 0) {
        auto &input_edge = node->input_edges()[index];
        input_edge.options |= AGInputOptionsEnabled;
    }
}

void *Graph::output_value_ref(data::ptr<Node> node, const swift::metadata &value_type) {
    if (!node->is_updating()) {
        precondition_failure("attribute is not evaluating: %u", node);
    }

    if (!node->is_value_initialized()) {
        return nullptr;
    }

    const AttributeType &type = attribute_type(node->type_id());
    if (&type.value_metadata() != &value_type) {
        precondition_failure("invalid value type for attribute: %u (saw %s, expected %s)", node,
                             type.value_metadata().name(false), value_type.name(false));
    }

    return node->get_value();
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
