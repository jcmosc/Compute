#pragma once

#include <ComputeCxx/IAGBase.h>

#if TARGET_OS_MAC
#include <CoreFoundation/CFString.h>
#else
#include <SwiftCorelibsCoreFoundation/CFString.h>
#endif

IAG_ASSUME_NONNULL_BEGIN
IAG_IMPLICIT_BRIDGING_ENABLED

IAG_EXTERN_C_BEGIN

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextern-c-compat"
typedef struct IAG_SWIFT_NAME(_Metadata) IAGSwiftMetadata {
} IAGSwiftMetadata;
#pragma GCC diagnostic pop

typedef const IAGSwiftMetadata *IAGTypeID IAG_SWIFT_STRUCT IAG_SWIFT_NAME(Metadata);

typedef struct IAGTypeSignature {
    uint8_t bytes[20];
} IAG_SWIFT_NAME(Signature) IAGTypeSignature;

typedef IAG_CLOSED_ENUM(uint32_t, IAGTypeKind) {
    IAGTypeKindNone,
    IAGTypeKindClass,
    IAGTypeKindStruct,
    IAGTypeKindEnum,
    IAGTypeKindOptional,
    IAGTypeKindTuple,
    IAGTypeKindFunction,
    IAGTypeKindExistential,
    IAGTypeKindMetatype,
} IAG_SWIFT_NAME(Metadata.Kind);

#if TARGET_OS_MAC
IAG_EXPORT
IAG_REFINED_FOR_SWIFT
CFStringRef IAGTypeDescription(IAGTypeID typeID);
#else
IAG_EXPORT
IAG_REFINED_FOR_SWIFT
CFStringRef IAGTypeCopyDescription(IAGTypeID typeID);
#endif

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTypeKind IAGTypeGetKind(IAGTypeID typeID) IAG_SWIFT_NAME(getter:Metadata.kind(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
const IAGTypeSignature IAGTypeGetSignature(IAGTypeID typeID) IAG_SWIFT_NAME(getter:Metadata.signature(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
const void *_Nullable IAGTypeGetDescriptor(IAGTypeID typeID) IAG_SWIFT_NAME(getter:Metadata.descriptor(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
const void *_Nullable IAGTypeNominalDescriptor(IAGTypeID typeID) IAG_SWIFT_NAME(getter:Metadata.nominalDescriptor(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
const char *_Nullable IAGTypeNominalDescriptorName(IAGTypeID typeID)
    IAG_SWIFT_NAME(getter:Metadata.nominalDescriptorName(self:));

typedef IAG_OPTIONS(uint32_t, IAGTypeApplyOptions) {
    IAGTypeApplyOptionsEnumerateStructFields = 0,
    IAGTypeApplyOptionsEnumerateClassFields = 1 << 0,
    IAGTypeApplyOptionsContinueAfterUnknownField = 1 << 1,
    IAGTypeApplyOptionsEnumerateEnumCases = 1 << 2,
} IAG_SWIFT_NAME(Metadata.ApplyOptions);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGTypeApplyFields(IAGTypeID typeID,
                       void (*apply)(const char *field_name,
                                     size_t field_offset,
                                     IAGTypeID field_type,
                                     const void *_Nullable context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                       const void *apply_context);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGTypeApplyFields2(IAGTypeID typeID, IAGTypeApplyOptions options,
                        bool (*_Nonnull apply)(const char *field_name,
                                               size_t field_offset,
                                               IAGTypeID field_type,
                                               const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                        const void *apply_context);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGTypeApplyEnumData(IAGTypeID typeID, void *value,
                         void (*body)(uint32_t tag,
                                      IAGTypeID field_type,
                                      const void *field_value,
                                      void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                         void *context);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGTypeApplyMutableEnumData(IAGTypeID typeID, void *value,
                                void (*body)(uint32_t tag,
                                             IAGTypeID field_type,
                                             void *field_value,
                                             void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                                void *context);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
uint64_t IAGTypeGetEnumTag(IAGTypeID typeID, const void *value) IAG_SWIFT_NAME(IAGTypeID.enumTag(self:_:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGTypeProjectEnumData(IAGTypeID typeID, void *value) IAG_SWIFT_NAME(IAGTypeID.projectEnumData(self:_:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGTypeInjectEnumTag(IAGTypeID typeID, uint32_t tag, void *value) IAG_SWIFT_NAME(IAGTypeID.injectEnumTag(self:tag:_:));

IAG_EXTERN_C_END

IAG_IMPLICIT_BRIDGING_DISABLED
IAG_ASSUME_NONNULL_END
