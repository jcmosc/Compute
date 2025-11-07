#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef long AGUniqueID;

AGUniqueID AGMakeUniqueID(void) CF_SWIFT_NAME(makeUniqueID());

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
