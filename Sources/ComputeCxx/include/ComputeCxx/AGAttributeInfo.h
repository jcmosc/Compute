#pragma once

#include <ComputeCxx/AGAttributeType.h>
#include <ComputeCxx/AGBase.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

typedef struct AGAttributeInfo {
    const AGAttributeType *type;
    const void *body;
} AGAttributeInfo;

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
