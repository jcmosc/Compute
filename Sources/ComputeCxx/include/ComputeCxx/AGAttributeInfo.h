#pragma once

#include <CoreFoundation/CFBase.h>

#include <ComputeCxx/AGAttributeType.h>
#include <ComputeCxx/AGSwiftSupport.h>

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef struct AGAttributeInfo {
    const AGAttributeType *type;
    const void *body;
} AGAttributeInfo;

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
