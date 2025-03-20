#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFData.h>

#include "AGSwiftSupport.h"
#include "Attribute/AGAttribute.h"
#include "Attribute/AGWeakAttribute.h"
#include "Swift/AGType.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

// MARK: CFType

typedef struct CF_BRIDGED_TYPE(id) AGGraphStorage *AGGraphRef AG_SWIFT_NAME(Graph);
typedef struct AGUnownedGraphContext *AGUnownedGraphContextRef;

CF_EXPORT
CF_REFINED_FOR_SWIFT
CFTypeID AGGraphGetTypeID();

// MARK: Graph Context

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGGraphRef AGGraphCreate();

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGGraphRef AGGraphCreateShared(AGGraphRef _Nullable graph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGUnownedGraphContextRef AGGraphGetGraphContext(AGGraphRef graph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGGraphRef AGGraphContextGetGraph(AGUnownedGraphContextRef context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void *AGGraphGetContext();

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetContext();

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
uint64_t AGGraphGetCounter(AGGraphRef graph, AGGraphCounterQuery query);

// MARK: Subgraphs

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphBeginDeferringSubgraphInvalidation(AGGraphRef graph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphEndDeferringSubgraphInvalidation(AGGraphRef graph);

// MARK: Attribute types

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint32_t AGGraphInternAttributeType(AGGraphRef graph, AGTypeID type,
                                    void *_Nonnull (*_Nonnull intern)(const void *context AG_SWIFT_CONTEXT)
                                        AG_SWIFT_CC(swift),
                                    const void *context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphVerifyType(AGAttribute attribute, AGTypeID type);

// MARK: Attributes

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateAttribute(uint32_t type_id, void *body, void *value);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGGraphRef AGGraphGetAttributeGraph(AGAttribute attribute);

// TODO: fix circular types

// CF_EXPORT
// CF_REFINED_FOR_SWIFT
// AGSubgraphRef AGGraphGetAttributeSubgraph(AGAttribute attribute);

// CF_EXPORT
// CF_REFINED_FOR_SWIFT
// AGSubgraphRef AGGraphGetAttributeSubgraph2(AGAttribute attribute);

// TODO: need this?
// typedef struct AGAttributeType {
//    AGTypeID typeID;
//    AGTypeID valueTypeID;
//    AGClosureStorage update;
//    AGAttributeVTable vTable;
//    AGAttributeTypeFlags flags;
//} AGAttributeType;

typedef struct AGAttributeInfo {
    const void *type;
    const void *body;
} AGAttributeInfo;

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttributeInfo AGGraphGetAttributeInfo(AGAttribute attribute);

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint8_t AGGraphGetFlags(AGAttribute attribute);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetFlags(AGAttribute attribute, uint8_t flags);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphMutateAttribute(AGAttribute attribute, AGTypeID type, bool flag,
                            const void (*modify)(void *body, const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                            const void *context);

typedef CF_OPTIONS(uint32_t, AGSearchOptions) {
    AGSearchOptionsSearchInputs = 1 << 0,
    AGSearchOptionsSearchOutputs = 1 << 1,
    AGSearchOptionsTraverseGraphContexts = 1 << 2,
};

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphSearch(AGAttribute attribute, AGSearchOptions options,
                   bool (*predicate)(AGAttribute attribute, const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                   const void *context);

// MARK: Cached attributes

CF_EXPORT
void AGGraphReadCachedAttribute();

CF_EXPORT
void AGGraphReadCachedAttributeIfExists();

// MARK: Current attribute

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGGraphGetCurrentAttribute();

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphCurrentAttributeWasModified();

// MARK: Indirect attributes

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateIndirectAttribute(AGAttribute attribute);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateIndirectAttribute2(AGAttribute attribute, size_t size);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGGraphGetIndirectAttribute(AGAttribute attribute);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetIndirectAttribute(AGAttribute attribute, AGAttribute source);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphResetIndirectAttribute(AGAttribute attribute, bool non_nil);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGGraphGetIndirectDependency(AGAttribute attribute);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetIndirectDependency(AGAttribute attribute, AGAttribute dependency);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphRegisterDependency(AGAttribute dependency, uint8_t input_edge_flags);

// MARK: Offset attributes

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateOffsetAttribute(AGAttribute attribute, uint32_t offset);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGGraphCreateOffsetAttribute2(AGAttribute attribute, uint32_t offset, size_t size);

// MARK: Updates

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphInvalidate(AGGraphRef graph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphInvalidateAllValues(AGGraphRef graph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphInvalidateValue(AGAttribute attribute);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetInvalidationCallback(AGGraphRef graph,
                                    void (*callback)(AGAttribute, const void *context AG_SWIFT_CONTEXT)
                                        AG_SWIFT_CC(swift),
                                    const void *callback_context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphUpdateValue(AGAttribute attribute, uint8_t options);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphCancelUpdate();

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphCancelUpdateIfNeeded();

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphUpdateWasCancelled();

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetUpdate(void *update);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetUpdateCallback(AGGraphRef graph,
                              void (*callback)(const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                              const void *callback_context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphClearUpdate();

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint64_t AGGraphGetDeadline(AGGraphRef graph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetDeadline(AGGraphRef graph, uint64_t deadline);

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphHasDeadlinePassed();

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetNeedsUpdate(AGGraphRef graph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphWithUpdate(AGAttribute attribute, void (*function)(const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                       const void *context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphWithoutUpdate(void (*function)(const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                          const void *context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphWithMainThreadHandler(AGGraphRef graph,
                                  void (*function)(const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                                  const void *body_context,
                                  void (*main_thread_handler)(void (*thunk)(void *),
                                                              const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                                  const void *main_thread_handler_context);

// MARK: Values

typedef struct AGValue {
    void *value;
    bool changed;
} AGValue;

typedef CF_OPTIONS(uint32_t, AGValueOptions) {
    // options & 3 == AG::InputEdge::Flags
    AGValueOptionsInputEdgeFlagsUnprefetched = 1 << 0,
    AGValueOptionsInputEdgeFlagsUnknown1 = 1 << 1,

    AGValueOptions0x04 = 1 << 2,

    //    AGValueOptionsUnknown1 = 1 << 0,
    //    AGValueOptionsValidateDirect = 1 << 1,
    //    AGValueOptionsUnknown3 = 1 << 2,
    //    AGValueOptionsResolveIndirect = 1 << 3,
    //    AGValueOptionsResolveWeak = 1 << 4,
};

typedef CF_OPTIONS(uint32_t, AGGraphUpdateStatus) {
    AGGraphUpdateStatusNoChange = 0,
    AGGraphUpdateStatusChanged = 1,
    AGGraphUpdateStatusOption2 = 2,
    AGGraphUpdateStatusNeedsCallMainHandler = 3,
};

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphHasValue(AGAttribute attribute);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGValue AGGraphGetValue(AGAttribute attribute, AGValueOptions options, AGTypeID type);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGValue AGGraphGetWeakValue(AGWeakAttribute attribute, AGValueOptions options, AGTypeID type);

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphSetValue(AGAttribute attribute, void *value, AGTypeID type);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGGraphUpdateStatus AGGraphPrefetchValue(AGAttribute attribute);

// MARK: Inputs

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGValue AGGraphGetInputValue(AGAttribute attribute, AGValueOptions options, AGTypeID type);

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint32_t AGGraphAddInput(AGAttribute attribute, AGAttribute input, uint8_t input_edge_flags);

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphAnyInputsChanged(const AGAttribute *exclude_attributes, uint64_t exclude_attributes_count);

// MARK: Outputs

CF_EXPORT
CF_REFINED_FOR_SWIFT
void *AGGraphGetOutputValue(AGTypeID type);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetOutputValue(void *value, AGTypeID type);

// MARK: Tracing

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphIsTracingActive(AGGraphRef graph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphPrepareTrace();

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint64_t AGGraphAddTrace(AGGraphRef graph, void *interface, void *trace_info);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphRemoveTrace(AGGraphRef graph, uint64_t trace_id);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphStartTracing(AGGraphRef graph, uint32_t tracing_flags);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphStartTracing2(AGGraphRef graph, uint32_t tracing_flags, uint32_t unknown);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphStopTracing(AGGraphRef graph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSyncTracing(AGGraphRef graph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
CFStringRef AGGraphCopyTracePath(AGGraphRef graph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphSetTrace(AGGraphRef graph, void *trace_vtable, void *trace);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphResetTrace(AGGraphRef graph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphTraceEventEnabled(AGGraphRef graph, uint32_t event_id);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphAddTraceEvent(AGGraphRef graph, const char *event_name, void *value, AGTypeID type);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphAddNamedTraceEvent(AGGraphRef graph, uint32_t event_id, uint32_t num_event_args, const uint64_t *event_args,
                               CFDataRef data, uint32_t arg6);

CF_EXPORT
CF_REFINED_FOR_SWIFT
const char *AGGraphGetTraceEventName(uint32_t event_id);

CF_EXPORT
CF_REFINED_FOR_SWIFT
const char *AGGraphGetTraceEventSubsystem(uint32_t event_id);

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint32_t AGGraphRegisterNamedTraceEvent(const char *key, const char *name);

// MARK: Profiler

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGGraphIsProfilingEnabled(AGAttribute attribute);

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint64_t AGGraphBeginProfileEvent(AGAttribute attribute, const char *event_name);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphEndProfileEvent(AGAttribute attribute, const char *event_name, uint64_t start_time, bool changed);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphStartProfiling(AGGraphRef graph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphStopProfiling(AGGraphRef graph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphMarkProfile(AGGraphRef graph, const char *event_name, uint64_t time);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphResetProfile(AGGraphRef graph);

// MARK: Description

CF_EXPORT
CF_REFINED_FOR_SWIFT
CFStringRef AGGraphDescription();

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphArchiveJSON(const char *filename);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGGraphArchiveJSON2(const char *filename, bool exclude_values);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
