#pragma once

#include <CoreFoundation/CFBase.h>

#include "Graph/AGGraph.h"

CF_ASSUME_NONNULL_BEGIN
CF_IMPLICIT_BRIDGING_ENABLED

CF_EXTERN_C_BEGIN

// MARK: CFType

typedef struct CF_BRIDGED_TYPE(id) AGSubgraphStorage *AGSubgraphRef CF_SWIFT_NAME(Subgraph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
CFTypeID AGSubgraphGetTypeID() CF_SWIFT_NAME(getter:AGSubgraphRef.typeID());

// MARK: Current subgraph

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGSubgraphRef _Nullable AGSubgraphGetCurrent() CF_SWIFT_NAME(getter:AGSubgraphRef.current());

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphSetCurrent(AGSubgraphRef _Nullable subgraph) CF_SWIFT_NAME(setter:AGSubgraphRef.current(_:));

// MARK: Graph Context

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGSubgraphRef AGSubgraphCreate(AGGraphRef graph) CF_SWIFT_NAME(AGSubgraphRef.init(graph:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGSubgraphRef AGSubgraphCreate2(AGGraphRef graph, AGAttribute attribute)
    CF_SWIFT_NAME(AGSubgraphRef.init(graph:attribute:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGUnownedGraphRef _Nullable AGSubgraphGetCurrentGraphContext()
    CF_SWIFT_NAME(getter:AGSubgraphRef.currentGraphContext());

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGGraphRef AGSubgraphGetGraph(AGSubgraphRef subgraph) CF_SWIFT_NAME(getter:AGSubgraphRef.graph(self:));

CF_EXTERN_C_END

CF_IMPLICIT_BRIDGING_DISABLED
CF_ASSUME_NONNULL_END
