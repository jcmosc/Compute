#pragma once

#include <ComputeCxx/AGBase.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

typedef uint32_t AGAttribute AG_SWIFT_STRUCT AG_SWIFT_NAME(AnyAttribute);

AG_EXPORT
const AGAttribute AGAttributeNil;

typedef AG_OPTIONS(uint8_t, AGAttributeFlags) {
    AGAttributeFlagsNone = 0,
    AGAttributeFlagsAll = 0xFF,
} AG_SWIFT_NAME(AGSubgraphRef.Flags);

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
