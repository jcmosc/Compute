#pragma once

#include <ComputeCxx/IAGBase.h>

IAG_ASSUME_NONNULL_BEGIN

IAG_EXTERN_C_BEGIN

typedef IAG_OPTIONS(uint32_t, IAGValueOptions) {
    IAGValueOptionsNone = 0,
    IAGValueOptionsInputOptionsUnprefetched = 1 << 0,
    IAGValueOptionsInputOptionsSyncMainRef = 1 << 1,
    IAGValueOptionsInputOptionsMask = 3,

    IAGValueOptionsIncrementGraphVersion = 1 << 2, // AsTopLevelOutput
};

typedef IAG_OPTIONS(uint8_t, IAGValueState) {
    IAGValueStateNone = 0,
    IAGValueStateDirty = 1 << 0,
    IAGValueStatePending = 1 << 1,
    IAGValueStateUpdating = 1 << 2,
    IAGValueStateValueExists = 1 << 3,
    IAGValueStateMainThread = 1 << 4,
    IAGValueStateMainRef = 1 << 5,
    IAGValueStateRequiresMainThread = 1 << 6,
    IAGValueStateSelfModified = 1 << 7,
};

IAG_EXTERN_C_END

IAG_ASSUME_NONNULL_END
