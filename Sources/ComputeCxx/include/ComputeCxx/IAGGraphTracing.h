#pragma once

#include <ComputeCxx/IAGBase.h>
#include <ComputeCxx/IAGGraph.h>
#include <ComputeCxx/IAGUniqueID.h>

typedef IAG_OPTIONS(uint32_t, IAGGraphTraceFlags){
    IAGGraphTraceFlagsEnabled = 1 << 0,
    IAGGraphTraceFlagsFull = 1 << 1,
    IAGGraphTraceFlagsBacktrace = 1 << 2,
    IAGGraphTraceFlagsPrepare = 1 << 3,
    IAGGraphTraceFlagsCustom = 1 << 4,
    IAGGraphTraceFlagsAll = 1 << 5,
} IAG_SWIFT_NAME(Graph.TraceFlags);

typedef struct IAGTraceType *IAGTraceTypeRef;

typedef uint32_t IAGNamedTraceEventID IAG_SWIFT_STRUCT IAG_SWIFT_NAME(Graph.NamedTraceEventID);

typedef IAG_OPTIONS(uint32_t, IAGNamedTraceEventFlags){
    IAGNamedTraceEventFlagsRecordBacktrace = 1ul << 31,
} IAG_SWIFT_NAME(Graph.NamedTraceEventFlags);

IAG_ASSUME_NONNULL_BEGIN
IAG_IMPLICIT_BRIDGING_ENABLED

IAG_EXTERN_C_BEGIN

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphStartTracing(IAGGraphRef _Nullable graph, IAGGraphTraceFlags trace_flags)
    IAG_SWIFT_NAME(Graph.startTracing(_:flags:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphStartTracing2(IAGGraphRef _Nullable graph, IAGGraphTraceFlags trace_flags, CFArrayRef _Nullable subsystems)
    IAG_SWIFT_NAME(Graph.startTracing(_:flags:subsystems:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphStopTracing(IAGGraphRef _Nullable graph) IAG_SWIFT_NAME(Graph.stopTracing(_:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphSyncTracing(IAGGraphRef _Nullable graph) IAG_SWIFT_NAME(Graph.syncTracing(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
CFStringRef _Nullable IAGGraphCopyTracePath(IAGGraphRef _Nullable graph) IAG_SWIFT_NAME(Graph.tracePath(_:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphSetTrace(IAGGraphRef graph, const IAGTraceTypeRef trace, void *_Nullable context)
    IAG_SWIFT_NAME(Graph.setTrace(self:_:context:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphResetTrace(IAGGraphRef graph) IAG_SWIFT_NAME(Graph.resetTrace(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGUniqueID IAGGraphAddTrace(IAGGraphRef graph, const IAGTraceTypeRef trace, void *_Nullable context)
    IAG_SWIFT_NAME(Graph.addTrace(self:_:context:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphRemoveTrace(IAGGraphRef graph, IAGUniqueID trace_id) IAG_SWIFT_NAME(Graph.removeTrace(self:traceID:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGGraphIsTracingActive(IAGGraphRef graph) IAG_SWIFT_NAME(getter:Graph.isTracingActive(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphPrepareTrace(IAGGraphRef graph, const IAGTraceTypeRef trace, void *_Nullable context);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphAddTraceEvent(IAGGraphRef graph, const char *event_name, const void *value, IAGTypeID type)
    IAG_SWIFT_NAME(Graph.addTraceEvent(self:name:value:type:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGGraphTraceEventEnabled(IAGGraphRef graph, uint32_t event_id) IAG_SWIFT_NAME(Graph.traceEventEnabled(self:for:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGNamedTraceEventID IAGGraphRegisterNamedTraceEvent(const char *event_name, const char *event_subsystem)
    IAG_SWIFT_NAME(Graph.registerNamedTraceEvent(name:subsystem:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
const char *_Nullable IAGGraphGetTraceEventName(IAGNamedTraceEventID event_id)
    IAG_SWIFT_NAME(Graph.traceEventName(for:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
const char *_Nullable IAGGraphGetTraceEventSubsystem(IAGNamedTraceEventID event_id)
    IAG_SWIFT_NAME(Graph.traceEventSubsystem(for:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphAddNamedTraceEvent(IAGGraphRef graph, IAGNamedTraceEventID event_id, size_t event_arg_count,
                                const uint32_t *_Nullable IAG_COUNTED_BY(event_arg_count) event_args,
                                CFDataRef _Nullable data, IAGNamedTraceEventFlags flags)
    IAG_SWIFT_NAME(Graph.addNamedTraceEvent(self:eventID:eventArgCount:eventArgs:data:flags:));

IAG_EXTERN_C_END

IAG_IMPLICIT_BRIDGING_DISABLED
IAG_ASSUME_NONNULL_END
