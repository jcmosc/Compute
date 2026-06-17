#pragma once

#include "ComputeCxx/IAGBase.h"

#if TARGET_OS_MAC
#include "CoreFoundationPrivate/CFRuntime.h"
#else
#include <SwiftCorelibsCoreFoundation/CFRuntime.h>
#endif

#include "Graph.h"

IAG_ASSUME_NONNULL_BEGIN

struct IAGGraphStorage;

namespace IAG {

class Graph::Context {
  private:
    Graph *_graph;
    const void *_context = nullptr;
    uint64_t _id;

    ClosureFunctionAV<void, IAGAttribute> _invalidation_callback = {nullptr, nullptr};
    ClosureFunctionVV<void> _update_callback = {nullptr, nullptr};

    uint64_t _deadline = UINT64_MAX;
    uint64_t _graph_version;
    bool _needs_update;
    bool _invalidated;

    void call_invalidation(AttributeID attribute);

  public:
    Context(Graph *graph);
    ~Context();

    uint64_t id() const { return _id; }

    IAGGraphStorage *to_cf() const { return reinterpret_cast<IAGGraphStorage *>((char *)this - sizeof(CFRuntimeBase)); };
    static Context *from_cf(IAGGraphStorage *storage);

    Graph &graph() const { return *_graph; };

    const void *_Nullable context() const { return _context; };
    void set_context(const void *_Nullable context) { _context = context; };

    void set_invalidation_callback(IAG::ClosureFunctionAV<void, IAGAttribute> callback) {
        _invalidation_callback = callback;
    }
    void set_update_callback(IAG::ClosureFunctionVV<void> callback) { _update_callback = callback; }

    uint64_t deadline() const { return _deadline; };
    void set_deadline(uint64_t deadline);

    uint64_t graph_version() const { return _graph_version; }; // controls invalidation callback

    bool needs_update() const { return _needs_update; };
    void set_needs_update();

    bool invalidated() const { return _invalidated; };
    void set_invalidated(bool invalidated) { _invalidated = invalidated; };

    bool thread_is_updating();

    void call_invalidation_if_needed(AttributeID attribute) {
        if (graph_version() == graph().version()) {
            return;
        }
        call_invalidation(attribute);
    }
    void call_update();
};

} // namespace IAG

IAG_ASSUME_NONNULL_END
