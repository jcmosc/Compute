#pragma once

#include <CoreFoundation/CFBase.h>
#include <swift/bridging>

#include "AGSwiftSupport.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

struct AGOpaqueValue;

class AGClosureStorage {
  public:
    const AGOpaqueValue *_Nullable function;
    const AGOpaqueValue *_Nullable context;
} SWIFT_SHARED_REFERENCE(AGRetainClosure, AGReleaseClosure);

typedef struct AGClosureStorage *AGClosureRef AG_SWIFT_NAME(Closure);

void AGRetainClosure(AGClosureRef closure);
void AGReleaseClosure(AGClosureRef closure);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
