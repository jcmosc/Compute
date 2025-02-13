#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

CF_EXPORT
CF_REFINED_FOR_SWIFT
const char *AGGraphGetTraceEventName(uint32_t event_id);

CF_EXPORT
CF_REFINED_FOR_SWIFT
const char *AGGraphGetTraceEventSubsystem(uint32_t event_id);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
