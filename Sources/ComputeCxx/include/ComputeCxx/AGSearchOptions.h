#pragma once

#include <CoreFoundation/CFBase.h>

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef CF_OPTIONS(uint32_t, AGSearchOptions) {
    AGSearchOptionsSearchInputs = 1 << 0,
    AGSearchOptionsSearchOutputs = 1 << 1,
    AGSearchOptionsTraverseGraphContexts = 1 << 2,
} CF_SWIFT_NAME(SearchOptions);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
