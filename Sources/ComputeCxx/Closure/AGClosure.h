#pragma once

#include <CoreFoundation/CFBase.h>

#include "AGSwiftSupport.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

// TODO: add Swift annotation for retain relase...

typedef struct CF_BRIDGED_TYPE(id) AGClosureStorage *AGClosureRef AG_SWIFT_NAME(Closure);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGClosureRef AGRetainClosure(AGClosureRef closure);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGReleaseClosure(AGClosureRef closure);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
