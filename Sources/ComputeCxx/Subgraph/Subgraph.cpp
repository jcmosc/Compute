#include "Subgraph.h"

#include "AGSubgraph-Private.h"
#include "Graph/Context.h"
#include "Trace/Trace.h"

namespace AG {

Subgraph::Subgraph(SubgraphObject *object, Graph::Context &context, AttributeID attribute) {
    _object = object;

    Graph *graph = &context.graph();
    _graph = graph;
    _context_id = context.id();

    graph->add_subgraph(*this);

    context.graph().foreach_trace([this](Trace &trace) { trace.created(*this); });
}

Subgraph::~Subgraph() {
    if (auto observers_ptr = _observers.get()) {
        auto observers = *observers_ptr;

        notify_observers();
        delete observers;
    }
}

#pragma mark - CFType

Subgraph *Subgraph::from_cf(AGSubgraphStorage *storage) { return storage->subgraph; }

AGSubgraphStorage *Subgraph::to_cf() const { return reinterpret_cast<AGSubgraphStorage *>(_object); }

void Subgraph::clear_object() {
    auto object = _object;
    if (object) {
        object->clear_subgraph();
        _object = nullptr;

        if (current_subgraph() == this) {
            set_current_subgraph(nullptr);
            CFRelease(object);
        }
    }
}

#pragma mark - Current subgraph

pthread_key_t Subgraph::_current_subgraph_key;

void Subgraph::make_current_subgraph_key() { pthread_key_create(&_current_subgraph_key, 0); }

Subgraph *Subgraph::current_subgraph() { return (Subgraph *)pthread_getspecific(_current_subgraph_key); }

void Subgraph::set_current_subgraph(Subgraph *subgraph) { pthread_setspecific(_current_subgraph_key, subgraph); }

#pragma mark - Observers

uint64_t Subgraph::add_observer(ClosureFunctionVV<void> callback) {
    if (!_observers) {
        _observers =
            alloc_bytes(sizeof(vector<Observer, 0, uint64_t> *), 7).unsafe_cast<vector<Observer, 0, uint64_t> *>();
        *_observers = new vector<Observer, 0, uint64_t>();
        ;
    }

    auto observer_id = AGMakeUniqueID();
    (*_observers)->push_back(Observer(callback, observer_id));
    return observer_id;
}

void Subgraph::remove_observer(uint64_t observer_id) {
    if (auto observers_ptr = _observers.get()) {
        auto observers = *observers_ptr;
        auto iter = std::remove_if(observers->begin(), observers->end(), [&observer_id](auto observer) -> bool {
            if (observer.observer_id == observer_id) {
                return true;
            }
            return false;
        });
        observers->erase(iter);
    }
}

void Subgraph::notify_observers() {
    if (auto observers_ptr = _observers.get()) {
        auto observers = *observers_ptr;
        while (!observers->empty()) {
            observers->back().callback();
            observers->pop_back();
        }
    }
}

#pragma mark - Graph

void Subgraph::invalidate_and_delete_(bool delete_zone_data) {
    if (delete_zone_data) {
        mark_deleted();
    }

    if (!is_invalidating()) {
        // TODO: parents

        Graph &graph = *_graph;
        if (!graph.is_deferring_subgraph_invalidation() && !graph.has_main_handler()) {
            invalidate_now(graph);
            graph.invalidate_subgraphs();
            return;
        }

        invalidate_deferred(*_graph);
    }
}

void Subgraph::invalidate_deferred(Graph &graph) {
    auto old_invalidation_state = _invalidation_state;
    if (old_invalidation_state != InvalidationState::Deferred) {
        graph.defer_subgraph_invalidation(*this);
        _invalidation_state = InvalidationState::Deferred;
        if (old_invalidation_state == InvalidationState::None) {
            graph.foreach_trace([this](Trace &trace) { trace.invalidate(*this); });
        }
    }
}

void Subgraph::invalidate_now(Graph &graph) {
    graph.will_invalidate_subgraph();

    auto removed_subgraphs = vector<Subgraph *, 16, uint64_t>();
    auto invalidating_subgraphs = std::stack<Subgraph *, vector<Subgraph *, 16, uint64_t>>();

    auto old_invalidation_state = _invalidation_state;
    if (_invalidation_state != InvalidationState::Completed) {
        _invalidation_state = InvalidationState::Completed;
        if (old_invalidation_state == InvalidationState::None) {
            graph.foreach_trace([this](Trace &trace) { trace.invalidate(*this); });
        }

        clear_object();

        invalidating_subgraphs.push(this);
        while (!invalidating_subgraphs.empty()) {
            Subgraph *subgraph = invalidating_subgraphs.top();
            invalidating_subgraphs.pop();

            graph.foreach_trace([subgraph](Trace &trace) { trace.destroy(*subgraph); });

            notify_observers();
            graph.remove_subgraph(*subgraph);

            subgraph->mark_deleted();
            removed_subgraphs.push_back(subgraph);

            // TODO: children
        }
    }

    // TODO: remove nodes
    // TODO: destroy nodes

    for (Subgraph *removed_subgraph : removed_subgraphs) {
        delete removed_subgraph;
    }

    graph.did_invalidate_subgraph();
}

void Subgraph::graph_destroyed() {
    auto old_invalidation_state = _invalidation_state;
    _invalidation_state = InvalidationState::GraphDestroyed;

    if (old_invalidation_state == InvalidationState::None) {
        graph()->foreach_trace([this](Trace &trace) { trace.invalidate(*this); });
    }

    notify_observers();

    // TODO: destroy nodes
    // TODO: parents
    // TODO: children

    clear();
}

} // namespace AG
