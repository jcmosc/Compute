#pragma once

#include <ComputeCxx/IAGBase.h>

IAG_ASSUME_NONNULL_BEGIN

IAG_EXTERN_C_BEGIN

typedef IAG_OPTIONS(uint32_t, IAGCachedValueOptions) {
    IAGCachedValueOptionsNone = 0,
    IAGCachedValueOptionsUnprefetched = 1,
} IAG_SWIFT_NAME(CachedValueOptions);

IAG_EXTERN_C_END

IAG_ASSUME_NONNULL_END
