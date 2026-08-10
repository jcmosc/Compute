#pragma once

#include <AttributeGraph/AGBase.h>
#include <AttributeGraph/AGGraph.h>
#include <AttributeGraph/AGUniqueID.h>

typedef AG_OPTIONS(uint32_t, AGGraphProfileFlags){
    AGGraphProfileFlagsEnabled = 1 << 0,
    AGGraphProfileFlagsApplicationEvents = 1 << 1,
} AG_SWIFT_NAME(Graph.ProfileFlags);

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphStartProfiling(AGGraphRef _Nullable graph) AG_SWIFT_NAME(Graph.startProfiling(_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphStopProfiling(AGGraphRef _Nullable graph) AG_SWIFT_NAME(Graph.stopProfiling(_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphResetProfile(AGGraphRef _Nullable graph) AG_SWIFT_NAME(Graph.resetProfile(_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGGraphIsProfilingEnabled(AGAttribute attribute) AG_SWIFT_NAME(getter:AnyAttribute.isProfilingEnabled(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphMarkProfile(AGGraphRef _Nullable graph, const char *event_name)
    AG_SWIFT_NAME(Graph.markProfile(_:name:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint64_t AGGraphBeginProfileEvent(AGAttribute attribute, const char *event_name)
    AG_SWIFT_NAME(AnyAttribute.beginProfileEvent(self:name:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGGraphEndProfileEvent(AGAttribute attribute, const char *event_name, uint64_t start_time, bool changed)
    AG_SWIFT_NAME(AnyAttribute.endProfileEvent(self:name:startTime:changed:));

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
