#pragma once

#include <ComputeCxx/IAGBase.h>
#include <ComputeCxx/IAGGraph.h>
#include <ComputeCxx/IAGUniqueID.h>

typedef IAG_OPTIONS(uint32_t, IAGGraphTraceFlags) {
    IAGGraphTraceFlagsEnabled = 1 << 0,
    IAGGraphTraceFlagsFull = 1 << 1,
    IAGGraphTraceFlagsBacktrace = 1 << 2,
    IAGGraphTraceFlagsPrepare = 1 << 3,
    IAGGraphTraceFlagsCustom = 1 << 4,
    IAGGraphTraceFlagsAll = 1 << 5,
} IAG_SWIFT_NAME(IAGGraphRef.TraceFlags);

typedef struct IAGTraceType *IAGTraceTypeRef;

typedef uint32_t IAGNamedTraceEventID IAG_SWIFT_STRUCT IAG_SWIFT_NAME(Graph.NamedTraceEventID);

IAG_ASSUME_NONNULL_BEGIN

IAG_EXTERN_C_BEGIN

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphStartTracing(IAGGraphRef _Nullable graph, IAGGraphTraceFlags trace_flags)
    IAG_SWIFT_NAME(IAGGraphRef.startTracing(_:flags:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphStartTracing2(IAGGraphRef _Nullable graph, IAGGraphTraceFlags trace_flags,
                          CFArrayRef _Nullable subsystems)
    IAG_SWIFT_NAME(IAGGraphRef.startTracing(_:flags:subsystems:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphStopTracing(IAGGraphRef _Nullable graph) IAG_SWIFT_NAME(IAGGraphRef.stopTracing(_:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphSyncTracing(IAGGraphRef graph) IAG_SWIFT_NAME(IAGGraphRef.syncTracing(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
CFStringRef IAGGraphCopyTracePath(IAGGraphRef graph) IAG_SWIFT_NAME(getter:IAGGraphRef.tracePath(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGUniqueID IAGGraphAddTrace(IAGGraphRef graph, const IAGTraceTypeRef trace, void *_Nullable context)
    IAG_SWIFT_NAME(IAGGraphRef.addTrace(self:_:context:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphRemoveTrace(IAGGraphRef graph, IAGUniqueID trace_id) IAG_SWIFT_NAME(IAGGraphRef.removeTrace(self:traceID:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphSetTrace(IAGGraphRef graph, const IAGTraceTypeRef trace, void *_Nullable context)
    IAG_SWIFT_NAME(IAGGraphRef.setTrace(self:_:context:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphResetTrace(IAGGraphRef graph) IAG_SWIFT_NAME(IAGGraphRef.resetTrace(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGGraphIsTracingActive(IAGGraphRef graph) IAG_SWIFT_NAME(getter:IAGGraphRef.isTracingActive(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphPrepareTrace(IAGGraphRef graph, const IAGTraceTypeRef trace, void *_Nullable context);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGGraphTraceEventEnabled(IAGGraphRef graph, uint32_t event_id)
    IAG_SWIFT_NAME(IAGGraphRef.traceEventEnabled(self:for:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphAddTraceEvent(IAGGraphRef graph, const char *event_name, const void *value, IAGTypeID type)
    IAG_SWIFT_NAME(IAGGraphRef.addTraceEvent(self:name:value:type:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphAddNamedTraceEvent(IAGGraphRef graph, IAGNamedTraceEventID event_id, uint32_t event_arg_count, const void **event_args,
                               CFDataRef data, uint32_t arg6)
    IAG_SWIFT_NAME(IAGGraphRef.addNamedTraceEvent(self:eventID:eventArgCount:eventArgs:data:arg6:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
const char *_Nullable IAGGraphGetTraceEventName(IAGNamedTraceEventID event_id) IAG_SWIFT_NAME(IAGGraphRef.traceEventName(for:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
const char *_Nullable IAGGraphGetTraceEventSubsystem(IAGNamedTraceEventID event_id)
    IAG_SWIFT_NAME(IAGGraphRef.traceEventSubsystem(for:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGNamedTraceEventID IAGGraphRegisterNamedTraceEvent(const char *event_name, const char *event_subsystem)
    IAG_SWIFT_NAME(IAGGraphRef.registerNamedTraceEvent(name:subsystem:));

IAG_EXTERN_C_END

IAG_ASSUME_NONNULL_END
