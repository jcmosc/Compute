#include "Context.h"

#include "AGGraph-Private.h"
#include "Attribute/AttributeID.h"
#include "Subgraph/Subgraph.h"
#include "UniqueID/AGUniqueID.h"

namespace AG {

Graph::Context::Context(Graph *graph) {
    _graph = graph;
    _context_info = nullptr;
    _unique_id = AGMakeUniqueID();

    Graph::retain(graph);
    graph->_contexts_by_id.insert(_unique_id, this);
}

Graph::Context::~Context() {
    _graph->_contexts_by_id.remove(_unique_id);
    Graph::release(_graph);
}

Graph::Context *Graph::Context::from_cf(AGGraphStorage *storage) {
    if (storage->context._invalidated) {
        precondition_failure("invalidated graph");
    }
    return &storage->context;
}

} // namespace AG
