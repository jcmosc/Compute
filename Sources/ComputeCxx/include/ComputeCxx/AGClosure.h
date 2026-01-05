#pragma once

#include <ComputeCxx/AGBase.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

typedef struct AG_SWIFT_NAME(_AGClosureStorage) AGClosureStorage {
    const void *thunk;
    const void *_Nullable context;
} AGClosureStorage;

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGClosureStorage AGRetainClosure(void (*closure)(void *_Nullable context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                                 void *_Nullable closure_context);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGReleaseClosure(AGClosureStorage closure);

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
