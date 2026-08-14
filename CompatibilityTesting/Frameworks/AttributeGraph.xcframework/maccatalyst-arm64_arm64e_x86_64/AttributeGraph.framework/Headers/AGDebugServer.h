#pragma once

#include <AttributeGraph/AGBase.h>

#if TARGET_OS_MAC

#include <CoreFoundation/CFURL.h>

typedef void *AGDebugServerRef AG_SWIFT_STRUCT AG_SWIFT_NAME(DebugServer);

typedef AG_OPTIONS(uint32_t, AGDebugServerOptions){
    AGDebugServerOptionsEnabled = 1 << 0,
    AGDebugServerOptionsNetworkInterface = 1 << 1,
} AG_SWIFT_NAME(DebugServer.Options);

typedef struct AG_SWIFT_NAME(DebugServer.MessageHeader) AGDebugServerMessageHeader {
    uint32_t token;
    uint32_t reserved1;
    uint32_t body_length;
    uint32_t reserved2;
} AGDebugServerMessageHeader;

AG_ASSUME_NONNULL_BEGIN
AG_IMPLICIT_BRIDGING_ENABLED

AG_EXTERN_C_BEGIN

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGDebugServerStart(AGDebugServerOptions options) AG_SWIFT_NAME(DebugServer.start(options:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGDebugServerStop(void) AG_SWIFT_NAME(DebugServer.stop());

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGDebugServerRun(uint32_t timeout) AG_SWIFT_NAME(DebugServer.run(timeout:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
CFURLRef _Nullable AGDebugServerCopyURL(void) AG_SWIFT_NAME(getter:DebugServer.url());

AG_EXTERN_C_END

AG_IMPLICIT_BRIDGING_DISABLED
AG_ASSUME_NONNULL_END

#endif
