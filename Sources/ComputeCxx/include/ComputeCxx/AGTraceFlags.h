#pragma once

#include <CoreFoundation/CFBase.h>

#include <ComputeCxx/AGGraph.h>

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef CF_OPTIONS(uint32_t, AGTraceFlags) {
    AGTraceFlagsEnabled = 1 << 0,
    AGTraceFlagsFull = 1 << 1,
    AGTraceFlagsBacktrace = 1 << 2,
    AGTraceFlagsPrepare = 1 << 3,
    AGTraceFlagsCustom = 1 << 4,
    AGTraceFlagsAll = 1 << 5,
};

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
