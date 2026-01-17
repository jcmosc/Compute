#pragma once

#include <ComputeCxx/AGBase.h>
#include <ComputeCxx/AGGraph.h>

typedef AG_OPTIONS(uint32_t, AGGraphTraceOptions) {
    AGGraphTraceOptionsEnabled = 1 << 0,
    AGGraphTraceOptionsFull = 1 << 1,
    AGGraphTraceOptionsBacktrace = 1 << 2,
    AGGraphTraceOptionsPrepare = 1 << 3,
    AGGraphTraceOptionsCustom = 1 << 4,
    AGGraphTraceOptionsAll = 1 << 5,
} AG_SWIFT_NAME(AGGraphRef.TraceOptions);

typedef struct AGTraceType *AGTraceTypeRef;

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphStartTracing(AGGraphRef _Nullable graph, AGGraphTraceOptions trace_options)
    AG_SWIFT_NAME(AGGraphRef.startTracing(_:options:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphStartTracing2(AGGraphRef _Nullable graph, AGGraphTraceOptions trace_options,
                          CFArrayRef _Nullable subsystems)
    AG_SWIFT_NAME(AGGraphRef.startTracing(_:options:subsystems:));

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
uint64_t AGGraphAddTrace(AGGraphRef graph, const AGTraceTypeRef trace, void *_Nullable context)
    AG_SWIFT_NAME(AGGraphRef.addTrace(self:_:context:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphRemoveTrace(AGGraphRef graph, uint64_t trace_id) AG_SWIFT_NAME(AGGraphRef.removeTrace(self:traceID:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetTrace(AGGraphRef graph, const AGTraceTypeRef trace, void *_Nullable context)
    AG_SWIFT_NAME(AGGraphRef.setTrace(self:_:context:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphResetTrace(AGGraphRef graph) AG_SWIFT_NAME(AGGraphRef.resetTrace(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphIsTracingActive(AGGraphRef graph) AG_SWIFT_NAME(getter:AGGraphRef.isTracingActive(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphPrepareTrace(AGGraphRef graph, const AGTraceTypeRef trace, void *_Nullable context);

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

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
