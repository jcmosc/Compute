#pragma once

#include <ComputeCxx/AGBase.h>
#include <ComputeCxx/AGAttribute.h>
#include <ComputeCxx/AGTreeValue.h>
#include <ComputeCxx/AGType.h>

AG_ASSUME_NONNULL_BEGIN

AG_EXTERN_C_BEGIN

typedef struct _AGTreeElement *AGTreeElement AG_SWIFT_STRUCT AG_SWIFT_NAME(TreeElement);

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGTypeID AGTreeElementGetType(AGTreeElement tree_element) AG_SWIFT_NAME(getter:AGTreeElement.type(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGTreeElementGetValue(AGTreeElement tree_element);

AG_EXPORT
AG_REFINED_FOR_SWIFT
uint32_t AGTreeElementGetFlags(AGTreeElement tree_element) AG_SWIFT_NAME(getter:AGTreeElement.flags(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGTreeElement _Nullable AGTreeElementGetParent(AGTreeElement tree_element) AG_SWIFT_NAME(getter:AGTreeElement.parent(self:));

// MARK: Iterating values

typedef struct AGTreeElementValueIterator {
    uintptr_t parent_elt;
    uintptr_t next_elt;
} AG_SWIFT_NAME(Values) AGTreeElementValueIterator;

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGTreeElementValueIterator AGTreeElementMakeValueIterator(AGTreeElement tree_element)
    AG_SWIFT_NAME(getter:AGTreeElement.values(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGTreeValue _Nullable AGTreeElementGetNextValue(AGTreeElementValueIterator *iter) AG_SWIFT_NAME(AGTreeElementValueIterator.next(self:));

// MARK: Iterating nodes

typedef struct AGTreeElementNodeIterator {
    uintptr_t elt;
    unsigned long node_index;
} AG_SWIFT_NAME(Nodes) AGTreeElementNodeIterator;

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGTreeElementNodeIterator AGTreeElementMakeNodeIterator(AGTreeElement tree_element)
    AG_SWIFT_NAME(getter:AGTreeElement.nodes(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGAttribute AGTreeElementGetNextNode(AGTreeElementNodeIterator *iter);

// MARK: Iterating children

typedef struct AGTreeElementChildIterator {
    uintptr_t parent_elt;
    uintptr_t next_elt;
    size_t subgraph_index;
} AG_SWIFT_NAME(Children) AGTreeElementChildIterator;

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGTreeElementChildIterator AGTreeElementMakeChildIterator(AGTreeElement tree_element)
    AG_SWIFT_NAME(getter:AGTreeElement.children(self:));

AG_EXPORT
AG_REFINED_FOR_SWIFT
AGTreeElement _Nullable AGTreeElementGetNextChild(AGTreeElementChildIterator *iter) AG_SWIFT_NAME(AGTreeElementChildIterator.next(self:));

AG_EXTERN_C_END

AG_ASSUME_NONNULL_END
