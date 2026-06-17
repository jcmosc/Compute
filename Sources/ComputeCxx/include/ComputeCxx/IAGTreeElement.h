#pragma once

#include <ComputeCxx/IAGBase.h>
#include <ComputeCxx/IAGAttribute.h>
#include <ComputeCxx/IAGTreeValue.h>
#include <ComputeCxx/IAGType.h>

IAG_ASSUME_NONNULL_BEGIN

IAG_EXTERN_C_BEGIN

typedef struct _IAGTreeElement *IAGTreeElement IAG_SWIFT_STRUCT IAG_SWIFT_NAME(TreeElement);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTypeID IAGTreeElementGetType(IAGTreeElement tree_element) IAG_SWIFT_NAME(getter:IAGTreeElement.type(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGAttribute IAGTreeElementGetValue(IAGTreeElement tree_element);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
uint32_t IAGTreeElementGetFlags(IAGTreeElement tree_element) IAG_SWIFT_NAME(getter:IAGTreeElement.flags(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTreeElement _Nullable IAGTreeElementGetParent(IAGTreeElement tree_element) IAG_SWIFT_NAME(getter:IAGTreeElement.parent(self:));

// MARK: Iterating values

typedef struct IAGTreeElementValueIterator {
    uintptr_t parent_elt;
    uintptr_t next_elt;
} IAG_SWIFT_NAME(Values) IAGTreeElementValueIterator;

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTreeElementValueIterator IAGTreeElementMakeValueIterator(IAGTreeElement tree_element)
    IAG_SWIFT_NAME(getter:IAGTreeElement.values(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTreeValue _Nullable IAGTreeElementGetNextValue(IAGTreeElementValueIterator *iter) IAG_SWIFT_NAME(IAGTreeElementValueIterator.next(self:));

// MARK: Iterating nodes

typedef struct IAGTreeElementNodeIterator {
    uintptr_t elt;
    unsigned long node_index;
} IAG_SWIFT_NAME(Nodes) IAGTreeElementNodeIterator;

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTreeElementNodeIterator IAGTreeElementMakeNodeIterator(IAGTreeElement tree_element)
    IAG_SWIFT_NAME(getter:IAGTreeElement.nodes(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGAttribute IAGTreeElementGetNextNode(IAGTreeElementNodeIterator *iter);

// MARK: Iterating children

typedef struct IAGTreeElementChildIterator {
    uintptr_t parent_elt;
    uintptr_t next_elt;
    size_t subgraph_index;
} IAG_SWIFT_NAME(Children) IAGTreeElementChildIterator;

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTreeElementChildIterator IAGTreeElementMakeChildIterator(IAGTreeElement tree_element)
    IAG_SWIFT_NAME(getter:IAGTreeElement.children(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTreeElement _Nullable IAGTreeElementGetNextChild(IAGTreeElementChildIterator *iter) IAG_SWIFT_NAME(IAGTreeElementChildIterator.next(self:));

IAG_EXTERN_C_END

IAG_ASSUME_NONNULL_END
