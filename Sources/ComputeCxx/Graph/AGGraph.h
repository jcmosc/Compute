#pragma once

#include <CoreFoundation/CFBase.h>

#include "AGSwiftSupport.h"
#include "Swift/AGType.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef struct CF_BRIDGED_TYPE(id) AGGraphStorage *AGGraphRef AG_SWIFT_NAME(Graph);

void AGGraphCreate();

void AGGraphSetOutputValue(void *value, AGTypeID type);

void AGGraphArchiveJSON(const char *filename);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
