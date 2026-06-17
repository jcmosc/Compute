#pragma once

#include <ComputeCxx/IAGBase.h>

IAG_ASSUME_NONNULL_BEGIN

IAG_EXTERN_C_BEGIN

typedef IAG_OPTIONS(uint8_t, IAGChangedValueFlags) {
    IAGChangedValueFlagsChanged = 1 << 0,
    IAGChangedValueFlagsRequiresMainThread = 1 << 1,
};

typedef struct IAGChangedValue {
    const void *value;
    IAGChangedValueFlags flags;
} IAGChangedValue;

typedef struct IAGWeakChangedValue {
    const void *_Nullable value;
    IAGChangedValueFlags flags;
} IAGWeakChangedValue;

IAG_EXTERN_C_END

IAG_ASSUME_NONNULL_END
