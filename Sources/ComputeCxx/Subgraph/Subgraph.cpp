#include "Subgraph.h"

#include "AGSubgraph-Private.h"

namespace AG {

Subgraph *Subgraph::from_cf(AGSubgraphStorage *storage) { return storage->subgraph; }

AGSubgraphStorage *Subgraph::to_cf() const { return reinterpret_cast<AGSubgraphStorage *>(_object); }

} // namespace AG
