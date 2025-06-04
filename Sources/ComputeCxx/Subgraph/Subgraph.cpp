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

void Subgraph::invalidate_and_delete_(bool delete_subgraph) {
    // TODO: not implemented
}

} // namespace AG
