#pragma once

#include <ComputeCxx/IAGBase.h>
#include <ComputeCxx/IAGType.h>

IAG_ASSUME_NONNULL_BEGIN
IAG_IMPLICIT_BRIDGING_ENABLED

IAG_EXTERN_C_BEGIN

typedef struct IAGFieldRange {
    size_t offset;
    size_t size;
} IAGFieldRange IAG_SWIFT_STRUCT IAG_SWIFT_NAME(FieldRange);

typedef struct IAGComparisonStateStorage *IAGComparisonState IAG_SWIFT_STRUCT IAG_SWIFT_NAME(ComparisonState);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
const void *IAGComparisonStateGetDestination(IAGComparisonState state);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
const void *IAGComparisonStateGetSource(IAGComparisonState state);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGFieldRange IAGComparisonStateGetFieldRange(IAGComparisonState state);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTypeID IAGComparisonStateGetFieldType(IAGComparisonState state);

typedef IAG_ENUM(uint8_t, IAGComparisonMode) {
    IAGComparisonModeBitwise = 0,
    IAGComparisonModeIndirect = 1,
    IAGComparisonModeEquatableUnlessPOD = 2,
    IAGComparisonModeEquatableAlways = 3,
} IAG_SWIFT_NAME(ComparisonMode);

typedef IAG_OPTIONS(uint32_t, IAGComparisonOptions) {
    IAGComparisonOptionsComparisonModeBitwise = 0,
    IAGComparisonOptionsComparisonModeIndirect = 1,
    IAGComparisonOptionsComparisonModeEquatableUnlessPOD = 2,
    IAGComparisonOptionsComparisonModeEquatableAlways = 3,
    IAGComparisonOptionsComparisonModeMask = 0xff,

    IAGComparisonOptionsCopyOnWrite = 1 << 8,
    IAGComparisonOptionsFetchLayoutsSynchronously = 1 << 9,
    IAGComparisonOptionsTraceCompareFailed = 1ul << 31, // -1 signed int
} IAG_SWIFT_NAME(ComparisonOptions);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGCompareValues(const void *_Nonnull destination, const void *_Nonnull source, IAGTypeID type_id,
                     IAGComparisonOptions options);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
const unsigned char *_Nullable IAGPrefetchCompareValues(IAGTypeID type_id, IAGComparisonOptions options,
                                                       uint32_t priority) IAG_SWIFT_NAME(prefetchCompareValues(type:options:priority:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGOverrideComparisonForTypeDescriptor(void *descriptor, IAGComparisonMode mode);

IAG_EXTERN_C_END

IAG_IMPLICIT_BRIDGING_DISABLED
IAG_ASSUME_NONNULL_END
