#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "AGSwiftSupport.h"

CF_ASSUME_NONNULL_BEGIN
CF_IMPLICIT_BRIDGING_ENABLED

CF_EXTERN_C_BEGIN

typedef const struct AGSwiftMetadata *AGTypeID AG_SWIFT_STRUCT AG_SWIFT_NAME(Metadata);

typedef struct AGTypeSignature {
    uint32_t data[20];
} AGTypeSignature AG_SWIFT_NAME(Signature);

typedef CF_CLOSED_ENUM(uint32_t, AGTypeKind) {
    AGTypeKindNone,
    AGTypeKindClass,
    AGTypeKindStruct,
    AGTypeKindEnum,
    AGTypeKindOptional,
    AGTypeKindTuple,
    AGTypeKindFunction,
    AGTypeKindExistential,
    AGTypeKindMetatype,
} CF_SWIFT_NAME(Metadata.Kind);

CF_EXPORT
CF_REFINED_FOR_SWIFT
CFStringRef AGTypeDescription(AGTypeID typeID);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTypeKind AGTypeGetKind(AGTypeID typeID) AG_SWIFT_NAME(getter:Metadata.kind(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
const AGTypeSignature AGTypeGetSignature(AGTypeID typeID) AG_SWIFT_NAME(getter:Metadata.signature(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
const void *AGTypeGetDescriptor(AGTypeID typeID);

CF_EXPORT
CF_REFINED_FOR_SWIFT
const void *AGTypeNominalDescriptor(AGTypeID typeID);

CF_EXPORT
CF_REFINED_FOR_SWIFT
const char *AGTypeNominalDescriptorName(AGTypeID typeID);

CF_EXTERN_C_END

CF_IMPLICIT_BRIDGING_DISABLED
CF_ASSUME_NONNULL_END
