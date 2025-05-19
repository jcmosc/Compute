#include "AGGraph-Private.h"

#include <os/lock.h>

#include "Context.h"
#include "Graph.h"
#include "Private/CFRuntime.h"

namespace {

CFRuntimeClass &graph_type_id() {
    static auto finalize = [](CFTypeRef graph_ref) {
        AGGraphStorage *storage = (AGGraphRef)graph_ref;
        if (!storage->context.invalidated()) {
            storage->context.AG::Graph::Context::~Context();
        }
    };
    static CFRuntimeClass klass = {
        0,                // version
        "AGGraphStorage", // className
        NULL,             // init
        NULL,             // copy
        finalize,
        NULL, // equal
        NULL, // hash
        NULL, // copyFormattingDesc
        NULL, // copyDebugDesc,
        NULL, // reclaim
        NULL, // refcount
        0     // requiredAlignment
    };
    return klass;
}

} // namespace

CFTypeID AGGraphGetTypeID() {
    static CFTypeID type = _CFRuntimeRegisterClass(&graph_type_id());
    return type;
}

AGGraphRef AGGraphCreate() { return AGGraphCreateShared(nullptr); };

AGGraphRef AGGraphCreateShared(AGGraphRef original) {
    CFIndex extra_bytes = sizeof(struct AGGraphStorage) - sizeof(CFRuntimeBase);
    AGGraphRef instance =
        (AGGraphRef)_CFRuntimeCreateInstance(kCFAllocatorDefault, AGGraphGetTypeID(), extra_bytes, NULL);
    if (!instance) {
        AG::precondition_failure("memory allocation failure.");
    }

    AG::Graph *graph;
    if (original) {
        if (original->context.invalidated()) {
            AG::precondition_failure("invalidated graph");
        }
        graph = &original->context.graph();
        AG::Graph::retain(graph);
    } else {
        graph = new AG::Graph();
    }

    new (&instance->context) AG::Graph::Context(graph); // performs a retain
    AG::Graph::release(graph);

    instance->context.set_invalidated(false);

    return instance;
};

AGUnownedGraphRef AGGraphGetGraphContext(AGGraphRef graph) {
    AG::Graph::Context *graph_context = AG::Graph::Context::from_cf(graph);
    AG::Graph *unowned_graph = &graph_context->graph();
    return reinterpret_cast<AGUnownedGraphRef>(unowned_graph);
}

AGGraphRef AGGraphContextGetGraph(AGUnownedGraphContextRef storage) {
    auto graph_context = reinterpret_cast<AG::Graph::Context *>(storage);
    return graph_context->to_cf();
}
