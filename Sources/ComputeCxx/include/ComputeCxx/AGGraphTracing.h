#pragma once

#include <ComputeCxx/AGBase.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

typedef AG_OPTIONS(uint32_t, AGGraphTraceOptions) {
    AGGraphTraceOptionsEnabled = 1 << 0,
    AGGraphTraceOptionsFull = 1 << 1,
    AGGraphTraceOptionsBacktrace = 1 << 2,
    AGGraphTraceOptionsPrepare = 1 << 3,
    AGGraphTraceOptionsCustom = 1 << 4,
    AGGraphTraceOptionsAll = 1 << 5,
} AG_SWIFT_NAME(AGGraphRef.TraceOptions);

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
