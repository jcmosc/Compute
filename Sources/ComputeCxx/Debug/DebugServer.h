#pragma once

#include <CoreFoundation/CFBase.h>

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class DebugServer {
  public:
    static void start(uint32_t port);
};

} // namespace AG

CF_ASSUME_NONNULL_END
