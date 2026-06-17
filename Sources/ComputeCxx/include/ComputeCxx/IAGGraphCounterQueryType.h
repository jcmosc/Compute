#pragma once

#include <ComputeCxx/IAGBase.h>

typedef IAG_ENUM(uint32_t, IAGGraphCounterQueryType) {
    IAGGraphCounterQueryTypeNodes,
    IAGGraphCounterQueryTypeTransactions,
    IAGGraphCounterQueryTypeUpdates,
    IAGGraphCounterQueryTypeChanges,
    IAGGraphCounterQueryTypeContextID,
    IAGGraphCounterQueryTypeGraphID,
    IAGGraphCounterQueryTypeContextThreadUpdating,
    IAGGraphCounterQueryTypeThreadUpdating,
    IAGGraphCounterQueryTypeContextNeedsUpdate,
    IAGGraphCounterQueryTypeNeedsUpdate,
    IAGGraphCounterQueryTypeMainThreadUpdates,
    IAGGraphCounterQueryTypeCreatedNodes,
    IAGGraphCounterQueryTypeSubgraphs,
    IAGGraphCounterQueryTypeCreatedSubgraphs,
} IAG_SWIFT_NAME(IAGGraphRef.CounterQueryType);
