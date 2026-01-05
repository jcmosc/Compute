#pragma once

#include <ComputeCxx/AGBase.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

typedef AG_OPTIONS(uint8_t, AGChangedValueFlags) {
    AGChangedValueFlagsChanged = 1 << 0,
    AGChangedValueFlagsRequiresMainThread = 1 << 1,
};

typedef struct AGChangedValue {
    const void *value;
    AGChangedValueFlags flags;
} AGChangedValue;

typedef struct AGWeakChangedValue {
    const void *_Nullable value;
    AGChangedValueFlags flags;
} AGWeakChangedValue;

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
