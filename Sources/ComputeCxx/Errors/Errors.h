#pragma once

#include "ComputeCxx/AGBase.h"

AG_ASSUME_NONNULL_BEGIN

namespace AG {

[[noreturn]] void precondition_failure(const char *format, ...);
void non_fatal_precondition_failure(const char *format, ...);

} // namespace AG

AG_ASSUME_NONNULL_END
