#include "TreeElement.h"

namespace AG {

void Graph::TreeDataElement::sort_nodes() {
    if (!_sorted) {
        std::sort(
            _nodes.begin(), _nodes.end(), [](const TreeElementNodePair &a, const TreeElementNodePair &b) -> bool {
                return a.first != b.first ? a.first.offset() < b.first.offset() : a.second.offset() < b.second.offset();
            });
        _sorted = true;
    }
};

}; // namespace AG
