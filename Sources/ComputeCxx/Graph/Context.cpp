#include "Context.h"

#include "AGGraph-Private.h"
#include "Attribute/AttributeID/AttributeID.h"
#include "ComputeCxx/AGUniqueID.h"
#include "Subgraph/Subgraph.h"

namespace AG {

Graph::Context::Context(Graph *graph) : _graph(graph), _id(AGMakeUniqueID()) {
    Graph::retain(graph);
    graph->_contexts_by_id.insert(_id, this);
}

Graph::Context::~Context() {
    _graph->_contexts_by_id.remove(_id);
    Graph::release(_graph);
}

Graph::Context *Graph::Context::from_cf(AGGraphStorage *storage) {
    if (storage->context._invalidated) {
        precondition_failure("invalidated graph");
    }
    return &storage->context;
}

} // namespace AG
