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

    bool invalidated() const { return _invalidated; };
    void set_invalidated(bool invalidated) { _invalidated = invalidated; };
};

} // namespace AG

CF_ASSUME_NONNULL_END
