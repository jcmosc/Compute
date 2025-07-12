#pragma once

#include <CoreFoundation/CFBase.h>

#include "ComputeCxx/AGSubgraph.h"
#include "Private/CFRuntime.h"

namespace AG {
class Subgraph;
}

CF_ASSUME_NONNULL_BEGIN

struct AGSubgraphStorage {
    CFRuntimeBase base;
    AG::Subgraph *_Nullable subgraph;
};

CF_ASSUME_NONNULL_END
