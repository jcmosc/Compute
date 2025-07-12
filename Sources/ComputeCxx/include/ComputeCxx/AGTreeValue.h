#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include <ComputeCxx/AGAttribute.h>
#include <ComputeCxx/AGType.h>

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef uint32_t AGTreeValue AG_SWIFT_STRUCT AG_SWIFT_NAME(TreeValue);

CF_EXPORT
AGTypeID AGTreeValueGetType(AGTreeValue tree_value) CF_SWIFT_NAME(getter:AGTreeValue.type(self:));

CF_EXPORT
AGAttribute AGTreeValueGetValue(AGTreeValue tree_value) CF_SWIFT_NAME(getter:AGTreeValue.value(self:));

CF_EXPORT
const char *AGTreeValueGetKey(AGTreeValue tree_value) CF_SWIFT_NAME(getter:AGTreeValue.key(self:));

CF_EXPORT
uint32_t AGTreeValueGetFlags(AGTreeValue tree_value) CF_SWIFT_NAME(getter:AGTreeValue.flags(self:));

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
