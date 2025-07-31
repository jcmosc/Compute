#include "UpdateStack.h"

#include <ranges>

#include "Attribute/AttributeData/Node/IndirectNode.h"
#include "Attribute/AttributeID/OffsetAttributeID.h"
#include "Subgraph/Subgraph.h"
#include "Trace/Trace.h"

namespace AG {

Graph::UpdateStack::UpdateStack(Graph *graph, AGGraphUpdateOptions options)
    : _graph(graph), _thread(pthread_self()), _next(current_update()), _next_thread(graph->_current_update_thread),
      _options(options) {

    if (_next != nullptr) {
        _options =
            AGGraphUpdateOptions(_options | (_next.get()->_options & AGGraphUpdateOptionsCancelIfPassedDeadline));
    }

    graph->_current_update_thread = _thread;

    if (graph->_deferring_subgraph_invalidation == false) {
        graph->_deferring_subgraph_invalidation = true;
        _options = AGGraphUpdateOptions(_options & AGGraphUpdateOptionsEndDeferringSubgraphInvalidationOnExit);
    }

    Graph::set_current_update(util::tagged_ptr<UpdateStack>(this, options & AGGraphUpdateOptionsInitializeCleared));
}

Graph::UpdateStack::~UpdateStack() {
    for (auto &frame : _frames) {
        frame.attribute->set_updating(false);
    }

    if (_thread != _graph->_current_update_thread) {
        non_fatal_precondition_failure("invalid graph update (access from multiple threads?)");
    }

    _graph->_current_update_thread = _next_thread;
    Graph::set_current_update(_next);

    if (_options & AGGraphUpdateOptionsEndDeferringSubgraphInvalidationOnExit) {
        _graph->_deferring_subgraph_invalidation = false;
    }
}

Graph::UpdateStack::Frame *Graph::UpdateStack::global_top() {
    for (auto update = this; update != nullptr; update = update->next().get()) {
        if (!update->frames().empty()) {
            return &update->frames().back();
        }
    }
    return nullptr;
}

void Graph::UpdateStack::cancel() {
    for (auto update = current_update(); update != nullptr; update = update.get()->next()) {
        for (auto &frame : std::ranges::reverse_view(update.get()->frames())) {
            if (frame.cancelled) {
                return;
            }
            frame.cancelled = true;
        }
        if (update.get()->_options & AGGraphUpdateOptionsAbortIfCancelled) {
            break;
        }
    }
}

bool Graph::UpdateStack::cancelled() {
    for (auto update = current_update(); update != nullptr; update = update.get()->next()) {
        if (!update.get()->frames().empty()) {
            auto &last_frame = update.get()->frames().back();
            return last_frame.cancelled;
        }
    }
    return false;
}

bool Graph::UpdateStack::push(data::ptr<Node> node_ptr, Node &node, bool ignore_cycles, bool initialize_value) {
    if (!node.is_updating() && _frames.size() + 1 <= _frames.capacity()) {
        node.set_updating(true);

        Frame frame = Frame(node_ptr);
        if (node.is_pending() || (!node.is_value_initialized() && initialize_value)) {
            frame.pending = true;
        }

        _frames.push_back(frame);
        return true;
    }

    return push_slow(node_ptr, node, ignore_cycles, initialize_value);
}

bool Graph::UpdateStack::push_slow(data::ptr<Node> node_ptr, Node &node, bool ignore_cycles, bool initialize_value) {
    NodeState old_state = node.state();

    if ((old_state & (NodeState::Updating | old_state & NodeState::UpdatingCyclic)) != NodeState(0)) {
        if (ignore_cycles) {
            return false;
        }

        Frame *top = global_top();
        if (top != nullptr && !top->cyclic) {
            // First time we are detecting a cycle for this attribute
            this->_graph->print_cycle(node_ptr);
        }

        if (node.is_value_initialized()) {
            return false;
        }

        // update default
        const AttributeType &attribute_type = _graph->attribute_type(node.type_id());
        if (auto updateDefault = attribute_type.vtable().update_default) {

            Frame frame = Frame(node_ptr);
            if (node.is_pending() || (!node.is_value_initialized() && initialize_value)) {
                frame.pending = true;
            }

            _frames.push_back(frame);

            void *self = node.get_self(attribute_type);
            updateDefault(reinterpret_cast<const AGAttributeType *>(&attribute_type), self);

            _frames.pop_back();

            if (node.is_value_initialized()) {
                return false;
            }
        }

        if ((old_state & NodeState::UpdatingCyclic) != NodeState(0)) {
            precondition_failure("cyclic graph: %u", node_ptr);
        }
    }

    node.set_updating(true);

    Frame frame = Frame(node_ptr);
    if (node.is_pending() || (!node.is_value_initialized() && initialize_value)) {
        frame.pending = true;
    }
    if ((old_state & (NodeState::Updating | old_state & NodeState::UpdatingCyclic)) != NodeState(0)) {
        frame.cyclic = true;
    }
    _frames.push_back(frame);

    return true;
}

Graph::UpdateStatus Graph::UpdateStack::update() {
    while (true) {
        Frame &frame = _frames.back();
        data::ptr<Node> node = frame.attribute;

        // Check cancelled

        if (_options & AGGraphUpdateOptionsCancelIfPassedDeadline) {
            if (!frame.cancelled && _graph->passed_deadline()) {
                cancel();
            }
        }

        if (frame.cancelled) {
            if (_options & AGGraphUpdateOptionsAbortIfCancelled) {
                return UpdateStatus::Aborted;
            }

            bool changed = false;
            if (!node->is_value_initialized()) {

                // update default
                const AttributeType &attribute_type = _graph->attribute_type(node->type_id());
                if (auto callback = attribute_type.vtable().update_default) {
                    void *self = node->get_self(attribute_type);
                    callback(reinterpret_cast<const AGAttributeType *>(&attribute_type), self);
                    changed = true;
                }

                if (node->is_value_initialized()) {
                    node->set_updating(false);

                    _frames.pop_back();
                    if (_frames.empty()) {
                        return changed ? UpdateStatus::Changed : UpdateStatus::Unchanged;
                    }
                }
            }
        }

        if (!frame.pending && frame.num_pushed_inputs > 0 &&
            node->input_edges()[frame.num_pushed_inputs - 1].options & AGInputOptionsChanged) {
            frame.pending = true;
        }

        // Push inputs

        for (auto input_index = frame.num_pushed_inputs, num_inputs = node->input_edges().size();
             input_index != num_inputs; ++input_index) {
            const InputEdge &input_edge = node->input_edges()[input_index];

            AttributeID input_attribute = input_edge.attribute;
            while (input_attribute.is_indirect_node()) {
                // TODO: warning: variable 'input_attribute' is uninitialized when used
                // within its own initialization
                auto input_attribute_source = input_attribute.get_indirect_node()->source().identifier();
                if (input_attribute.get_indirect_node()->is_mutable()) {
                    if (AttributeID dependency = input_attribute.get_indirect_node()->to_mutable().dependency()) {
                        if (!dependency.get_node()->is_value_initialized() || dependency.get_node()->is_dirty()) {
                            frame.num_pushed_inputs = input_index;
                            push(dependency.get_node(), *dependency.get_node().get(), false, false);
                            // go to top regardless
                            return update();
                        }
                    }
                }
                input_attribute = input_attribute_source;
            }

            if (auto input_node = input_attribute.get_node()) {
                if (input_edge.options & AGInputOptionsChanged) {
                    frame.pending = true;
                }

                if (!input_node->is_value_initialized() || input_node->is_dirty()) {

                    if (!(input_edge.options & AGInputOptionsChanged) && input_attribute.subgraph()->is_valid()) {
                        frame.num_pushed_inputs = input_index + 1;
                        if (push(input_node, *input_node.get(), true, true)) {
                            // go to top
                            return update();
                        }
                    }

                    frame.pending = true;
                }
            }
        }

        // Update value

        bool changed = false;
        if (frame.pending) {
            if (_graph->has_main_handler() && node->is_main_thread()) {
                return Graph::UpdateStatus::NeedsCallMainHandler;
            }

            _graph->foreach_trace([&frame](Trace &trace) { trace.begin_update(frame.attribute); });
            uint64_t old_change_count = _graph->_change_count;

            const AttributeType &attribute_type = _graph->attribute_type(node->type_id());
            void *self = node->get_self(attribute_type);

            attribute_type.update(self, AGAttribute(AttributeID(frame.attribute)));

            if (!node->is_value_initialized()) {
                if (attribute_type.value_metadata().vw_size() > 0) {
                    precondition_failure("attribute failed to set an initial value: %u, %s", frame.attribute,
                                         attribute_type.body_metadata().name(false));
                }

                // set a dummy 0-byte value
                struct {
                } value = {};
                AGGraphSetOutputValue(&value, AGTypeID(&attribute_type.value_metadata()));
            }

            changed = _graph->_change_count != old_change_count;
            _graph->foreach_trace([&frame, &changed](Trace &trace) { trace.end_update(frame.attribute, changed); });
        }

        // Reset flags

        bool reset_node_flags = true;
        if (frame.pending || frame.flag3 || frame.cancelled) {
            reset_node_flags = (frame.flag3 || frame.cancelled) ? !frame.cancelled : true;

            uint32_t input_index = node->input_edges().size();
            for (InputEdge &input_edge : std::ranges::reverse_view(node->input_edges())) {
                input_index -= 1;

                bool reset_edge_pending = true;
                if (frame.flag3 || frame.cancelled) {
                    auto input_attribute = input_edge.attribute.resolve(TraversalOptions::None).attribute();
                    if (!input_attribute.is_node() || input_attribute.get_node()->is_dirty()) {
                        reset_edge_pending = false;
                        reset_node_flags = false;
                    }
                }

                if (reset_edge_pending) {
                    if (frame.pending && !frame.cancelled) {
                        if (input_edge.options & AGInputOptionsChanged) {
                            _graph->foreach_trace([&frame, &input_edge](Trace &trace) {
                                trace.set_edge_pending(frame.attribute, input_edge.attribute, false);
                            });
                            input_edge.options &= ~AGInputOptionsChanged;
                        }
                    }
                }

                if (frame.pending && !frame.cancelled) {
                    bool was_enabled = input_edge.options & AGInputOptionsEnabled;
                    input_edge.options &= ~AGInputOptionsEnabled;
                    if (!was_enabled && !(input_edge.options & AGInputOptionsAlwaysEnabled)) {
                        _graph->remove_input(frame.attribute, input_index);
                    }
                }
            }
        }

        node->set_updating(false);

        if (reset_node_flags) {
            if (node->is_self_modified()) {
                node->set_self_modified(false);
            }
            if (node->is_dirty()) {
                _graph->foreach_trace([&frame](Trace &trace) { trace.set_dirty(frame.attribute, false); });
                node->set_dirty(false);
            }
            node->set_main_thread(node->requires_main_thread());
        }

        if (node->is_pending()) {
            _graph->foreach_trace([&frame](Trace &trace) { trace.set_pending(frame.attribute, false); });
            node->set_pending(false);
        }

        _frames.pop_back();
        if (_frames.empty()) {
            return UpdateStatus::Changed;
        }
    }
}

} // namespace AG
