#include "ComputeCxx/AGTreeElement.h"

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
    return AGTreeElement(uintptr_t(tree_element_id->parent));
}

#pragma mark - Iterating values

AGTreeElementValueIterator
AGTreeElementMakeValueIterator(AGTreeElement tree_element) {
    auto tree_element_id = AG::Graph::TreeElementID(tree_element);
    return AGTreeElementValueIterator(uintptr_t(tree_element),
                                      tree_element_id->first_value);
}

AGTreeValue AGTreeElementGetNextValue(AGTreeElementValueIterator iter) {
    AGTreeValue next_value = reinterpret_cast<AGTreeValue>(iter.next_elt);
    if (next_value) {
        auto next_value_id = AG::Graph::TreeValueID(next_value);
        iter.next_elt = next_value_id->next;
    }
    return next_value;
}

#pragma mark - Iterating nodes

AGTreeElementNodeIterator
AGTreeElementMakeNodeIterator(AGTreeElement tree_element) {
    return {uintptr_t(tree_element), 0};
}

AGAttribute AGTreeElementGetNextNode(AGTreeElementNodeIterator *iter) {
    AGTreeElement tree_element = reinterpret_cast<AGTreeElement>(iter->elt);
    auto tree_element_id = AG::Graph::TreeElementID(tree_element);
    AG::AttributeID node = tree_element_id.subgraph()->tree_node_at_index(
        tree_element_id, iter->node_index);
    if (!node || node.is_nil()) {
        return AGAttributeNil;
    }
    iter->node_index += 1;
    return AGAttribute(node);
}

#pragma mark - Iterating children

AGTreeElementChildIterator
AGTreeElementMakeChildIterator(AGTreeElement tree_element) {
    auto tree_element_id = AG::Graph::TreeElementID(tree_element);
    auto child = tree_element_id->first_child;
    return AGTreeElementChildIterator((uintptr_t)tree_element, child, 0);
}

AGTreeElement AGTreeElementGetNextChild(AGTreeElementChildIterator *iter) {
    AGTreeElement next_child = reinterpret_cast<AGTreeElement>(iter->next_elt);
    if (next_child) {
        iter->next_elt = AG::Graph::TreeElementID(next_child)->next_sibling;
        return next_child;
    }

    if (!iter->subgraph_index) {
        iter->subgraph_index = true; // +1 or set to true?
        auto tree_element = reinterpret_cast<AGTreeElement>(iter->parent_elt);
        auto tree_element_id = AG::Graph::TreeElementID(tree_element);
        auto subgraph = tree_element_id.subgraph();
        auto subgraph_child = subgraph->tree_subgraph_child(tree_element_id);
        if (subgraph_child) {
            iter->next_elt = subgraph_child->next_sibling;
            return AGTreeElement((uintptr_t)subgraph_child);
        }
    }

    return 0;
}
