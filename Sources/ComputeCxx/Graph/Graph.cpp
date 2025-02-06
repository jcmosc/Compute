#include "Graph.h"

#include <mach/mach_time.h>
#include <os/log.h>
#include <set>

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
#include "Trace.h"
#include "UpdateStack.h"

namespace AG {

#pragma mark - Invalidating

Graph::without_invalidating::without_invalidating(Graph *graph) {
    _graph = graph;
    _graph_was_deferring_invalidation = graph->_deferring_invalidation;
    graph->_deferring_invalidation = true;
}

Graph::without_invalidating::~without_invalidating() {
    if (_graph && _graph_was_deferring_invalidation == false) {
        _graph->_deferring_invalidation = false;
        _graph->invalidate_subgraphs();
    }
}

#pragma mark - Context

#pragma mark - Subgraphs

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
    if (_deferring_invalidation) {
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
        _contexts_by_id.for_each([](uint64_t context_id, Context *graph_context,
                                    const void *closure_context) { graph_context->call_update(); },
                                 nullptr);
    }
}

void Graph::reset_update(data::ptr<Node> node) {
    for (auto update_stack = current_update(); update_stack != nullptr; update_stack = update_stack.get()->previous()) {
        for (auto frame : update_stack.get()->frames()) {
            if (frame.attribute == node) {
                frame.flags &= 0xf;
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

    void *effective_value = nullptr;
    if (type.use_graph_as_initial_value()) {
        effective_value = this;
    }
    if (value != nullptr || type.value_metadata().vw_size() != 0) {
        effective_value = value;
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
    *node = Node(Node::State(type.node_initial_state()), type_id, 0x20);

    if (type_id >= 0x100000) {
        precondition_failure("too many node types allocated");
    }

    void *self = (uint8_t *)node.get() + type.attribute_offset();

    node->set_state(node->state().with_self_initialized(true));
    if (node->state().is_unknown3() && !type.value_metadata().getValueWitnesses()->isPOD()) {
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
    _num_node_values += 1;

    if (type.self_metadata().vw_size() != 0) {
        void *self_dest = self;
        if (node->flags().has_indirect_self()) {
            self_dest = *(void **)self_dest;
        }
        type.self_metadata().vw_initializeWithCopy((swift::opaque_value *)self_dest, (swift::opaque_value *)body);
    }

    if (effective_value != nullptr) {
        value_set_internal(node, *node.get(), effective_value, type.value_metadata());
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
        return UpdateStatus::Option0;
    }

    _update_attribute_count += 1;
    if (node.state().updates_on_main()) {
        _update_attribute_on_main_count += 1;
    }

    UpdateStack update_stack = UpdateStack(this, options);

    foreach_trace([&update_stack, &attribute, &options](Trace &trace) {
        trace.begin_update(update_stack, attribute.to_node_ptr(), options);
    });

    UpdateStatus status = UpdateStatus::Option1;
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
                if (node.state().is_unknown3()) {
                    new_unknown0x20 = true;
                } else {
                    new_unknown0x20 =
                        std::any_of(node.inputs().begin(), node.inputs().end(), [](auto input_edge) -> bool {
                            auto resolved =
                                input_edge.value.resolve(AttributeID::TraversalOptions::EvaluateWeakReferences);
                            if (resolved.attribute().is_direct() &&
                                resolved.attribute().to_node().flags().value4_unknown0x20()) {
                                return true;
                            }
                        });
                }
            }
            if (node.flags().value4_unknown0x20() != new_unknown0x20) {
                node.flags().set_value4_unknown0x20(new_unknown0x20);
                output_edge_arrays.push_back(node.outputs());
            }
        } else if (attribute.is_indirect() && attribute.to_indirect_node().is_mutable()) {
            MutableIndirectNode &node = attribute.to_indirect_node().to_mutable();
            output_edge_arrays.push_back(node.outputs());
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
    if (node->state().is_evaluating()) {
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
    auto resolved = attribute.resolve(AttributeID::TraversalOptions::SkipMutableReference);
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
                    auto input =
                        input_edge.value.resolve(AttributeID::TraversalOptions::SkipMutableReference).attribute();

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
                source = source.resolve(AttributeID::TraversalOptions::SkipMutableReference).attribute();

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

    auto offset_attribute = attribute.resolve(AttributeID::TraversalOptions::SkipMutableReference);
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

void Graph::indirect_attribute_set(data::ptr<IndirectNode> attribute, AttributeID source) {
    if (!attribute->is_mutable()) {
        precondition_failure("not an indirect attribute: %u", attribute);
    }
    if (AttributeID(attribute).subgraph()->graph() != source.subgraph()->graph()) {
        precondition_failure("attribute references can't cross graph namespaces");
    }

    foreach_trace([&attribute, &source](Trace &trace) { trace.set_source(attribute, source); });

    OffsetAttributeID resolved_source = source.resolve(AttributeID::TraversalOptions::SkipMutableReference);

    // TODO: ...
}

void Graph::indirect_attribute_reset(data::ptr<IndirectNode> attribute, bool flag) {}

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
            remove_output_edge((data::ptr<Node>)old_dependency, indirect_attribute);
        }
        attribute->to_mutable().set_dependency(dependency);
        if (dependency) {
            add_output_edge((data::ptr<MutableIndirectNode>)dependency, indirect_attribute);
            if (dependency.to_node().state().is_dirty()) {
                propagate_dirty(indirect_attribute);
            }
        }
    }
}

#pragma mark - Values

// Status: Verified
bool Graph::value_exists(data::ptr<Node> node) { return node->state().is_value_initialized(); }

AGValueState Graph::value_state(AttributeID attribute) {
    if (!attribute.is_direct()) {
        auto resolved_attribute = attribute.resolve(AttributeID::TraversalOptions::AssertNotNil);
        attribute = resolved_attribute.attribute();
    }
    if (!attribute.is_direct()) {
        return 0;
    }

    auto node = attribute.to_node();
    return (node.state().is_dirty() ? 1 : 0) << 0 | (node.state().is_pending() ? 1 : 0) << 1 |
           (node.state().is_evaluating() ? 1 : 0) << 2 | (node.state().is_value_initialized() ? 1 : 0) << 3 |
           (node.state().updates_on_main() ? 1 : 0) << 4 | (node.flags().value4_unknown0x20() ? 1 : 0) << 5 |
           (node.state().is_unknown3() ? 1 : 0) << 6 | (node.flags().value4_unknown0x40() ? 1 : 0) << 7;
}

void Graph::value_mark(data::ptr<Node> node) {
    //    iVar2 = tpidrro_el0;
    //    uVar6 = *(uint *)(iVar2 + _current_update_key * 8);
    //    if (((((uVar6 & 1) == 0) &&
    //          (puVar7 = (undefined8 *)(uVar6 & 0xfffffffffffffffe), puVar7 != (undefined8 *)0x0)) &&
    //         ((Graph *)*puVar7 == this)) && (((*node & 0xc0) != 0 || (0x1f < node[5])))) {
    //        /* WARNING: Subroutine does not return */
    //        precondition_failure("setting value during update: %u",(char)node_ptr,in_w2);
    //    }

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
    //    iVar8 = tpidrro_el0;
    //    uVar5 = *(uint *)(iVar8 + _current_update_key * 8);
    //    if (1 < uVar5 && (uVar5 & 1) == 0) {
    //        /* WARNING: Subroutine does not return */
    //        precondition_failure("invalidating all values during update",in_w1,in_w2);
    //    }

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
                }

                if (attribute.is_direct()) {
                    auto node = attribute.to_node();
                    AttributeType &type = *_types[node.type_id()];
                    if (!type.use_graph_as_initial_value()) {
                        node.set_state(node.state().with_unknown3(true));
                        subgraph->add_dirty_flags(node.flags().value3());
                    }
                    for (auto input_edge : node.inputs()) {
                        input_edge.flags |= 8;
                    }
                }
            }
        }
    }
}

bool Graph::value_set(data::ptr<Node> node, const swift::metadata &value_type, const void *value) {
    if (!node->inputs().empty() && node->state().is_value_initialized()) {
        precondition_failure("can only set initial value of computed attributes: %u", node);
    }

    //    iVar1 = tpidrro_el0;
    //    uVar4 = *(uint *)(iVar1 + _current_update_key * 8);
    //    if (((((uVar4 & 1) == 0) &&
    //          (puVar5 = (undefined8 *)(uVar4 & 0xfffffffffffffffe), puVar5 != (undefined8 *)0x0)) &&
    //         ((Graph *)*puVar5 == this)) &&
    //        ((0x1f < node->child || ((node->lifecycle_flags & 0xc0U) != 0)))) {
    //        /* WARNING: Subroutine does not return */
    //        precondition_failure("setting value during update: %u",(char)attribute,(char)node);
    //    }

    bool changed = value_set_internal(node, *node.get(), value, value_type);
    if (changed) {
        propagate_dirty(node);
    }
    return changed;
}

bool Graph::value_set_internal(data::ptr<Node> node_ptr, Node &node, const void *value,
                               const swift::metadata &value_type) {
    foreach_trace([&node_ptr, &value](Trace &trace) { trace.set_value(node_ptr, value); });

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
        if (LayoutDescriptor::compare(layout, (const unsigned char *)value_dest, (const unsigned char *)value,
                                      value_type.vw_size(), comparison_options)) {
            return false;
        }

        if (_traces.empty()) {
            // TODO: finish
        } else {
            mark_changed(AttributeID(node_ptr), type, value_dest, value, 0);
        }

        value_type.vw_assignWithCopy((swift::opaque_value *)value_dest, (swift::opaque_value *)value);
    } else {
        // not initialized yet
        node.allocate_value(*this, *AttributeID(node_ptr).subgraph());

        // TODO: wrap in initialize_value on Node...
        node.set_state(node.state().with_value_initialized(true));
        mark_changed(node_ptr, nullptr, nullptr, nullptr);

        void *value_dest = node.get_value();
        value_type.vw_initializeWithCopy((swift::opaque_value *)value_dest, (swift::opaque_value *)value);
    }
}

// MARK: Marks

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

#pragma mark - Tracing

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

void Graph::trace_assertion_failure(bool all_stop_tracing, const char *format, ...) {
    // TODO: Not implemented
}

#pragma mark - Printing

void Graph::print_data() {
    data::table::shared().print();
    data::zone::print_header();
    for (auto subgraph : _subgraphs) {
        subgraph->data::zone::print(); // TODO: make first field..
    }
}

} // namespace AG
