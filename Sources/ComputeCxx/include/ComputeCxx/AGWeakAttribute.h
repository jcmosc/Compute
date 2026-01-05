#pragma once

#include <ComputeCxx/AGBase.h>
#include <ComputeCxx/AGAttribute.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

typedef struct AGWeakAttribute {
    struct {
        AGAttribute identifier;
        uint32_t seed;
    } _details;
} AG_SWIFT_NAME(AnyWeakAttribute) AGWeakAttribute;

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGWeakAttribute AGCreateWeakAttribute(AGAttribute attribute);

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGWeakAttributeGetAttribute(AGWeakAttribute attribute);

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
