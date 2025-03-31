#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "Attribute/AGAttribute.h"
#include "Swift/AGType.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef uint32_t AGTreeValue;

CF_EXPORT
AGTypeID AGTreeValueGetType(AGTreeValue tree_value);

CF_EXPORT
AGAttribute AGTreeValueGetValue(AGTreeValue tree_value);

CF_EXPORT
const char *AGTreeValueGetKey(AGTreeValue tree_value);

CF_EXPORT
uint32_t AGTreeValueGetFlags(AGTreeValue tree_value);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
