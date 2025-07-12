#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include <ComputeCxx/AGSwiftSupport.h>

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef uint32_t AGAttribute AG_SWIFT_STRUCT AG_SWIFT_NAME(AnyAttribute);

CF_EXPORT
const AGAttribute AGAttributeNil;

typedef CF_OPTIONS(uint8_t, AGAttributeFlags) {
    AGAttributeFlagsDefault = 0,
    AGAttributeFlagsActive = 1 << 0,
    AGAttributeFlagsRemovable = 1 << 1,
    AGAttributeFlagsInvalidatable = 1 << 2,
    
    AGAttributeFlagsMask = 0xFF,
};

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
