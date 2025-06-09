#pragma once

#include <CoreFoundation/CFBase.h>

#include "Graph/AGGraph.h"
#include "Graph/Tree/AGTreeElement.h"

CF_ASSUME_NONNULL_BEGIN
CF_IMPLICIT_BRIDGING_ENABLED

CF_EXTERN_C_BEGIN

// MARK: CFType

typedef struct CF_BRIDGED_TYPE(id) AGSubgraphStorage *AGSubgraphRef CF_SWIFT_NAME(Subgraph);

CF_EXPORT
CF_REFINED_FOR_SWIFT
CFTypeID AGSubgraphGetTypeID() CF_SWIFT_NAME(getter:AGSubgraphRef.typeID());

// MARK: Current subgraph

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGSubgraphRef _Nullable AGSubgraphGetCurrent() CF_SWIFT_NAME(getter:AGSubgraphRef.current());

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphSetCurrent(AGSubgraphRef _Nullable subgraph) CF_SWIFT_NAME(setter:AGSubgraphRef.current(_:));

// MARK: Graph Context

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGSubgraphRef AGSubgraphCreate(AGGraphRef graph) CF_SWIFT_NAME(AGSubgraphRef.init(graph:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGSubgraphRef AGSubgraphCreate2(AGGraphRef graph, AGAttribute attribute)
    CF_SWIFT_NAME(AGSubgraphRef.init(graph:attribute:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGUnownedGraphRef _Nullable AGSubgraphGetCurrentGraphContext()
    CF_SWIFT_NAME(getter:AGSubgraphRef.currentGraphContext());

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGGraphRef AGSubgraphGetGraph(AGSubgraphRef subgraph) CF_SWIFT_NAME(getter:AGSubgraphRef.graph(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGSubgraphIsValid(AGSubgraphRef subgraph) CF_SWIFT_NAME(getter:AGSubgraphRef.isValid(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphInvalidate(AGSubgraphRef subgraph) CF_SWIFT_NAME(AGSubgraphRef.invalidate(self:));

// MARK: Observers

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint64_t AGSubgraphAddObserver(AGSubgraphRef subgraph,
                               void (*observer)(void *_Nullable context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                               void *_Nullable observer_context);

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphRemoveObserver(AGSubgraphRef subgraph, uint64_t observer_id)
    CF_SWIFT_NAME(AGSubgraphRef.removeObserver(self:observerID:));

// MARK: Children

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphAddChild(AGSubgraphRef subgraph, AGSubgraphRef child) CF_SWIFT_NAME(AGSubgraphRef.addChild(self:_:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphAddChild2(AGSubgraphRef subgraph, AGSubgraphRef child, uint8_t tag)
    CF_SWIFT_NAME(AGSubgraphRef.addChild(self:_:tag:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphRemoveChild(AGSubgraphRef subgraph, AGSubgraphRef child)
    CF_SWIFT_NAME(AGSubgraphRef.removeChild(self:_:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGSubgraphRef AGSubgraphGetChild(AGSubgraphRef subgraph, uint32_t index,
                                 uint8_t *_Nullable tag_out) CF_RETURNS_NOT_RETAINED
    CF_SWIFT_NAME(AGSubgraphRef.child(self:at:tag:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint32_t AGSubgraphGetChildCount(AGSubgraphRef subgraph) CF_SWIFT_NAME(getter:AGSubgraphRef.childCount(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGSubgraphRef AGSubgraphGetParent(AGSubgraphRef subgraph, int64_t index) CF_RETURNS_NOT_RETAINED
    CF_SWIFT_NAME(AGSubgraphRef.parent(self:at:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint64_t AGSubgraphGetParentCount(AGSubgraphRef subgraph) CF_SWIFT_NAME(getter:AGSubgraphRef.parentCount(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGSubgraphIsAncestor(AGSubgraphRef subgraph, AGSubgraphRef other)
    CF_SWIFT_NAME(AGSubgraphRef.isAncestor(self:of:));

// MARK: Flags

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGSubgraphIntersects(AGSubgraphRef subgraph, AGAttributeFlags mask)
    CF_SWIFT_NAME(AGSubgraphRef.intersects(self:mask:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGSubgraphIsDirty(AGSubgraphRef subgraph, AGAttributeFlags mask) CF_SWIFT_NAME(AGSubgraphRef.isDirty(self:mask:));

// MARK: Attributes

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGSubgraphRef AGGraphGetAttributeSubgraph(AGAttribute attribute);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGSubgraphRef AGGraphGetAttributeSubgraph2(AGAttribute attribute) CF_SWIFT_NAME(getter:AGAttribute.subgraph(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphApply(AGSubgraphRef subgraph, uint32_t options,
                     void (*body)(void *context AG_SWIFT_CONTEXT, AGAttribute) AG_SWIFT_CC(swift), void *body_context);

// MARK: Tree

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTreeElement AGSubgraphGetTreeRoot(AGSubgraphRef subgraph) CF_SWIFT_NAME(getter:AGSubgraphRef.treeRoot(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphBeginTreeElement(AGAttribute value, AGTypeID type, uint32_t flags)
    CF_SWIFT_NAME(AGSubgraphRef.beginTreeElement(value:type:flags:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphEndTreeElement(AGAttribute value) CF_SWIFT_NAME(AGSubgraphRef.endTreeElement(value:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphSetTreeOwner(AGSubgraphRef subgraph, AGAttribute owner)
    CF_SWIFT_NAME(setter:AGSubgraphRef.treeOwner(self:_:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphAddTreeValue(AGAttribute value, AGTypeID type, const char *key, uint32_t flags)
    CF_SWIFT_NAME(AGSubgraphRef.addTreeValue(value:type:forKey:flags:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
bool AGSubgraphShouldRecordTree() CF_SWIFT_NAME(getter:AGSubgraphRef.shouldRecordTree());

CF_EXPORT
CF_REFINED_FOR_SWIFT
void AGSubgraphSetShouldRecordTree() CF_SWIFT_NAME(AGSubgraphRef.setShouldRecordTree());

CF_EXTERN_C_END

CF_IMPLICIT_BRIDGING_DISABLED
CF_ASSUME_NONNULL_END
