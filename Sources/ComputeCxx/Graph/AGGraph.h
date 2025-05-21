#pragma once

#include <CoreFoundation/CFBase.h>

#include "AGSwiftSupport.h"
#include "Attribute/AttributeType/AGAttributeType.h"
#include "Swift/AGType.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

// MARK: CFType

typedef struct CF_BRIDGED_TYPE(id) AGGraphStorage *AGGraphRef AG_SWIFT_NAME(Graph);
typedef void *AGUnownedGraphRef AG_SWIFT_STRUCT;
typedef struct AGGraphContextStorage *AGUnownedGraphContextRef AG_SWIFT_STRUCT;

CF_EXPORT
CF_REFINED_FOR_SWIFT
CFTypeID AGGraphGetTypeID() CF_SWIFT_NAME(getter:AGGraphRef.typeID());

// MARK: Graph Context

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGGraphRef AGGraphCreate() CF_SWIFT_NAME(AGGraphRef.init());

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGGraphRef AGGraphCreateShared(AGGraphRef _Nullable graph) CF_SWIFT_NAME(AGGraphRef.init(shared:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGUnownedGraphRef AGGraphGetGraphContext(AGGraphRef graph) CF_SWIFT_NAME(getter:AGGraphRef.graphContext(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGGraphRef AGGraphContextGetGraph(AGUnownedGraphContextRef context)
    CF_SWIFT_NAME(getter:AGUnownedGraphContextRef.graph(self:));

// MARK: User context

CF_EXPORT
CF_REFINED_FOR_SWIFT
const void *_Nullable AGGraphGetContext(AGGraphRef graph) CF_SWIFT_NAME(getter:AGGraphRef.context(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetContext(AGGraphRef graph, const void *_Nullable context)
    CF_SWIFT_NAME(setter:AGGraphRef.context(self:_:));

// MARK: Counter

typedef CF_ENUM(uint32_t, AGGraphCounterQuery) {
//    AGGraphCounterQueryNodeCount,
//    AGGraphCounterQueryTransactionCount,
//    AGGraphCounterQueryUpdateCount,
//    AGGraphCounterQueryChangeCount,
    AGGraphCounterQueryContextID,
    AGGraphCounterQueryGraphID,
//    AGGraphCounterQueryContextThreadUpdating,
//    AGGraphCounterQueryThreadUpdating,
//    AGGraphCounterQueryContextNeedsUpdate,
//    AGGraphCounterQueryNeedsUpdate,
//    AGGraphCounterQueryMainThreadUpdateCount,
//    AGGraphCounterQueryNodeTotalCount,
//    AGGraphCounterQuerySubgraphCount,
//    AGGraphCounterQuerySubgraphTotalCount,
};

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint64_t AGGraphGetCounter(AGGraphRef graph, AGGraphCounterQuery query) CF_SWIFT_NAME(AGGraphRef.counter(self:for:));

// MARK: Attribute types

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint32_t AGGraphInternAttributeType(AGUnownedGraphRef graph, AGTypeID type,
                                    const AGAttributeType *_Nonnull (*_Nonnull make_attribute_type)(
                                        const void *_Nullable context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                                    const void *_Nullable make_attribute_type_context);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
