#pragma once

#include <ComputeCxx/IAGBase.h>

IAG_ASSUME_NONNULL_BEGIN

IAG_EXTERN_C_BEGIN

typedef uint32_t IAGAttribute IAG_SWIFT_STRUCT IAG_SWIFT_NAME(AnyAttribute);

IAG_EXPORT
const IAGAttribute IAGAttributeNil;

typedef IAG_OPTIONS(uint8_t, IAGAttributeFlags) {
    IAGAttributeFlagsNone = 0,
    IAGAttributeFlagsAll = 0xFF,
} IAG_SWIFT_NAME(IAGSubgraphRef.Flags);

IAG_EXTERN_C_END

IAG_ASSUME_NONNULL_END
