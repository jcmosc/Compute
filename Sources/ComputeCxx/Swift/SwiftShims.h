#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFString.h>
#include <SwiftEquatableSupport.h>

#include "AGSwiftSupport.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

AG_SWIFT_CC(swift)
bool AGDispatchEquatable(const void *lhs_value, const void *rhs_value, const ::swift::Metadata *type,
                         const ::swift::equatable_support::EquatableWitnessTable *wt);

AG_SWIFT_CC(swift)
bool AGSetTypeForKey(CFMutableDictionaryRef dict, CFStringRef key, const ::swift::Metadata *type);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
