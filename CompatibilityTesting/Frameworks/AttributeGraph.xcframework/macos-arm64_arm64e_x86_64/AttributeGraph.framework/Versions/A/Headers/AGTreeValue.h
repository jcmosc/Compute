#pragma once

#include <AttributeGraph/AGAttribute.h>
#include <AttributeGraph/AGBase.h>
#include <AttributeGraph/AGType.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

typedef struct _AGTreeValue *AGTreeValue AG_SWIFT_STRUCT AG_SWIFT_NAME(TreeValue);

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGTypeID AGTreeValueGetType(AGTreeValue tree_value) AG_SWIFT_NAME(getter:TreeValue.type(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGTreeValueGetValue(AGTreeValue tree_value) AG_SWIFT_NAME(getter:TreeValue.value(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
const char *AGTreeValueGetKey(AGTreeValue tree_value) AG_SWIFT_NAME(getter:TreeValue.key(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint32_t AGTreeValueGetFlags(AGTreeValue tree_value) AG_SWIFT_NAME(getter:TreeValue.flags(self:));

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
