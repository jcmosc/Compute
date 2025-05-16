#pragma once

#include <CoreFoundation/CFBase.h>

#include "AGComparison.h"

CF_ASSUME_NONNULL_BEGIN

typedef struct AGComparisonStateStorage {
    const void *destination;
    const void *source;
    AGFieldRange field_range;
    AGTypeID field_type;
} AGComparisonStateStorage;

CF_ASSUME_NONNULL_END
