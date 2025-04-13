#include "AGTreeElement.h"

#include "Subgraph/Subgraph.h"
#include "TreeElement.h"

AGTypeID AGTreeElementGetType(AGTreeElement tree_element) {
    auto tree_element_id = AG::TreeElementID::from_storage(tree_element);
    auto type = tree_element_id.to_ptr()->type;
    return AGTypeID(type);
}

// TODO: rename to value
AGAttribute AGTreeElementGetValue(AGTreeElement tree_element) {
    auto tree_element_id = AG::TreeElementID::from_storage(tree_element);
    auto node = tree_element_id.to_ptr()->node;
    return AGAttribute(node);
}

uint32_t AGTreeElementGetFlags(AGTreeElement tree_element) {
    auto tree_element_id = AG::TreeElementID::from_storage(tree_element);
    return tree_element_id.to_ptr()->flags;
}

AGTreeElement AGTreeElementGetParent(AGTreeElement tree_element) {
    auto tree_element_id = AG::TreeElementID::from_storage(tree_element);
    return AG::TreeElementID(tree_element_id.to_ptr()->parent).to_storage();
}

#pragma mark - Iterating values

AGTreeElementValueIterator AGTreeElementMakeValueIterator(AGTreeElement tree_element) {
    auto tree_element_id = AG::TreeElementID::from_storage(tree_element);
    auto tree_value = AG::TreeValueID(tree_element_id.to_ptr()->first_value);
    return AGTreeElementValueIterator(tree_element, tree_value.to_storage());
}

AGTreeValue AGTreeElementGetNextValue(AGTreeElementValueIterator iter) {
    AGTreeValue tree_value = iter.next_value;
    if (tree_value) {
        auto tree_value_id = AG::TreeValueID::from_storage(tree_value);
        iter.next_value = AG::TreeValueID(tree_value_id.to_tree_value().next).to_storage();
    }
    return tree_value;
}

#pragma mark - Iterating values

AGTreeElementNodeIterator AGTreeElementMakeNodeIterator(AGTreeElement tree_element) { return {tree_element, 0}; }

AGAttribute AGTreeElementGetNextNode(AGTreeElementNodeIterator *iter) {
    auto tree_element_id = AG::TreeElementID::from_storage(iter->tree_element);
    AG::AttributeID node =
        tree_element_id.subgraph()->tree_node_at_index(tree_element_id.to_ptr(), iter->index);
    if (!node.has_value()) {
        return AGAttributeNil;
    }
    iter->index += 1;
    return node;
}

#pragma mark - Iterating children

AGTreeElementChildIterator AGTreeElementMakeChildIterator(AGTreeElement tree_element) {
    auto tree_element_id = AG::TreeElementID::from_storage(tree_element);
    auto child = AG::TreeElementID(tree_element_id.to_ptr()->first_child);
    return AGTreeElementChildIterator(tree_element, child.to_storage(), 0);
}

AGTreeElement AGTreeElementGetNextChild(AGTreeElementChildIterator *iter) {
    AGTreeElement next_child = iter->next_child;
    if (next_child) {
        iter->next_child = AG::TreeElementID(AG::TreeElementID::from_storage(next_child).to_ptr()->next_sibling).to_storage();
        return next_child;
    }

    if (!iter->iterated_subgraph) {
        iter->iterated_subgraph = true;
        auto tree_element_id = AG::TreeElementID::from_storage(iter->tree_element);
        auto subgraph = tree_element_id.subgraph();
        auto next_child = subgraph->tree_subgraph_child(tree_element_id.to_ptr());
        if (next_child) {
            iter->next_child = AG::TreeElementID(AG::TreeElementID(next_child).to_ptr()->next_sibling).to_storage();
            return AG::TreeElementID(next_child).to_storage();
        }
    }

    return 0;
}
