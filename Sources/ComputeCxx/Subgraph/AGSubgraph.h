#pragma once

#include <CoreFoundation/CFBase.h>

#include "Attribute/AGAttribute.h"
#include "Graph/AGGraph.h"
#include "Graph/Tree/AGTreeElement.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef struct CF_BRIDGED_TYPE(id) AGSubgraphStorage *AGSubgraphRef CF_SWIFT_NAME(Subgraph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
CFTypeID AGSubgraphGetTypeID();

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGSubgraphRef AGSubgraphCreate(AGGraphRef graph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGSubgraphRef AGSubgraphCreate2(AGGraphRef graph, AGAttribute attribute);

// MARK: Current subgraph

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGSubgraphRef _Nullable AGSubgraphGetCurrent() CF_SWIFT_NAME(getter:AGSubgraphRef.current());

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphSetCurrent(AGSubgraphRef _Nullable subgraph) CF_SWIFT_NAME(setter:AGSubgraphRef.current(_:));

// MARK: Graph

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGGraphRef AGSubgraphGetGraph(AGSubgraphRef subgraph) CF_SWIFT_NAME(getter:AGSubgraphRef.graph(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGUnownedGraphContextRef AGSubgraphGetCurrentGraphContext(AGSubgraphRef subgraph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGSubgraphRef AGGraphGetAttributeSubgraph(AGAttribute attribute);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGSubgraphRef AGGraphGetAttributeSubgraph2(AGAttribute attribute);

// MARK: Children

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphAddChild(AGSubgraphRef subgraph, AGSubgraphRef child);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphAddChild2(AGSubgraphRef subgraph, AGSubgraphRef child, uint32_t flags);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphRemoveChild(AGSubgraphRef subgraph, AGSubgraphRef child);

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint32_t AGSubgraphGetChildCount(AGSubgraphRef subgraph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGSubgraphRef AGSubgraphGetChild(AGSubgraphRef subgraph, uint32_t index, uint32_t *flags_out);

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint64_t AGSubgraphGetParentCount(AGSubgraphRef subgraph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGSubgraphRef AGSubgraphGetParent(AGSubgraphRef subgraph, int64_t index);

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGSubgraphIsAncestor(AGSubgraphRef subgraph, AGSubgraphRef possible_descendant);

// MARK: Attributes

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGSubgraphIsValid(AGSubgraphRef subgraph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGSubgraphIsDirty(AGSubgraphRef subgraph, uint8_t mask);

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGSubgraphIntersects(AGSubgraphRef subgraph, uint8_t mask);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphInvalidate(AGSubgraphRef subgraph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphUpdate(AGSubgraphRef subgraph, uint8_t flags);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphApply(AGSubgraphRef subgraph, AGAttributeFlags flags,
                     void (*function)(const void *context AG_SWIFT_CONTEXT, AGAttribute) AG_SWIFT_CC(swift),
                     const void *function_context);

// MARK: Tree

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTreeElement AGSubgraphGetTreeRoot(AGSubgraphRef subgraph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphSetTreeOwner(AGSubgraphRef subgraph, AGAttribute owner);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphAddTreeValue(AGAttribute value, AGTypeID type, const char *key, uint32_t flags);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphBeginTreeElement(AGAttribute value, AGTypeID type, uint32_t flags);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphEndTreeElement(AGAttribute value);

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGSubgraphShouldRecordTree();

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphSetShouldRecordTree();

// MARK: Observers

CF_EXPORT CF_REFINED_FOR_SWIFT uint64_t AGSubgraphAddObserver(AGSubgraphRef subgraph,
                                                              void (*observer)(const void *context AG_SWIFT_CONTEXT)
                                                                  AG_SWIFT_CC(swift),
                                                              const void *observer_context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphRemoveObserver(AGSubgraphRef subgraph, uint64_t observer_id);

// MARK: Index

CF_EXPORT CF_REFINED_FOR_SWIFT uint32_t AGSubgraphGetIndex(AGSubgraphRef subgraph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphSetIndex(AGSubgraphRef subgraph, uint32_t index);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
