#pragma once

#include <CoreFoundation/CFBase.h>

CF_ASSUME_NONNULL_BEGIN

struct AGClosureStorage {
    void *function;
    void *context;
};

AGClosureStorage AGRetainClosure(AGClosureStorage closure);
void AGReleaseClosure(AGClosureStorage closure);

CF_ASSUME_NONNULL_END
