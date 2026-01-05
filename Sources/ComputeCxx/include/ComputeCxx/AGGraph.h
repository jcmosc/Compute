#pragma once

#include <ComputeCxx/AGBase.h>

#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFDictionary.h>

#include <ComputeCxx/AGAttribute.h>
#include <ComputeCxx/AGAttributeInfo.h>
#include <ComputeCxx/AGAttributeType.h>
#include <ComputeCxx/AGChangedValue.h>
#include <ComputeCxx/AGComparison.h>
#include <ComputeCxx/AGGraphCounterQueryType.h>
#include <ComputeCxx/AGInputOptions.h>
#include <ComputeCxx/AGSearchOptions.h>
#include <ComputeCxx/AGTraceFlags.h>
#include <ComputeCxx/AGType.h>
#include <ComputeCxx/AGValue.h>
#include <ComputeCxx/AGWeakAttribute.h>

AG_ASSUME_NONNULL_BEGIN
AG_IMPLICIT_BRIDGING_ENABLED

AG_EXTERN_C_BEGIN

// MARK: CFType

typedef struct AG_BRIDGED_TYPE(id) AGGraphStorage *AGGraphRef AG_SWIFT_NAME(Graph);
typedef void *AGUnownedGraphContextRef AG_SWIFT_STRUCT;

typedef struct AGTrace *AGTraceRef;

AG_EXPORT
AG_REFINED_FOR_SWIFT
CFTypeID AGGraphGetTypeID(void) AG_SWIFT_NAME(getter:AGGraphRef.typeID());

// MARK: Graph Context

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGGraphRef AGGraphCreate(void) AG_SWIFT_NAME(AGGraphRef.init());

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGGraphRef AGGraphCreateShared(AGGraphRef _Nullable graph) AG_SWIFT_NAME(AGGraphRef.init(shared:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGUnownedGraphContextRef AGGraphGetGraphContext(AGGraphRef graph) AG_SWIFT_NAME(getter:AGGraphRef.graphContext(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGGraphRef AGGraphContextGetGraph(void *context);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphInvalidate(AGGraphRef graph) AG_SWIFT_NAME(AGGraphRef.invalidate(self:));

// MARK: User context

AG_EXPORT
AG_REFINED_FOR_SWIFT
const void *_Nullable AGGraphGetContext(AGGraphRef graph) AG_SWIFT_NAME(getter:AGGraphRef.context(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetContext(AGGraphRef graph, const void *_Nullable context)
    AG_SWIFT_NAME(setter:AGGraphRef.context(self:_:));

// MARK: Counter

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint64_t AGGraphGetCounter(AGGraphRef graph, AGGraphCounterQueryType query)
    AG_SWIFT_NAME(AGGraphRef.counter(self:for:));

// MARK: Main handler

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphWithMainThreadHandler(AGGraphRef graph,
                                  void (*body)(const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                                  const void *body_context,
                                  void (*main_thread_handler)(const void *context AG_SWIFT_CONTEXT,
                                                              void (*trampoline_thunk)(const void *),
                                                              const void *trampoline) AG_SWIFT_CC(swift),
                                  const void *main_thread_handler_context);

// MARK: Subgraphs

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphBeginDeferringSubgraphInvalidation(AGGraphRef graph)
    AG_SWIFT_NAME(AGGraphRef.beginDeferringSubgraphInvalidation(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphEndDeferringSubgraphInvalidation(AGGraphRef graph, bool was_deferring)
    AG_SWIFT_NAME(AGGraphRef.endDeferringSubgraphInvalidation(self:wasDeferring:));

// MARK: Attribute types

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint32_t AGGraphInternAttributeType(AGUnownedGraphContextRef graph, AGTypeID type,
                                    const AGAttributeType *_Nonnull (*_Nonnull make_attribute_type)(
                                        const void *_Nullable context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                                    const void *_Nullable make_attribute_type_context);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphVerifyType(AGAttribute attribute, AGTypeID type) AG_SWIFT_NAME(AGAttribute.verifyType(self:type:));

// MARK: Attributes

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateAttribute(uint32_t type_id, const void *body, const void *_Nullable value)
    AG_SWIFT_NAME(AGAttribute.init(type:body:value:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGGraphRef AGGraphGetAttributeGraph(AGAttribute attribute) AG_SWIFT_NAME(getter:AGAttribute.graph(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttributeInfo AGGraphGetAttributeInfo(AGAttribute attribute) AG_SWIFT_NAME(getter:AGAttribute.info(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttributeFlags AGGraphGetFlags(AGAttribute attribute) AG_SWIFT_NAME(getter:AGAttribute.flags(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetFlags(AGAttribute attribute, AGAttributeFlags flags) AG_SWIFT_NAME(setter:AGAttribute.flags(self:_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint32_t AGGraphAddInput(AGAttribute attribute, AGAttribute input, AGInputOptions options)
    AG_SWIFT_NAME(AGAttribute.addInput(self:_:options:));

// MARK: Offset attributes

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateOffsetAttribute(AGAttribute attribute, uint32_t offset);

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateOffsetAttribute2(AGAttribute attribute, uint32_t offset, size_t size);

// MARK: Indirect attributes

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateIndirectAttribute(AGAttribute attribute) AG_SWIFT_NAME(AGAttribute.createIndirect(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateIndirectAttribute2(AGAttribute attribute, size_t size);

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGGraphGetIndirectAttribute(AGAttribute attribute) AG_SWIFT_NAME(getter:AGAttribute.source(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetIndirectAttribute(AGAttribute attribute, AGAttribute source)
    AG_SWIFT_NAME(setter:AGAttribute.source(self:_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphResetIndirectAttribute(AGAttribute attribute, bool non_nil);

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGGraphGetIndirectDependency(AGAttribute attribute);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetIndirectDependency(AGAttribute attribute, AGAttribute dependency);

// MARK: Search

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphSearch(AGAttribute attribute, AGSearchOptions options,
                   bool (*predicate)(const void *context AG_SWIFT_CONTEXT, AGAttribute attribute) AG_SWIFT_CC(swift),
                   const void *predicate_context);

// MARK: Body

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphMutateAttribute(AGAttribute attribute, AGTypeID type, bool invalidating,
                            void (*modify)(const void *context AG_SWIFT_CONTEXT, void *body) AG_SWIFT_CC(swift),
                            const void *modify_context);

// MARK: Value

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGChangedValue AGGraphGetValue(AGAttribute attribute, AGValueOptions options, AGTypeID type);

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGWeakChangedValue AGGraphGetWeakValue(AGWeakAttribute attribute, AGValueOptions options, AGTypeID type);

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGChangedValue AGGraphGetInputValue(AGAttribute attribute, AGAttribute input, AGValueOptions options, AGTypeID type);

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphSetValue(AGAttribute attribute, const void *value, AGTypeID type);

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphHasValue(AGAttribute attribute) AG_SWIFT_NAME(getter:AGAttribute.hasValue(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGValueState AGGraphGetValueState(AGAttribute attribute) AG_SWIFT_NAME(getter:AGAttribute.valueState(self:));

typedef AG_OPTIONS(uint32_t, AGGraphUpdateOptions) {
    AGGraphUpdateOptionsNone = 0,
    AGGraphUpdateOptionsInTransaction = 1 << 0,
    AGGraphUpdateOptionsAbortIfCancelled = 1 << 1,
    AGGraphUpdateOptionsCancelIfPassedDeadline = 1 << 2,
    AGGraphUpdateOptionsInitializeCleared = 1 << 3,
    AGGraphUpdateOptionsEndDeferringSubgraphInvalidationOnExit = 1 << 4,
};

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphUpdateValue(AGAttribute attribute, AGGraphUpdateOptions options)
    AG_SWIFT_NAME(AGAttribute.updateValue(self:options:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint32_t AGGraphPrefetchValue(AGAttribute attribute) AG_SWIFT_NAME(AGAttribute.prefetchValue(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphInvalidateValue(AGAttribute attribute) AG_SWIFT_NAME(AGAttribute.invalidateValue(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphInvalidateAllValues(AGGraphRef graph) AG_SWIFT_NAME(AGGraphRef.invalidateAllValues(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetInvalidationCallback(AGGraphRef graph,
                                    void (*callback)(const void *context AG_SWIFT_CONTEXT, AGAttribute)
                                        AG_SWIFT_CC(swift),
                                    const void *callback_context);

// MARK: Update

typedef AG_ENUM(uint32_t, AGGraphUpdateStatus) {
    AGGraphUpdateStatusNoChange = 0,
    AGGraphUpdateStatusChanged = 1,
    AGGraphUpdateStatusAborted = 2,
    AGGraphUpdateStatusNeedsCallMainHandler = 3,
};

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetUpdate(const void *update) AG_SWIFT_NAME(AGGraphRef.setUpdate(_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
const void *AGGraphClearUpdate(void) AG_SWIFT_NAME(AGGraphRef.clearUpdate());

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphCancelUpdate(void) AG_SWIFT_NAME(AGGraphRef.cancelUpdate());

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphCancelUpdateIfNeeded(void) AG_SWIFT_NAME(AGGraphRef.cancelUpdateIfNeeded());

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphUpdateWasCancelled(void) AG_SWIFT_NAME(getter:AGGraphRef.updateWasCancelled());

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint64_t AGGraphGetDeadline(AGGraphRef graph) AG_SWIFT_NAME(getter:AGGraphRef.deadline(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetDeadline(AGGraphRef graph, uint64_t deadline) AG_SWIFT_NAME(setter:AGGraphRef.deadline(self:_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphHasDeadlinePassed(void) AG_SWIFT_NAME(getter:AGGraphRef.hasDeadlinePassed());

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetNeedsUpdate(AGGraphRef graph) AG_SWIFT_NAME(AGGraphRef.setNeedsUpdate(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphWithUpdate(AGAttribute attribute, void (*body)(const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                       const void *body_context);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphWithoutUpdate(void (*body)(const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                          const void *body_context);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetUpdateCallback(AGGraphRef graph,
                              void (*callback)(const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                              const void *callback_context);

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGGraphGetCurrentAttribute(void);

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphCurrentAttributeWasModified(void) AG_SWIFT_NAME(getter:AGAttribute.currentWasModified());

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphAnyInputsChanged(const AGAttribute *AG_COUNTED_BY(count) exclude_attributes, size_t count);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void *_Nullable AGGraphGetOutputValue(AGTypeID type);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetOutputValue(const void *value, AGTypeID type);

// MARK: Trace

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphStartTracing(AGGraphRef _Nullable graph, AGTraceFlags trace_flags)
    AG_SWIFT_NAME(AGGraphRef.startTracing(_:flags:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphStartTracing2(AGGraphRef _Nullable graph, AGTraceFlags trace_flags, CFArrayRef _Nullable subsystems)
    AG_SWIFT_NAME(AGGraphRef.startTracing(_:flags:subsystems:)); // TODO: flags or options

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphStopTracing(AGGraphRef _Nullable graph) AG_SWIFT_NAME(AGGraphRef.stopTracing(_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSyncTracing(AGGraphRef graph) AG_SWIFT_NAME(AGGraphRef.syncTracing(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
CFStringRef AGGraphCopyTracePath(AGGraphRef graph) AG_SWIFT_NAME(getter:AGGraphRef.tracePath(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint64_t AGGraphAddTrace(AGGraphRef graph, const AGTraceRef trace, void *_Nullable context)
    AG_SWIFT_NAME(AGGraphRef.addTrace(self:_:context:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphRemoveTrace(AGGraphRef graph, uint64_t trace_id) AG_SWIFT_NAME(AGGraphRef.removeTrace(self:traceID:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetTrace(AGGraphRef graph, const AGTraceRef trace, void *_Nullable context)
    AG_SWIFT_NAME(AGGraphRef.setTrace(self:_:context:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphResetTrace(AGGraphRef graph) AG_SWIFT_NAME(AGGraphRef.resetTrace(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphIsTracingActive(AGGraphRef graph) AG_SWIFT_NAME(getter:AGGraphRef.isTracingActive(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphPrepareTrace(AGGraphRef graph, const AGTraceRef trace, void *_Nullable context);

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphTraceEventEnabled(AGGraphRef graph, uint32_t event_id)
    AG_SWIFT_NAME(AGGraphRef.traceEventEnabled(self:for:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphAddTraceEvent(AGGraphRef graph, const char *event_name, const void *value, AGTypeID type)
    AG_SWIFT_NAME(AGGraphRef.addTraceEvent(self:name:value:type:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphAddNamedTraceEvent(AGGraphRef graph, uint32_t event_id, uint32_t event_arg_count, const void *event_args,
                               CFDataRef data, uint32_t arg6)
    AG_SWIFT_NAME(AGGraphRef.addNamedTraceEvent(self:eventID:eventArgCount:eventArgs:data:arg6:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
const char *_Nullable AGGraphGetTraceEventName(uint32_t event_id) AG_SWIFT_NAME(AGGraphRef.traceEventName(for:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
const char *_Nullable AGGraphGetTraceEventSubsystem(uint32_t event_id)
    AG_SWIFT_NAME(AGGraphRef.traceEventSubsystem(for:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint32_t AGGraphRegisterNamedTraceEvent(const char *event_name, const char *event_subsystem)
    AG_SWIFT_NAME(AGGraphRef.registerNamedTraceEvent(name:subsystem:));

// MARK: Description

AG_EXPORT
AG_REFINED_FOR_SWIFT
CFTypeRef _Nullable AGGraphDescription(AGGraphRef _Nullable graph, CFDictionaryRef options)
    AG_SWIFT_NAME(AGGraphRef.description(_:options:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphArchiveJSON(const char *_Nullable filename) AG_SWIFT_NAME(AGGraphRef.archiveJSON(name:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphArchiveJSON2(const char *filename, bool exclude_values)
    AG_SWIFT_NAME(AGGraphRef.archiveJSON(name:excludeValues:));

AG_EXTERN_C_END

AG_IMPLICIT_BRIDGING_DISABLED
AG_ASSUME_NONNULL_END
