#pragma once

#include "ComputeCxx/IAGBase.h"
#include "ComputeCxx/IAGComparison.h"

IAG_ASSUME_NONNULL_BEGIN

typedef struct IAGComparisonStateStorage {
    const void *destination;
    const void *source;
    IAGFieldRange field_range;
    IAGTypeID field_type;
} IAGComparisonStateStorage;

IAG_ASSUME_NONNULL_END
