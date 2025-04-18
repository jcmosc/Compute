#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "AGSwiftSupport.h"
#include "Swift/AGType.h"

CF_ASSUME_NONNULL_BEGIN
CF_IMPLICIT_BRIDGING_ENABLED

CF_EXTERN_C_BEGIN

struct AGFieldRange {
    size_t offset;
    size_t size;
};

typedef struct AGComparisonStateStorage *_Nonnull AGComparisonState;

const void *AGComparisonStateGetDestination(AGComparisonState state);
const void *AGComparisonStateGetSource(AGComparisonState state);
AGFieldRange AGComparisonStateGetFieldRange(AGComparisonState state);
AGTypeID AGComparisonStateGetFieldType(AGComparisonState state);

typedef CF_OPTIONS(uint32_t, AGComparisonOptions) {
    AGComparisonOptionsNone = 0,
    AGComparisonOptionsCopyOnWrite = 1 << 8,
    AGComparisonOptionsFetchLayoutsSynchronously = 1 << 9,
    AGComparisonOptionsReportFailures = 1ul << 31,
};

typedef CF_OPTIONS(uint16_t, AGComparisonMode) {
    AGComparisonModeDefault = 0,
};

bool AGCompareValues(const void *destination, const void *source, AGTypeID type_id, AGComparisonOptions options);

const unsigned char *AGPrefetchCompareValues(AGTypeID type_id, AGComparisonOptions options, uint32_t priority);

void AGOverrideComparisonForTypeDescriptor(void *descriptor, AGComparisonMode mode);

CF_EXTERN_C_END

CF_IMPLICIT_BRIDGING_DISABLED
CF_ASSUME_NONNULL_END
