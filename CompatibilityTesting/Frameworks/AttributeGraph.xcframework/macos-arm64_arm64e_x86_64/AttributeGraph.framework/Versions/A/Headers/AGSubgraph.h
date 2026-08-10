#pragma once

#include <AttributeGraph/AGBase.h>
#include <AttributeGraph/AGGraph.h>
#include <AttributeGraph/AGTreeElement.h>
#include <AttributeGraph/AGUniqueID.h>

AG_ASSUME_NONNULL_BEGIN
AG_IMPLICIT_BRIDGING_ENABLED

AG_EXTERN_C_BEGIN

// MARK: CFType

typedef struct AG_BRIDGED_TYPE(id) AGSubgraphStorage *AGSubgraphRef AG_SWIFT_NAME(Subgraph);

AG_EXPORT
AG_REFINED_FOR_SWIFT
CFTypeID AGSubgraphGetTypeID(void) AG_SWIFT_NAME(getter:Subgraph.typeID());

// MARK: Current subgraph

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGSubgraphRef _Nullable AGSubgraphGetCurrent(void) AG_SWIFT_NAME(getter:Subgraph.current());

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGSubgraphSetCurrent(AGSubgraphRef _Nullable subgraph) AG_SWIFT_NAME(setter:Subgraph.current(_:));

// MARK: Graph Context

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGSubgraphRef AGSubgraphCreate(AGGraphRef graph) AG_SWIFT_NAME(Subgraph.init(graph:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGSubgraphRef AGSubgraphCreate2(AGGraphRef graph, AGAttribute attribute)
    AG_SWIFT_NAME(Subgraph.init(graph:attribute:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGUnownedGraphContextRef _Nullable AGSubgraphGetCurrentGraphContext(void)
    AG_SWIFT_NAME(getter:Subgraph.currentGraphContext());

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGGraphRef AGSubgraphGetGraph(AGSubgraphRef subgraph) AG_SWIFT_NAME(getter:Subgraph.graph(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGSubgraphIsValid(AGSubgraphRef subgraph) AG_SWIFT_NAME(getter:Subgraph.isValid(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGSubgraphInvalidate(AGSubgraphRef subgraph) AG_SWIFT_NAME(Subgraph.invalidate(self:));

// MARK: Index

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint32_t AGSubgraphGetIndex(AGSubgraphRef subgraph) AG_SWIFT_NAME(getter:Subgraph.index(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGSubgraphSetIndex(AGSubgraphRef subgraph, uint32_t index) AG_SWIFT_NAME(setter:Subgraph.index(self:_:));

// MARK: Observers

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGUniqueID AGSubgraphAddObserver(AGSubgraphRef subgraph,
                                   void (*observer)(const void *_Nullable context AG_SWIFT_CONTEXT)
                                       AG_SWIFT_CC(swift),
                                   const void *_Nullable observer_context);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGSubgraphRemoveObserver(AGSubgraphRef subgraph, AGUniqueID observer_id)
    AG_SWIFT_NAME(Subgraph.removeObserver(self:_:));

// MARK: Children

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGSubgraphAddChild(AGSubgraphRef subgraph, AGSubgraphRef child) AG_SWIFT_NAME(Subgraph.addChild(self:_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGSubgraphAddChild2(AGSubgraphRef subgraph, AGSubgraphRef child, uint8_t tag)
    AG_SWIFT_NAME(Subgraph.addChild(self:_:tag:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGSubgraphRemoveChild(AGSubgraphRef subgraph, AGSubgraphRef child)
    AG_SWIFT_NAME(Subgraph.removeChild(self:_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGSubgraphRef AGSubgraphGetChild(AGSubgraphRef subgraph, uint32_t index, uint8_t *_Nullable tag_out)
    AG_SWIFT_NAME(Subgraph.child(self:at:tag:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint32_t AGSubgraphGetChildCount(AGSubgraphRef subgraph) AG_SWIFT_NAME(getter:Subgraph.childCount(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGSubgraphRef AGSubgraphGetParent(AGSubgraphRef subgraph, int64_t index) AG_SWIFT_NAME(Subgraph.parent(self:at:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint64_t AGSubgraphGetParentCount(AGSubgraphRef subgraph) AG_SWIFT_NAME(getter:Subgraph.parentCount(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGSubgraphIsAncestor(AGSubgraphRef subgraph, AGSubgraphRef other) AG_SWIFT_NAME(Subgraph.isAncestor(self:of:));

// MARK: Flags

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGSubgraphIntersects(AGSubgraphRef subgraph, AGAttributeFlags flags)
    AG_SWIFT_NAME(Subgraph.intersects(self:flags:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGSubgraphIsDirty(AGSubgraphRef subgraph, AGAttributeFlags flags) AG_SWIFT_NAME(Subgraph.isDirty(self:flags:));

// MARK: Attributes

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGSubgraphRef AGGraphGetAttributeSubgraph(AGAttribute attribute) AG_SWIFT_NAME(getter:AnyAttribute.subgraph(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGSubgraphRef _Nullable AGGraphGetAttributeSubgraph2(AGAttribute attribute)
    AG_SWIFT_NAME(getter:AnyAttribute.subgraphOrNil(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGSubgraphApply(AGSubgraphRef subgraph, uint32_t options,
                      void (*body)(AGAttribute, const void *context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                      const void *body_context);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGSubgraphUpdate(AGSubgraphRef subgraph, AGAttributeFlags flags) AG_SWIFT_NAME(Subgraph.update(self:flags:));

// MARK: Tree

AG_EXPORT
AG_REFINED_FOR_SWIFT
_Nullable AGTreeElement AGSubgraphGetTreeRoot(AGSubgraphRef subgraph)
    AG_SWIFT_NAME(getter:Subgraph.treeRoot(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGSubgraphBeginTreeElement(AGAttribute value, AGTypeID type, uint32_t flags);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGSubgraphEndTreeElement(AGAttribute value);

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGSubgraphSetTreeOwner(AGSubgraphRef subgraph, AGAttribute owner)
    AG_SWIFT_NAME(Subgraph.setTreeOwner(self:_:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGSubgraphAddTreeValue(AGAttribute value, AGTypeID type, const char *key, uint32_t flags);

AG_EXPORT
AG_REFINED_FOR_SWIFT
bool AGSubgraphShouldRecordTree(void) AG_SWIFT_NAME(getter:Subgraph.shouldRecordTree());

AG_EXPORT
AG_REFINED_FOR_SWIFT
void AGSubgraphSetShouldRecordTree(void) AG_SWIFT_NAME(Subgraph.setShouldRecordTree());

AG_EXTERN_C_END

AG_IMPLICIT_BRIDGING_DISABLED
AG_ASSUME_NONNULL_END
