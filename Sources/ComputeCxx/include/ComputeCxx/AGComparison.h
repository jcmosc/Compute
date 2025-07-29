#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include <ComputeCxx/AGSwiftSupport.h>
#include <ComputeCxx/AGType.h>

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef struct AGFieldRange {
    size_t offset;
    size_t size;
} AGFieldRange AG_SWIFT_STRUCT AG_SWIFT_NAME(FieldRange);

typedef struct AGComparisonStateStorage *AGComparisonState AG_SWIFT_STRUCT AG_SWIFT_NAME(ComparisonState);

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
} CF_SWIFT_NAME(ComparisonMode);

typedef CF_OPTIONS(uint32_t, AGComparisonOptions) {
    AGComparisonOptionsComparisonModeBitwise = 0,
    AGComparisonOptionsComparisonModeIndirect = 1,
    AGComparisonOptionsComparisonModeEquatableUnlessPOD = 2,
    AGComparisonOptionsComparisonModeEquatableAlways = 3,
    AGComparisonOptionsComparisonModeMask = 0xff,

    AGComparisonOptionsCopyOnWrite = 1 << 8,
    AGComparisonOptionsFetchLayoutsSynchronously = 1 << 9,
    AGComparisonOptionsTraceCompareFailed = 1ul << 31, // -1 signed int
} CF_SWIFT_NAME(ComparisonOptions);

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGCompareValues(const void *_Nonnull destination, const void *_Nonnull source, AGTypeID type_id,
                     AGComparisonOptions options);

CF_EXPORT
CF_REFINED_FOR_SWIFT
const unsigned char *_Nullable AGPrefetchCompareValues(AGTypeID type_id, AGComparisonOptions options,
                                                       uint32_t priority);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGOverrideComparisonForTypeDescriptor(void *descriptor, AGComparisonMode mode);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
