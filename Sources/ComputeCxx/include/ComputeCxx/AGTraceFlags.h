#pragma once

#include <ComputeCxx/AGBase.h>
#include <ComputeCxx/AGGraph.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

typedef AG_OPTIONS(uint32_t, AGTraceFlags) {
    AGTraceFlagsEnabled = 1 << 0,
    AGTraceFlagsFull = 1 << 1,
    AGTraceFlagsBacktrace = 1 << 2,
    AGTraceFlagsPrepare = 1 << 3,
    AGTraceFlagsCustom = 1 << 4,
    AGTraceFlagsAll = 1 << 5,
} AG_SWIFT_NAME(AGGraphRef.TraceFlags); // TODO: what is real name

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
