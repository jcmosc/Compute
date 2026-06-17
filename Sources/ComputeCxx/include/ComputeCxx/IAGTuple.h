#pragma once

#include <ComputeCxx/IAGBase.h>
#include <ComputeCxx/IAGType.h>

IAG_ASSUME_NONNULL_BEGIN
IAG_IMPLICIT_BRIDGING_ENABLED

IAG_EXTERN_C_BEGIN

typedef IAG_ENUM(uint32_t, IAGTupleCopyOptions) {
    IAGTupleCopyOptionsAssignCopy = 0,
    IAGTupleCopyOptionsInitCopy = 1,
    IAGTupleCopyOptionsAssignTake = 2,
    IAGTupleCopyOptionsInitTake = 3,
} IAG_SWIFT_NAME(TupleType.CopyOptions);

typedef const struct IAGSwiftMetadata *IAGTupleType IAG_SWIFT_STRUCT IAG_SWIFT_NAME(TupleType);

typedef struct IAGUnsafeTuple {
    IAGTupleType type;
    const void *value;
} IAG_SWIFT_NAME(UnsafeTuple) IAGUnsafeTuple;

typedef struct IAGUnsafeMutableTuple {
    IAGTupleType type;
    void *value;
} IAG_SWIFT_NAME(UnsafeMutableTuple) IAGUnsafeMutableTuple;

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTupleType IAGNewTupleType(size_t count, const IAGTypeID _Nonnull *_Nonnull elements)
    IAG_SWIFT_NAME(TupleType.init(count:elements:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
size_t IAGTupleCount(IAGTupleType tuple_type) IAG_SWIFT_NAME(getter:IAGTupleType.count(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
size_t IAGTupleSize(IAGTupleType tuple_type) IAG_SWIFT_NAME(getter:IAGTupleType.size(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTypeID IAGTupleElementType(IAGTupleType tuple_type, size_t index) IAG_SWIFT_NAME(TupleType.elementType(self:at:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
size_t IAGTupleElementSize(IAGTupleType tuple_type, size_t index) IAG_SWIFT_NAME(TupleType.elementSize(self:at:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
size_t IAGTupleElementOffset(IAGTupleType tuple_type, size_t index) IAG_SWIFT_NAME(TupleType.elementOffset(self:at:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
size_t IAGTupleElementOffsetChecked(IAGTupleType tuple_type, size_t index, IAGTypeID element_type)
    IAG_SWIFT_NAME(TupleType.elementOffset(self:at:type:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void *IAGTupleGetElement(IAGTupleType tuple_type, void *tuple_value, size_t index, void *element_value,
                        IAGTypeID element_type, IAGTupleCopyOptions options);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void *IAGTupleSetElement(IAGTupleType tuple_type, void *tuple_value, size_t index, const void *element_value,
                        IAGTypeID element_type, IAGTupleCopyOptions options);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGTupleDestroy(IAGTupleType tuple_type, void *tuple_value) IAG_SWIFT_NAME(TupleType.destroy(self:_:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGTupleDestroyElement(IAGTupleType tuple_type, void *tuple_value, size_t index)
    IAG_SWIFT_NAME(TupleType.destroy(self:_:at:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGTupleWithBuffer(IAGTupleType tuple_type, size_t count,
                       void (*function)(const IAGUnsafeMutableTuple mutable_tuple, void *context IAG_SWIFT_CONTEXT)
                           IAG_SWIFT_CC(swift),
                       void *context);

IAG_EXTERN_C_END

IAG_IMPLICIT_BRIDGING_DISABLED
IAG_ASSUME_NONNULL_END
