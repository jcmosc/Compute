#pragma once

#include <CoreFoundation/CFBase.h>

typedef CF_ENUM(uint32_t, AGGraphCounterQueryType) {
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
} CF_SWIFT_NAME(AGGraphRef.CounterQueryType);
