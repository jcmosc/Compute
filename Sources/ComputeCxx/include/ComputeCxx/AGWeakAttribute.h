#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include <ComputeCxx/AGAttribute.h>
#include <ComputeCxx/AGSwiftSupport.h>

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef struct AGWeakAttribute {
    struct {
        AGAttribute identifier;
        uint32_t seed;
    } _details;
} CF_SWIFT_NAME(AnyWeakAttribute) AGWeakAttribute;

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGWeakAttribute AGCreateWeakAttribute(AGAttribute attribute);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGWeakAttributeGetAttribute(AGWeakAttribute attribute);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
