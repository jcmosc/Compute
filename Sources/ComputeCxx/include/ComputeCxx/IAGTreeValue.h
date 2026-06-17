#pragma once

#include <ComputeCxx/IAGBase.h>
#include <ComputeCxx/IAGAttribute.h>
#include <ComputeCxx/IAGType.h>

IAG_ASSUME_NONNULL_BEGIN

IAG_EXTERN_C_BEGIN

typedef struct _IAGTreeValue *IAGTreeValue IAG_SWIFT_STRUCT IAG_SWIFT_NAME(TreeValue);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTypeID IAGTreeValueGetType(IAGTreeValue tree_value) IAG_SWIFT_NAME(getter:IAGTreeValue.type(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGAttribute IAGTreeValueGetValue(IAGTreeValue tree_value) IAG_SWIFT_NAME(getter:IAGTreeValue.value(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
const char *IAGTreeValueGetKey(IAGTreeValue tree_value) IAG_SWIFT_NAME(getter:IAGTreeValue.key(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
uint32_t IAGTreeValueGetFlags(IAGTreeValue tree_value) IAG_SWIFT_NAME(getter:IAGTreeValue.flags(self:));

IAG_EXTERN_C_END

IAG_ASSUME_NONNULL_END
