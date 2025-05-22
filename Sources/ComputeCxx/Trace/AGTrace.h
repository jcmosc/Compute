#pragma once

#include <CoreFoundation/CFBase.h>

#include "Graph/AGGraph.h"

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

    void (*_Nullable beginTrace)(void *context, AGGraphRef graph);
    void (*_Nullable endTrace)(void *context, AGGraphRef graph);

    void (*_Nullable beginUpdateSubgraph)(void *context, AGSubgraphStorage *subgraph);
    void (*_Nullable endUpdateSubgraph)(void *context, AGSubgraphStorage *subgraph);
    void (*_Nullable beginUpdate)(void *context, AGAttribute attribute);
    void (*_Nullable endUpdate)(void *context, bool changed);
    void (*_Nullable beginUpdateAttribute)(void *context);
    void (*_Nullable endUpdateAttribute)(void *context);
    void (*_Nullable beginUpdateGraph)(void *context, AGGraphRef graph);
    void (*_Nullable endUpdateGraph)(void *context, AGGraphRef graph);

    void (*_Nullable beginInvalidation)(void *context, AGGraphRef graph, AGAttribute attribute);
    void (*_Nullable endInvalidation)(void *context, AGGraphRef graph, AGAttribute attribute);

    void (*_Nullable beginModify)(void *context);
    void (*_Nullable endModify)(void *context);

    void (*_Nullable beginEvent)(void *context, AGAttribute attribute, const char *event_name);
    void (*_Nullable endEvent)(void *context, AGAttribute attribute, const char *event_name);

    void (*_Nullable createdGraph)(void *context, AGGraphRef graph);
    void (*_Nullable destroyGraph)(void *context, AGGraphRef graph);
    void (*_Nullable needsUpdate)(void *context, AGGraphRef graph);

    void (*_Nullable createdSubgraph)(void *context, AGSubgraphStorage *subgraph);
    void (*_Nullable invalidateSubgraph)(void *context, AGSubgraphStorage *subgraph);
    void (*_Nullable addChildSubgraph)(void *context, AGSubgraphStorage *subgraph, AGSubgraphStorage *child);
    void (*_Nullable removeChildSubgraph)(void *context, AGSubgraphStorage *subgraph, AGSubgraphStorage *child);

    void (*_Nullable addedAttribute)(void *context);
    void (*_Nullable addEdge)(void *context);
    void (*_Nullable removeEdge)(void *context);
    void (*_Nullable setEdgePending)(void *context);

    void (*_Nullable setDirty)(void *context);
    void (*_Nullable setPending)(void *context);
    void (*_Nullable setValue)(void *context);
    void (*_Nullable markValue)(void *context);

    void (*_Nullable addedIndirectAttribute)(void *context, AGAttribute attribute);
    void (*_Nullable setSource)(void *context, AGAttribute attribute);
    void (*_Nullable setDependency)(void *context, AGAttribute attribute);

    void (*_Nullable markProfile)(void *context, const char *event_name);

    void (*_Nullable customEvent)(void *context, AGGraphRef graph, const char *event_name, const void *value,
                                  AGTypeID type);
    void (*_Nullable namedEvent)(void *context, AGGraphRef graph, uint32_t eventID, uint32_t eventArgsCount,
                                 const uint64_t *eventArgs, CFDataRef data, uint32_t arg6);
    bool (*_Nullable namedEventEnabled)(void *context);
    void (*_Nullable setDeadline)(void *context);
    void (*_Nullable passedDeadline)(void *context);

    void (*_Nullable compareFailed)(void *context, AGAttribute attribute, AGComparisonState comparisonState);
} AGTrace;

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
