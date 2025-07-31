#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFString.h>

#include <ComputeCxx/AGGraph.h>
#include <ComputeCxx/AGSwiftSupport.h>

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef CFStringRef AGDescriptionOption AG_SWIFT_STRUCT CF_SWIFT_NAME(AGGraphRef.DescriptionOption);

CF_EXPORT
const AGDescriptionOption AGDescriptionFormat CF_SWIFT_NAME(AGDescriptionOption.descriptionFormat);

CF_EXPORT
const AGDescriptionOption AGDescriptionMaxFrames CF_SWIFT_NAME(AGDescriptionOption.maxFrames);

CF_EXPORT
const AGDescriptionOption AGDescriptionIncludeValues CF_SWIFT_NAME(AGDescriptionOption.includeValues);

CF_EXPORT
const AGDescriptionOption AGDescriptionTruncationLimit CF_SWIFT_NAME(AGDescriptionOption.truncationLimit);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
