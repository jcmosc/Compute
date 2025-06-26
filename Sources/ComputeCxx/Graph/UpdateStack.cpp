#include "UpdateStack.h"

#include <ranges>

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
    for (auto frame : _frames) {
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
            auto last_frame = update.get()->frames().back();
            return last_frame.cancelled;
        }
    }
    return false;
}

} // namespace AG
