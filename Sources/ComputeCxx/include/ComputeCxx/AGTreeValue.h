#pragma once

#include <ComputeCxx/AGBase.h>
#include <ComputeCxx/AGAttribute.h>
#include <ComputeCxx/AGType.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

typedef struct _AGTreeValue *AGTreeValue AG_SWIFT_STRUCT AG_SWIFT_NAME(TreeValue);

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGTypeID AGTreeValueGetType(AGTreeValue tree_value) AG_SWIFT_NAME(getter:AGTreeValue.type(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGTreeValueGetValue(AGTreeValue tree_value) AG_SWIFT_NAME(getter:AGTreeValue.value(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
const char *AGTreeValueGetKey(AGTreeValue tree_value) AG_SWIFT_NAME(getter:AGTreeValue.key(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint32_t AGTreeValueGetFlags(AGTreeValue tree_value) AG_SWIFT_NAME(getter:AGTreeValue.flags(self:));

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
