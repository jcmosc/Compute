#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "AGAttribute.h"
#include "AGSwiftSupport.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef struct AGWeakAttribute {
    AGAttribute attribute;
    uint32_t subgraph_id;
} CF_SWIFT_NAME(AnyWeakAttribute) AGWeakAttribute;

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGWeakAttribute AGCreateWeakAttribute(AGAttribute attribute);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGWeakAttributeGetAttribute(AGWeakAttribute attribute);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
