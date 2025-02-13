#include "UpdateStack.h"

#include "AGGraph.h"
#include "Attribute/AttributeType.h"
#include "Attribute/Node/IndirectNode.h"
#include "Attribute/Node/Node.h"
#include "Attribute/OffsetAttributeID.h"
#include "Subgraph/Subgraph.h"
#include "Trace/Trace.h"

namespace AG {

Graph::UpdateStack::UpdateStack(Graph *graph, uint8_t options) {
    _graph = graph;
    _thread = pthread_self();
    _previous = current_update();
    _previous_thread = graph->_current_update_thread;

    _options = options;
    if (_previous != nullptr) {
        _options = _options | (_previous.get()->_options & 4);
    }

    graph->_current_update_thread = _thread;

    if (graph->_batch_invalidate_subgraphs == false) {
        graph->_batch_invalidate_subgraphs = true;
        _options &= Option::InvalidateSubgraphs;
    }

    Graph::set_current_update(TaggedPointer<UpdateStack>(this, options & Option::SetTag ? 1 : 0));
}

Graph::UpdateStack::~UpdateStack() {
    for (auto frame : _frames) {
        frame.attribute->set_state(frame.attribute->state().with_updating(false));
    }

    if (_thread != _graph->_current_update_thread) {
        non_fatal_precondition_failure("invalid graph update (access from multiple threads?)");
    }

    _graph->_current_update_thread = _previous_thread;
    Graph::set_current_update(_previous);

    if (_options & Option::InvalidateSubgraphs) {
        _graph->_batch_invalidate_subgraphs = false;
    }
}

Graph::UpdateStack::Frame *Graph::UpdateStack::global_top() {
    for (TaggedPointer<Graph::UpdateStack> update_stack = this; update_stack != nullptr;
         update_stack = update_stack.get()->previous()) {
        if (!update_stack.get()->frames().empty()) {
            return &update_stack.get()->frames().back();
        }
    }
    return nullptr;
}

void Graph::UpdateStack::cancel() {
    for (TaggedPointer<Graph::UpdateStack> update_stack = current_update(); update_stack != nullptr;
         update_stack = update_stack.get()->previous()) {
        auto frames = update_stack.get()->frames();
        for (auto frame = frames.rbegin(), end = frames.rend(); frame != end; ++frame) {
            if (frame->cancelled) {
                return;
            }
            frame->cancelled = true;
        }
        if (update_stack.get()->_options & 2) {
            break;
        }
    }
}

bool Graph::UpdateStack::cancelled() {
    for (TaggedPointer<Graph::UpdateStack> update_stack = this; update_stack != nullptr;
         update_stack = update_stack.get()->previous()) {
        if (!update_stack.get()->frames().empty()) {
            auto last_frame = update_stack.get()->frames().back();
            return last_frame.cancelled;
        }
    }
    return false;
}

bool Graph::UpdateStack::push(data::ptr<Node> attribute, Node &node, bool flag1, bool treat_no_value_as_pending) {
    if (!node.state().is_updating() && _frames.size() + 1 <= _frames.capacity()) {
        node.set_state(node.state().with_updating(true));

        Graph::UpdateStack::Frame frame = {attribute, 0};
        if (node.state().is_pending() || (!node.state().is_value_initialized() && treat_no_value_as_pending)) {
            frame.needs_update = true;
        }
        _frames.push_back(frame);
        return true;
    }

    return push_slow(attribute, node, flag1, treat_no_value_as_pending);
}

bool Graph::UpdateStack::push_slow(data::ptr<Node> attribute, Node &node, bool ignore_cycles,
                                   bool treat_no_value_as_pending) {
    Node::State old_state = node.state();

    if (old_state.is_updating()) {
        if (ignore_cycles) {
            return false;
        }

        Graph::UpdateStack::Frame *top = global_top();
        if (top != nullptr && !top->cyclic) {
            // First time we are detecting a cycle for this attribute
            this->_graph->print_cycle(attribute);
        }

        if (node.state().is_value_initialized()) {
            return false;
        }

        const AttributeType &attribute_type = _graph->attribute_type(node.type_id());
        auto callback = attribute_type.vt_get_update_stack_callback();
        if (callback != nullptr) {

            Graph::UpdateStack::Frame frame = {attribute, 0};
            if (node.state().is_pending() || (!node.state().is_value_initialized() && treat_no_value_as_pending)) {
                frame.needs_update = true;
            }

            _frames.push_back(frame);

            void *self = node.get_self(attribute_type);
            callback(&attribute_type, self);

            _frames.pop_back();

            if (node.state().is_value_initialized()) {
                return false;
            }
        }

        if (old_state.is_updating_cyclic()) {
            precondition_failure("cyclic graph: %u", attribute);
        }
    }

    node.set_state(node.state().with_updating(true));

    Graph::UpdateStack::Frame frame = {attribute, 0};
    if (node.state().is_pending() || (!node.state().is_value_initialized() && treat_no_value_as_pending)) {
        frame.needs_update = true;
    }
    if (old_state.is_updating()) {
        frame.cyclic = true;
    }
    _frames.push_back(frame);

    return true;
}

Graph::UpdateStatus Graph::UpdateStack::update() {

    while (true) {
        auto frame = _frames.back();
        Node *node = frame.attribute.get();

        if (_options >> 2) {
            if (!frame.cancelled && _graph->passed_deadline()) {
                cancel();
            }
        }

        if (frame.cancelled) {
            if (_options >> 1) {
                return UpdateStatus::Option2;
            }

            bool changed = false;
            if (!node->state().is_value_initialized()) {
                const AttributeType &type = _graph->attribute_type(node->type_id());
                void *self = node->get_self(type);
                if (auto callback = type.vt_get_update_stack_callback()) {
                    callback(&type, self);
                    changed = true;
                }
                if (node->state().is_value_initialized()) {
                    node->set_state(node->state().with_updating(false));
                    _frames.pop_back();
                    if (_frames.empty()) {
                        return changed ? UpdateStatus::Changed : UpdateStatus::NoChange;
                    }
                }
            }
        }

        if (!frame.needs_update && frame.num_pushed_inputs > 0 &&
            node->inputs()[frame.num_pushed_inputs - 1].is_pending()) {
            frame.needs_update = true;
        }

        // Push inputs

        for (auto input_index = frame.num_pushed_inputs, num_inputs = node->inputs().size(); input_index != num_inputs;
             ++input_index) {
            InputEdge &input = node->inputs()[input_index];

            AttributeID input_attribute = input.value;
            while (input_attribute.is_indirect()) {
                AttributeID input_attribute = input_attribute.to_indirect_node().source().attribute();

                if (input_attribute.to_indirect_node().is_mutable()) {
                    // TDOO: is dependency always direct??
                    if (AttributeID dependency = input_attribute.to_indirect_node().to_mutable().dependency()) {
                        if (dependency.to_node().state().is_dirty() ||
                            !dependency.to_node().state().is_value_initialized()) {

                            frame.num_pushed_inputs = input_index;

                            push(dependency.to_node_ptr(), dependency.to_node(), false, false);
                            // go to top regardless
                            return update();
                        }
                    }
                }
            }

            if (input_attribute.is_direct()) {
                Node &input_node = input_attribute.to_node();

                if (input.is_pending()) {
                    frame.needs_update = true;
                }

                if (input_node.state().is_dirty() || !input_node.state().is_value_initialized()) {

                    if (!input.is_pending() &&
                        input_attribute.subgraph()->validation_state() == Subgraph::ValidationState::Valid) {

                        frame.num_pushed_inputs = input_index + 1;

                        if (push(input_attribute.to_node_ptr(), input_node, true, true)) {
                            // go to top
                            return update();
                        }
                    }

                    frame.needs_update = true;
                }
            }
        }

        // Update value

        bool changed = false;
        if (frame.needs_update) {
            if (_graph->main_handler() != nullptr && node->state().is_main_thread()) {
                return Graph::UpdateStatus::NeedsCallMainHandler;
            }

            _graph->foreach_trace([&frame](Trace &trace) { trace.begin_update(frame.attribute); });
            uint64_t mark_changed_count = _graph->_mark_changed_count;

            const AttributeType &type = _graph->attribute_type(node->type_id());
            void *self = node->get_self(type);

            type.perform_update(self);

            if (!node->state().is_value_initialized()) {
                if (type.value_metadata().vw_size() > 0) {
                    precondition_failure("attribute failed to set an initial value: %u, %s", frame.attribute,
                                         type.self_metadata().name(false));
                }

                struct {
                } value = {};
                AGGraphSetOutputValue(&value, AGTypeID(&type.value_metadata()));
            }

            changed = _graph->_mark_changed_count != mark_changed_count;
            _graph->foreach_trace([&frame, &changed](Trace &trace) { trace.end_update(frame.attribute, changed); });
        }

        // Reset flags

        bool reset_node_flags = !frame.cancelled;
        for (uint32_t input_index = node->inputs().size() - 1; input_index >= 0; --input_index) {
            InputEdge &input = node->inputs()[input_index];
            AttributeID input_attribute = input.value;

            bool reset_input_flags = true;
            if (frame.flag3 || frame.cancelled) {
                input_attribute = input_attribute.resolve(TraversalOptions::None).attribute();
                if (!input_attribute.is_direct() || input_attribute.to_node().state().is_dirty()) {
                    reset_input_flags = false;
                    reset_node_flags = false;
                }
            }

            if (reset_input_flags) {
                if (frame.needs_update && !frame.cancelled) {
                    if (input.is_pending()) {
                        _graph->foreach_trace([&frame, &input](Trace &trace) {
                            trace.set_edge_pending(frame.attribute, input.value, false);
                        });
                        input.set_pending(false);
                    }
                }
            }

            if (frame.needs_update && !frame.cancelled) {
                bool was_unknown4 = input.is_unknown4();
                input.set_unknown4(false);
                if (!was_unknown4 && !input.is_unknown2()) {
                    _graph->remove_input(frame.attribute, input_index);
                }
            }
        }

        node->set_state(node->state().with_updating(false));

        if (reset_node_flags) {
            // only skips if frame.cancelled
            if (node->flags().value4_unknown0x40()) {
                node->flags().set_value4_unknown0x40(false);
            }
            if (node->state().is_dirty()) {
                _graph->foreach_trace([&frame](Trace &trace) { trace.set_dirty(frame.attribute, false); });
                node->set_state(node->state().with_dirty(false));
            }
            if (!node->state().is_main_thread_only()) {
                node->set_state(node->state().with_main_thread(false));
            }
        }

        if (node->state().is_pending()) {
            _graph->foreach_trace([&frame](Trace &trace) { trace.set_pending(frame.attribute, false); });
            node->set_state(node->state().with_pending(false));
        }

        _frames.pop_back();
        if (_frames.empty()) {
            return changed ? UpdateStatus::Changed : UpdateStatus::NoChange;
        }
    }
}

} // namespace AG
