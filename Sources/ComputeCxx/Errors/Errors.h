#pragma once

#include <CoreFoundation/CFBase.h>

CF_ASSUME_NONNULL_BEGIN

namespace AG {

void precondition_failure(const char *format, ...);
void non_fatal_precondition_failure(const char *format, ...);

} // namespace AG

CF_ASSUME_NONNULL_END
