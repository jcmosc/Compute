#include "Subgraph.h"

#include <ranges>

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

    if (is_invalidating()) {
        return;
    }

    for (auto parent : _parents) {
        parent->remove_child(*this, true);
    }
    _parents.clear();

    // Store the graph locally since the `this` pointer may be deleted
    Graph &graph = *_graph;
    if (!graph.is_deferring_subgraph_invalidation() && !graph.has_main_handler()) {
        invalidate_now(graph);
        graph.invalidate_subgraphs();
    } else {
        invalidate_deferred(graph);
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

            // Invalidate all children that belong to the same context
            // Update parent and child vectors of subgraphs that aren't being invalidated
            for (auto child : subgraph->_children) {
                if (child.subgraph()->context_id() == _context_id) {
                    // for each other parent of the child, remove the child from that parent
                    for (auto other_parent : child.subgraph()->_parents) {
                        if (other_parent == subgraph) {
                            continue;
                        }

                        auto iter = std::remove_if(other_parent->_children.begin(), other_parent->_children.end(),
                                                   [&child](auto other_parent_child) -> bool {
                                                       return other_parent_child.subgraph() == child.subgraph();
                                                   });
                        other_parent->_children.erase(iter);
                    }

                    child.subgraph()->_parents.clear();

                    // Invalidate this child
                    auto old_invalidation_state = child.subgraph()->_invalidation_state;
                    if (child.subgraph()->_invalidation_state != InvalidationState::Completed) { // TODO: check
                        child.subgraph()->_invalidation_state = InvalidationState::Completed;
                        if (old_invalidation_state == InvalidationState::None) {
                            graph.foreach_trace([&child](Trace &trace) { trace.invalidate(*child.subgraph()); });
                        }
                        child.subgraph()->clear_object();
                        invalidating_subgraphs.push(child.subgraph());
                    }
                } else {
                    // don't invalidate this child but remove this subgraph from its parents vector
                    auto iter =
                        std::remove(child.subgraph()->_parents.begin(), child.subgraph()->_parents.end(), subgraph);
                    child.subgraph()->_parents.erase(iter);
                }
            }
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

    _parents.clear();
    _children.clear();

    clear();
}

#pragma mark - Children

void Subgraph::add_child(Subgraph &child, uint8_t flags) {
    if (child.graph() != graph()) {
        precondition_failure("child subgraph must have same graph");
    }
    for (auto parent : child._parents) {
        if (parent == this) {
            precondition_failure("child already attached to new parent");
        }
    }
    graph()->foreach_trace([this, &child](Trace &trace) { trace.add_child(*this, child); });
    _children.push_back(SubgraphChild(&child, flags));

    // TODO: propogate flags
    // TODO: propogate dirty flags

    child._parents.push_back(this);
}

void Subgraph::remove_child(Subgraph &child, bool suppress_trace) {
    auto parent_iter = std::remove(child._parents.begin(), child._parents.end(), this);
    child._parents.erase(parent_iter);

    if (!suppress_trace) {
        graph()->foreach_trace([this, &child](Trace &trace) { trace.remove_child(*this, child); });
    }

    auto child_iter = std::remove_if(_children.begin(), _children.end(), [&child](auto subgraph_child) -> bool {
        return subgraph_child.subgraph() == &child;
    });
    _children.erase(child_iter);
}

bool Subgraph::ancestor_of(const Subgraph &other) {
    auto untraversed_parents = std::stack<const Subgraph *, vector<const Subgraph *, 32, uint64_t>>();
    const Subgraph *candidate = &other;
    while (true) {
        if (candidate == nullptr) {
            // previous candidate was a top-level subgraph
            if (untraversed_parents.empty()) {
                return false;
            }
            candidate = untraversed_parents.top();
            untraversed_parents.pop();
        }

        if (candidate == this) {
            return true;
        }

        // partition parents into first and remaining
        candidate = candidate->_parents.empty() ? nullptr : candidate->_parents.front();
        for (Subgraph *parent : std::ranges::drop_view(other._parents, 1)) {
            untraversed_parents.push(parent);
        }
    }
}

} // namespace AG
