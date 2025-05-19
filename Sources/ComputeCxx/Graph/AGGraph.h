#pragma once

#include <CoreFoundation/CFBase.h>

#include "AGSwiftSupport.h"
#include "Swift/AGType.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

// MARK: CFType

typedef struct CF_BRIDGED_TYPE(id) AGGraphStorage *AGGraphRef AG_SWIFT_NAME(Graph);
typedef void *AGUnownedGraphRef AG_SWIFT_STRUCT;
typedef struct AGGraphContextStorage *AGUnownedGraphContextRef AG_SWIFT_STRUCT;

CF_EXPORT
CF_REFINED_FOR_SWIFT
CFTypeID AGGraphGetTypeID() AG_SWIFT_NAME(getter:AGGraphRef.typeID());

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

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
