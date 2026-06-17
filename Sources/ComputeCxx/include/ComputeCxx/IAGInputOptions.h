#pragma once

#include <ComputeCxx/IAGBase.h>

IAG_ASSUME_NONNULL_BEGIN

IAG_EXTERN_C_BEGIN

typedef IAG_OPTIONS(uint8_t, IAGInputOptions) {
    IAGInputOptionsNone = 0,
    IAGInputOptionsUnprefetched = 1 << 0,
    IAGInputOptionsSyncMainRef = 1 << 1,
    IAGInputOptionsAlwaysEnabled = 1 << 2,
    IAGInputOptionsChanged = 1 << 3,
    IAGInputOptionsEnabled = 1 << 4,
};

IAG_EXTERN_C_END

IAG_ASSUME_NONNULL_END
