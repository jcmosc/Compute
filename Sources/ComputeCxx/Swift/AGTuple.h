#pragma once

#include <CoreFoundation/CFBase.h>

#include "AGSwiftSupport.h"
#include "AGType.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef CF_ENUM(uint32_t, AGTupleCopyOptions) {
    AGTupleCopyOptionsInitialize = 1 << 0,
    AGTupleCopyOptionsWithTake = 1 << 1,

    AGTupleCopyOptionsAssignCopy = 0,
    AGTupleCopyOptionsInitCopy = 1,
    AGTupleCopyOptionsAssignTake = 2,
    AGTupleCopyOptionsInitTake = 3,
} CF_SWIFT_NAME(TupleType.CopyOptions);

typedef const AGSwiftMetadata *AGTupleType AG_SWIFT_STRUCT CF_SWIFT_NAME(TupleType);

typedef struct AGUnsafeTuple {
    AGTupleType type;
    const void *value;
} CF_SWIFT_NAME(UnsafeTuple) AGUnsafeTuple;

typedef struct AGUnsafeMutableTuple {
    AGTupleType type;
    void *value;
} CF_SWIFT_NAME(UnsafeMutableTuple) AGUnsafeMutableTuple;

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTupleType AGNewTupleType(uint32_t count, const AGTypeID _Nonnull *_Nonnull elements)
    CF_SWIFT_NAME(TupleType.init(count:elements:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
size_t AGTupleCount(AGTupleType tuple_type) CF_SWIFT_NAME(getter:AGTupleType.count(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
size_t AGTupleSize(AGTupleType tuple_type) CF_SWIFT_NAME(getter:AGTupleType.size(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTypeID AGTupleElementType(AGTupleType tuple_type, uint32_t index) CF_SWIFT_NAME(TupleType.elementType(self:at:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
size_t AGTupleElementSize(AGTupleType tuple_type, uint32_t index) CF_SWIFT_NAME(TupleType.elementSize(self:at:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
size_t AGTupleElementOffset(AGTupleType tuple_type, uint32_t index) CF_SWIFT_NAME(TupleType.elementOffset(self:at:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
size_t AGTupleElementOffsetChecked(AGTupleType tuple_type, uint32_t index, AGTypeID element_type)
    CF_SWIFT_NAME(TupleType.elementOffset(self:at:type:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void *AGTupleGetElement(AGTupleType tuple_type, void *tuple_value, uint32_t index, const void *element_value,
                        AGTypeID element_type, AGTupleCopyOptions options);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void *AGTupleSetElement(AGTupleType tuple_type, void *tuple_value, uint32_t index, const void *element_value,
                        AGTypeID element_type, AGTupleCopyOptions options);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGTupleDestroy(AGTupleType tuple_type, void *tuple_value) CF_SWIFT_NAME(TupleType.destroy(self:_:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGTupleDestroyElement(AGTupleType tuple_type, void *tuple_value, uint32_t index)
    CF_SWIFT_NAME(TupleType.destroy(self:_:at:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGTupleWithBuffer(AGTupleType tuple_type, size_t count,
                       const void (*function)(const AGUnsafeMutableTuple mutable_tuple, const void *context),
                       const void *context);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
