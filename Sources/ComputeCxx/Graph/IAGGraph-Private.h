#pragma once

#include "ComputeCxx/IAGBase.h"

#if TARGET_OS_MAC
#include "CoreFoundationPrivate/CFRuntime.h"
#else
#include <SwiftCorelibsCoreFoundation/CFRuntime.h>
#endif

#include "ComputeCxx/IAGGraph.h"
#include "Graph/Context.h"

IAG_ASSUME_NONNULL_BEGIN

struct IAGGraphStorage {
    CFRuntimeBase base;
    IAG::Graph::Context context;
};

IAG_ASSUME_NONNULL_END
