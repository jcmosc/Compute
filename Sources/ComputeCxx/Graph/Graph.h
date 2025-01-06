#pragma once

#include <CoreFoundation/CFBase.h>

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Graph {
  public:
    static void trace_assertion_failure(bool all_stop_tracing, const char *format, ...);
};

} // namespace AG

CF_ASSUME_NONNULL_END
