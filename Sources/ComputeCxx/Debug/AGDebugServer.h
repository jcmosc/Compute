#pragma once

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFURL.h>

#include "AGSwiftSupport.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef void *AGDebugServerRef AG_SWIFT_NAME(DebugServer);

CFURLRef AGDebugServerCopyURL();

void AGDebugServerRun(uint32_t timeout);

void AGDebugServerStart(AGDebugServerRef server, uint32_t options);

void AGDebugServerStop(AGDebugServerRef server);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
