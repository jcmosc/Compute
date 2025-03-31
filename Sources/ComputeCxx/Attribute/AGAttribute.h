#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "AGSwiftSupport.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef uint32_t AGAttribute AG_SWIFT_STRUCT AG_SWIFT_NAME(AnyAttribute);

CF_EXPORT
const AGAttribute AGAttributeNil;



CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
