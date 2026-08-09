#pragma once

#include <ComputeCxx/IAGBase.h>
#include <ComputeCxx/IAGGraph.h>
#include <ComputeCxx/IAGUniqueID.h>

typedef IAG_OPTIONS(uint32_t, IAGGraphProfileFlags){
    IAGGraphProfileFlagsEnabled = 1 << 0,
} IAG_SWIFT_NAME(Graph.ProfileFlags);

IAG_ASSUME_NONNULL_BEGIN

IAG_EXTERN_C_BEGIN

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphStartProfiling(IAGGraphRef _Nullable graph) IAG_SWIFT_NAME(Graph.startProfiling(_:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphStopProfiling(IAGGraphRef _Nullable graph) IAG_SWIFT_NAME(Graph.stopProfiling(_:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphResetProfile(IAGGraphRef _Nullable graph) IAG_SWIFT_NAME(Graph.resetProfile(_:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGGraphIsProfilingEnabled(IAGAttribute attribute) IAG_SWIFT_NAME(getter:AnyAttribute.isProfilingEnabled(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphMarkProfile(IAGGraphRef _Nullable graph, const char *event_name)
    IAG_SWIFT_NAME(Graph.markProfile(_:name:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
uint64_t IAGGraphBeginProfileEvent(IAGAttribute attribute, const char *event_name)
    IAG_SWIFT_NAME(AnyAttribute.beginProfileEvent(self:name:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGGraphEndProfileEvent(IAGAttribute attribute, const char *event_name, uint64_t start_time, bool changed)
    IAG_SWIFT_NAME(AnyAttribute.endProfileEvent(self:name:startTime:changed:));

IAG_EXTERN_C_END

IAG_ASSUME_NONNULL_END
