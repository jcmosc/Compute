#pragma once

#include <CoreFoundation/CFBase.h>

#include "AGGraph.h"
#include "Graph/Context.h"
#include "Private/CFRuntime.h"

CF_ASSUME_NONNULL_BEGIN

struct AGUnownedGraphContext {
    AG::Graph::Context value;
};

struct AGGraphStorage {
    CFRuntimeBase base;
    AG::Graph::Context context;
};

CF_ASSUME_NONNULL_END
