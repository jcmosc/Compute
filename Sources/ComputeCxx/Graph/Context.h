#pragma once

#include <CoreFoundation/CFBase.h>

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Graph::Context {
  private:
    Graph *_graph;

    uint32_t _value_ref_counter;

  public:
    Graph &graph() const { return *_graph; };

    uint64_t unique_id();

    uint32_t value_ref_counter() { return _value_ref_counter; };
    void call_invalidation(AttributeID attribute);

    void call_update();
};

} // namespace AG

CF_ASSUME_NONNULL_END
