#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "AGTreeValue.h"
#include "Attribute/AGAttribute.h"
#include "Swift/AGType.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXTERN_C_BEGIN

typedef uint32_t AGTreeElement;

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTypeID AGTreeElementGetType(AGTreeElement tree_element);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGTreeElementGetValue(AGTreeElement tree_element);

CF_EXPORT
CF_REFINED_FOR_SWIFT
uint32_t AGTreeElementGetFlags(AGTreeElement tree_element);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTreeElement AGTreeElementGetParent(AGTreeElement tree_element);

// MARK: Iterating values

typedef struct AGTreeElementValueIterator {
    AGTreeElement tree_element;
    AGTreeValue next_value;
} AGTreeElementValueIterator;

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTreeElementValueIterator AGTreeElementMakeValueIterator(AGTreeElement tree_element);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTreeValue AGTreeElementGetNextValue(AGTreeElementValueIterator iter);

// MARK: Iterating nodes

typedef struct AGTreeElementNodeIterator {
    AGTreeElement tree_element;
    uint32_t index;
} AGTreeElementNodeIterator;

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTreeElementNodeIterator AGTreeElementMakeNodeIterator(AGTreeElement tree_element);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGAttribute AGTreeElementGetNextNode(AGTreeElementNodeIterator *iter);

// MARK: Iterating children

typedef struct AGTreeElementChildIterator {
    AGTreeElement tree_element;
    AGTreeElement next_child;
    bool iterated_subgraph;
} AGTreeElementChildIterator;

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTreeElementChildIterator AGTreeElementMakeChildIterator(AGTreeElement tree_element);

CF_EXPORT
CF_REFINED_FOR_SWIFT
AGTreeElement AGTreeElementGetNextChild(AGTreeElementChildIterator *iter);

CF_EXTERN_C_END

CF_ASSUME_NONNULL_END
