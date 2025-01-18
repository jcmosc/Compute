#pragma once

#include <CoreFoundation/CFBase.h>

#include "AGSwiftSupport.h"

CF_ASSUME_NONNULL_BEGIN
CF_IMPLICIT_BRIDGING_ENABLED

CF_EXTERN_C_BEGIN

typedef const struct AGSwiftMetadata *AGTypeID AG_SWIFT_STRUCT AG_SWIFT_NAME(Metadata);

CF_EXPORT
CF_REFINED_FOR_SWIFT
CFStringRef AGTypeDescription(AGTypeID typeID);

CF_EXTERN_C_END

CF_IMPLICIT_BRIDGING_DISABLED
CF_ASSUME_NONNULL_END
