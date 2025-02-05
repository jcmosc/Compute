#include "UpdateStack.h"

#include "Attribute/AttributeType.h"
#include "Attribute/Node/Node.h"

namespace AG {

Graph::UpdateStack::Frame *Graph::UpdateStack::global_top() {
    for (TaggedPointer<Graph::UpdateStack> update_stack = this; update_stack != nullptr;
         update_stack = update_stack.get()->previous()) {
        if (!update_stack.get()->frames().empty()) {
            return &update_stack.get()->frames().back();
        }
    }
    return nullptr;
}

bool Graph::UpdateStack::push(data::ptr<Node> attribute, Node &node, bool flag1, bool flag2) {
    if (!node.state().is_evaluating()) {
        node.set_state(node.state().with_in_update_stack(true));

        // TODO: check where capacity is being checked
        uint32_t frame_flags = 0;
        if (node.state().is_pending() || (!node.state().is_value_initialized() && flag2)) {
            frame_flags = 1;
        }
        _frames.push_back({attribute, frame_flags});
        return true;
    }

    return push_slow(attribute, node, flag1, flag2);
}

bool Graph::UpdateStack::push_slow(data::ptr<Node> attribute, Node &node, bool flag1, bool flag2) {
    uint8_t is_evaluating = node.state().is_evaluating();

    if (is_evaluating) {
        // is already updating
        if (!flag1) {
            Graph::UpdateStack::Frame *top = global_top();
            if (top != nullptr && (top->flags & 2) == 0) {
                this->_graph->print_cycle(attribute);
            }

            if (!node.state().is_value_initialized()) {
                const AttributeType &attribute_type = _graph->attribute_type(node.type_id());

                auto callback = attribute_type.vt_get_update_stack_callback();
                if (callback != nullptr) {

                    Graph::UpdateStack::Frame frame = {attribute, 0};
                    if (node.state().is_pending() || (!node.state().is_value_initialized() && flag2)) {
                        frame.flags = 1;
                    }

                    _frames.push_back(frame);

                    void *self = node.get_self(attribute_type);
                    callback(&attribute_type, self);

                    _frames.pop_back();

                    if (node.state().is_value_initialized()) {
                        return false;
                    }
                }
                if (is_evaluating == 3) { // both flags set
                    precondition_failure("cyclic graph: %u", attribute);
                }
            }
        }
    }

    node.set_state(node.state().with_in_update_stack(true));

    Graph::UpdateStack::Frame frame = {attribute, 0};
    if (node.state().is_pending() || (!node.state().is_value_initialized() && flag2)) {
        frame.flags = 1;
    }
    if (is_evaluating) {
        frame.flags |= 0x2; // mark so we can print cycles
    }
    _frames.push_back(frame);

    return true;
}

} // namespace AG
