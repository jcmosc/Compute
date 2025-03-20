#pragma once

#include <CoreFoundation/CFBase.h>

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef struct CF_BRIDGED_TYPE(id) AGSubgraphStorage *AGSubgraphRef CF_SWIFT_NAME(Subgraph);

CF_EXPORT
CFTypeID AGSubgraphGetTypeID();

bool AGSubgraphShouldRecordTree();

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
