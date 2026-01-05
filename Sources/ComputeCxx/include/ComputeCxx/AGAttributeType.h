#pragma once

#include <ComputeCxx/AGBase.h>

#include <ComputeCxx/AGClosure.h>
#include <ComputeCxx/AGType.h>

AG_ASSUME_NONNULL_BEGIN
AG_IMPLICIT_BRIDGING_ENABLED

AG_EXTERN_C_BEGIN

typedef struct AGAttributeType AGAttributeType;

typedef struct AG_SWIFT_NAME(_AttributeVTable) AGAttributeVTable {
    unsigned long version;
    void (*_Nullable type_destroy)(AGAttributeType *);
    void (*_Nullable self_destroy)(const AGAttributeType *, void *);
    CFStringRef _Nullable (*_Nullable self_description)(const AGAttributeType *, const void *);
    CFStringRef _Nullable (*_Nullable value_description)(const AGAttributeType *, const void *);
    void (*_Nullable update_default)(const AGAttributeType *, void *);
} AGAttributeVTable;

typedef AG_OPTIONS(uint32_t, AGAttributeTypeFlags) {
    AGAttributeTypeFlagsComparisonModeBitwise = 0,
    AGAttributeTypeFlagsComparisonModeIndirect = 1,
    AGAttributeTypeFlagsComparisonModeEquatableUnlessPOD = 2,
    AGAttributeTypeFlagsComparisonModeEquatableAlways = 3,
    AGAttributeTypeFlagsComparisonModeMask = 0x03,

    AGAttributeTypeFlagsHasDestroySelf = 1 << 2,
    AGAttributeTypeFlagsMainThread = 1 << 3,
    AGAttributeTypeFlagsExternal = 1 << 4,
    AGAttributeTypeFlagsAsyncThread = 1 << 5,
} AG_SWIFT_NAME(_AttributeType.Flags);

typedef struct AG_SWIFT_NAME(_AttributeType) AGAttributeType {
    AGTypeID self_id;
    AGTypeID value_id;
    AGClosureStorage update;
    const AGAttributeVTable *vtable;
    AGAttributeTypeFlags flags;

    uint32_t internal_offset;
    const unsigned char *_Nullable value_layout;

    struct {
        AGTypeID type_id;
        const void *witness_table;
    } body_conformance;
} AGAttributeType;

AG_EXTERN_C_END

AG_IMPLICIT_BRIDGING_DISABLED
AG_ASSUME_NONNULL_END
