#pragma once

#include <ComputeCxx/AGBase.h>
#include <CoreFoundation/CFString.h>

#include <ComputeCxx/AGGraph.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

typedef CFStringRef AGDescriptionOption AG_SWIFT_STRUCT AG_SWIFT_NAME(DescriptionOption);

AG_EXPORT
const AGDescriptionOption AGDescriptionFormat AG_SWIFT_NAME(AGDescriptionOption.format);

AG_EXPORT
const AGDescriptionOption AGDescriptionMaxFrames AG_SWIFT_NAME(AGDescriptionOption.maxFrames);

AG_EXPORT
const AGDescriptionOption AGDescriptionIncludeValues AG_SWIFT_NAME(AGDescriptionOption.includeValues);

AG_EXPORT
const AGDescriptionOption AGDescriptionTruncationLimit AG_SWIFT_NAME(AGDescriptionOption.truncationLimit);


AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
