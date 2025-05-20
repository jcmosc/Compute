#pragma once

#include <CoreFoundation/CFBase.h>

CF_ASSUME_NONNULL_BEGIN

struct AGClosureStorage {
    const void *function;
    const void *context;
};

CF_ASSUME_NONNULL_END
