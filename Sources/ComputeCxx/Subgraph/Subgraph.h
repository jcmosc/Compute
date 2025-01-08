#pragma once

#include <CoreFoundation/CFBase.h>

#include "Data/Zone.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Graph;

class Subgraph : public data::zone {
  public:
    Graph &graph() const;
};

} // namespace AG

CF_ASSUME_NONNULL_END
