#pragma once

#include <ComputeCxx/IAGBase.h>
#include <ComputeCxx/IAGAttribute.h>

IAG_ASSUME_NONNULL_BEGIN

IAG_EXTERN_C_BEGIN

typedef struct IAGWeakAttribute {
    struct {
        IAGAttribute identifier;
        uint32_t seed;
    } _details;
} IAG_SWIFT_NAME(AnyWeakAttribute) IAGWeakAttribute;

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGWeakAttribute IAGCreateWeakAttribute(IAGAttribute attribute);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGAttribute IAGWeakAttributeGetAttribute(IAGWeakAttribute attribute);

IAG_EXTERN_C_END

IAG_ASSUME_NONNULL_END
