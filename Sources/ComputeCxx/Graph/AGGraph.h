#pragma once

#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFDictionary.h>

#include "AGSwiftSupport.h"
#include "Attribute/AttributeData/Edge/AGInputOptions.h"
#include "Attribute/AttributeID/AGAttribute.h"
#include "Attribute/AttributeType/AGAttributeType.h"
#include "Comparison/AGComparison.h"
#include "Swift/AGType.h"
#include "Trace/AGTraceFlags.h"

CF_ASSUME_NONNULL_BEGIN
CF_IMPLICIT_BRIDGING_ENABLED

CF_EXTERN_C_BEGIN

// MARK: CFType

typedef struct CF_BRIDGED_TYPE(id) AGGraphStorage *AGGraphRef AG_SWIFT_NAME(Graph);
typedef void *AGUnownedGraphRef AG_SWIFT_STRUCT;
typedef struct AGGraphContextStorage *AGUnownedGraphContextRef AG_SWIFT_STRUCT;

typedef struct AGTrace *AGTraceRef;

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
    AGGraphCounterQuerySubgraphCount,
    AGGraphCounterQuerySubgraphTotalCount,
};

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint64_t AGGraphGetCounter(AGGraphRef graph, AGGraphCounterQuery query) CF_SWIFT_NAME(AGGraphRef.counter(self:for:));

// MARK: Subgraphs

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphBeginDeferringSubgraphInvalidation(AGGraphRef graph)
    CF_SWIFT_NAME(AGGraphRef.beginDeferringSubgraphInvalidation(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphEndDeferringSubgraphInvalidation(AGGraphRef graph, bool was_deferring)
    CF_SWIFT_NAME(AGGraphRef.endDeferringSubgraphInvalidation(self:wasDeferring:));

// MARK: Attribute types

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint32_t AGGraphInternAttributeType(AGUnownedGraphRef graph, AGTypeID type,
                                    const AGAttributeType *_Nonnull (*_Nonnull make_attribute_type)(
                                        void *_Nullable context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                                    void *_Nullable make_attribute_type_context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphVerifyType(AGAttribute attribute, AGTypeID type) CF_SWIFT_NAME(AGAttribute.verifyType(self:type:));

// MARK: Attributes

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateAttribute(uint32_t type_id, const void *body, const void *_Nullable value)
    CF_SWIFT_NAME(AGAttribute.init(type:body:value:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGGraphRef AGGraphGetAttributeGraph(AGAttribute attribute) CF_SWIFT_NAME(getter:AGAttribute.graph(self:));

typedef struct AGAttributeInfo {
    const AGAttributeType *type;
    const void *body;
} AGAttributeInfo;

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttributeInfo AGGraphGetAttributeInfo(AGAttribute attribute) CF_SWIFT_NAME(getter:AGAttribute.info(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttributeFlags AGGraphGetFlags(AGAttribute attribute) CF_SWIFT_NAME(getter:AGAttribute.flags(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetFlags(AGAttribute attribute, AGAttributeFlags flags) CF_SWIFT_NAME(setter:AGAttribute.flags(self:_:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint32_t AGGraphAddInput(AGAttribute attribute, AGAttribute input, AGInputOptions options)
    CF_SWIFT_NAME(AGAttribute.addInput(self:_:options:));

// MARK: Update

typedef CF_ENUM(uint32_t, AGGraphUpdateStatus) {
    AGGraphUpdateStatusNoChange = 0,
    AGGraphUpdateStatusChanged = 1,
    AGGraphUpdateStatusOption2 = 2,
    AGGraphUpdateStatusNeedsCallMainHandler = 3,
};

// MARK: Trace

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphStartTracing(AGGraphRef graph, AGTraceFlags trace_flags)
    CF_SWIFT_NAME(AGGraphRef.startTracing(self:flags:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphStartTracing2(AGGraphRef graph, AGTraceFlags trace_flags, CFArrayRef _Nullable subsystems)
    CF_SWIFT_NAME(AGGraphRef.startTracing(self:flags:subsystems:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphStopTracing(AGGraphRef graph) CF_SWIFT_NAME(AGGraphRef.stopTracing(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSyncTracing(AGGraphRef graph) CF_SWIFT_NAME(AGGraphRef.syncTracing(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
CFStringRef AGGraphCopyTracePath(AGGraphRef graph) CF_SWIFT_NAME(getter:AGGraphRef.tracePath(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint64_t AGGraphAddTrace(AGGraphRef graph, const AGTraceRef trace, void *_Nullable context)
    CF_SWIFT_NAME(AGGraphRef.addTrace(self:_:context:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphRemoveTrace(AGGraphRef graph, uint64_t trace_id) CF_SWIFT_NAME(AGGraphRef.removeTrace(self:traceID:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetTrace(AGGraphRef graph, const AGTraceRef trace, void *_Nullable context)
    CF_SWIFT_NAME(AGGraphRef.setTrace(self:_:context:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphResetTrace(AGGraphRef graph) CF_SWIFT_NAME(AGGraphRef.resetTrace(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphIsTracingActive(AGGraphRef graph) CF_SWIFT_NAME(getter:AGGraphRef.isTracingActive(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphPrepareTrace(AGGraphRef graph, const AGTraceRef trace, void *_Nullable context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphTraceEventEnabled(AGGraphRef graph, uint32_t event_id)
    CF_SWIFT_NAME(AGGraphRef.traceEventEnabled(self:for:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphAddTraceEvent(AGGraphRef graph, const char *event_name, const void *value, AGTypeID type)
    CF_SWIFT_NAME(AGGraphRef.addTraceEvent(self:name:value:type:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphAddNamedTraceEvent(AGGraphRef graph, uint32_t event_id, uint32_t event_arg_count, const void *event_args,
                               CFDataRef data, uint32_t arg6)
    CF_SWIFT_NAME(AGGraphRef.addNamedTraceEvent(self:eventID:eventArgCount:eventArgs:data:arg6:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
const char *_Nullable AGGraphGetTraceEventName(uint32_t event_id) CF_SWIFT_NAME(AGGraphRef.traceEventName(for:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
const char *_Nullable AGGraphGetTraceEventSubsystem(uint32_t event_id)
    CF_SWIFT_NAME(AGGraphRef.traceEventSubsystem(for:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint32_t AGGraphRegisterNamedTraceEvent(const char *event_name, const char *event_subsystem)
    CF_SWIFT_NAME(AGGraphRef.registerNamedTraceEvent(name:subsystem:));

// MARK: Description

CF_EXPORT
CF_REFINED_FOR_SWIFT
CFTypeRef _Nullable AGGraphDescription(AGGraphRef _Nullable graph, CFDictionaryRef options)
    CF_SWIFT_NAME(AGGraphRef.description(_:options:));

CF_EXTERN_C_END

CF_IMPLICIT_BRIDGING_DISABLED
CF_ASSUME_NONNULL_END
