#pragma once

#include <CoreFoundation/CFBase.h>
#include <swift/bridging>

#include "AGSwiftSupport.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

struct AGOpaqueValue;

struct AGClosureStorage {
    const AGOpaqueValue *_Nullable function;
    const AGOpaqueValue *_Nullable context;

    AGClosureStorage() : function(nullptr), context(nullptr){};
    AGClosureStorage(const AGOpaqueValue *fun, const AGOpaqueValue *ctx) : function(fun), context(ctx){};
} SWIFT_SHARED_REFERENCE(AGRetainClosure, AGReleaseClosure);

void AGRetainClosure(AGClosureStorage *closure);
void AGReleaseClosure(AGClosureStorage *closure);


typedef struct AGClosureStorage *AGClosureRef AG_SWIFT_NAME(Closure);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
