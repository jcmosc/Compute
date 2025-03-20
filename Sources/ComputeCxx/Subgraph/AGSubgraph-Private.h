#pragma once

#include <CoreFoundation/CFBase.h>

#include "AGSubgraph.h"
#include "Private/CFRuntime.h"
#include "Subgraph.h"

CF_ASSUME_NONNULL_BEGIN

struct AGSubgraphStorage {
    CFRuntimeBase base;
    AG::Subgraph *subgraph;
};

CF_ASSUME_NONNULL_END
