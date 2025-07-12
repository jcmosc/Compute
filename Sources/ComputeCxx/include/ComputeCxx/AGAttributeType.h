#pragma once

#include <ComputeCxx/AGClosure.h>
#include <ComputeCxx/AGType.h>

CF_ASSUME_NONNULL_BEGIN
CF_IMPLICIT_BRIDGING_ENABLED

CF_EXTERN_C_BEGIN

struct AGAttributeType;

typedef struct AGAttributeVTable {
    void (*_Nullable allocate)(AGAttributeType *);
    void (*_Nonnull deallocate)(AGAttributeType *);

    void (*_Nonnull destroySelf)(const AGAttributeType *, void *);
    CFStringRef _Nullable (*_Nonnull bodyDescription)(const AGAttributeType *, const void *);
    CFStringRef _Nullable (*_Nonnull valueDescription)(const AGAttributeType *, const void *);
    void (*_Nullable initializeValue)(const AGAttributeType *, void *);
} AGAttributeVTable;

typedef CF_OPTIONS(uint32_t, AGAttributeTypeFlags) {
    AGAttributeTypeFlagsComparisonModeBitwise = 0,
    AGAttributeTypeFlagsComparisonModeIndirect = 1,
    AGAttributeTypeFlagsComparisonModeEquatableUnlessPOD = 2,
    AGAttributeTypeFlagsComparisonModeEquatableAlways = 3,
    AGAttributeTypeFlagsComparisonModeMask = 0x03,

    AGAttributeTypeFlagsHasDestroySelf = 1 << 2,
    AGAttributeTypeFlagsMainThread = 1 << 3,
    AGAttributeTypeFlagsExternal = 1 << 4,
    AGAttributeTypeFlagsAsyncThread = 1 << 5,
};

typedef struct AGAttributeType {
    AGTypeID selfType;
    AGTypeID valueType;
    AGClosureStorage update;
    const AGAttributeVTable *_Nullable callbacks;
    AGAttributeTypeFlags flags;

    uint32_t selfOffset;
    const unsigned char *_Nullable layout;

    AGTypeID attributeBodyType;
    const void *attributeBodyWitnessTable;
} AGAttributeType;

CF_EXTERN_C_END

CF_IMPLICIT_BRIDGING_DISABLED
CF_ASSUME_NONNULL_END
