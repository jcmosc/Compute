#pragma once

#include <ComputeCxx/IAGBase.h>
#include <ComputeCxx/IAGGraph.h>
#include <ComputeCxx/IAGTreeElement.h>
#include <ComputeCxx/IAGUniqueID.h>

IAG_ASSUME_NONNULL_BEGIN
IAG_IMPLICIT_BRIDGING_ENABLED

IAG_EXTERN_C_BEGIN

// MARK: CFType

typedef struct IAG_BRIDGED_TYPE(id) IAGSubgraphStorage *IAGSubgraphRef IAG_SWIFT_NAME(Subgraph);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
CFTypeID IAGSubgraphGetTypeID(void) IAG_SWIFT_NAME(getter:Subgraph.typeID());

// MARK: Current subgraph

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGSubgraphRef _Nullable IAGSubgraphGetCurrent(void) IAG_SWIFT_NAME(getter:Subgraph.current());

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGSubgraphSetCurrent(IAGSubgraphRef _Nullable subgraph) IAG_SWIFT_NAME(setter:Subgraph.current(_:));

// MARK: Graph Context

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGSubgraphRef IAGSubgraphCreate(IAGGraphRef graph) IAG_SWIFT_NAME(Subgraph.init(graph:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGSubgraphRef IAGSubgraphCreate2(IAGGraphRef graph, IAGAttribute attribute)
    IAG_SWIFT_NAME(Subgraph.init(graph:attribute:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGUnownedGraphContextRef _Nullable IAGSubgraphGetCurrentGraphContext(void)
    IAG_SWIFT_NAME(getter:Subgraph.currentGraphContext());

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGGraphRef IAGSubgraphGetGraph(IAGSubgraphRef subgraph) IAG_SWIFT_NAME(getter:Subgraph.graph(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGSubgraphIsValid(IAGSubgraphRef subgraph) IAG_SWIFT_NAME(getter:Subgraph.isValid(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGSubgraphInvalidate(IAGSubgraphRef subgraph) IAG_SWIFT_NAME(Subgraph.invalidate(self:));

// MARK: Index

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
uint32_t IAGSubgraphGetIndex(IAGSubgraphRef subgraph) IAG_SWIFT_NAME(getter:Subgraph.index(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGSubgraphSetIndex(IAGSubgraphRef subgraph, uint32_t index) IAG_SWIFT_NAME(setter:Subgraph.index(self:_:));

// MARK: Observers

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGUniqueID IAGSubgraphAddObserver(IAGSubgraphRef subgraph,
                                 void (*observer)(const void *_Nullable context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                                 const void *_Nullable observer_context);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGSubgraphRemoveObserver(IAGSubgraphRef subgraph, IAGUniqueID observer_id)
    IAG_SWIFT_NAME(Subgraph.removeObserver(self:_:));

// MARK: Children

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGSubgraphAddChild(IAGSubgraphRef subgraph, IAGSubgraphRef child) IAG_SWIFT_NAME(Subgraph.addChild(self:_:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGSubgraphAddChild2(IAGSubgraphRef subgraph, IAGSubgraphRef child, uint8_t tag)
    IAG_SWIFT_NAME(Subgraph.addChild(self:_:tag:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGSubgraphRemoveChild(IAGSubgraphRef subgraph, IAGSubgraphRef child)
    IAG_SWIFT_NAME(Subgraph.removeChild(self:_:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGSubgraphRef IAGSubgraphGetChild(IAGSubgraphRef subgraph, uint32_t index, uint8_t *_Nullable tag_out)
    IAG_SWIFT_NAME(Subgraph.child(self:at:tag:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
uint32_t IAGSubgraphGetChildCount(IAGSubgraphRef subgraph) IAG_SWIFT_NAME(getter:Subgraph.childCount(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGSubgraphRef IAGSubgraphGetParent(IAGSubgraphRef subgraph, int64_t index) IAG_SWIFT_NAME(Subgraph.parent(self:at:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
uint64_t IAGSubgraphGetParentCount(IAGSubgraphRef subgraph) IAG_SWIFT_NAME(getter:Subgraph.parentCount(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGSubgraphIsAncestor(IAGSubgraphRef subgraph, IAGSubgraphRef other)
    IAG_SWIFT_NAME(Subgraph.isAncestor(self:of:));

// MARK: Flags

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGSubgraphIntersects(IAGSubgraphRef subgraph, IAGAttributeFlags flags)
    IAG_SWIFT_NAME(Subgraph.intersects(self:flags:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGSubgraphIsDirty(IAGSubgraphRef subgraph, IAGAttributeFlags flags) IAG_SWIFT_NAME(Subgraph.isDirty(self:flags:));

// MARK: Attributes

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGSubgraphRef IAGGraphGetAttributeSubgraph(IAGAttribute attribute) IAG_SWIFT_NAME(getter:AnyAttribute.subgraph(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGSubgraphRef _Nullable IAGGraphGetAttributeSubgraph2(IAGAttribute attribute)
    IAG_SWIFT_NAME(getter:AnyAttribute.subgraphOrNil(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGSubgraphApply(IAGSubgraphRef subgraph, uint32_t options,
                     void (*body)(IAGAttribute, const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                     const void *body_context);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGSubgraphUpdate(IAGSubgraphRef subgraph, IAGAttributeFlags flags) IAG_SWIFT_NAME(Subgraph.update(self:flags:));

// MARK: Tree

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
_Nullable IAGTreeElement IAGSubgraphGetTreeRoot(IAGSubgraphRef subgraph) IAG_SWIFT_NAME(getter:Subgraph.treeRoot(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGSubgraphBeginTreeElement(IAGAttribute value, IAGTypeID type, uint32_t flags);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGSubgraphEndTreeElement(IAGAttribute value);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGSubgraphSetTreeOwner(IAGSubgraphRef subgraph, IAGAttribute owner)
    IAG_SWIFT_NAME(Subgraph.setTreeOwner(self:_:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGSubgraphAddTreeValue(IAGAttribute value, IAGTypeID type, const char *key, uint32_t flags);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
bool IAGSubgraphShouldRecordTree(void) IAG_SWIFT_NAME(getter:Subgraph.shouldRecordTree());

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
void IAGSubgraphSetShouldRecordTree(void) IAG_SWIFT_NAME(Subgraph.setShouldRecordTree());

IAG_EXTERN_C_END

IAG_IMPLICIT_BRIDGING_DISABLED
IAG_ASSUME_NONNULL_END
