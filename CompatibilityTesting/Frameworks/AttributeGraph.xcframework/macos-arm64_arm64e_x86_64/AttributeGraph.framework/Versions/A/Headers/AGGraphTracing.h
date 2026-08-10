#pragma once

#include <AttributeGraph/AGBase.h>
#include <AttributeGraph/AGGraph.h>
#include <AttributeGraph/AGUniqueID.h>

typedef AG_OPTIONS(uint32_t, AGGraphTraceFlags){
    AGGraphTraceFlagsEnabled = 1 << 0,
    AGGraphTraceFlagsFull = 1 << 1,
    AGGraphTraceFlagsBacktrace = 1 << 2,
    AGGraphTraceFlagsPrepare = 1 << 3,
    AGGraphTraceFlagsCustom = 1 << 4,
    AGGraphTraceFlagsAll = 1 << 5,
} AG_SWIFT_NAME(Graph.TraceFlags);

typedef struct AGTraceType *AGTraceTypeRef;

typedef uint32_t AGNamedTraceEventID AG_SWIFT_STRUCT AG_SWIFT_NAME(Graph.NamedTraceEventID);

typedef AG_OPTIONS(uint32_t, AGNamedTraceEventFlags){
    AGNamedTraceEventFlagsRecordBacktrace = 1ul << 31,
} AG_SWIFT_NAME(Graph.NamedTraceEventFlags);

AG_ASSUME_NONNULL_BEGIN
AG_IMPLICIT_BRIDGING_ENABLED

AG_EXTERN_C_BEGIN

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphStartTracing(AGGraphRef _Nullable graph, AGGraphTraceFlags trace_flags)
    AG_SWIFT_NAME(Graph.startTracing(_:flags:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphStartTracing2(AGGraphRef _Nullable graph, AGGraphTraceFlags trace_flags, CFArrayRef _Nullable subsystems)
    AG_SWIFT_NAME(Graph.startTracing(_:flags:subsystems:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphStopTracing(AGGraphRef _Nullable graph) AG_SWIFT_NAME(Graph.stopTracing(_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSyncTracing(AGGraphRef _Nullable graph) AG_SWIFT_NAME(Graph.syncTracing(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
CFStringRef _Nullable AGGraphCopyTracePath(AGGraphRef _Nullable graph) AG_SWIFT_NAME(Graph.tracePath(_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphSetTrace(AGGraphRef graph, const AGTraceTypeRef trace, void *_Nullable context)
    AG_SWIFT_NAME(Graph.setTrace(self:_:context:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphResetTrace(AGGraphRef graph) AG_SWIFT_NAME(Graph.resetTrace(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGUniqueID AGGraphAddTrace(AGGraphRef graph, const AGTraceTypeRef trace, void *_Nullable context)
    AG_SWIFT_NAME(Graph.addTrace(self:_:context:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphRemoveTrace(AGGraphRef graph, AGUniqueID trace_id) AG_SWIFT_NAME(Graph.removeTrace(self:traceID:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphIsTracingActive(AGGraphRef graph) AG_SWIFT_NAME(getter:Graph.isTracingActive(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphPrepareTrace(AGGraphRef graph, const AGTraceTypeRef trace, void *_Nullable context);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphAddTraceEvent(AGGraphRef graph, const char *event_name, const void *value, AGTypeID type)
    AG_SWIFT_NAME(Graph.addTraceEvent(self:name:value:type:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphTraceEventEnabled(AGGraphRef graph, uint32_t event_id) AG_SWIFT_NAME(Graph.traceEventEnabled(self:for:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGNamedTraceEventID AGGraphRegisterNamedTraceEvent(const char *event_name, const char *event_subsystem)
    AG_SWIFT_NAME(Graph.registerNamedTraceEvent(name:subsystem:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
const char *_Nullable AGGraphGetTraceEventName(AGNamedTraceEventID event_id)
    AG_SWIFT_NAME(Graph.traceEventName(for:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
const char *_Nullable AGGraphGetTraceEventSubsystem(AGNamedTraceEventID event_id)
    AG_SWIFT_NAME(Graph.traceEventSubsystem(for:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphAddNamedTraceEvent(AGGraphRef graph, AGNamedTraceEventID event_id, size_t event_arg_count,
                                const uint32_t *_Nullable AG_COUNTED_BY(event_arg_count) event_args,
                                CFDataRef _Nullable data, AGNamedTraceEventFlags flags)
    AG_SWIFT_NAME(Graph.addNamedTraceEvent(self:eventID:eventArgCount:eventArgs:data:flags:));

AG_EXTERN_C_END

AG_IMPLICIT_BRIDGING_DISABLED
AG_ASSUME_NONNULL_END
