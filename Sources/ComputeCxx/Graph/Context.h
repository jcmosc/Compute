#pragma once

#include <CoreFoundation/CFBase.h>

#include "Attribute/AGAttribute.h"
#include "Attribute/AttributeID.h"
#include "Graph.h"
#include "Private/CFRuntime.h"

CF_ASSUME_NONNULL_BEGIN

struct AGGraphStorage;

namespace AG {

class Graph::Context {
  private:
    Graph *_graph;
    const void *_context_info;
    uint64_t _unique_id;

    ClosureFunctionAV<void, AGAttribute> _invalidation_callback;
    ClosureFunctionVV<void> _update_callback;

    uint64_t _deadline;
    uint64_t _graph_version;
    bool _needs_update;
    bool _invalidated; // set to true after destru

  public:
    Context(Graph *graph);
    ~Context();

    AGGraphStorage *to_cf() const { return reinterpret_cast<AGGraphStorage *>((char *)this - sizeof(CFRuntimeBase)); };
    static Context *from_cf(AGGraphStorage *storage);

    Graph &graph() const { return *_graph; };

    const void *context_info() const { return _context_info; };
    void set_context_info(const void *context_info) { _context_info = context_info; };

    uint64_t unique_id() const { return _unique_id; };

    void set_invalidation_callback(AG::ClosureFunctionAV<void, AGAttribute> callback) {
        _invalidation_callback = callback;
    }
    void set_update_callback(AG::ClosureFunctionVV<void> callback) { _update_callback = callback; }

    uint64_t deadline() const { return _deadline; };
    void set_deadline(uint64_t deadline);
    
    uint64_t graph_version() const { return _graph_version; };

    bool needs_update() const { return _needs_update; };
    void set_needs_update();

    bool invalidated() const { return _invalidated; };
    void set_invalidated(bool invalidated) { _invalidated = invalidated; };

    bool thread_is_updating();

    void call_invalidation(AttributeID attribute);
    void call_update();
};

} // namespace AG

CF_ASSUME_NONNULL_END
