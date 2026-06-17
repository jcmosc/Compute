#pragma once

#include "ComputeCxx/IAGBase.h"

#if TARGET_OS_MAC
#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFString.h>
#else
#include <SwiftCorelibsCoreFoundation/CFDictionary.h>
#include <SwiftCorelibsCoreFoundation/CFString.h>
#endif

IAG_ASSUME_NONNULL_BEGIN

IAG_EXTERN_C_BEGIN

IAG_SWIFT_CC(swift)
bool IAGDispatchEquatable(const void *lhs_value, const void *rhs_value, const ::swift::Metadata *type,
                         const IAG::swift::equatable_witness_table *wt);

#ifdef __OBJC__
IAG_SWIFT_CC(swift)
bool IAGSetTypeForKey(NSMutableDictionary *dict, NSString *key, const ::swift::Metadata *type);
#endif

IAG_EXTERN_C_END

IAG_ASSUME_NONNULL_END
