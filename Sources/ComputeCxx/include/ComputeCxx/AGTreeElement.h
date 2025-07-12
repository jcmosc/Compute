#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include <ComputeCxx/AGAttribute.h>
#include <ComputeCxx/AGTreeValue.h>
#include <ComputeCxx/AGType.h>

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef uint32_t AGTreeElement AG_SWIFT_STRUCT AG_SWIFT_NAME(TreeElement);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTypeID AGTreeElementGetType(AGTreeElement tree_element) CF_SWIFT_NAME(getter:AGTreeElement.type(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGTreeElementGetValue(AGTreeElement tree_element) CF_SWIFT_NAME(getter:AGTreeElement.value(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint32_t AGTreeElementGetFlags(AGTreeElement tree_element) CF_SWIFT_NAME(getter:AGTreeElement.flags(self:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTreeElement AGTreeElementGetParent(AGTreeElement tree_element) CF_SWIFT_NAME(getter:AGTreeElement.parent(self:));

// MARK: Iterating values

typedef struct AGTreeElementValueIterator {
    AGTreeElement treeElement;
    AGTreeValue nextValue;
} AG_SWIFT_NAME(Values) AGTreeElementValueIterator;

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTreeElementValueIterator AGTreeElementMakeValueIterator(AGTreeElement tree_element)
    CF_SWIFT_NAME(AGTreeElementValueIterator.init(treeElement:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTreeValue AGTreeElementGetNextValue(AGTreeElementValueIterator iter)
    CF_SWIFT_NAME(getter:AGTreeElementValueIterator.next(self:));

// MARK: Iterating nodes

typedef struct AGTreeElementNodeIterator {
    AGTreeElement treeElement;
    uint32_t index;
} AG_SWIFT_NAME(Nodes) AGTreeElementNodeIterator;

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTreeElementNodeIterator AGTreeElementMakeNodeIterator(AGTreeElement tree_element)
    CF_SWIFT_NAME(AGTreeElementNodeIterator.init(treeElement:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGTreeElementGetNextNode(AGTreeElementNodeIterator *iter)
    CF_SWIFT_NAME(getter:AGTreeElementValueIterator.next(self:));

// MARK: Iterating children

typedef struct AGTreeElementChildIterator {
    AGTreeElement treeElement;
    AGTreeElement nextChild;
    bool iteratedSubgraphChildren;
} AG_SWIFT_NAME(Children) AGTreeElementChildIterator;

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTreeElementChildIterator AGTreeElementMakeChildIterator(AGTreeElement tree_element)
    CF_SWIFT_NAME(AGTreeElementChildIterator.init(treeElement:));

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTreeElement AGTreeElementGetNextChild(AGTreeElementChildIterator *iter)
    CF_SWIFT_NAME(getter:AGTreeElementChildIterator.next(self:));

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
