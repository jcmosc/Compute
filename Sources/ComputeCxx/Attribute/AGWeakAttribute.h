#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "AGAttribute.h"
#include "AGSwiftSupport.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef uint64_t AGWeakAttribute AG_SWIFT_STRUCT AG_SWIFT_NAME(AnyWeakAttribute);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGWeakAttribute AGCreateWeakAttribute(AGAttribute attribute);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGWeakAttributeGetAttribute(AGWeakAttribute attribute);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
