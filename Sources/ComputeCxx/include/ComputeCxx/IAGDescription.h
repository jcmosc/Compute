#pragma once

#include <ComputeCxx/IAGBase.h>

#if TARGET_OS_MAC
#include <CoreFoundation/CFString.h>
#else
#include <SwiftCorelibsCoreFoundation/CFString.h>
#endif

#include <ComputeCxx/IAGGraph.h>

IAG_ASSUME_NONNULL_BEGIN

IAG_EXTERN_C_BEGIN

#if TARGET_OS_MAC

typedef CFStringRef IAGDescriptionOption IAG_SWIFT_STRUCT IAG_SWIFT_NAME(DescriptionOption);

IAG_EXPORT
const IAGDescriptionOption IAGDescriptionFormat IAG_SWIFT_NAME(IAGDescriptionOption.format);

IAG_EXPORT
const IAGDescriptionOption IAGDescriptionMaxFrames IAG_SWIFT_NAME(IAGDescriptionOption.maxFrames);

IAG_EXPORT
const IAGDescriptionOption IAGDescriptionIncludeValues IAG_SWIFT_NAME(IAGDescriptionOption.includeValues);

IAG_EXPORT
const IAGDescriptionOption IAGDescriptionTruncationLimit IAG_SWIFT_NAME(IAGDescriptionOption.truncationLimit);

#endif

IAG_EXTERN_C_END

IAG_ASSUME_NONNULL_END
