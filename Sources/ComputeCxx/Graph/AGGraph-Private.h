#pragma once

#include "ComputeCxx/AGBase.h"

#if TARGET_OS_MAC
#include "CoreFoundationPrivate/CFRuntime.h"
#else
#include <SwiftCorelibsCoreFoundation/CFRuntime.h>
#endif

#include "ComputeCxx/AGGraph.h"
#include "Graph/Context.h"

AG_ASSUME_NONNULL_BEGIN

struct AGGraphStorage {
    CFRuntimeBase base;
    AG::Graph::Context context;
};

AG_ASSUME_NONNULL_END
