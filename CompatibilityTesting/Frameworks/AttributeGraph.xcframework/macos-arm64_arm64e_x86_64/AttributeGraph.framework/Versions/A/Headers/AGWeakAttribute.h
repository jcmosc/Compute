#pragma once

#include <AttributeGraph/AGAttribute.h>
#include <AttributeGraph/AGBase.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

typedef struct {
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
