#include "Context.h"

#include "AGGraph-Private.h"
#include "Attribute/AttributeID/AttributeID.h"
#include "ComputeCxx/AGUniqueID.h"
#include "Subgraph/Subgraph.h"
#include "Trace/Trace.h"
#include "UpdateStack.h"

namespace AG {

Graph::Context::Context(Graph *graph) : _graph(graph), _id(AGMakeUniqueID()) {
    Graph::retain(graph);
    graph->_contexts_by_id.insert(_id, this);
    graph->foreach_trace([this](Trace &trace) { trace.created(*this); });
}

Graph::Context::~Context() {
    _graph->foreach_trace([this](Trace &trace) { trace.destroy(*this); });

    bool removed = _graph->_contexts_by_id.remove(_id);
    if (removed && _deadline != UINT64_MAX) {
        uint64_t min_deadline = UINT64_MAX;
        _graph->_contexts_by_id.for_each(
            [](const uint64_t context_id, Context *const context, void *min_deadline_ref) {
                if (context->_deadline < *(uint64_t *)min_deadline_ref) {
                    *(uint64_t *)min_deadline_ref = context->_deadline;
                }
            },
            &min_deadline);
        _graph->set_deadline(min_deadline);
    }

    if (_graph->_ref_count != 1) {
        auto batch = without_invalidating(_graph);
        for (auto subgraph : _graph->subgraphs()) {
            if (subgraph->context_id() == _id) {
                subgraph->invalidate_and_delete_(true);
            }
        }
        // ~without_invalidating
    }
    Graph::release(_graph);
}

Graph::Context *Graph::Context::from_cf(AGGraphStorage *storage) {
    if (storage->context._invalidated) {
        precondition_failure("invalidated graph");
    }
    return &storage->context;
}

void Graph::Context::set_deadline(uint64_t deadline) {
    if (_deadline == deadline) {
        return;
    }

    _deadline = deadline;
    _graph->foreach_trace([this, &deadline](Trace &trace) { trace.set_deadline(deadline); });

    uint64_t min_deadline = UINT64_MAX;
    _graph->_contexts_by_id.for_each(
        [](const uint64_t context_id, Context *const context, void *min_deadline_ref) {
            if (context->_deadline < *(uint64_t *)min_deadline_ref) {
                *(uint64_t *)min_deadline_ref = context->_deadline;
            }
        },
        &min_deadline);
    _graph->set_deadline(min_deadline);
}

void Graph::Context::set_needs_update() {
    if (_needs_update) {
        return;
    }
    _graph->foreach_trace([this](Trace &trace) { trace.needs_update(*this); });
    _needs_update = true;
    _graph->set_needs_update(true);
}

bool Graph::Context::thread_is_updating() {
    for (auto update = Graph::current_update(); update != nullptr; update = update.get()->next()) {
        if (update.get()->graph() == _graph) {
            return _graph->is_context_updating(_id);
        }
    }
    return false;
}

void Graph::Context::call_invalidation(AttributeID attribute) {
    _graph_version = _graph->_version;

    if (_invalidation_callback) {
        auto old_update = current_update();
        if (old_update.value() != 0) {
            set_current_update(old_update.with_tag(true));
        }

        _graph->foreach_trace([this, &attribute](Trace &trace) { trace.begin_invalidation(*this, attribute); });
        _invalidation_callback(attribute);
        _graph->foreach_trace([this, &attribute](Trace &trace) { trace.end_invalidation(*this, attribute); });

        set_current_update(old_update);
    }
}

void Graph::Context::call_update() {
    if (!_needs_update) {
        return;
    }
    _needs_update = false;

    if (_update_callback) {
        auto update =
            UpdateStack(_graph, AGGraphUpdateOptions(AGGraphUpdateOptionsInitializeCleared |
                                                     AGGraphUpdateOptionsEndDeferringSubgraphInvalidationOnExit));

        _graph->foreach_trace([this](Trace &trace) { trace.begin_update(*this); });
        _update_callback();
        _graph->foreach_trace([this](Trace &trace) { trace.end_update(*this); });

        // ~UpdateStack()
    }
}

} // namespace AG
