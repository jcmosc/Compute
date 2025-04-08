#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "AGSwiftSupport.h"
#include "Closure/AGClosure.h"
#include "Swift/AGType.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef uint32_t AGAttribute AG_SWIFT_STRUCT AG_SWIFT_NAME(AnyAttribute);

CF_EXPORT
const AGAttribute AGAttributeNil;

typedef CF_OPTIONS(uint8_t, AGAttributeFlags) {
    AGAttributeFlagsNone = 0,
};

typedef CF_OPTIONS(uint32_t, AGAttributeTypeFlags) {
    AGAttributeTypeFlagsNone = 0,
    AGAttributeTypeFlagsOption4 = 4,
    AGAttributeTypeFlagsOption8 = 8,
    AGAttributeTypeFlagsOption16 = 16,
};

typedef struct AGAttributeVTable {
    void *callback0;
    void *callback1;
    void *callback2;
    void *callback3;
    void *callback4;
    void *callback5;
} AGAttributeVTable;

typedef struct AGAttributeType {
    AGTypeID body_type_id;
    AGTypeID value_type_id;
    void (*update_function)(const void *, void *, AGAttribute);
    const void *_Nullable update_function_context;
    AGAttributeVTable *_Nullable callbacks;
    AGAttributeTypeFlags flags;

    // set after construction
    uint32_t attribute_offset;
    const unsigned char *_Nullable layout;

    AGTypeID initial_body_type_id;
    AGTypeID initial_body_witness_table;
} AGAttributeType;

typedef struct AGAttributeInfo {
    const AGAttributeType *type;
    const void *body;
} AGAttributeInfo;

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
