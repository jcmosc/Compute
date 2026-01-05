#pragma once

#include "ComputeCxx/AGBase.h"
#include "ComputeCxx/AGComparison.h"

AG_ASSUME_NONNULL_BEGIN

typedef struct AGComparisonStateStorage {
    const void *destination;
    const void *source;
    AGFieldRange field_range;
    AGTypeID field_type;
} AGComparisonStateStorage;

AG_ASSUME_NONNULL_END
