#pragma once

#include <CoreFoundation/CFBase.h>

#include <ComputeCxx/AGSwiftSupport.h>

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef CF_OPTIONS(uint32_t, AGValueOptions) {
    AGValueOptionsNone = 0,
    AGValueOptionsInputOptionsUnprefetched = 1 << 0,
    AGValueOptionsInputOptionsSyncMainRef = 1 << 1,
    AGValueOptionsInputOptionsMask = 3,

    AGValueOptionsIncrementGraphVersion = 1 << 2, // AsTopLevelOutput
};

typedef CF_OPTIONS(uint8_t, AGValueState) {
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

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
