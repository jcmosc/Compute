#pragma once

#include <ComputeCxx/IAGBase.h>

IAG_ASSUME_NONNULL_BEGIN

IAG_EXTERN_C_BEGIN

typedef IAG_OPTIONS(uint32_t, IAGSearchOptions) {
    IAGSearchOptionsSearchInputs = 1 << 0,
    IAGSearchOptionsSearchOutputs = 1 << 1,
    IAGSearchOptionsTraverseGraphContexts = 1 << 2,
} IAG_SWIFT_NAME(SearchOptions);

IAG_EXTERN_C_END

IAG_ASSUME_NONNULL_END
