#pragma once

#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFDictionary.h>

#include <ComputeCxx/AGAttribute.h>
#include <ComputeCxx/AGAttributeInfo.h>
#include <ComputeCxx/AGAttributeType.h>
#include <ComputeCxx/AGChangedValue.h>
#include <ComputeCxx/AGComparison.h>
#include <ComputeCxx/AGInputOptions.h>
#include <ComputeCxx/AGSearchOptions.h>
#include <ComputeCxx/AGSwiftSupport.h>
#include <ComputeCxx/AGTraceFlags.h>
#include <ComputeCxx/AGType.h>
#include <ComputeCxx/AGValue.h>
#include <ComputeCxx/AGWeakAttribute.h>

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
CFTypeID AGGraphGetTypeID(void) CF_SWIFT_NAME(getter:AGGraphRef.typeID());

// MARK: Graph Context

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGGraphRef AGGraphCreate(void) CF_SWIFT_NAME(AGGraphRef.init());

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

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphInvalidate(AGGraphRef graph);

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
    AGGraphCounterQueryNodeCount,
    AGGraphCounterQueryTransactionCount,
    AGGraphCounterQueryUpdateCount,
    AGGraphCounterQueryChangeCount,
    AGGraphCounterQueryContextID,
    AGGraphCounterQueryGraphID,
    AGGraphCounterQueryContextThreadUpdating,
    AGGraphCounterQueryThreadUpdating,
    AGGraphCounterQueryContextNeedsUpdate,
    AGGraphCounterQueryNeedsUpdate,
    AGGraphCounterQueryMainThreadUpdateCount,
    AGGraphCounterQueryNodeTotalCount,
    AGGraphCounterQuerySubgraphCount,
    AGGraphCounterQuerySubgraphTotalCount,
};

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint64_t AGGraphGetCounter(AGGraphRef graph, AGGraphCounterQuery query) CF_SWIFT_NAME(AGGraphRef.counter(self:for:));

// MARK: Main handler

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphWithMainThreadHandler(AGGraphRef graph, void (*body)(void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                                  void *body_context,
                                  void (*main_thread_handler)(const void *context AG_SWIFT_CONTEXT,
                                                              void (*trampoline_thunk)(const void *),
                                                              const void *trampoline) AG_SWIFT_CC(swift),
                                  const void *main_thread_handler_context);

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

// MARK: Offset attributes

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateOffsetAttribute(AGAttribute attribute, uint32_t offset);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateOffsetAttribute2(AGAttribute attribute, uint32_t offset, size_t size);

// MARK: Indirect attributes

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateIndirectAttribute(AGAttribute attribute);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateIndirectAttribute2(AGAttribute attribute, size_t size);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGGraphGetIndirectAttribute(AGAttribute attribute) CF_SWIFT_NAME(getter:AGAttribute.indirectSource(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetIndirectAttribute(AGAttribute attribute, AGAttribute source)
    CF_SWIFT_NAME(setter:AGAttribute.indirectSource(self:_:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphResetIndirectAttribute(AGAttribute attribute, bool non_nil);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGGraphGetIndirectDependency(AGAttribute attribute);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetIndirectDependency(AGAttribute attribute, AGAttribute dependency);

// MARK: Search

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphSearch(AGAttribute attribute, AGSearchOptions options,
                   bool (*predicate)(void *context AG_SWIFT_CONTEXT, AGAttribute attribute) AG_SWIFT_CC(swift),
                   void *predicate_context);

// MARK: Body

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphMutateAttribute(AGAttribute attribute, AGTypeID type, bool invalidating,
                            void (*modify)(void *context AG_SWIFT_CONTEXT, void *body) AG_SWIFT_CC(swift),
                            void *modify_context);

// MARK: Value


CF_EXPORT
CF_REFINED_FOR_SWIFT
AGChangedValue AGGraphGetValue(AGAttribute attribute, AGValueOptions options, AGTypeID type);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGChangedValue AGGraphGetWeakValue(AGWeakAttribute attribute, AGValueOptions options, AGTypeID type);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGChangedValue AGGraphGetInputValue(AGAttribute attribute, AGAttribute input, AGValueOptions options, AGTypeID type);

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphSetValue(AGAttribute attribute, const void *value, AGTypeID type);

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphHasValue(AGAttribute attribute) CF_SWIFT_NAME(getter:AGAttribute.hasValue(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGValueState AGGraphGetValueState(AGAttribute attribute) CF_SWIFT_NAME(getter:AGAttribute.valueState(self:));

typedef CF_OPTIONS(uint32_t, AGGraphUpdateOptions) {
    AGGraphUpdateOptionsNone = 0,
    AGGraphUpdateOptionsInTransaction = 1 << 0,
    AGGraphUpdateOptionsAbortIfCancelled = 1 << 1,
    AGGraphUpdateOptionsCancelIfPassedDeadline = 1 << 2,
    AGGraphUpdateOptionsInitializeCleared = 1 << 3,
    AGGraphUpdateOptionsEndDeferringSubgraphInvalidationOnExit = 1 << 4,
};

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphUpdateValue(AGAttribute attribute, AGGraphUpdateOptions options)
    CF_SWIFT_NAME(AGAttribute.updateValue(self:options:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint32_t AGGraphPrefetchValue(AGAttribute attribute) CF_SWIFT_NAME(AGAttribute.prefetchValue(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphInvalidateValue(AGAttribute attribute) CF_SWIFT_NAME(AGAttribute.invalidateValue(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphInvalidateAllValues(AGGraphRef graph) CF_SWIFT_NAME(AGGraphRef.invalidateAllValues(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetInvalidationCallback(AGGraphRef graph,
                                    void (*callback)(const void *context AG_SWIFT_CONTEXT, AGAttribute)
                                        AG_SWIFT_CC(swift),
                                    const void *callback_context);

// MARK: Update

typedef CF_ENUM(uint32_t, AGGraphUpdateStatus) {
    AGGraphUpdateStatusNoChange = 0,
    AGGraphUpdateStatusChanged = 1,
    AGGraphUpdateStatusAborted = 2,
    AGGraphUpdateStatusNeedsCallMainHandler = 3,
};

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetUpdate(const void *update) CF_SWIFT_NAME(AGGraphRef.setUpdate(_:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
const void *AGGraphClearUpdate(void) CF_SWIFT_NAME(AGGraphRef.clearUpdate());

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphCancelUpdate(void) CF_SWIFT_NAME(AGGraphRef.cancelUpdate());

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphCancelUpdateIfNeeded(void) CF_SWIFT_NAME(AGGraphRef.cancelUpdateIfNeeded());

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphUpdateWasCancelled(void) CF_SWIFT_NAME(getter:AGGraphRef.updateWasCancelled());

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint64_t AGGraphGetDeadline(AGGraphRef graph) CF_SWIFT_NAME(getter:AGGraphRef.deadline(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetDeadline(AGGraphRef graph, uint64_t deadline) CF_SWIFT_NAME(setter:AGGraphRef.deadline(self:_:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphHasDeadlinePassed(void) CF_SWIFT_NAME(getter:AGGraphRef.hasDeadlinePassed());

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetNeedsUpdate(AGGraphRef graph) CF_SWIFT_NAME(AGGraphRef.setNeedsUpdate(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphWithUpdate(AGAttribute attribute, void (*body)(void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                       void *body_context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphWithoutUpdate(void (*body)(void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift), void *body_context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetUpdateCallback(AGGraphRef graph, void (*callback)(void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                              void *callback_context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGGraphGetCurrentAttribute(void);

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphCurrentAttributeWasModified(void) CF_SWIFT_NAME(getter:AGAttribute.currentWasModified());

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphAnyInputsChanged(const AGAttribute *exclude_attributes, uint64_t exclude_attributes_count);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void *_Nullable AGGraphGetOutputValue(AGTypeID type);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetOutputValue(const void *value, AGTypeID type);

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

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphArchiveJSON(const char *_Nullable filename);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphArchiveJSON2(const char *filename, bool exclude_values);

CF_EXTERN_C_END

CF_IMPLICIT_BRIDGING_DISABLED
CF_ASSUME_NONNULL_END
