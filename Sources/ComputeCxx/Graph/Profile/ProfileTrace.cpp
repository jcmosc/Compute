#include "ProfileTrace.h"

#include "Graph/UpdateStack.h"

namespace AG {

void Graph::ProfileTrace::begin_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node, uint32_t options) {
    if (update_stack.graph()->is_profiling()) {
        
    }
}

void Graph::ProfileTrace::end_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node,
                                     Graph::UpdateStatus update_status) {}

void Graph::ProfileTrace::begin_update(data::ptr<Node> node) {}

void Graph::ProfileTrace::end_update(data::ptr<Node> node, bool changed) {}

} // namespace AG
