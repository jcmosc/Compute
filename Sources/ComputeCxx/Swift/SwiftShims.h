#pragma once

#include "ComputeCxx/AGBase.h"
#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFString.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

AG_SWIFT_CC(swift)
bool AGDispatchEquatable(const void *lhs_value, const void *rhs_value, const ::swift::Metadata *type,
                         const AG::swift::equatable_witness_table *wt);

#ifdef __OBJC__
AG_SWIFT_CC(swift)
bool AGSetTypeForKey(NSMutableDictionary *dict, NSString *key, const ::swift::Metadata *type);
#endif

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
