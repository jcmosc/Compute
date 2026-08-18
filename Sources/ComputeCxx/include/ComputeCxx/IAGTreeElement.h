#pragma once

#include <ComputeCxx/IAGAttribute.h>
#include <ComputeCxx/IAGBase.h>
#include <ComputeCxx/IAGTreeValue.h>
#include <ComputeCxx/IAGType.h>

IAG_ASSUME_NONNULL_BEGIN

IAG_EXTERN_C_BEGIN

typedef struct _IAGTreeElement *IAGTreeElement IAG_SWIFT_STRUCT IAG_SWIFT_NAME(TreeElement);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTypeID IAGTreeElementGetType(IAGTreeElement tree_element) IAG_SWIFT_NAME(getter:TreeElement.type(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGAttribute IAGTreeElementGetValue(IAGTreeElement tree_element);

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
uint32_t IAGTreeElementGetFlags(IAGTreeElement tree_element) IAG_SWIFT_NAME(getter:TreeElement.flags(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTreeElement _Nullable IAGTreeElementGetParent(IAGTreeElement tree_element)
    IAG_SWIFT_NAME(getter:TreeElement.parent(self:));

// MARK: Iterating values

typedef struct {
    uintptr_t parent_elt;
    uintptr_t next_elt;
} IAG_SWIFT_NAME(TreeElement.Values) IAGTreeElementValueIterator;

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTreeElementValueIterator IAGTreeElementMakeValueIterator(IAGTreeElement tree_element)
    IAG_SWIFT_NAME(getter:TreeElement.values(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTreeValue _Nullable IAGTreeElementGetNextValue(IAGTreeElementValueIterator *iter)
    IAG_SWIFT_NAME(TreeElement.Values.next(self:));

// MARK: Iterating nodes

typedef struct {
    uintptr_t elt;
    unsigned long node_index;
} IAG_SWIFT_NAME(TreeElement.Nodes) IAGTreeElementNodeIterator;

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTreeElementNodeIterator IAGTreeElementMakeNodeIterator(IAGTreeElement tree_element)
    IAG_SWIFT_NAME(getter:TreeElement.nodes(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGAttribute IAGTreeElementGetNextNode(IAGTreeElementNodeIterator *iter);

// MARK: Iterating children

typedef struct {
    uintptr_t parent_elt;
    uintptr_t next_elt;
    size_t subgraph_index;
} IAG_SWIFT_NAME(TreeElement.Children) IAGTreeElementChildIterator;

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTreeElementChildIterator IAGTreeElementMakeChildIterator(IAGTreeElement tree_element)
    IAG_SWIFT_NAME(getter:TreeElement.children(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTreeElement _Nullable IAGTreeElementGetNextChild(IAGTreeElementChildIterator *iter)
    IAG_SWIFT_NAME(TreeElement.Children.next(self:));

IAG_EXPORT
IAG_REFINED_FOR_SWIFT
IAGTreeElement _Nullable IAGTreeElementGetNextChild2(IAGTreeElementChildIterator *iter, bool include_child_subgraphs)
    IAG_SWIFT_NAME(TreeElement.Children.next(self:includeChildSubgraphs:));

IAG_EXTERN_C_END

IAG_ASSUME_NONNULL_END
