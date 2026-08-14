#pragma once

#include <ComputeCxx/IAGBase.h>

#if TARGET_OS_MAC

#include <CoreFoundation/CFURL.h>

typedef void *IAGDebugServerRef IAG_SWIFT_STRUCT IAG_SWIFT_NAME(DebugServer);

typedef IAG_OPTIONS(uint32_t, IAGDebugServerOptions){
    IAGDebugServerOptionsEnabled = 1 << 0,
    IAGDebugServerOptionsNetworkInterface = 1 << 1,
} IAG_SWIFT_NAME(DebugServer.Options);

typedef struct IAG_SWIFT_NAME(DebugServer.MessageHeader) IAGDebugServerMessageHeader {
    uint32_t token;
    uint32_t reserved1;
    uint32_t body_length;
    uint32_t reserved2;
} IAGDebugServerMessageHeader;

IAG_ASSUME_NONNULL_BEGIN
IAG_IMPLICIT_BRIDGING_ENABLED

IAG_EXTERN_C_BEGIN

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGDebugServerStart(IAGDebugServerOptions options) IAG_SWIFT_NAME(DebugServer.start(options:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGDebugServerStop(void) IAG_SWIFT_NAME(DebugServer.stop());

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGDebugServerRun(uint32_t timeout) IAG_SWIFT_NAME(DebugServer.run(timeout:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
CFURLRef _Nullable IAGDebugServerCopyURL(void) IAG_SWIFT_NAME(getter:DebugServer.url());

IAG_EXTERN_C_END

IAG_IMPLICIT_BRIDGING_DISABLED
IAG_ASSUME_NONNULL_END

#endif
