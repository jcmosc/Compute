#pragma once

#include <AttributeGraph/AGBase.h>

#if TARGET_OS_MAC
#ifdef __OBJC__
#import <Foundation/Foundation.h>
#else
#include <CoreFoundation/CFString.h>
#endif
#else
#include <SwiftCorelibsCoreFoundation/CFString.h>
#endif

#include <AttributeGraph/AGGraph.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

#if TARGET_OS_MAC

#ifdef __OBJC__
typedef NSString *AGDescriptionOption AG_SWIFT_STRUCT AG_SWIFT_NAME(DescriptionOption);
#else
typedef CFStringRef AGDescriptionOption AG_SWIFT_STRUCT AG_SWIFT_NAME(DescriptionOption);
#endif

AG_EXPORT
const AGDescriptionOption AGDescriptionFormat AG_SWIFT_NAME(DescriptionOption.format);

AG_EXPORT
const AGDescriptionOption AGDescriptionMaxFrames AG_SWIFT_NAME(DescriptionOption.maxFrames);

AG_EXPORT
const AGDescriptionOption AGDescriptionIncludeValues AG_SWIFT_NAME(DescriptionOption.includeValues);

AG_EXPORT
const AGDescriptionOption AGDescriptionTruncationLimit AG_SWIFT_NAME(DescriptionOption.truncationLimit);

#endif

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
