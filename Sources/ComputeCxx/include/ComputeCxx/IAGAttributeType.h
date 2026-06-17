#pragma once

#include <ComputeCxx/IAGBase.h>

#if TARGET_OS_MAC
#include <CoreFoundation/CFString.h>
#else
#include <SwiftCorelibsCoreFoundation/CFString.h>
#endif

#include <ComputeCxx/IAGClosure.h>
#include <ComputeCxx/IAGType.h>

IAG_ASSUME_NONNULL_BEGIN
IAG_IMPLICIT_BRIDGING_ENABLED

IAG_EXTERN_C_BEGIN

typedef struct IAGAttributeType IAGAttributeType;

typedef struct IAG_SWIFT_NAME(_AttributeVTable) IAGAttributeVTable {
    unsigned long version;
    void (*_Nullable type_destroy)(IAGAttributeType *);
    void (*_Nullable self_destroy)(const IAGAttributeType *, void *);
#if TARGET_OS_MAC
    CFStringRef _Nullable (*_Nullable self_description)(const IAGAttributeType *, const void *);
    CFStringRef _Nullable (*_Nullable value_description)(const IAGAttributeType *, const void *);
#else
    CFStringRef _Nullable (*_Nullable copy_self_description)(const IAGAttributeType *, const void *);
    CFStringRef _Nullable (*_Nullable copy_value_description)(const IAGAttributeType *, const void *);
#endif
    void (*_Nullable update_default)(const IAGAttributeType *, void *);
} IAGAttributeVTable;

typedef IAG_OPTIONS(uint32_t, IAGAttributeTypeFlags) {
    IAGAttributeTypeFlagsComparisonModeBitwise = 0,
    IAGAttributeTypeFlagsComparisonModeIndirect = 1,
    IAGAttributeTypeFlagsComparisonModeEquatableUnlessPOD = 2,
    IAGAttributeTypeFlagsComparisonModeEquatableAlways = 3,
    IAGAttributeTypeFlagsComparisonModeMask = 0x03,

    IAGAttributeTypeFlagsHasDestroySelf = 1 << 2,
    IAGAttributeTypeFlagsMainThread = 1 << 3,
    IAGAttributeTypeFlagsExternal = 1 << 4,
    IAGAttributeTypeFlagsAsyncThread = 1 << 5,
} IAG_SWIFT_NAME(_AttributeType.Flags);

typedef struct IAG_SWIFT_NAME(_AttributeType) IAGAttributeType {
    IAGTypeID self_id;
    IAGTypeID value_id;
    IAGClosureStorage update;
    const IAGAttributeVTable *vtable;
    IAGAttributeTypeFlags flags;

    uint32_t internal_offset;
    const unsigned char *_Nullable value_layout;

    struct {
        IAGTypeID type_id;
        const void *witness_table;
    } body_conformance;
} IAGAttributeType;

IAG_EXTERN_C_END

IAG_IMPLICIT_BRIDGING_DISABLED
IAG_ASSUME_NONNULL_END
