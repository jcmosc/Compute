#pragma once

#include <ComputeCxx/IAGBase.h>

IAG_ASSUME_NONNULL_BEGIN

IAG_EXTERN_C_BEGIN

typedef struct IAG_SWIFT_NAME(_IAGClosureStorage) IAGClosureStorage {
    const void *thunk;
    const void *_Nullable context;
} IAGClosureStorage;

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGClosureStorage IAGRetainClosure(const void *thunk, const void *_Nullable context);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGReleaseClosure(IAGClosureStorage closure);

IAG_EXTERN_C_END

IAG_ASSUME_NONNULL_END
