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

#pragma mark - User context

const void *AGGraphGetContext(AGGraphRef graph) {
    auto graph_context = AG::Graph::Context::from_cf(graph);
    return graph_context->context();
}

void AGGraphSetContext(AGGraphRef graph, const void *context) {
    auto graph_context = AG::Graph::Context::from_cf(graph);
    graph_context->set_context(context);
}

#pragma mark - Counter

uint64_t AGGraphGetCounter(AGGraphRef graph, AGGraphCounterQuery query) {
    auto graph_context = AG::Graph::Context::from_cf(graph);
    switch (query) {
        //    case AGGraphCounterQueryNodeCount:
        //        return graph_context->graph().node_count();
        //    case AGGraphCounterQueryTransactionCount:
        //        return graph_context->graph().transaction_count();
        //    case AGGraphCounterQueryUpdateCount:
        //        return graph_context->graph().update_count();
        //    case AGGraphCounterQueryChangeCount:
        //        return graph_context->graph().change_count();
    case AGGraphCounterQueryContextID:
        return graph_context->id();
    case AGGraphCounterQueryGraphID:
        return graph_context->graph().id();
        //    case AGGraphCounterQueryContextThreadUpdating:
        //        return graph_context->is_thread_updating();
        //    case AGGraphCounterQueryThreadUpdating:
        //        return graph_context->graph().is_thread_updating();
        //    case AGGraphCounterQueryContextNeedsUpdate:
        //        return graph_context->needs_update();
        //    case AGGraphCounterQueryNeedsUpdate:
        //        return graph_context->graph().needs_update();
        //    case AGGraphCounterQueryMainThreadUpdateCount:
        //        return graph_context->graph().main_thread_update_count();
        //    case AGGraphCounterQueryNodeTotalCount:
        //        return graph_context->graph().total_node_count();
        //    case AGGraphCounterQuerySubgraphCount:
        //        return graph_context->graph().subgraph_count();
        //    case AGGraphCounterQuerySubgraphTotalCount:
        //        return graph_context->graph().total_subgraph_count();
    default:
        return 0;
    }
}

#pragma mark - Attribute types

uint32_t AGGraphInternAttributeType(AGUnownedGraphRef unowned_graph, AGTypeID type,
                                    const AGAttributeType *(*make_attribute_type)(const void *context AG_SWIFT_CONTEXT)
                                        AG_SWIFT_CC(swift),
                                    const void *make_attribute_type_context) {
    auto metadata = reinterpret_cast<const AG::swift::metadata *>(type);
    AG::Graph *graph = reinterpret_cast<AG::Graph *>(unowned_graph);
    return graph->intern_type(
        metadata, AG::ClosureFunctionVP<const AGAttributeType *>(make_attribute_type, make_attribute_type_context));
}
