#pragma once

#include <ComputeCxx/AGBase.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

typedef AG_OPTIONS(uint32_t, AGSearchOptions) {
    AGSearchOptionsSearchInputs = 1 << 0,
    AGSearchOptionsSearchOutputs = 1 << 1,
    AGSearchOptionsTraverseGraphContexts = 1 << 2,
} AG_SWIFT_NAME(SearchOptions);

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
