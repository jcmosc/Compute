#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdlib.h>

CF_ASSUME_NONNULL_BEGIN

namespace util {

class free_deleter {
  public:
    template <typename T> void operator()(T *_Nullable p) {
        if (p) {
            free((void *)p);
        }
    }
};

} // namespace util

CF_ASSUME_NONNULL_END
