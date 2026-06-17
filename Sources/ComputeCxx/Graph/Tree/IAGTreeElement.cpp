#include "ComputeCxx/IAGTreeElement.h"

#include "Subgraph/Subgraph.h"
#include "TreeElement.h"

IAGTypeID IAGTreeElementGetType(IAGTreeElement tree_element) {
    auto tree_element_id = IAG::Graph::TreeElementID(tree_element);
    auto type = tree_element_id->type;
    return IAGTypeID(type);
}

IAGAttribute IAGTreeElementGetValue(IAGTreeElement tree_element) {
    auto tree_element_id = IAG::Graph::TreeElementID(tree_element);
    auto value = tree_element_id->value;
    return IAGAttribute(value);
}

uint32_t IAGTreeElementGetFlags(IAGTreeElement tree_element) {
    auto tree_element_id = IAG::Graph::TreeElementID(tree_element);
    return tree_element_id->flags;
}

IAGTreeElement IAGTreeElementGetParent(IAGTreeElement tree_element) {
    auto tree_element_id = IAG::Graph::TreeElementID(tree_element);
    return IAGTreeElement(uintptr_t(tree_element_id->parent));
}

#pragma mark - Iterating values

IAGTreeElementValueIterator
IAGTreeElementMakeValueIterator(IAGTreeElement tree_element) {
    auto tree_element_id = IAG::Graph::TreeElementID(tree_element);
    return IAGTreeElementValueIterator(uintptr_t(tree_element),
                                      tree_element_id->first_value);
}

IAGTreeValue IAGTreeElementGetNextValue(IAGTreeElementValueIterator *iter) {
    IAGTreeValue next_value = reinterpret_cast<IAGTreeValue>(iter->next_elt);
    if (next_value) {
        auto next_value_id = IAG::Graph::TreeValueID(next_value);
        iter->next_elt = next_value_id->next;
    }
    return next_value;
}

#pragma mark - Iterating nodes

IAGTreeElementNodeIterator
IAGTreeElementMakeNodeIterator(IAGTreeElement tree_element) {
    return {uintptr_t(tree_element), 0};
}

IAGAttribute IAGTreeElementGetNextNode(IAGTreeElementNodeIterator *iter) {
    IAGTreeElement tree_element = reinterpret_cast<IAGTreeElement>(iter->elt);
    auto tree_element_id = IAG::Graph::TreeElementID(tree_element);
    IAG::AttributeID node = tree_element_id.subgraph()->tree_node_at_index(
        tree_element_id, iter->node_index);
    if (!node || node.is_nil()) {
        return IAGAttributeNil;
    }
    iter->node_index += 1;
    return IAGAttribute(node);
}

#pragma mark - Iterating children

IAGTreeElementChildIterator
IAGTreeElementMakeChildIterator(IAGTreeElement tree_element) {
    auto tree_element_id = IAG::Graph::TreeElementID(tree_element);
    auto child = tree_element_id->first_child;
    return IAGTreeElementChildIterator((uintptr_t)tree_element, child, 0);
}

IAGTreeElement IAGTreeElementGetNextChild(IAGTreeElementChildIterator *iter) {
    IAGTreeElement next_child = reinterpret_cast<IAGTreeElement>(iter->next_elt);
    if (next_child) {
        iter->next_elt = IAG::Graph::TreeElementID(next_child)->next_sibling;
        return next_child;
    }

    if (!iter->subgraph_index) {
        iter->subgraph_index = true; // +1 or set to true?
        auto tree_element = reinterpret_cast<IAGTreeElement>(iter->parent_elt);
        auto tree_element_id = IAG::Graph::TreeElementID(tree_element);
        auto subgraph = tree_element_id.subgraph();
        auto subgraph_child = subgraph->tree_subgraph_child(tree_element_id);
        if (subgraph_child) {
            iter->next_elt = subgraph_child->next_sibling;
            return IAGTreeElement((uintptr_t)subgraph_child);
        }
    }

    return 0;
}
