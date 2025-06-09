#include "AGTreeElement.h"

#include "Subgraph/Subgraph.h"
#include "TreeElement.h"

AGTypeID AGTreeElementGetType(AGTreeElement tree_element) {
    auto tree_element_id = AG::Graph::TreeElementID(tree_element);
    auto type = tree_element_id->type;
    return AGTypeID(type);
}

AGAttribute AGTreeElementGetValue(AGTreeElement tree_element) {
    auto tree_element_id = AG::Graph::TreeElementID(tree_element);
    auto value = tree_element_id->value;
    return AGAttribute(value);
}

uint32_t AGTreeElementGetFlags(AGTreeElement tree_element) {
    auto tree_element_id = AG::Graph::TreeElementID(tree_element);
    return tree_element_id->flags;
}

AGTreeElement AGTreeElementGetParent(AGTreeElement tree_element) {
    auto tree_element_id = AG::Graph::TreeElementID(tree_element);
    return AGTreeElement(tree_element_id->parent);
}

#pragma mark - Iterating values

AGTreeElementValueIterator AGTreeElementMakeValueIterator(AGTreeElement tree_element) {
    auto tree_element_id = AG::Graph::TreeElementID(tree_element);
    return AGTreeElementValueIterator(tree_element, tree_element_id->first_value);
}

AGTreeValue AGTreeElementGetNextValue(AGTreeElementValueIterator iter) {
    AGTreeValue next_value = iter.nextValue;
    if (next_value) {
        auto next_value_id = AG::Graph::TreeValueID(next_value);
        iter.nextValue = AGTreeValue(next_value_id->next);
    }
    return next_value;
}

#pragma mark - Iterating nodes

AGTreeElementNodeIterator AGTreeElementMakeNodeIterator(AGTreeElement tree_element) { return {tree_element, 0}; }

AGAttribute AGTreeElementGetNextNode(AGTreeElementNodeIterator *iter) {
    auto tree_element_id = AG::Graph::TreeElementID(iter->treeElement);
    AG::AttributeID node = tree_element_id.subgraph()->tree_node_at_index(tree_element_id, iter->index);
    if (!node || node.is_nil()) {
        return AGAttributeNil;
    }
    iter->index += 1;
    return AGAttribute(node);
}

#pragma mark - Iterating children

AGTreeElementChildIterator AGTreeElementMakeChildIterator(AGTreeElement tree_element) {
    auto tree_element_id = AG::Graph::TreeElementID(tree_element);
    auto child = tree_element_id->first_child;
    return AGTreeElementChildIterator(tree_element, child, 0);
}

AGTreeElement AGTreeElementGetNextChild(AGTreeElementChildIterator *iter) {
    AG::Graph::TreeElementID next_child = AG::Graph::TreeElementID(iter->nextChild);
    if (next_child) {
        iter->nextChild = AGTreeElement(next_child->next_sibling);
        return next_child;
    }

    if (!iter->iteratedSubgraphChildren) {
        iter->iteratedSubgraphChildren = true;
        auto tree_element_id = AG::Graph::TreeElementID(iter->treeElement);
        auto subgraph = tree_element_id.subgraph();
        auto subgraph_child = subgraph->tree_subgraph_child(tree_element_id);
        if (subgraph_child) {
            iter->nextChild = AG::Graph::TreeElementID(subgraph_child->next_sibling);
            return AGTreeElement(subgraph_child);
        }
    }

    return 0;
}
