#pragma once

#include <CoreFoundation/CFBase.h>

CF_ASSUME_NONNULL_BEGIN

namespace util {

class free_deleter {
  public:
    template <typename T> void operator()(T *_Nullable ptr) {
        if (ptr) {
            free((void *)ptr);
        }
    }
};

} // namespace util

CF_ASSUME_NONNULL_END
