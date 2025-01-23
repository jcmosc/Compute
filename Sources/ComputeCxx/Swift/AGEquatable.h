#pragma once

#include <CoreFoundation/CFBase.h>

#include "AGType.h"
#include "Metadata.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

bool AGDispatchEquatable(const void *lhs, const void *rhs, AGTypeID type,
                         const AG::swift::equatable_witness_table *equatable);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
