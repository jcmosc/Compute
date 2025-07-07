#pragma once

#include <CoreFoundation/CFBase.h>

#include "AGSwiftSupport.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

struct AGClosureStorage {
    void *function;
    void *context;
};

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGClosureStorage AGRetainClosure(void (*closure)(void *_Nullable context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                                 void *_Nullable closure_context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGReleaseClosure(AGClosureStorage closure);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
