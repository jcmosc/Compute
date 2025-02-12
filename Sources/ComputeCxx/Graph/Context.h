#pragma once

#include <CoreFoundation/CFBase.h>

#include "Graph.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Graph::Context {
  private:
    Graph *_graph;
    void *_field_0x08;
    uint64_t _unique_id;

    uint64_t _graph_version;

  public:
    Graph &graph() const { return *_graph; };

    uint64_t unique_id() { return _unique_id; };

    uint64_t graph_version() { return _graph_version; };
    void call_invalidation(AttributeID attribute);

    void call_update();
};

} // namespace AG

CF_ASSUME_NONNULL_END
