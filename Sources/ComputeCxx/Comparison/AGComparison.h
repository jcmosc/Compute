#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "AGSwiftSupport.h"
#include "Swift/AGType.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

struct AGFieldRange {
    size_t offset;
    size_t size;
};

typedef struct AGComparisonStateStorage *AGComparisonState;

CF_EXPORT
CF_REFINED_FOR_SWIFT
const void *AGComparisonStateGetDestination(AGComparisonState state);

CF_EXPORT
CF_REFINED_FOR_SWIFT
const void *AGComparisonStateGetSource(AGComparisonState state);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGFieldRange AGComparisonStateGetFieldRange(AGComparisonState state);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTypeID AGComparisonStateGetFieldType(AGComparisonState state);

typedef CF_ENUM(uint8_t, AGComparisonMode) {
    AGComparisonModeBitwise = 0,
    AGComparisonModeIndirect = 1,
    AGComparisonModeEquatableUnlessPOD = 2,
    AGComparisonModeEquatableAlways = 3,
};

typedef CF_OPTIONS(uint32_t, AGComparisonOptions) {
    AGComparisonOptionsComparisonModeMask = 0xff,

    AGComparisonOptionsCopyOnWrite = 1 << 8,
    AGComparisonOptionsFetchLayoutsSynchronously = 1 << 9,
    AGComparisonOptionsReportFailures = 1ul << 31, // -1 signed int
};

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGCompareValues(const void *destination, const void *source, AGTypeID type_id, AGComparisonOptions options);

CF_EXPORT
CF_REFINED_FOR_SWIFT
const unsigned char *_Nullable AGPrefetchCompareValues(AGTypeID type_id, AGComparisonOptions options,
                                                       uint32_t priority);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGOverrideComparisonForTypeDescriptor(void *descriptor, AGComparisonMode mode);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
