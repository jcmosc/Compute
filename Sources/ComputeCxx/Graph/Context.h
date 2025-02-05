#pragma once

#include <CoreFoundation/CFBase.h>

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Graph::Context {
  private:
    Graph *_graph;

  public:
    Graph &graph() const { return *_graph; };
    
    uint64_t unique_id();
    
    void call_update();
};

} // namespace AG

CF_ASSUME_NONNULL_END
