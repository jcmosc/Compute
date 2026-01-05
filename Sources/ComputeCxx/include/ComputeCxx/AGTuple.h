#pragma once

#include <ComputeCxx/AGBase.h>
#include <ComputeCxx/AGType.h>

AG_ASSUME_NONNULL_BEGIN
AG_IMPLICIT_BRIDGING_ENABLED

AG_EXTERN_C_BEGIN

typedef AG_ENUM(uint32_t, AGTupleCopyOptions) {
    AGTupleCopyOptionsAssignCopy = 0,
    AGTupleCopyOptionsInitCopy = 1,
    AGTupleCopyOptionsAssignTake = 2,
    AGTupleCopyOptionsInitTake = 3,
} AG_SWIFT_NAME(TupleType.CopyOptions);

typedef const struct AGSwiftMetadata *AGTupleType AG_SWIFT_STRUCT AG_SWIFT_NAME(TupleType);

typedef struct AGUnsafeTuple {
    AGTupleType type;
    const void *value;
} AG_SWIFT_NAME(UnsafeTuple) AGUnsafeTuple;

typedef struct AGUnsafeMutableTuple {
    AGTupleType type;
    void *value;
} AG_SWIFT_NAME(UnsafeMutableTuple) AGUnsafeMutableTuple;

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGTupleType AGNewTupleType(size_t count, const AGTypeID _Nonnull *_Nonnull elements)
    AG_SWIFT_NAME(TupleType.init(count:elements:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
size_t AGTupleCount(AGTupleType tuple_type) AG_SWIFT_NAME(getter:AGTupleType.count(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
size_t AGTupleSize(AGTupleType tuple_type) AG_SWIFT_NAME(getter:AGTupleType.size(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGTypeID AGTupleElementType(AGTupleType tuple_type, size_t index) AG_SWIFT_NAME(TupleType.elementType(self:at:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
size_t AGTupleElementSize(AGTupleType tuple_type, size_t index) AG_SWIFT_NAME(TupleType.elementSize(self:at:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
size_t AGTupleElementOffset(AGTupleType tuple_type, size_t index) AG_SWIFT_NAME(TupleType.elementOffset(self:at:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
size_t AGTupleElementOffsetChecked(AGTupleType tuple_type, size_t index, AGTypeID element_type)
    AG_SWIFT_NAME(TupleType.elementOffset(self:at:type:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void *AGTupleGetElement(AGTupleType tuple_type, void *tuple_value, size_t index, void *element_value,
                        AGTypeID element_type, AGTupleCopyOptions options);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void *AGTupleSetElement(AGTupleType tuple_type, void *tuple_value, size_t index, const void *element_value,
                        AGTypeID element_type, AGTupleCopyOptions options);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGTupleDestroy(AGTupleType tuple_type, void *tuple_value) AG_SWIFT_NAME(TupleType.destroy(self:_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGTupleDestroyElement(AGTupleType tuple_type, void *tuple_value, size_t index)
    AG_SWIFT_NAME(TupleType.destroy(self:_:at:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGTupleWithBuffer(AGTupleType tuple_type, size_t count,
                       void (*function)(void *context AG_SWIFT_CONTEXT, const AGUnsafeMutableTuple mutable_tuple)
                           AG_SWIFT_CC(swift),
                       void *context);

AG_EXTERN_C_END

AG_IMPLICIT_BRIDGING_DISABLED
AG_ASSUME_NONNULL_END
