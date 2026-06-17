#include "ComputeCxx/IAGTreeValue.h"

#include "Graph/Graph.h"
#include "Subgraph/Subgraph.h"
#include "TreeElement.h"

IAGTypeID IAGTreeValueGetType(IAGTreeValue tree_value) {
    auto tree_value_id = IAG::Graph::TreeValueID(tree_value);
    return IAGTypeID(tree_value_id->type);
}

IAGAttribute IAGTreeValueGetValue(IAGTreeValue tree_value) {
    auto tree_value_id = IAG::Graph::TreeValueID(tree_value);
    return IAGAttribute(tree_value_id->value);
}

const char *IAGTreeValueGetKey(IAGTreeValue tree_value) {
    auto tree_value_id = IAG::Graph::TreeValueID(tree_value);
    auto key_id = tree_value_id->key_id;
    return tree_value_id.subgraph()->graph()->key_name(key_id);
}

uint32_t IAGTreeValueGetFlags(IAGTreeValue tree_value) {
    auto tree_value_id = IAG::Graph::TreeValueID(tree_value);
    return tree_value_id->flags;
}
