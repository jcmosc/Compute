#pragma once

#include "ComputeCxx/AGBase.h"

#include "ComputeCxx/AGGraph.h"
#include "Graph/Context.h"
#include "Private/CFRuntime.h"

AG_ASSUME_NONNULL_BEGIN

struct AGGraphStorage {
    CFRuntimeBase base;
    AG::Graph::Context context;
};

AG_ASSUME_NONNULL_END
