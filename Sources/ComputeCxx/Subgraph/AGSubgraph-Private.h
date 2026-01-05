#pragma once

#include "ComputeCxx/AGBase.h"

#if TARGET_OS_MAC
#include "CoreFoundationPrivate/CFRuntime.h"
#else
#include <SwiftCorelibsCoreFoundation/CFRuntime.h>
#endif

#include "ComputeCxx/AGSubgraph.h"

AG_ASSUME_NONNULL_BEGIN

namespace AG {
class Subgraph;
}

struct AGSubgraphStorage {
    CFRuntimeBase base;
    AG::Subgraph *_Nullable subgraph;
};

AG_ASSUME_NONNULL_END
