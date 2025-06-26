#pragma once

#include <CoreFoundation/CFBase.h>

#include "Graph.h"
#include "Private/CFRuntime.h"

CF_ASSUME_NONNULL_BEGIN

struct AGGraphStorage;

namespace AG {

class Graph::Context {
  private:
    Graph *_graph;
    const void *_context = nullptr;
    uint64_t _id;

    ClosureFunctionAV<void, AGAttribute> _invalidation_callback = {nullptr, nullptr};
    ClosureFunctionVV<void> _update_callback = {nullptr, nullptr};

    uint64_t _deadline;
    bool _needs_update;
    bool _invalidated;

  public:
    Context(Graph *graph);
    ~Context();

    uint64_t id() const { return _id; }

    AGGraphStorage *to_cf() const { return reinterpret_cast<AGGraphStorage *>((char *)this - sizeof(CFRuntimeBase)); };
    static Context *from_cf(AGGraphStorage *storage);

    Graph &graph() const { return *_graph; };

    const void *_Nullable context() const { return _context; };
    void set_context(const void *_Nullable context) { _context = context; };

    void set_invalidation_callback(AG::ClosureFunctionAV<void, AGAttribute> callback) {
        _invalidation_callback = callback;
    }
    void set_update_callback(AG::ClosureFunctionVV<void> callback) { _update_callback = callback; }

    uint64_t deadline() const { return _deadline; };
    void set_deadline(uint64_t deadline);

    bool needs_update() const { return _needs_update; };
    void set_needs_update();

    bool invalidated() const { return _invalidated; };
    void set_invalidated(bool invalidated) { _invalidated = invalidated; };

    void call_update();
};

} // namespace AG

CF_ASSUME_NONNULL_END
