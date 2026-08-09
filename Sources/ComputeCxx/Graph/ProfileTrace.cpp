#include "ProfileTrace.h"

#include "Graph/UpdateStack.h"

#include <mach/mach_time.h>

namespace IAG {

void Graph::ProfileTrace::begin_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node, uint32_t options) {
    if (!update_stack.graph()->is_profiling_enabled()) {
        return;
    }
    uint64_t start_time = mach_absolute_time();
    _update_data.insert({&update_stack, {start_time, 0, 0}});
}

void Graph::ProfileTrace::end_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node,
                                     IAGGraphUpdateStatus update_status) {
    auto iter = _update_data.find(&update_stack);
    if (iter == _update_data.end()) {
        return;
    }

    UpdateData &data = iter->second;
    uint64_t start_time = data.stack_start_time;
    _update_data.erase(iter);

    if (auto next_update_stack = update_stack.next().get()) {
        if (!next_update_stack->graph()->is_profiling_enabled()) {
            return;
        }
        auto parent_iter = _update_data.find(next_update_stack);
        if (parent_iter == _update_data.end()) {
            return;
        }
        uint64_t end_time = mach_absolute_time();
        UpdateData &parent_data = parent_iter->second;
        parent_data.child_stack_duration += end_time - start_time;
    }
}

void Graph::ProfileTrace::begin_update(data::ptr<Node> node) {
    auto update = current_update();
    UpdateStack *update_stack = update.tag() ? nullptr : update.get();

    auto iter = _update_data.find(update_stack);
    if (iter == _update_data.end()) {
        return;
    }

    uint64_t start_time = update_stack->graph()->is_profiling_enabled() ? mach_absolute_time() : 0;
    UpdateData &data = iter->second;
    data.update_start_time = start_time;
}

void Graph::ProfileTrace::end_update(data::ptr<Node> node, bool changed) {
    auto update = current_update();
    UpdateStack *update_stack = update.tag() ? nullptr : update.get();

    auto iter = _update_data.find(update_stack);
    if (iter == _update_data.end()) {
        return;
    }

    UpdateData &data = iter->second;
    if (data.update_start_time == 0) {
        return;
    }

    uint64_t end_time = update_stack->graph()->is_profiling_enabled() ? mach_absolute_time() : 0;
    uint64_t duration = end_time - data.update_start_time;
    if (duration >= data.child_stack_duration) {
        duration = duration - data.child_stack_duration;
    } else {
        duration = 0;
    }
    data.child_stack_duration = 0;
    update_stack->graph()->add_profile_update(node, duration, changed);
}

} // namespace IAG
