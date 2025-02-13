#pragma once

#include <CoreFoundation/CFBase.h>

#include "Attribute/AttributeID.h"
#include "Graph.h"
#include "Private/CFRuntime.h"

CF_ASSUME_NONNULL_BEGIN

struct AGGraphStorage;

namespace AG {

class Graph::Context {
  private:
    Graph *_graph;
    void *_field_0x08;
    uint64_t _unique_id;

    ClosureFunctionAV<void, uint32_t> _invalidation_closure;
    ClosureFunctionVV<void> _update_closure;

    uint64_t _deadline;
    uint64_t _graph_version;
    bool _needs_update;
    bool _invalidated;

  public:
    Context(Graph *graph);
    ~Context();

    AGGraphStorage *to_cf() { return reinterpret_cast<AGGraphStorage *>((char *)this - sizeof(CFRuntimeBase)); };
    static Context *from_cf(AGGraphStorage *storage);

    Graph &graph() const { return *_graph; };
    uint64_t unique_id() const { return _unique_id; };
    uint64_t graph_version() const { return _graph_version; };

    void set_deadline(uint64_t deadline);
    void set_needs_update();

    bool thread_is_updating();

    void call_invalidation(AttributeID attribute);
    void call_update();
};

} // namespace AG

struct AGGraphStorage {
    CFRuntimeBase _base;
    AG::Graph::Context _context;
};

CF_ASSUME_NONNULL_END
