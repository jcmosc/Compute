#pragma once

#include <ComputeCxx/IAGAttributeType.h>
#include <ComputeCxx/IAGBase.h>

IAG_ASSUME_NONNULL_BEGIN

IAG_EXTERN_C_BEGIN

typedef struct IAGAttributeInfo {
    const IAGAttributeType *type;
    const void *body;
} IAGAttributeInfo;

IAG_EXTERN_C_END

IAG_ASSUME_NONNULL_END
