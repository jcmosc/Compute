#pragma once

#include "AGSwiftSupport.h"

extern "C" const ::swift::Metadata *_Nullable swift_getTypeByMangledNameInContext(
    const char *_Nullable typeNameStart, size_t typeNameLength, const void *_Nullable context,
    const void *_Nullable const *_Nullable genericArgs) AG_SWIFT_CC(swift);
