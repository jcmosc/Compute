#pragma once

#include <CoreFoundation/CFBase.h>

#include "AGSubgraph.h"
#include "Data/Zone.h"

CF_ASSUME_NONNULL_BEGIN

struct AGSubgraphStorage;

namespace AG {

class Graph;
class SubgraphObject;

class Subgraph : public data::zone {
  private:
    SubgraphObject *_object;
    Graph *_graph;
    uint64_t _context_id;

  public:
    AGSubgraphStorage *to_cf() const;
    static Subgraph *from_cf(AGSubgraphStorage *storage);

    Graph *graph() const { return _graph; };

    uint64_t context_id() const { return _context_id; }
};

} // namespace AG

CF_ASSUME_NONNULL_END
