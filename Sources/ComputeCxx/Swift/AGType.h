#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "AGSwiftSupport.h"

CF_ASSUME_NONNULL_BEGIN
CF_IMPLICIT_BRIDGING_ENABLED

CF_EXTERN_C_BEGIN

typedef const struct AGSwiftMetadata *AGTypeID AG_SWIFT_STRUCT AG_SWIFT_NAME(Metadata);

typedef struct AG_SWIFT_NAME(Signature) AGTypeSignature {
    uint32_t data[5];
} AGTypeSignature;

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
AGTypeKind AGTypeGetKind(AGTypeID typeID) CF_SWIFT_NAME(getter:Metadata.kind(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
const AGTypeSignature AGTypeGetSignature(AGTypeID typeID) CF_SWIFT_NAME(getter:Metadata.signature(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
const void *_Nullable AGTypeGetDescriptor(AGTypeID typeID) CF_SWIFT_NAME(getter:Metadata.descriptor(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
const void *_Nullable AGTypeNominalDescriptor(AGTypeID typeID) CF_SWIFT_NAME(getter:Metadata.nominalDescriptor(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
const char *_Nullable AGTypeNominalDescriptorName(AGTypeID typeID)
    CF_SWIFT_NAME(getter:Metadata.nominalDescriptorName(self:));

typedef CF_OPTIONS(uint32_t, AGTypeApplyOptions) {
    AGTypeApplyOptionsNone = 0,
    AGTypeApplyOptionsHeapClasses = 1 << 0,
    AGTypeApplyOptions_2 = 1 << 1,
    AGTypeApplyOptionsEnumCases = 1 << 2,
} CF_SWIFT_NAME(Metadata.ApplyOptions);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGTypeApplyFields(AGTypeID typeID,
                       void (*body)(const char *field_name, size_t field_size, AGTypeID field_type,
                                    void *_Nullable context),
                       void *_Nullable context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGTypeApplyFields2(AGTypeID typeID, AGTypeApplyOptions options,
                        bool (*body)(const char *field_name, size_t field_size, AGTypeID field_type,
                                     void *_Nullable context),
                        void *_Nullable context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGTypeApplyEnumData(AGTypeID typeID, void *value,
                         void (*body)(uint32_t tag, AGTypeID field_type, void *field_value));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGTypeApplyMutableEnumData(AGTypeID typeID, void *value,
                                void (*body)(uint32_t tag, AGTypeID field_type, void *field_value));

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint64_t AGTypeGetEnumTag(AGTypeID typeID, void *value);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGTypeProjectEnumData(AGTypeID typeID);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGTypeInjectEnumTag(AGTypeID typeID);

CF_EXTERN_C_END

CF_IMPLICIT_BRIDGING_DISABLED
CF_ASSUME_NONNULL_END
