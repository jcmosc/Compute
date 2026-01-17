#pragma once

#include <ComputeCxx/AGBase.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

typedef AG_OPTIONS(uint32_t, AGCachedValueOptions) {
    AGCachedValueOptionsNone = 0,
    AGCachedValueOptionsUnprefetched = 1,
} AG_SWIFT_NAME(CachedValueOptions);

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
