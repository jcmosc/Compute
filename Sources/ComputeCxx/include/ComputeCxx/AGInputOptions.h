#pragma once

#include <ComputeCxx/AGBase.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

typedef AG_OPTIONS(uint8_t, AGInputOptions) {
    AGInputOptionsNone = 0,
    AGInputOptionsUnprefetched = 1 << 0,
    AGInputOptionsSyncMainRef = 1 << 1,
    AGInputOptionsAlwaysEnabled = 1 << 2,
    AGInputOptionsChanged = 1 << 3,
    AGInputOptionsEnabled = 1 << 4,
};

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
