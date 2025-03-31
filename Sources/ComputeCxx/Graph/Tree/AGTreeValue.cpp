#include "AGTreeValue.h"

#include "Graph/Graph.h"
#include "Subgraph/Subgraph.h"
#include "TreeElement.h"

AGTypeID AGTreeValueGetType(AGTreeValue tree_value) {
    auto tree_value_id = AG::TreeValueID(tree_value);
    return AGTypeID(tree_value_id.to_tree_value().type);
}

AGAttribute AGTreeValueGetValue(AGTreeValue tree_value) {
    auto tree_value_id = AG::TreeValueID(tree_value);
    return AGAttribute(tree_value_id.to_tree_value().value);
}

const char *AGTreeValueGetKey(AGTreeValue tree_value) {
    auto tree_value_id = AG::TreeValueID(tree_value);
    auto key_id = tree_value_id.to_tree_value().value;
    return tree_value_id.subgraph()->graph()->key_name(key_id);
}

uint32_t AGTreeValueGetFlags(AGTreeValue tree_value) {
    auto tree_value_id = AG::TreeValueID(tree_value);
    return tree_value_id.to_tree_value().flags;
}
