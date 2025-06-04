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

Subgraph::~Subgraph() {}

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

#pragma mark - Graph

void Subgraph::invalidate_and_delete_(bool delete_subgraph) {
    // TODO: not implemented
}

} // namespace AG
