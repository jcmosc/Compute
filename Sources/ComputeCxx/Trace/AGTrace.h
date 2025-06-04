#pragma once

#include <CoreFoundation/CFBase.h>

#include "Graph/AGGraph.h"
#include "Subgraph/AGSubgraph.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef CF_ENUM(uint64_t, AGTraceEvents) {
    AGTraceEventsCustom = 1,
    AGTraceEventsNamed = 2,
    AGTraceEventsDeadline = 3,
    AGTraceEventsCompareFailed = 4,
};

typedef struct AGTrace {
    AGTraceEvents events;

    void (*_Nullable beginTrace)(void *_Nullable context, AGGraphRef graph);
    void (*_Nullable endTrace)(void *_Nullable context, AGGraphRef graph);

    void (*_Nullable beginUpdateSubgraph)(void *_Nullable context, AGSubgraphRef subgraph);
    void (*_Nullable endUpdateSubgraph)(void *_Nullable context, AGSubgraphRef subgraph);
    void (*_Nullable beginUpdate)(void *_Nullable context, AGAttribute attribute);
    void (*_Nullable endUpdate)(void *_Nullable context, bool changed);
    void (*_Nullable beginUpdateAttribute)(void *_Nullable context);
    void (*_Nullable endUpdateAttribute)(void *_Nullable context);
    void (*_Nullable beginUpdateGraph)(void *_Nullable context, AGGraphRef graph);
    void (*_Nullable endUpdateGraph)(void *_Nullable context, AGGraphRef graph);

    void (*_Nullable beginInvalidation)(void *_Nullable context, AGGraphRef graph, AGAttribute attribute);
    void (*_Nullable endInvalidation)(void *_Nullable context, AGGraphRef graph, AGAttribute attribute);

    void (*_Nullable beginModify)(void *_Nullable context);
    void (*_Nullable endModify)(void *_Nullable context);

    void (*_Nullable beginEvent)(void *_Nullable context, AGAttribute attribute, const char *event_name);
    void (*_Nullable endEvent)(void *_Nullable context, AGAttribute attribute, const char *event_name);

    void (*_Nullable createdGraph)(void *_Nullable context, AGGraphRef graph);
    void (*_Nullable destroyGraph)(void *_Nullable context, AGGraphRef graph);
    void (*_Nullable needsUpdate)(void *_Nullable context, AGGraphRef graph);

    void (*_Nullable createdSubgraph)(void *_Nullable context, AGSubgraphRef subgraph);
    void (*_Nullable invalidateSubgraph)(void *_Nullable context, AGSubgraphRef subgraph);
    void (*_Nullable addChildSubgraph)(void *_Nullable context, AGSubgraphRef subgraph, AGSubgraphRef child);
    void (*_Nullable removeChildSubgraph)(void *_Nullable context, AGSubgraphRef subgraph, AGSubgraphRef child);

    void (*_Nullable addedAttribute)(void *_Nullable context);
    void (*_Nullable addEdge)(void *_Nullable context);
    void (*_Nullable removeEdge)(void *_Nullable context);
    void (*_Nullable setEdgePending)(void *_Nullable context);

    void (*_Nullable setDirty)(void *_Nullable context);
    void (*_Nullable setPending)(void *_Nullable context);
    void (*_Nullable setValue)(void *_Nullable context);
    void (*_Nullable markValue)(void *_Nullable context);

    void (*_Nullable addedIndirectAttribute)(void *_Nullable context, AGAttribute attribute);
    void (*_Nullable setSource)(void *_Nullable context, AGAttribute attribute);
    void (*_Nullable setDependency)(void *_Nullable context, AGAttribute attribute);

    void (*_Nullable markProfile)(void *_Nullable context, const char *event_name);

    void (*_Nullable customEvent)(void *_Nullable context, AGGraphRef graph, const char *event_name, const void *value,
                                  AGTypeID type);
    void (*_Nullable namedEvent)(void *_Nullable context, AGGraphRef graph, uint32_t eventID, uint32_t eventArgCount,
                                 const void *eventArgs, CFDataRef data, uint32_t arg6);
    bool (*_Nullable namedEventEnabled)(void *_Nullable context);
    void (*_Nullable setDeadline)(void *_Nullable context);
    void (*_Nullable passedDeadline)(void *_Nullable context);

    void (*_Nullable compareFailed)(void *_Nullable context, AGAttribute attribute, AGComparisonState comparisonState);
} AGTrace;

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
