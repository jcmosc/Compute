#pragma once

#include "ComputeCxx/AGBase.h"

#include "ComputeCxx/AGSubgraph.h"
#include "Private/CFRuntime.h"

AG_ASSUME_NONNULL_BEGIN

namespace AG {
class Subgraph;
}

struct AGSubgraphStorage {
    CFRuntimeBase base;
    AG::Subgraph *_Nullable subgraph;
};

AG_ASSUME_NONNULL_END
