#pragma once

#include <CoreFoundation/CFBase.h>

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef CF_OPTIONS(uint8_t, AGInputOptions) {
    AGInputOptionsNone = 0,
    AGInputOptionsUnprefetched = 1 << 0,
    AGInputOptionsUnknown1 = 1 << 1,
    AGInputOptionsAlwaysEnabled = 1 << 2,
    AGInputOptionsChanged = 1 << 3,
    AGInputOptionsEnabled = 1 << 4,
};

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
