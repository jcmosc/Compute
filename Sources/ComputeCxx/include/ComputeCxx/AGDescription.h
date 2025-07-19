#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFString.h>

#include <ComputeCxx/AGGraph.h>

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

CF_EXPORT
const CFStringRef AGDescriptionFormat CF_SWIFT_NAME(AGGraphRef.descriptionFormat);

CF_EXPORT
const CFStringRef AGDescriptionMaxFrames CF_SWIFT_NAME(AGGraphRef.maxFrames);

CF_EXPORT
const CFStringRef AGDescriptionIncludeValues CF_SWIFT_NAME(AGGraphRef.includeValues);

CF_EXPORT
const CFStringRef AGDescriptionTruncationLimit CF_SWIFT_NAME(AGGraphRef.truncationLimit);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
