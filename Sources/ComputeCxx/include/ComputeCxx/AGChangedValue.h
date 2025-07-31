#pragma once

#include <CoreFoundation/CFBase.h>

#include <ComputeCxx/AGSwiftSupport.h>

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef CF_OPTIONS(uint8_t, AGChangedValueFlags) {
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

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
