#include "ProfileTrace.h"

#include <mach/mach_time.h>

#include "Graph/UpdateStack.h"

namespace AG {

void Graph::ProfileTrace::begin_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node, uint32_t options) {
    if (update_stack.graph()->is_profiling()) {
        uint64_t time = mach_absolute_time();
        UpdateData &data = _map.try_emplace(&update_stack).first->second;
        data.start_time = time;
        data.child_update_stack_duration = 0;
        data.update_node_start_time = 0;
    }
}

void Graph::ProfileTrace::end_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node,
                                     Graph::UpdateStatus update_status) {
    auto found = _map.find(&update_stack);
    if (found != _map.end()) {
        UpdateData &data = found->second;
        _map.erase(found);

        if (auto previous_update_stack = update_stack.previous().get()) {
            if (previous_update_stack->graph()->is_profiling()) {
                auto found_previous = _map.find(previous_update_stack);
                if (found_previous != _map.end()) {
                    UpdateData &previous_data = found_previous->second;
                    uint64_t end_time = previous_update_stack->graph()->is_profiling() ? mach_absolute_time() : 0;
                    previous_data.child_update_stack_duration += end_time - data.start_time;
                }
            }
        }
    }
}

void Graph::ProfileTrace::begin_update(data::ptr<Node> node) {
    auto update = current_update();
    UpdateStack *update_stack = update.tag() ? nullptr : update.get();

    auto found = _map.find(update_stack);
    if (found != _map.end()) {
        UpdateData &data = found->second;
        data.update_node_start_time = update_stack->graph()->is_profiling() ? mach_absolute_time() : 0;
    }
}

void Graph::ProfileTrace::end_update(data::ptr<Node> node, bool changed) {
    auto update = current_update();
    UpdateStack *update_stack = update.tag() ? nullptr : update.get();
    
    auto found = _map.find(update_stack);
    if (found != _map.end()) {
        UpdateData &data = found->second;
        if (data.update_node_start_time != 0) {
            uint64_t end_time = update_stack->graph()->is_profiling() ? mach_absolute_time() : 0;
            uint64_t duration = 0;
            if (end_time - data.update_node_start_time >= data.child_update_stack_duration) {
                duration = (end_time - data.update_node_start_time) - data.child_update_stack_duration;
            }
            data.child_update_stack_duration = 0;
            update_stack->graph()->add_profile_update(node, duration, changed);
        }
        
    }
}

} // namespace AG
