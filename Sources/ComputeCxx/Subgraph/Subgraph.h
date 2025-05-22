#pragma once

#include <CoreFoundation/CFBase.h>

#include "Data/Zone.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Graph;

class Subgraph : public data::zone {
  private:
    Graph *_graph;
    uint64_t _context_id;

  public:
    Graph &graph() const { return *_graph; };

    uint64_t context_id() const { return _context_id; }
};

} // namespace AG

CF_ASSUME_NONNULL_END
