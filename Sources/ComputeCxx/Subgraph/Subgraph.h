#pragma once

#include <CoreFoundation/CFBase.h>

#include "Data/Zone.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Graph;

class Subgraph : public data::zone {
  private:
    Graph *_graph;

  public:
    Graph &graph() const { return *_graph; };
};

} // namespace AG

CF_ASSUME_NONNULL_END
