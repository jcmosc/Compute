#pragma once

#include "ComputeCxx/IAGBase.h"

#if TARGET_OS_MAC
#include "CoreFoundationPrivate/CFRuntime.h"
#else
#include <SwiftCorelibsCoreFoundation/CFRuntime.h>
#endif

#include "ComputeCxx/IAGSubgraph.h"

IAG_ASSUME_NONNULL_BEGIN

namespace IAG {
class Subgraph;
}

struct IAGSubgraphStorage {
    CFRuntimeBase base;
    IAG::Subgraph *_Nullable subgraph;
};

IAG_ASSUME_NONNULL_END
