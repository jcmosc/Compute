#pragma once

#include "Closure/AGClosure.h"
#include "Swift/AGType.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

struct AGAttributeType;

typedef struct AGAttributeVTable {
    void (*callback0)(AGAttributeType *);
    void (*deallocate)(AGAttributeType *);

    void (*destroySelf)(const AGAttributeType *, void *);
    CFStringRef _Nonnull (*_Nonnull selfDescription)(const AGAttributeType *, void *);
    CFStringRef _Nonnull (*_Nonnull valueDescription)(const AGAttributeType *, void *);
    void (*_Nullable initializeValue)(const AGAttributeType *, void *);
} AGAttributeVTable;

typedef CF_OPTIONS(uint32_t, AGAttributeTypeFlags) {
    AGAttributeTypeFlagsComparisonModeMask = 0x03,

    AGAttributeTypeFlagsHasDestroySelf = 1 << 2,
    AGAttributeTypeFlagsMainThread = 1 << 3,
    AGAttributeTypeFlagsExternal = 1 << 4,
    AGAttributeTypeFlagsThreadSafe = 1 << 5,
};

typedef struct AGAttributeType {
    AGTypeID selfType;
    AGTypeID valueType;
    AGClosureStorage update;
    const AGAttributeVTable *_Nullable callbacks;
    AGAttributeTypeFlags flags;

    uint32_t selfOffset;
    const unsigned char *_Nullable layout;

    AGTypeID initialSelfType;
    const void *initialAttributeBodyWitnessTable;
} AGAttributeType;

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
