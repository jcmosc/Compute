#pragma once

#include <ComputeCxx/AGBase.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

typedef AG_OPTIONS(uint32_t, AGValueOptions) {
    AGValueOptionsNone = 0,
    AGValueOptionsInputOptionsUnprefetched = 1 << 0,
    AGValueOptionsInputOptionsSyncMainRef = 1 << 1,
    AGValueOptionsInputOptionsMask = 3,

    AGValueOptionsIncrementGraphVersion = 1 << 2, // AsTopLevelOutput
};

typedef AG_OPTIONS(uint8_t, AGValueState) {
    AGValueStateNone = 0,
    AGValueStateDirty = 1 << 0,
    AGValueStatePending = 1 << 1,
    AGValueStateUpdating = 1 << 2,
    AGValueStateValueExists = 1 << 3,
    AGValueStateMainThread = 1 << 4,
    AGValueStateMainRef = 1 << 5,
    AGValueStateRequiresMainThread = 1 << 6,
    AGValueStateSelfModified = 1 << 7,
};

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
