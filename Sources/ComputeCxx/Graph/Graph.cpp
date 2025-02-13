#include "Graph.h"

#include <CoreFoundation/CFString.h>
#include <mach/mach_time.h>
#include <os/log.h>
#include <set>
#include <wchar.h>

#include "Attribute/AttributeType.h"
#include "Attribute/Node/IndirectNode.h"
#include "Attribute/Node/Node.h"
#include "Attribute/OffsetAttributeID.h"
#include "Attribute/WeakAttributeID.h"
#include "Context.h"
#include "Errors/Errors.h"
#include "KeyTable.h"
#include "Log/Log.h"
#include "Subgraph/Subgraph.h"
#include "Swift/Metadata.h"
#include "Trace/Trace.h"
#include "TraceRecorder.h"
#include "UpdateStack.h"
#include "Utilities/List.h"

namespace AG {

Graph *Graph::_all_graphs = nullptr;
os_unfair_lock Graph::_all_graphs_lock = OS_UNFAIR_LOCK_INIT;

pthread_key_t Graph::_current_update_key = 0;

Graph::Graph()
    : _heap(nullptr, 0, 0), _type_ids_by_metadata(nullptr, nullptr, nullptr, nullptr, &_heap),
      _contexts_by_id(nullptr, nullptr, nullptr, nullptr, &_heap), _num_contexts(1) {

    data::table::ensure_shared();

    // what is this check doing?
    if ((uintptr_t)this == 1) {
        print();
        print_attribute(nullptr);
        print_stack();
        print_data();
        write_to_file(nullptr, 0);
    }

    static dispatch_once_t make_keys;
    dispatch_once_f(&make_keys, nullptr, [](void *context) {
        pthread_key_create(&Graph::_current_update_key, 0);
        Subgraph::make_current_subgraph_key();
    });

    _types.push_back(nullptr); // AGAttributeNullType

    // TODO: debug server, trace, profile

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
        trace.~Trace();
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

bool Graph::is_context_updating(uint64_t context_id) {
    for (auto update_stack = current_update(); update_stack != nullptr; update_stack = update_stack.get()->previous()) {
        for (auto frame = update_stack.get()->frames().rbegin(), end = update_stack.get()->frames().rend();
             frame != end; ++frame) {
            auto subgraph = AttributeID(frame->attribute).subgraph();
            if (subgraph && subgraph->graph_context_id() == context_id) {
                return true;
            }
        }
    }
}

Graph::Context *Graph::main_context() {
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

#pragma mark - Invalidating

Graph::without_invalidating::without_invalidating(Graph *graph) {
    _graph = graph;
    _graph_old_batch_invalidate_subgraphs = graph->_batch_invalidate_subgraphs;
    graph->_batch_invalidate_subgraphs = true;
}

Graph::without_invalidating::~without_invalidating() {
    if (_graph && _graph_old_batch_invalidate_subgraphs == false) {
        _graph->_batch_invalidate_subgraphs = false;
        _graph->invalidate_subgraphs();
    }
}

#pragma mark - Context

#pragma mark - Subgraphs

void Graph::add_subgraph(Subgraph &subgraph) {
    _subgraphs.push_back(&subgraph);

    _num_subgraphs += 1;
    _num_subgraphs_created += 1;
}

void Graph::remove_subgraph(Subgraph &subgraph) {
    auto iter = std::remove(_subgraphs.begin(), _subgraphs.end(), &subgraph);
    _subgraphs.erase(iter);

    if (auto map = _tree_data_elements_by_subgraph.get()) {
        auto iter = map->find(&subgraph);
        if (iter != map->end()) {
            map->erase(iter);
        }
    }

    if (subgraph.other_state() & Subgraph::CacheState::Option1) {
        subgraph.set_other_state(subgraph.other_state() & ~Subgraph::CacheState::Option1); // added to graph

        auto iter = std::remove(_subgraphs_with_cached_nodes.begin(), _subgraphs_with_cached_nodes.end(), &subgraph);
        _subgraphs_with_cached_nodes.erase(iter);
    }

    _num_subgraphs -= 1;
}

void Graph::invalidate_subgraphs() {
    if (_batch_invalidate_subgraphs) {
        return;
    }

    if (_main_handler == nullptr) {
        auto iter = _subgraphs_with_cached_nodes.begin(), end = _subgraphs_with_cached_nodes.end();
        while (iter != end) {
            auto subgraph = *iter;
            subgraph->set_other_state(subgraph->other_state() | Subgraph::CacheState::Option2);
            subgraph->cache_collect();
            uint8_t cache_state = subgraph->other_state();
            subgraph->set_other_state(subgraph->other_state() & ~Subgraph::CacheState::Option2);

            if ((cache_state & Subgraph::CacheState::Option1) == 0) {
                end = _subgraphs_with_cached_nodes.erase(iter);
            } else {
                ++iter;
            }
        }
        while (!_invalidated_subgraphs.empty()) {
            auto subgraph = _invalidated_subgraphs.back();
            subgraph->invalidate_now(*this);
            _invalidated_subgraphs.pop_back();
        }
    }
}

#pragma mark - Updates

TaggedPointer<Graph::UpdateStack> Graph::current_update() {
    return TaggedPointer<UpdateStack>((UpdateStack *)pthread_getspecific(_current_update_key));
}

void Graph::set_current_update(TaggedPointer<UpdateStack> current_update) {
    pthread_setspecific(_current_update_key, (void *)current_update.value());
}

bool Graph::thread_is_updating() {
    for (auto update_stack = current_update(); update_stack != nullptr; update_stack = update_stack.get()->previous()) {
        if (update_stack.get()->graph() == this) {
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
    for (auto update_stack = current_update(); update_stack != nullptr; update_stack = update_stack.get()->previous()) {
        for (auto frame : update_stack.get()->frames()) {
            if (frame.attribute == node) {
                frame.num_pushed_inputs = 0;
            }
        }
    }
}

void Graph::collect_stack(vector<data::ptr<Node>, 0, uint64_t> &nodes) {
    for (auto update_stack = current_update(); update_stack != nullptr; update_stack = update_stack.get()->previous()) {
        auto frames = update_stack.get()->frames();
        for (auto iter = frames.rbegin(), end = frames.rend(); iter != end; ++iter) {
            nodes.push_back(iter->attribute);
        }
    }
}

void Graph::with_update(data::ptr<AG::Node> node, ClosureFunctionVV<void> body) {
    class scoped_update {
      private:
        UpdateStack _base;

      public:
        scoped_update(UpdateStack base, data::ptr<AG::Node> node) : _base(base) {
            _base.frames().push_back({node, node->state().is_pending()});
        };
        ~scoped_update() { _base.frames().pop_back(); }
    };

    scoped_update update = scoped_update(UpdateStack(this, 0), node);
    body();
    // ~scoped_update called
}

void Graph::call_main_handler(void *context, void (*body)(void *)) {
    assert(_main_handler);

    struct MainTrampoline {
        Graph *graph;
        pthread_t thread;
        void *context;
        void (*handler)(void *);

        static void thunk(void *arg) {
            auto trampoline = reinterpret_cast<MainTrampoline *>(arg);
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
    main_handler(MainTrampoline::thunk, &trampoline);

    _main_handler = main_handler;
    _main_handler_context = main_handler_context;
    _current_update_thread = current_update_thread;
}

bool Graph::passed_deadline() {
    if (_deadline == -1) {
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

#pragma mark - Attributes

const AttributeType &Graph::attribute_type(uint32_t type_id) const { return *_types[type_id]; }

const AttributeType &Graph::attribute_ref(data::ptr<Node> node, const void *_Nullable *_Nullable self_out) const {
    auto &type = attribute_type(node->type_id());
    if (self_out) {
        *self_out = node->get_self(type);
    }
    return type;
}

void Graph::attribute_modify(data::ptr<Node> node, const swift::metadata &metadata,
                             ClosureFunctionPV<void, void *> modify, bool flag) {
    if (!node->state().is_self_initialized()) {
        precondition_failure("no self data: %u", node);
    }

    auto type = attribute_type(node->type_id());
    if (&type.self_metadata() != &metadata) {
        precondition_failure("self type mismatch: %u", node);
    }

    foreach_trace([&node](Trace &trace) { trace.begin_modify(node); });

    void *body = node->get_self(type);
    modify(body);

    foreach_trace([&node](Trace &trace) { trace.end_modify(node); });

    if (flag) {
        node->flags().set_value4_unknown0x40(true);
        mark_pending(node, node.get());
    }
}

data::ptr<Node> Graph::add_attribute(Subgraph &subgraph, uint32_t type_id, void *body, void *value) {
    const AttributeType &type = attribute_type(type_id);

    void *value_source = nullptr;
    if (type.use_graph_as_initial_value()) {
        value_source = this;
    }
    if (value != nullptr || type.value_metadata().vw_size() != 0) {
        value_source = value;
    }

    void *buffer = nullptr;
    size_t size = type.self_metadata().vw_size();
    size_t alignment_mask = type.self_metadata().getValueWitnesses()->getAlignmentMask();
    if (!type.self_metadata().getValueWitnesses()->isBitwiseTakable() || type.self_metadata().vw_size() > 0x80) {
        size = sizeof(void *);
        alignment_mask = 7;
        buffer = subgraph.alloc_persistent(size);
    }

    size_t total_size = ((sizeof(Node) + alignment_mask) & ~alignment_mask) + size;

    data::ptr<Node> node;
    if (total_size > 0x10) {
        node = (data::ptr<Node>)subgraph.alloc_bytes_recycle(uint32_t(total_size), uint32_t(alignment_mask | 3));
    } else {
        node = (data::ptr<Node>)subgraph.alloc_bytes(uint32_t(total_size), uint32_t(alignment_mask | 3));
    }
    bool main_thread = type.main_thread();
    *node = Node(Node::State().with_main_thread(main_thread).with_main_thread_only(main_thread), type_id, 0x20);

    if (type_id >= 0x100000) {
        precondition_failure("too many node types allocated");
    }

    void *self = (uint8_t *)node.get() + type.attribute_offset();

    node->set_state(node->state().with_self_initialized(true));
    if (node->state().is_main_thread_only() && !type.value_metadata().getValueWitnesses()->isPOD()) {
        node->flags().set_value4_unknown0x20(!type.unknown_0x20()); // toggle
    } else {
        node->flags().set_value4_unknown0x20(false);
    }
    if (buffer != nullptr) {
        node->flags().set_has_indirect_self(true);
        *(void **)self = buffer;
    }

    if (!type.value_metadata().getValueWitnesses()->isBitwiseTakable() || type.value_metadata().vw_size() > 0x80) {
        node->flags().set_has_indirect_value(true);
    }

    _num_nodes += 1;
    _num_nodes_created += 1;

    if (type.self_metadata().vw_size() != 0) {
        void *self_dest = self;
        if (node->flags().has_indirect_self()) {
            self_dest = *(void **)self_dest;
        }
        type.self_metadata().vw_initializeWithCopy((swift::opaque_value *)self_dest, (swift::opaque_value *)body);
    }

    if (value_source != nullptr) {
        value_set_internal(node, *node.get(), value_source, type.value_metadata());
    } else {
        node->set_state(node->state().with_dirty(true).with_pending(true));
        subgraph.add_dirty_flags(node->flags().value3());
    }

    subgraph.add_node(node);
    return node;
}

Graph::UpdateStatus Graph::update_attribute(AttributeID attribute, uint8_t options) {
    if (!(options & 1) && _needs_update) {
        if (!thread_is_updating()) {
            call_update();
        }
    }

    Node &node = attribute.to_node();
    if (node.state().is_value_initialized() && !node.state().is_dirty()) {
        return UpdateStatus::NoChange;
    }

    _update_attribute_count += 1;
    if (node.state().is_main_thread()) {
        _update_attribute_on_main_count += 1;
    }

    UpdateStack update_stack = UpdateStack(this, options);

    foreach_trace([&update_stack, &attribute, &options](Trace &trace) {
        trace.begin_update(update_stack, attribute.to_node_ptr(), options);
    });

    UpdateStatus status = UpdateStatus::Changed;
    if (update_stack.push(attribute.to_node_ptr(), node, false, (options & 1) == 0)) {

        status = update_stack.update();
        if (status == UpdateStatus::NeedsCallMainHandler) {
            std::pair<UpdateStack *, UpdateStatus> context = {&update_stack, UpdateStatus::NeedsCallMainHandler};
            call_main_handler(&context, [](void *void_context) {
                auto inner_context = reinterpret_cast<std::pair<UpdateStack *, uint32_t> *>(void_context);
                TaggedPointer<UpdateStack> previous = Graph::current_update();
                inner_context->second = inner_context->first->update();
                Graph::set_current_update(previous);
            });
            status = context.second;

            _update_attribute_on_main_count += 1;
        }
    }

    foreach_trace([&update_stack, &attribute, &status](Trace &trace) {
        trace.end_update(update_stack, attribute.to_node_ptr(), status);
    });

    // ~UpdateStatus called
}

void Graph::update_main_refs(AttributeID attribute) {
    if (attribute.is_nil()) {
        return;
    }

    auto output_edge_arrays = vector<ConstOutputEdgeArrayRef, 64, uint64_t>();
    auto update_unknown0x20 = [this, &output_edge_arrays](const AttributeID &attribute) {
        if (attribute.is_nil()) {
            return;
        }

        if (attribute.is_direct()) {
            Node &node = attribute.to_node();
            const AttributeType &type = this->attribute_type(node.type_id());

            bool new_unknown0x20 = false;
            if (type.value_metadata().getValueWitnesses()->isPOD() || type.unknown_0x20()) {
                new_unknown0x20 = false;
            } else {
                if (node.state().is_main_thread_only()) {
                    new_unknown0x20 = true;
                } else {
                    new_unknown0x20 =
                        std::any_of(node.inputs().begin(), node.inputs().end(), [](auto input_edge) -> bool {
                            auto resolved = input_edge.value.resolve(TraversalOptions::EvaluateWeakReferences);
                            if (resolved.attribute().is_direct() &&
                                resolved.attribute().to_node().flags().value4_unknown0x20()) {
                                return true;
                            }
                        });
                }
            }
            if (node.flags().value4_unknown0x20() != new_unknown0x20) {
                node.flags().set_value4_unknown0x20(new_unknown0x20);
                output_edge_arrays.push_back({
                    &node.outputs().front(),
                    node.outputs().size(),
                });
            }
        } else if (attribute.is_indirect() && attribute.to_indirect_node().is_mutable()) {
            MutableIndirectNode &node = attribute.to_indirect_node().to_mutable();
            output_edge_arrays.push_back({
                &node.outputs().front(),
                node.outputs().size(),
            });
        }
    };

    update_unknown0x20(attribute);

    while (!output_edge_arrays.empty()) {
        ConstOutputEdgeArrayRef array = output_edge_arrays.back();
        output_edge_arrays.pop_back();

        for (auto output_edge = array.rbegin(), output_edge_end = array.rend(); output_edge != output_edge_end;
             ++output_edge) {
            update_unknown0x20(output_edge->value);
        }
    }
}

void Graph::remove_node(data::ptr<Node> node) {
    if (node->state().is_updating()) {
        precondition_failure("deleting updating attribute: %u\n", node);
    }

    for (auto input_edge : node->inputs()) {
        this->remove_removed_input(node, input_edge.value);
    }
    for (auto output_edge : node->outputs()) {
        this->remove_removed_output(node, output_edge.value, false);
    }

    // if (_profile_data != nullptr) {
    //   TODO: _profile_data->remove_node();
    // }
}

bool Graph::breadth_first_search(AttributeID attribute, SearchOptions options,
                                 ClosureFunctionAB<bool, uint32_t> predicate) const {
    auto resolved = attribute.resolve(TraversalOptions::SkipMutableReference);
    if (resolved.attribute().without_kind() == 0) {
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

        if (candidate.is_direct() && predicate(candidate)) {
            return true;
        }

        if (options & SearchOptions::SearchInputs) {
            if (candidate.is_direct()) {
                for (auto input_edge : candidate.to_node().inputs()) {
                    auto input = input_edge.value.resolve(TraversalOptions::SkipMutableReference).attribute();

                    if (seen.contains(input)) {
                        continue;
                    }
                    if (options & SearchOptions::TraverseGraphContexts ||
                        candidate.subgraph()->graph_context_id() == input.subgraph()->graph_context_id()) {
                        seen.insert(input);
                        queue.push_back(input);
                    }
                }
            } else if (candidate.is_indirect()) {
                // TODO: inputs view on IndirectNode with source?
                AttributeID source = candidate.to_indirect_node().source().attribute();
                source = source.resolve(TraversalOptions::SkipMutableReference).attribute();

                if (!seen.contains(source)) {
                    if (options & SearchOptions::TraverseGraphContexts ||
                        candidate.subgraph()->graph_context_id() == source.subgraph()->graph_context_id()) {
                        seen.insert(source);
                        queue.push_back(source);
                    }
                }
            }
        }
        if (options & SearchOptions::SearchOutputs) {
            if (candidate.is_direct()) {
                for (auto output_edge : candidate.to_node().outputs()) {
                    if (seen.contains(output_edge.value)) {
                        continue;
                    }
                    if (options & SearchOptions::TraverseGraphContexts ||
                        candidate.subgraph()->graph_context_id() == output_edge.value.subgraph()->graph_context_id()) {
                        seen.insert(output_edge.value);
                        queue.push_back(output_edge.value);
                    }
                }
            } else if (candidate.is_indirect()) {
                // TODO: how to know it is mutable?
                for (auto output_edge : candidate.to_indirect_node().to_mutable().outputs()) {
                    if (seen.contains(output_edge.value)) {
                        continue;
                    }
                    if (options & SearchOptions::TraverseGraphContexts ||
                        candidate.subgraph()->graph_context_id() == output_edge.value.subgraph()->graph_context_id()) {
                        seen.insert(output_edge.value);
                        queue.push_back(output_edge.value);
                    }
                }
            }
        }
    }

    return false;
}

#pragma mark - Indirect attributes

void Graph::add_indirect_attribute(Subgraph &subgraph, AttributeID attribute, uint32_t offset,
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
            // TODO: check args
            precondition_failure("invalid size for indirect attribute: %d vs %u", attribute_size.has_value(),
                                 offset_attribute.offset());
        }
    }

    if (is_mutable) {
        auto indirect_node = (data::ptr<MutableIndirectNode>)subgraph.alloc_bytes(sizeof(MutableIndirectNode), 3);

        // TODO: check accessing zone_id directly or through raw_page_seed
        // check references of raw_page_seed in ghidra
        uint32_t zone_id = attribute.without_kind() != 0 ? attribute.subgraph()->info().zone_id() : 0;
        auto source = WeakAttributeID(attribute, zone_id);
        bool traverses_graph_contexts = subgraph.graph_context_id() != attribute.subgraph()->graph_context_id();
        uint16_t node_size = size.has_value() && size.value() <= 0xfffe ? uint16_t(size.value()) : 0xffff;
        *indirect_node = MutableIndirectNode(source, traverses_graph_contexts, offset, node_size, source, offset);

        add_input_dependencies(AttributeID(indirect_node).with_kind(AttributeID::Kind::Indirect), attribute);
        subgraph.add_indirect((data::ptr<IndirectNode>)indirect_node, true);
    } else {
        auto indirect_node = (data::ptr<IndirectNode>)subgraph.alloc_bytes_recycle(sizeof(Node), 3);

        uint32_t zone_id = attribute.without_kind() != 0 ? attribute.subgraph()->info().zone_id() : 0;
        auto source = WeakAttributeID(attribute, zone_id);
        bool traverses_graph_contexts = subgraph.graph_context_id() != attribute.subgraph()->graph_context_id();
        uint16_t node_size = size.has_value() && size.value() <= 0xfffe ? uint16_t(size.value()) : 0xffff;
        *indirect_node = IndirectNode(source, traverses_graph_contexts, offset, node_size);

        subgraph.add_indirect(indirect_node, &subgraph != attribute.subgraph());
    }
}

void Graph::remove_indirect_node(data::ptr<IndirectNode> indirect_node_ptr) {
    IndirectNode &indirect_node = *indirect_node_ptr;
    if (indirect_node_ptr->is_mutable()) {
        AttributeID attribute = AttributeID(indirect_node_ptr);

        remove_removed_input(attribute, indirect_node.source().attribute());
        AttributeID dependency = indirect_node.to_mutable().dependency();
        if (dependency != 0) { // TODO: == nullptr operator...
            remove_removed_input(attribute, dependency);
        }
        for (auto output_edge : indirect_node.to_mutable().outputs()) {
            remove_removed_output(attribute, output_edge.value, false);
        }
        return;
    }

    while (true) {
        if (indirect_node_ptr->source().expired()) {
            return;
        }

        AttributeID source = indirect_node_ptr->source().attribute();
        if (source.subgraph()->validation_state() == Subgraph::ValidationState::Invalidated) {
            break;
        }

        if (source.is_direct()) {
            //    auto removed_outputs = vector<AttributeID, 8, uint64_t>();
            auto removed_outputs = vector<uint32_t, 8, uint64_t>();
            for (auto output_edge : source.to_node().outputs()) {
                if (remove_removed_output(AttributeID(indirect_node_ptr), output_edge.value, false)) {
                    removed_outputs.push_back(output_edge.value);
                }
            }
            for (auto output : removed_outputs) {
                remove_removed_input(AttributeID(output), AttributeID(indirect_node_ptr));
                //                remove_removed_input(output, attribute);
            }
            break;
        } else if (source.is_indirect()) {
            if (source.to_indirect_node().is_mutable()) {
                auto removed_outputs = vector<uint32_t, 8, uint64_t>();
                for (auto output_edge : source.to_indirect_node().to_mutable().outputs()) {
                    if (remove_removed_output(AttributeID(indirect_node_ptr), output_edge.value, false)) {
                        removed_outputs.push_back(output_edge.value);
                    }
                }
                for (auto output : removed_outputs) {
                    remove_removed_input(AttributeID(output), AttributeID(indirect_node_ptr));
                }
                break;
            } else {
                indirect_node_ptr = source.to_indirect_node_ptr();
            }
        } else {
            break;
        }
    }
}

void Graph::indirect_attribute_set(data::ptr<IndirectNode> attribute, AttributeID source) {
    if (!attribute->is_mutable()) {
        precondition_failure("not an indirect attribute: %u", attribute);
    }
    if (AttributeID(attribute).subgraph()->graph() != source.subgraph()->graph()) {
        precondition_failure("attribute references can't cross graph namespaces");
    }

    foreach_trace([&attribute, &source](Trace &trace) { trace.set_source(attribute, source); });

    OffsetAttributeID resolved_source = source.resolve(TraversalOptions::SkipMutableReference);
    source = resolved_source.attribute();
    uint32_t offset = resolved_source.offset();

    AttributeID previous_source = attribute->source().attribute();
    if (resolved_source.attribute() == previous_source) {
        if (resolved_source.offset() == attribute.offset()) {
            return;
        }
    } else {
        remove_input_dependencies(AttributeID(attribute), previous_source);
    }

    // common method with this and _reset from here?

    // TODO: check zone id
    attribute->modify(WeakAttributeID(resolved_source.attribute(), 0), resolved_source.offset());
    attribute->set_traverses_graph_contexts(AttributeID(attribute).subgraph()->graph_context_id() !=
                                            resolved_source.attribute().subgraph()->graph_context_id());

    if (resolved_source.attribute() != previous_source) {
        add_input_dependencies(AttributeID(attribute), resolved_source.attribute());
    }

    mark_changed(AttributeID(attribute), nullptr, 0, 0, 0);
    propagate_dirty(AttributeID(attribute));
}

bool Graph::indirect_attribute_reset(data::ptr<IndirectNode> attribute, bool non_nil) {
    if (!attribute->is_mutable()) {
        precondition_failure("not an indirect attribute: %u", attribute);
    }

    auto initial_source = attribute->to_mutable().initial_source();
    if (initial_source.attribute().without_kind() == 0 && non_nil) {
        return false;
    }

    WeakAttributeID new_source = {AttributeID::make_nil(), 0};
    uint32_t new_offset = 0;
    if (!initial_source.expired()) {
        new_source = initial_source;
        new_offset = attribute->to_mutable().initial_offset();
    }

    // common method with this and _set from here?

    AttributeID old_source_or_nil = attribute->source().evaluate();
    AttributeID new_source_or_nil = new_source.evaluate();

    foreach_trace([&attribute, &new_source_or_nil](Trace &trace) { trace.set_source(attribute, new_source_or_nil); });

    if (old_source_or_nil != new_source_or_nil) {
        remove_input_dependencies(AttributeID(attribute), old_source_or_nil);
    }

    attribute->modify(new_source, new_offset);
    attribute->set_traverses_graph_contexts(AttributeID(attribute).subgraph()->graph_context_id() !=
                                            new_source_or_nil.subgraph()->graph_context_id());

    if (old_source_or_nil != new_source_or_nil) {
        add_input_dependencies(AttributeID(attribute), new_source_or_nil);
    }

    mark_changed(AttributeID(attribute), nullptr, 0, 0, 0);
    propagate_dirty(AttributeID(attribute));

    return true;
}

const AttributeID &Graph::indirect_attribute_dependency(data::ptr<IndirectNode> attribute) {
    // Status: Verified
    if (!attribute->is_mutable()) {
        precondition_failure("not an indirect attribute: %u", attribute);
    }
    return attribute->to_mutable().dependency();
}

void Graph::indirect_attribute_set_dependency(data::ptr<IndirectNode> attribute, AttributeID dependency) {
    // Status: Verified
    if (dependency.without_kind()) {
        if (!dependency.is_direct()) {
            precondition_failure("indirect dependencies must be attributes");
        }
        if (AttributeID(attribute).subgraph() != dependency.subgraph()) {
            precondition_failure("indirect dependencies must share a subgraph with their attribute");
        }
    } else {
        dependency = AttributeID(0);
    }
    if (!attribute->is_mutable()) {
        precondition_failure("not an indirect attribute: %u", attribute);
    }

    foreach_trace([&attribute, &dependency](Trace &trace) { trace.set_dependency(attribute, dependency); });

    AttributeID old_dependency = attribute->to_mutable().dependency();
    if (old_dependency != dependency) {
        AttributeID indirect_attribute = AttributeID(attribute).with_kind(AttributeID::Kind::Indirect);
        if (old_dependency) {
            remove_output_edge(old_dependency.to_node_ptr(), indirect_attribute);
        }
        attribute->to_mutable().set_dependency(dependency);
        if (dependency) {
            add_output_edge(dependency.to_node_ptr(), indirect_attribute);
            if (dependency.to_node().state().is_dirty()) {
                propagate_dirty(indirect_attribute);
            }
        }
    }
}

#pragma mark - Values

void *Graph::value_ref(AttributeID attribute, bool evaluate_weak_references, const swift::metadata &value_type,
                       bool *did_update_out) {

    _version += 1;

    OffsetAttributeID resolved = attribute.resolve(
        TraversalOptions::UpdateDependencies | TraversalOptions::ReportIndirectionInOffset |
        (evaluate_weak_references ? TraversalOptions::EvaluateWeakReferences : TraversalOptions::AssertNotNil));

    if (evaluate_weak_references && (resolved.attribute().without_kind() == 0 || !resolved.attribute().is_direct())) {
        return nullptr;
    }

    Node &node = resolved.attribute().to_node();
    const AttributeType &type = attribute_type(node.type_id());

    if (!type.use_graph_as_initial_value()) {

        increment_update_count_if_needed();

        uint64_t old_page_seed = 0;
        if (evaluate_weak_references) {
            old_page_seed = data::table::shared().raw_page_seed(resolved.attribute().page_ptr());
        }

        UpdateStatus status = update_attribute(resolved.attribute(), 0);
        if (status != UpdateStatus::NoChange) {
            *did_update_out = true;
        }

        // check new page seed is same as old and zone is not deleted
        if ((old_page_seed >> 0x20) & 0xff) {
            uint64_t new_page_seed = data::table::shared().raw_page_seed(resolved.attribute().page_ptr());
            if ((new_page_seed >> 0x20) & 0xff) {
                if ((old_page_seed & 0x7fffffff) != (new_page_seed & 0xffffffff)) {
                    return nullptr;
                }
            }
        }
    }

    if (resolved.offset() == 0 && (&type.value_metadata() != &value_type)) {
        precondition_failure("invalid value type for attribute: %u (saw %s, expected %s)", resolved.attribute(),
                             type.value_metadata().name(false), value_type.name(false));
    }
    if (!node.state().is_value_initialized()) {
        precondition_failure("attribute being read has no value: %u", resolved.attribute());
    }

    void *value = node.get_value();
    if (resolved.offset() != 0) {
        value = (uint8_t *)value + (resolved.offset() - 1);
    }
    return value;
}

bool Graph::value_set(data::ptr<Node> node, const swift::metadata &value_type, const void *value) {
    if (!node->inputs().empty() && node->state().is_value_initialized()) {
        precondition_failure("can only set initial value of computed attributes: %u", node);
    }

    auto update = current_update();
    if (update.tag() == 0 && update.get() != nullptr && update.get()->graph() == this &&
        (!node->outputs().empty() || node->state().is_updating())) {
        precondition_failure("setting value during update: %u", node);
    }

    bool changed = value_set_internal(node, *node.get(), value, value_type);
    if (changed) {
        propagate_dirty(node);
    }
    return changed;
}

bool Graph::value_set_internal(data::ptr<Node> node_ptr, Node &node, const void *value_source,
                               const swift::metadata &value_type) {
    foreach_trace([&node_ptr, &value_source](Trace &trace) { trace.set_value(node_ptr, value_source); });

    AttributeType &type = *_types[node.type_id()];
    if (&type.value_metadata() != &value_type) {
        precondition_failure("invalid value type for attribute: %u (saw %s, expected %s)",
                             type.value_metadata().name(false), value_type.name(false));
    }

    if (node.state().is_value_initialized()) {
        // already initialized
        void *value_dest = node.get_value();

        LayoutDescriptor::ComparisonOptions comparison_options =
            LayoutDescriptor::ComparisonOptions(type.comparison_mode()) |
            LayoutDescriptor::ComparisonOptions::CopyOnWrite | LayoutDescriptor::ComparisonOptions::ReportFailures;
        if (type.layout() == nullptr) {
            type.set_layout(LayoutDescriptor::fetch(value_type, comparison_options, 0));
        }
        ValueLayout layout = type.layout() == ValueLayoutEmpty ? nullptr : type.layout();

        // TODO: make void * and rename to dest and source
        if (LayoutDescriptor::compare(layout, (const unsigned char *)value_dest, (const unsigned char *)value_source,
                                      value_type.vw_size(), comparison_options)) {
            return false;
        }

        if (_traces.empty()) {
            // TODO: finish
        } else {
            mark_changed(AttributeID(node_ptr), &type, value_dest, value_source, 0);
        }

        value_type.vw_assignWithCopy((swift::opaque_value *)value_dest, (swift::opaque_value *)value_source);
    } else {
        // not initialized yet
        node.allocate_value(*this, *AttributeID(node_ptr).subgraph());

        // TODO: wrap in initialize_value on Node...
        node.set_state(node.state().with_value_initialized(true));
        mark_changed(node_ptr, nullptr, nullptr, nullptr);

        void *value_dest = node.get_value();
        value_type.vw_initializeWithCopy((swift::opaque_value *)value_dest, (swift::opaque_value *)value_source);
    }
}

// Status: Verified
bool Graph::value_exists(data::ptr<Node> node) { return node->state().is_value_initialized(); }

AGValueState Graph::value_state(AttributeID attribute) {
    if (!attribute.is_direct()) {
        auto resolved_attribute = attribute.resolve(TraversalOptions::AssertNotNil);
        attribute = resolved_attribute.attribute();
    }
    if (!attribute.is_direct()) {
        return 0;
    }

    auto node = attribute.to_node();
    return (node.state().is_dirty() ? 1 : 0) << 0 | (node.state().is_pending() ? 1 : 0) << 1 |
           (node.state().is_updating() ? 1 : 0) << 2 | (node.state().is_value_initialized() ? 1 : 0) << 3 |
           (node.state().is_main_thread() ? 1 : 0) << 4 | (node.flags().value4_unknown0x20() ? 1 : 0) << 5 |
           (node.state().is_main_thread_only() ? 1 : 0) << 6 | (node.flags().value4_unknown0x40() ? 1 : 0) << 7;
}

void Graph::value_mark(data::ptr<Node> node) {
    auto update = current_update(); // TODO: investigate meaning of tags
    if (update.tag() == 0 && update.get() != nullptr && update.get()->graph() == this &&
        (!node->outputs().empty() || node->state().is_updating())) {
        precondition_failure("setting value during update: %u", node);
    }

    foreach_trace([&node](Trace &trace) { trace.mark_value(node); });

    AttributeType &type = *_types[node->type_id()];
    if (type.use_graph_as_initial_value()) {
        mark_changed(node, nullptr, nullptr, nullptr);
    } else {
        node->flags().set_value4_unknown0x40(true);

        if (!node->state().is_dirty()) {
            foreach_trace([&node](Trace &trace) { trace.set_dirty(node, true); });
            node->set_state(node->state().with_dirty(true));
        }
        if (!node->state().is_pending()) {
            foreach_trace([&node](Trace &trace) { trace.set_pending(node, true); });
            node->set_state(node->state().with_pending(true));
        }
        if (node->flags().value3()) {
            Subgraph *subgraph = AttributeID(node).subgraph();
            subgraph->add_dirty_flags(node->flags().value3());
        }
    }

    propagate_dirty(node);
}

void Graph::value_mark_all() {
    auto update = current_update();
    if (update.tag() == 0 && update.get() != nullptr) {
        precondition_failure("invalidating all values during update");
    }

    for (auto subgraph : _subgraphs) {
        for (data::ptr<data::page> page = subgraph->last_page(); page != nullptr; page = page->previous) {
            uint16_t relative_offset = page->relative_offset_1;
            while (relative_offset) {
                AttributeID attribute = AttributeID(page + relative_offset);
                if (attribute.is_nil()) {
                    break; // TODO: check if this should break out of entire loop
                }

                if (attribute.is_direct()) {
                    relative_offset = attribute.to_node().flags().relative_offset();
                } else if (attribute.is_indirect()) {
                    relative_offset = attribute.to_indirect_node().relative_offset();
                } else {
                    relative_offset = 0;
                }

                if (attribute.is_direct()) {
                    auto node = attribute.to_node();
                    AttributeType &type = *_types[node.type_id()];
                    if (!type.use_graph_as_initial_value()) {
                        node.set_state(node.state().with_dirty(true).with_pending(true));
                        subgraph->add_dirty_flags(node.flags().value3());
                    }
                    for (auto input_edge : node.inputs()) {
                        input_edge.set_pending(true);
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
        data::vector<OutputEdge> outputs;
        Node::State state;
    };

    char stack_buffer[0x2000] = {};
    auto heap = util::Heap(stack_buffer, sizeof(stack_buffer), 0);
    auto frames = util::ForwardList<Frame>(&heap);

    data::vector<OutputEdge> initial_outputs = {};
    Node::State initial_state = Node::State(0);
    if (attribute.is_direct()) {
        initial_outputs = attribute.to_node().outputs();
        initial_state = attribute.to_node().state();
    } else if (attribute.is_indirect()) {
        // TODO: how to make sure indirect is mutable?
        initial_outputs = attribute.to_indirect_node().to_mutable().outputs();

        OffsetAttributeID source = attribute.to_indirect_node().source().attribute().resolve(TraversalOptions::None);
        if (source.attribute().is_direct()) {
            initial_state = source.attribute().to_node().state();
        }
    }
    frames.emplace_front(initial_outputs, initial_state);

    while (!frames.empty()) {
        auto outputs = frames.front().outputs;
        auto state = frames.front().state;
        frames.pop_front();

        for (auto output_edge = outputs.rbegin(), end = outputs.rend(); output_edge != end; ++output_edge) {
            AttributeID output = output_edge->value;

            data::vector<OutputEdge> dirty_outputs = {};
            Node::State next_state = state;

            if (output.is_direct()) {
                Node &output_node = output.to_node();

                ArrayRef<const OutputEdge> more_outputs = {nullptr, 0};
                if (state.is_main_thread() && !output_node.state().is_main_thread()) {
                    output_node.set_state(output_node.state().with_main_thread(true));
                    dirty_outputs = output_node.outputs();
                }

                next_state = Node::State(output_node.state().data() | state.data());

                if (!output_node.state().is_dirty()) {
                    foreach_trace([&output](Trace &trace) { trace.set_dirty(output.to_node_ptr(), true); });

                    output_node.set_state(output_node.state().with_dirty(true));
                    if (auto subgraph = output.subgraph()) {
                        subgraph->add_dirty_flags(output_node.flags().value3());
                    }

                    dirty_outputs = output_node.outputs();

                    if (output_node.flags().inputs_traverse_graph_contexts() && !output_node.outputs().empty()) {
                        if (auto output_subgraph = output.subgraph()) {
                            auto context_id = output_subgraph->graph_context_id();
                            if (context_id && (attribute.subgraph() == nullptr ||
                                               context_id != attribute.subgraph()->graph_context_id())) {
                                if (auto context = _contexts_by_id.lookup(context_id, nullptr)) {
                                    if (context->graph_version() != context->graph()._version) {
                                        context->call_invalidation(attribute);
                                    }
                                }
                            }
                        }
                    }
                }

            } else if (output.is_indirect()) {
                IndirectNode &output_node = output.to_indirect_node();

                if (output_node.is_mutable()) {
                    dirty_outputs = output_node.to_mutable().outputs();

                    if (output_node.traverses_graph_contexts() && !output_node.to_mutable().outputs().empty()) {
                        if (auto output_subgraph = output.subgraph()) {
                            auto context_id = output_subgraph->graph_context_id();
                            if (context_id && (attribute.subgraph() == nullptr ||
                                               context_id != attribute.subgraph()->graph_context_id())) {
                                if (auto context = _contexts_by_id.lookup(context_id, nullptr)) {
                                    if (context->graph_version() != context->graph()._version) {
                                        context->call_invalidation(attribute);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (!dirty_outputs.empty()) {
                frames.emplace_front(dirty_outputs, next_state);
            }
        }
    }

    for (auto update = current_update(); update != nullptr; update = update.get()->previous()) {
        bool stop = false;
        auto frames = update.get()->frames();
        for (auto frame = frames.rbegin(), end = frames.rend(); frame != end; ++frame) {
            if (frame->attribute->state().is_main_thread()) {
                stop = true;
                break;
            }
            frame->attribute->set_state(frame->attribute->state().with_main_thread(true));
        }
        if (stop) {
            break;
        }
    }
}

#pragma mark - Inputs

void *Graph::input_value_ref_slow(data::ptr<AG::Node> node, AttributeID input_attribute, bool evaluate_weak_references,
                                  uint8_t input_flags, const swift::metadata &type, char *arg5, uint32_t index) {

    if ((input_flags >> 1) & 1) {
        auto comparator = InputEdge::Comparator(input_attribute, input_flags & 1,
                                                InputEdge::Flags::Unknown0 | InputEdge::Flags::Unknown2);
        index = index_of_input(*node, comparator);
    }

    if (index < 0) {
        node.assert_valid();
        if (AttributeID(node).subgraph() == nullptr || AttributeID(node).subgraph()->graph() != this) {
            precondition_failure("accessing attribute in a different namespace: %u", node);
        }
        if (!node->state().is_dirty()) {
            auto resolved = input_attribute.resolve(
                evaluate_weak_references
                    ? TraversalOptions::UpdateDependencies | TraversalOptions::EvaluateWeakReferences
                    : TraversalOptions::UpdateDependencies | TraversalOptions::AssertNotNil);

            if (evaluate_weak_references) {
                if (resolved.attribute().without_kind() == 0 || !resolved.attribute().is_direct()) {
                    return 0;
                }
            }
            update_attribute(resolved.attribute(), 0);
        }
        index = add_input(node, input_attribute, evaluate_weak_references, input_flags & 1);
        if (index < 0) {
            return nullptr;
        }
    }

    InputEdge &input_edge = node->inputs()[index];

    input_edge.set_unknown0(input_edge.is_unknown0() || (input_flags & 1) ? true : false);
    input_edge.set_unknown4(true);

    OffsetAttributeID resolved = input_edge.value.resolve(
        evaluate_weak_references ? TraversalOptions::UpdateDependencies | TraversalOptions::ReportIndirectionInOffset |
                                       TraversalOptions::EvaluateWeakReferences
                                 : TraversalOptions::UpdateDependencies | TraversalOptions::ReportIndirectionInOffset |
                                       TraversalOptions::AssertNotNil);

    if (evaluate_weak_references) {
        if (resolved.attribute().without_kind() == 0 || !resolved.attribute().is_direct()) {
            return nullptr;
        }
    }

    if (!resolved.attribute().to_node().state().is_value_initialized() ||
        resolved.attribute().to_node().state().is_dirty()) {
        if (evaluate_weak_references) {
            auto zone_id_before_update = data::table::shared().raw_page_seed(resolved.attribute().page_ptr());

            update_attribute(resolved.attribute(), 0);

            if (zone_id_before_update & 0xff00000000) {
                auto zone_id_after_update = data::table::shared().raw_page_seed(resolved.attribute().page_ptr());
                if (zone_id_after_update & 0xff00000000) {
                    if ((zone_id_before_update & 0x7fffffff) != (uint32_t)zone_id_after_update) {
                        return nullptr;
                    }
                }
            }
        } else {
            update_attribute(resolved.attribute(), 0);
        }
    }

    if (resolved.attribute().to_node().state().is_pending()) {
        *arg5 |= 1;
    }
    if ((input_flags >> 1) & 1 && resolved.attribute().to_node().state().is_self_initialized() &&
        !type.getValueWitnesses()->isPOD()) {
        resolved.attribute().to_node().set_state(
            resolved.attribute().to_node().state().with_main_thread_only(true)); // TODO: check
        *arg5 |= 2;
    }

    if (resolved.offset() == 0) {
        auto input_type = attribute_type(resolved.attribute().to_node().type_id()).value_metadata();
        if (&input_type != &type) {
            precondition_failure("invalid value type for attribute: %u (saw %s, expected %s)", input_edge.value,
                                 input_type.name(false), type.name(false));
        }
    }
    if (!resolved.attribute().to_node().state().is_value_initialized()) {
        precondition_failure("attribute being read has no value: %u", resolved.attribute());
    }

    void *value = resolved.attribute().to_node().get_value();
    if (resolved.offset() != 0) {
        value = (uint8_t *)value + resolved.offset() - 1;
    }
    return value;
}

void Graph::input_value_add(data::ptr<Node> node, AttributeID input_attribute, uint8_t input_flags) {
    input_attribute.to_node_ptr().assert_valid();

    auto comparator = InputEdge::Comparator(input_attribute, input_flags & 1,
                                            InputEdge::Flags::Unknown0 | InputEdge::Flags::Unknown2);
    auto index = index_of_input(*node, comparator);
    if (index >= 0) {
        auto input_edge = node->inputs()[index];
        input_edge.set_unknown4(true);
    }
}

uint32_t Graph::add_input(data::ptr<Node> node, AttributeID input, bool allow_nil, uint8_t input_edge_flags) {
    auto resolved = input.resolve(TraversalOptions::EvaluateWeakReferences);
    if (resolved.attribute().without_kind() == 0) {
        if (allow_nil) {
            return -1;
        }
        precondition_failure("reading from invalid source attribute: %u", input);
    }
    if (resolved.attribute() == node) {
        precondition_failure("cyclic edge: %u -> %u", resolved.attribute(), node);
    }

    foreach_trace([&node, &resolved, &input_edge_flags](Trace &trace) {
        trace.add_edge(node, resolved.attribute(), input_edge_flags);
    });

    auto subgraph = AttributeID(node).subgraph();
    auto graph_context_id = subgraph ? subgraph->graph_context_id() : 0;

    auto input_subgraph = resolved.attribute().subgraph();
    auto input_graph_context_id = input_subgraph ? input_subgraph->graph_context_id() : 0;

    if (graph_context_id != input_graph_context_id) {
        node->flags().set_inputs_traverse_graph_contexts(true);
    }

    InputEdge new_input_edge = {
        resolved.attribute(),
        InputEdge::Flags(input_edge_flags & 5),
    };
    if (node->state().is_dirty()) {
        new_input_edge.set_pending(true);
    }

    uint32_t index = -1;
    if (node->flags().inputs_unsorted()) {
        node->inputs().push_back(subgraph, new_input_edge);
        node->flags().set_inputs_unsorted(true);
        index = node->inputs().size() - 1;
    } else {
        auto pos = std::lower_bound(node->inputs().begin(), node->inputs().end(), new_input_edge);
        node->inputs().insert(subgraph, pos, new_input_edge);
        index = (uint32_t)(pos - node->inputs().begin());
    }

    add_input_dependencies(node, resolved.attribute());

    if (node->state().is_updating()) {
        reset_update(node);
    }
    if (node->state().is_dirty()) {
        foreach_trace([&node, &index, &input_edge_flags](Trace &trace) { trace.set_edge_pending(node, index, true); });
    }

    return index;
}

void Graph::remove_input(data::ptr<Node> node, uint32_t index) {
    remove_input_dependencies(AttributeID(node), node->inputs()[index].value);
    remove_input_edge(node, *node, index);
}

void Graph::remove_all_inputs(data::ptr<Node> node) {
    for (auto index = node->inputs().size() - 1; index >= 0; --index) {
        remove_input(node, index);
    }
    all_inputs_removed(node);
}

void Graph::add_input_dependencies(AttributeID attribute, AttributeID input) {
    auto resolved = input.resolve(TraversalOptions::SkipMutableReference);
    if (resolved.attribute().is_direct()) {
        add_output_edge(resolved.attribute().to_node_ptr(), attribute);
    } else if (resolved.attribute().is_indirect()) {
        assert(resolved.attribute().to_indirect_node().is_mutable());
        add_output_edge(resolved.attribute().to_mutable_indirect_node_ptr(), attribute);
    }
    update_main_refs(attribute);
}

void Graph::remove_input_dependencies(AttributeID attribute, AttributeID input) {
    auto resolved = input.resolve(TraversalOptions::SkipMutableReference);
    if (resolved.attribute().is_direct()) {
        remove_output_edge(resolved.attribute().to_node_ptr(), attribute);
    } else if (resolved.attribute().is_indirect()) {
        assert(resolved.attribute().to_indirect_node().is_mutable());
        remove_output_edge(resolved.attribute().to_mutable_indirect_node_ptr(), attribute);
    }
    update_main_refs(attribute);
}

void Graph::remove_input_edge(data::ptr<Node> node_ptr, Node &node, uint32_t index) {
    foreach_trace([&node_ptr, &index](Trace &trace) { trace.remove_edge(node_ptr, index); });

    node.inputs().erase(node.inputs().begin() + index);
    if (node.inputs().size() == 0) {
        all_inputs_removed(node_ptr);
    }
    reset_update(node_ptr);
}

void Graph::remove_removed_input(AttributeID attribute, AttributeID input) {
    auto resolved = input.resolve(TraversalOptions::SkipMutableReference | TraversalOptions::EvaluateWeakReferences);
    if (resolved.attribute().subgraph()->validation_state() != Subgraph::ValidationState::Invalidated) {
        if (resolved.attribute().is_direct()) {
            remove_output_edge(resolved.attribute().to_node_ptr(), attribute);
        } else if (resolved.attribute().is_indirect()) {
            assert(resolved.attribute().to_indirect_node().is_mutable());
            remove_output_edge(resolved.attribute().to_mutable_indirect_node_ptr(), attribute);
        }
    }
}

bool Graph::any_inputs_changed(data::ptr<Node> node, const AttributeID *attributes, uint64_t count) {
    for (auto input : node->inputs()) {
        input.set_unknown4(true);
        if (input.is_pending()) {
            const AttributeID *location = (const AttributeID *)wmemchr((const wchar_t *)attributes, input.value, count);
            if (location == 0) {
                location = attributes + sizeof(AttributeID) * count;
            }
            if ((location - attributes) / sizeof(AttributeID) == count) {
                return true;
            }
        }
    }
    return false;
}

void Graph::all_inputs_removed(data::ptr<Node> node) {
    node->flags().set_inputs_traverse_graph_contexts(false);
    node->flags().set_inputs_unsorted(false);
    if (!node->state().is_main_thread_only() && !attribute_type(node->type_id()).main_thread()) {
        node->set_state(node->state().with_main_thread_only(false));
    }
}

uint32_t Graph::index_of_input(Node &node, InputEdge::Comparator comparator) {
    if (node.inputs().size() > 8) {
        return index_of_input_slow(node, comparator);
    }
    uint32_t index = 0;
    for (auto input : node.inputs()) {
        if (comparator.match(input)) {
            return index;
        }
    }
    return -1;
}

uint32_t Graph::index_of_input_slow(Node &node, InputEdge::Comparator comparator) {
    node.sort_inputs_if_needed();
    InputEdge *search_start = std::find_if(node.inputs().begin(), node.inputs().end(), [&comparator](InputEdge &input) {
        return input.value == comparator.attribute;
    });
    uint32_t index = uint32_t(search_start - node.inputs().begin());
    for (auto input = search_start, end = node.inputs().end(); input != end; ++input) {
        if (comparator.match(*input)) {
            return index;
        }
        index += 1;
    }
    return -1;
}

bool Graph::compare_edge_values(InputEdge input_edge, const AttributeType *type, const void *destination_value,
                                const void *source_value) {
    if (type == nullptr) {
        return false;
    }

    if (!input_edge.value.is_indirect()) {
        return false;
    }

    auto indirect_node = input_edge.value.to_indirect_node();
    if (!indirect_node.has_size()) {
        return false;
    }

    auto size = indirect_node.size().value();
    if (size == 0) {
        return true;
    }

    auto resolved_offset = input_edge.value.resolve(TraversalOptions::None).offset();
    if (resolved_offset == 0 && type->value_metadata().vw_size() == size) {
        return false;
    }

    LayoutDescriptor::ComparisonOptions options =
        LayoutDescriptor::ComparisonOptions(type->comparison_mode()) | LayoutDescriptor::ComparisonOptions::CopyOnWrite;

    auto layout = type->layout();
    if (layout == 0) {
        layout = LayoutDescriptor::fetch(type->value_metadata(), options, 0);
    }
    if (layout == ValueLayoutEmpty) {
        layout = 0;
    }

    return LayoutDescriptor::compare_partial(layout, (unsigned char *)destination_value + resolved_offset,
                                             (unsigned char *)source_value + resolved_offset, resolved_offset, size,
                                             options);
}

#pragma mark - Outputs

void *Graph::output_value_ref(data::ptr<Node> node, const swift::metadata &value_type) {
    // Status: Verified
    if (!node->state().is_updating()) {
        precondition_failure("attribute is not evaluating: %u", node);
    }

    if (!node->state().is_value_initialized()) {
        return nullptr;
    }

    const AttributeType &type = attribute_type(node->type_id());
    if (&type.value_metadata() != &value_type) {
        precondition_failure("invalid value type for attribute: %u (saw %s, expected %s)", node,
                             type.value_metadata().name(false), value_type.name(false));
    }

    return node->get_value();
}

template <> void Graph::add_output_edge<Node>(data::ptr<Node> node, AttributeID output) {
    node->outputs().push_back(node.page_ptr()->zone, OutputEdge(output));
}

template <> void Graph::add_output_edge<MutableIndirectNode>(data::ptr<MutableIndirectNode> node, AttributeID output) {
    node->outputs().push_back(node.page_ptr()->zone, OutputEdge(output));
}

template <> void Graph::remove_output_edge<Node>(data::ptr<Node> node, AttributeID output) {
    for (auto iter = node->outputs().begin(), end = node->outputs().end(); iter != end; ++iter) {
        if (iter->value == output) {
            node->outputs().erase(iter);
            break;
        }
    }

    if (node->outputs().empty() && node->flags().cacheable()) {
        AttributeID(node).subgraph()->cache_insert(node);
    }
}

template <>
void Graph::remove_output_edge<MutableIndirectNode>(data::ptr<MutableIndirectNode> node, AttributeID output) {
    for (auto iter = node->outputs().begin(), end = node->outputs().end(); iter != end; ++iter) {
        if (iter->value == output) {
            node->outputs().erase(iter);
            break;
        }
    }
}

bool Graph::remove_removed_output(AttributeID attribute, AttributeID output, bool flag) {
    if (output.subgraph()->validation_state() == Subgraph::ValidationState::Invalidated) {
        return false;
    }

    if (output.is_direct()) {
        auto output_node_ptr = output.to_node_ptr();

        uint32_t index = 0;
        for (auto input : output_node_ptr->inputs()) {
            if (input.value.traverses(attribute, TraversalOptions::SkipMutableReference)) {
                remove_input_edge(output_node_ptr, *output_node_ptr.get(), index);
                return true;
            }
            index += 1;
        }
        return false;
    }

    if (!output.is_indirect()) {
        return false;
    }

    auto indirect_node = output.to_indirect_node_ptr();

    if (indirect_node->source().attribute() != attribute) {

        // clear dependency
        auto dependency = indirect_node->to_mutable().dependency();
        if (dependency && dependency == attribute) {
            foreach_trace(
                [&indirect_node](Trace &trace) { trace.set_dependency(indirect_node, AttributeID::make_nil()); });
            indirect_node->to_mutable().set_dependency(AttributeID(0)); // TODO: 0 or nullptr
            return true;
        }

        return false;
    }

    // reset source
    auto initial_source = indirect_node->to_mutable().initial_source();
    WeakAttributeID new_source = {AttributeID::make_nil(), 0};
    uint32_t new_offset = 0;
    if (initial_source.attribute().without_kind() != 0 && !initial_source.expired()) {
        new_source = initial_source;
        new_offset = indirect_node->to_mutable().initial_offset();
    }

    foreach_trace(
        [&indirect_node, &new_source](Trace &trace) { trace.set_source(indirect_node, new_source.attribute()); });
    indirect_node->modify(new_source, new_offset);

    if (new_source.attribute().without_kind() != 0 && !new_source.expired()) {
        add_input_dependencies(output, new_source.attribute());
    }

    return true;
}

#pragma mark - Marks

void Graph::mark_changed(data::ptr<Node> node, AttributeType *_Nullable type, const void *destination_value,
                         const void *source_value) {
    if (!_traces.empty()) {
        mark_changed(AttributeID(node), type, destination_value, source_value, 0);
        return;
    }

    uint32_t output_index = 0;
    for (auto output : node->outputs()) {
        if (!output.value.is_direct()) {
            mark_changed(AttributeID(node), type, destination_value, source_value, output_index);
            return;
        }
        for (auto input : output.value.to_node().inputs()) {
            if (input.value.resolve(TraversalOptions::None).attribute() != node) {
                continue;
            }
            if (input.is_pending()) {
                continue;
            }
            if (!input.value.is_direct()) {
                if (compare_edge_values(input, type, destination_value, source_value)) {
                    continue;
                }
            }
            input.set_pending(true);
        }
        output_index += 1;
    }

    _mark_changed_count += 1;
}

void Graph::mark_changed(AttributeID attribute, AttributeType *_Nullable type, const void *destination_value,
                         const void *source_value, uint32_t start_output_index) {
    if (attribute.is_nil()) {
        return;
    }

    // TODO: combine logic with propagate_dirty
    struct Frame {
        ConstOutputEdgeArrayRef outputs;
        AttributeID attribute;
    };

    char stack_buffer[0x2000] = {};
    auto heap = util::Heap(stack_buffer, sizeof(stack_buffer), 0);
    auto frames = util::ForwardList<Frame>(&heap);

    data::vector<OutputEdge> initial_outputs = {};
    if (attribute.is_direct()) {
        initial_outputs = attribute.to_node().outputs();
    } else if (attribute.is_indirect()) {
        // TODO: how to make sure indirect is mutable?
        initial_outputs = attribute.to_indirect_node().to_mutable().outputs();
    } else {
        return;
    }
    frames.emplace_front(ConstOutputEdgeArrayRef(&initial_outputs.front(), initial_outputs.size()), attribute);

    while (!frames.empty()) {
        auto outputs = frames.front().outputs;
        auto attribute = frames.front().attribute;
        frames.pop_front();

        for (auto output_edge = outputs.begin() + start_output_index, end = outputs.end(); output_edge != end;
             ++output_edge) {

            if (output_edge->value.is_direct()) {
                auto inputs = output_edge->value.to_node().inputs();
                for (uint32_t input_index = 0, num_inputs = inputs.size(); input_index < num_inputs; ++input_index) {
                    auto input_edge = inputs[input_index];
                    if (input_edge.value.resolve(TraversalOptions::SkipMutableReference).attribute() != attribute) {
                        continue;
                    }
                    if (input_edge.is_pending()) {
                        continue;
                    }
                    if (!input_edge.value.is_direct()) {
                        if (compare_edge_values(input_edge, type, destination_value, source_value)) {
                            continue;
                        }
                    }
                    foreach_trace([&output_edge, &input_index](Trace &trace) {
                        trace.set_edge_pending(output_edge->value.to_node_ptr(), input_index, true);
                    });
                    input_edge.set_pending(true);
                }
            } else if (output_edge->value.is_indirect()) {
                if (output_edge->value.to_indirect_node().to_mutable().dependency() != attribute) {
                    auto mutable_node = output_edge->value.to_indirect_node().to_mutable();
                    frames.emplace_front(
                        ConstOutputEdgeArrayRef(&mutable_node.outputs().front(), mutable_node.outputs().size()),
                        output_edge->value);
                }
            }
        }

        start_output_index = 0;
    }

    _mark_changed_count += 1;
}

void Graph::mark_pending(data::ptr<Node> node_ptr, Node *node) {
    if (!node->state().is_pending()) {
        foreach_trace([&node_ptr](Trace &trace) { trace.set_pending(node_ptr, true); });
        node->set_state(node->state().with_pending(true));
    }
    if (!node->state().is_dirty()) {
        foreach_trace([&node_ptr](Trace &trace) { trace.set_dirty(node_ptr, true); });
        node->set_state(node->state().with_dirty(true));

        uint8_t dirty_flags = node->flags().value3();
        Subgraph *subgraph = AttributeID(node_ptr).subgraph();
        if (dirty_flags && subgraph != nullptr) {
            subgraph->add_dirty_flags(dirty_flags);
        }

        propagate_dirty(node_ptr);
    }
}

#pragma mark - Interning

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

const char *Graph::key_name(uint32_t key_id) {
    if (_keys != nullptr && key_id < _keys->size()) {
        return _keys->get(key_id);
    }
    AG::precondition_failure("invalid string key id: %u", key_id);
}

uint32_t Graph::intern_type(swift::metadata *metadata, ClosureFunctionVP<void *> make_type) {
    uint32_t type_id = uint32_t(reinterpret_cast<uintptr_t>(_type_ids_by_metadata.lookup(metadata, nullptr)));
    if (type_id) {
        return type_id;
    }

    AttributeType *type = (AttributeType *)make_type();
    type->update_attribute_offset();

    static bool prefetch_layouts = []() -> bool {
        char *result = getenv("AG_PREFETCH_LAYOUTS");
        if (result) {
            return atoi(result) != 0;
        }
        return false;
    }();
    if (prefetch_layouts) {
        type->update_layout();
    }

    type_id = _types.size();
    if (type_id >= 0xffffff) {
        precondition_failure("overflowed max type id: %u", type_id);
    }
    _types.push_back(type);
    _type_ids_by_metadata.insert(metadata, reinterpret_cast<void *>(uintptr_t(type_id)));

    size_t self_size = type->self_metadata().vw_size();
    if (self_size >= 0x2000) {
        os_log_info(misc_log(), "large attribute self: %u bytes, %s", uint(self_size),
                    type->self_metadata().name(false));
    }

    size_t value_size = type->value_metadata().vw_size();
    if (value_size >= 0x2000) {
        os_log_info(misc_log(), "large attribute value: %u bytes, %s -> %s", uint(value_size),
                    type->self_metadata().name(false), type->value_metadata().name(false));
    }

    return type_id;
}

#pragma mark - Encoding

void Graph::encode_node(Encoder &encoder, const Node &node, bool flag) {
    // TODO: not implemented
}

void Graph::encode_indirect_node(Encoder &encoder, const IndirectNode &indirect_node) {
    // TODO: not implemented
}

void Graph::encode_tree(Encoder &encoder, data::ptr<TreeElement> tree) {
    // TODO: not implemented
}

#pragma mark - Tracing

void Graph::prepare_trace(Trace &trace) {
    _contexts_by_id.for_each([](const uint64_t context_id, Context *const context,
                                void *trace_ref) { ((Trace *)trace_ref)->created(*context); },
                             &trace);

    for (auto subgraph : _subgraphs) {
        trace.created(*subgraph);
    }
    for (auto subgraph : _subgraphs) {
        for (auto child : subgraph->children()) {
            trace.add_child(*subgraph, *child.subgraph());
        }
    }
    for (auto subgraph : _subgraphs) {
        for (uint32_t iteration = 0; iteration < 2; ++iteration) {
            for (data::ptr<data::page> page = subgraph->last_page(); page != nullptr; page = page->previous) {
                bool should_break = false;
                uint16_t relative_offset = iteration == 0 ? page->relative_offset_2 : page->relative_offset_1;
                while (relative_offset) {
                    AttributeID attribute = AttributeID(page + relative_offset);

                    if (attribute.is_direct()) {
                        relative_offset = attribute.to_node().flags().relative_offset();
                    } else if (attribute.is_indirect()) {
                        relative_offset = attribute.to_indirect_node().relative_offset();
                    } else if (attribute.is_nil()) {
                        relative_offset = 0;
                        should_break = true;
                        break;
                    } else {
                        relative_offset = 0;
                    }

                    if (attribute.is_direct()) {
                        auto node = attribute.to_node_ptr();
                        trace.added(node);
                        if (node->state().is_dirty()) {
                            trace.set_dirty(node, true);
                        }
                        if (node->state().is_pending()) {
                            trace.set_pending(node, true);
                        }
                        if (node->state().is_value_initialized()) {
                            void *value = node->get_value();
                            trace.set_value(node, value);
                        }
                    } else if (attribute.is_indirect()) {
                        auto indirect_node = attribute.to_indirect_node_ptr();
                        trace.added(indirect_node);
                    }
                }
                if (should_break) {
                    break;
                }
            }
        }
    }
    for (auto subgraph : _subgraphs) {
        for (uint32_t iteration = 0; iteration < 2; ++iteration) {
            for (data::ptr<data::page> page = subgraph->last_page(); page != nullptr; page = page->previous) {
                bool should_break = false;
                uint16_t relative_offset = iteration == 0 ? page->relative_offset_2 : page->relative_offset_1;
                while (relative_offset) {
                    AttributeID attribute = AttributeID(page + relative_offset);

                    if (attribute.is_direct()) {
                        relative_offset = attribute.to_node().flags().relative_offset();
                    } else if (attribute.is_indirect()) {
                        relative_offset = attribute.to_indirect_node().relative_offset();
                    } else if (attribute.is_nil()) {
                        relative_offset = 0;
                        should_break = true;
                        break;
                    } else {
                        relative_offset = 0;
                    }

                    if (attribute.is_direct()) {
                        auto node = attribute.to_node_ptr();
                        uint32_t edge_index = 0;
                        for (auto input_edge : node->inputs()) {
                            trace.add_edge(node, input_edge.value, input_edge.is_unknown2());
                            if (input_edge.is_pending()) {
                                trace.set_edge_pending(node, edge_index, true);
                            }
                            edge_index += 1;
                        }
                    } else if (attribute.is_indirect()) {
                        auto indirect_node = attribute.to_indirect_node_ptr();
                        trace.set_source(indirect_node, indirect_node->source().attribute());
                        if (indirect_node->is_mutable() && indirect_node->to_mutable().dependency() != 0) {
                            trace.set_dependency(indirect_node, indirect_node->to_mutable().dependency());
                        }
                    }
                }
                if (should_break) {
                    break;
                }
            }
        }
    }
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
                               [&trace_id](auto trace) -> bool { return trace->trace_id() == trace_id; });
    Trace *trace = *iter;
    trace->end_trace(*this);
    // destructor?
    _traces.erase(iter);
}

void Graph::start_tracing(uint8_t options, std::span<const char *> subsystems) {
    if (options & TraceRecorder::Options::CreateIfNeeded && _trace_recorder == nullptr) {
        _trace_recorder = new TraceRecorder(this, options, subsystems);
        if (options & TraceRecorder::Options::PrepareTrace) {
            prepare_trace(*_trace_recorder);
        }
        add_trace(_trace_recorder);
        
        // TODO: cleanup block
    }
}

void Graph::stop_tracing() {
    if (_trace_recorder) {
        remove_trace(_trace_recorder->unique_id());
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

void Graph::all_start_tracing(uint32_t arg, std::span<const char *, UINT64_MAX> span) {
    all_lock();
    for (auto graph = _all_graphs; graph != nullptr; graph = graph->_next) {
        graph->start_tracing(arg, span);
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

#pragma mark - Printing

void Graph::print() {
    // TODO: not implemented
}

void Graph::print_attribute(data::ptr<Node> node) {
    // TODO: not implemented
}

void Graph::print_cycle(data::ptr<Node> node) {
    // TODO: not implemented
}

void Graph::print_data() {
    data::table::shared().print();
    data::zone::print_header();
    for (auto subgraph : _subgraphs) {
        subgraph->data::zone::print(); // TODO: make first field..
    }
}

void Graph::print_stack() {
    // TODO: not implemented
}

} // namespace AG
