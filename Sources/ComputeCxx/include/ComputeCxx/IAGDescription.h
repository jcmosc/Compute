#pragma once

#include <ComputeCxx/IAGBase.h>

#if TARGET_OS_MAC
#ifdef __OBJC__
#import <Foundation/Foundation.h>
#else
#include <CoreFoundation/CFString.h>
#endif
#else
#include <SwiftCorelibsCoreFoundation/CFString.h>
#endif

#include <ComputeCxx/IAGGraph.h>

IAG_ASSUME_NONNULL_BEGIN

IAG_EXTERN_C_BEGIN

#if TARGET_OS_MAC

#ifdef __OBJC__
typedef NSString *IAGDescriptionOption IAG_SWIFT_STRUCT IAG_SWIFT_NAME(DescriptionOption);
#else
typedef CFStringRef IAGDescriptionOption IAG_SWIFT_STRUCT IAG_SWIFT_NAME(DescriptionOption);
#endif

IAG_EXPORT
const IAGDescriptionOption IAGDescriptionFormat IAG_SWIFT_NAME(DescriptionOption.format);

IAG_EXPORT
const IAGDescriptionOption IAGDescriptionMaxFrames IAG_SWIFT_NAME(DescriptionOption.maxFrames);

IAG_EXPORT
const IAGDescriptionOption IAGDescriptionIncludeValues IAG_SWIFT_NAME(DescriptionOption.includeValues);

IAG_EXPORT
const IAGDescriptionOption IAGDescriptionTruncationLimit IAG_SWIFT_NAME(DescriptionOption.truncationLimit);

#endif

IAG_EXTERN_C_END

IAG_ASSUME_NONNULL_END
