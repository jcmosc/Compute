#include "AGTreeElement.h"

#include "Subgraph/Subgraph.h"
#include "TreeElement.h"

AGTypeID AGTreeElementGetType(AGTreeElement tree_element) {
    auto tree_element_id = AG::TreeElementID(tree_element);
    auto type = tree_element_id.to_element_ptr()->type;
    return AGTypeID(type);
}

AGAttribute AGTreeElementGetValue(AGTreeElement tree_element) {
    auto tree_element_id = AG::TreeElementID(tree_element);
    auto node = tree_element_id.to_element_ptr()->node;
    return AGAttribute(node);
}

uint32_t AGTreeElementGetFlags(AGTreeElement tree_element) {
    auto tree_element_id = AG::TreeElementID(tree_element);
    return tree_element_id.to_element_ptr()->flags;
}

AGTreeElement AGTreeElementGetParent(AGTreeElement tree_element) {
    auto tree_element_id = AG::TreeElementID(tree_element);
    return tree_element_id.to_element_ptr()->parent;
}

#pragma mark - Iterating values

AGTreeElementValueIterator AGTreeElementMakeValueIterator(AGTreeElement tree_element) {
    auto tree_element_id = AG::TreeElementID(tree_element);
    auto tree_value = tree_element_id.to_element_ptr()->first_value;
    return AGTreeElementValueIterator(tree_element, tree_value);
}

AGTreeValue AGTreeElementGetNextValue(AGTreeElementValueIterator iter) {
    AGTreeValue tree_value = iter.next_value;
    if (tree_value) {
        auto tree_value_id = AG::TreeValueID(tree_value);
        iter.next_value = tree_value_id.to_tree_value().next;
    }
    return tree_value;
}

#pragma mark - Iterating values

AGTreeElementNodeIterator AGTreeElementMakeNodeIterator(AGTreeElement tree_element) { return {tree_element, 0}; }

AGAttribute AGTreeElementGetNextNode(AGTreeElementNodeIterator *iter) {
    auto tree_element_id = AG::TreeElementID(iter->tree_element);
    AG::AttributeID node =
        tree_element_id.subgraph()->tree_node_at_index(tree_element_id.to_element_ptr(), iter->index);
    if (node.without_kind() == 0) {
        return AGAttributeNil;
    }
    iter->index += 1;
    return node;
}

#pragma mark - Iterating children

AGTreeElementChildIterator AGTreeElementMakeChildIterator(AGTreeElement tree_element) {
    auto tree_element_id = AG::TreeElementID(tree_element);
    auto child = tree_element_id.to_element_ptr()->first_child;
    return AGTreeElementChildIterator(tree_element, child, 0);
}

AGTreeElement AGTreeElementGetNextChild(AGTreeElementChildIterator *iter) {
    AGTreeElement next_child = iter->next_child;
    if (next_child) {
        iter->next_child = AG::TreeElementID(next_child).to_element_ptr()->next_sibling;
        return next_child;
    }

    if (!iter->iterated_subgraph) {
        iter->iterated_subgraph = true;
        auto tree_element_id = AG::TreeElementID(iter->tree_element);
        auto subgraph = tree_element_id.subgraph();
        auto next_child = subgraph->tree_subgraph_child(tree_element_id.to_element_ptr());
        if (next_child) {
            iter->next_child = AG::TreeElementID(next_child).to_element_ptr()->next_sibling;
            return next_child;
        }
    }

    return 0;
}
