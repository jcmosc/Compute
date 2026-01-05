#pragma once

#include <ComputeCxx/AGBase.h>

typedef AG_ENUM(uint32_t, AGGraphCounterQueryType) {
    AGGraphCounterQueryTypeNodes,
    AGGraphCounterQueryTypeTransactions,
    AGGraphCounterQueryTypeUpdates,
    AGGraphCounterQueryTypeChanges,
    AGGraphCounterQueryTypeContextID,
    AGGraphCounterQueryTypeGraphID,
    AGGraphCounterQueryTypeContextThreadUpdating,
    AGGraphCounterQueryTypeThreadUpdating,
    AGGraphCounterQueryTypeContextNeedsUpdate,
    AGGraphCounterQueryTypeNeedsUpdate,
    AGGraphCounterQueryTypeMainThreadUpdates,
    AGGraphCounterQueryTypeCreatedNodes,
    AGGraphCounterQueryTypeSubgraphs,
    AGGraphCounterQueryTypeCreatedSubgraphs,
} AG_SWIFT_NAME(AGGraphRef.CounterQueryType);
