#pragma once

#include "ComputeCxx/IAGBase.h"

IAG_ASSUME_NONNULL_BEGIN

namespace IAG {

[[noreturn]] void precondition_failure(const char *format, ...);
void non_fatal_precondition_failure(const char *format, ...);

} // namespace IAG

IAG_ASSUME_NONNULL_END
