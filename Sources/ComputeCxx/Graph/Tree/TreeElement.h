#pragma once

#include "ComputeCxx/AGBase.h"
#include "ComputeCxx/AGTreeElement.h"
#include "Data/Pointer.h"
#include "Graph/Graph.h"
#include "TreeValue.h"

AG_ASSUME_NONNULL_BEGIN

namespace AG {

namespace swift {
class metadata;
}

class Graph::TreeElementID : public data::ptr<Graph::TreeElement> {
  public:
    explicit constexpr TreeElementID(std::nullptr_t = nullptr) : ptr(nullptr) {}
    explicit TreeElementID(data::ptr<Graph::TreeElement> tree_element)
        : ptr(tree_element.offset()) {};

    operator uintptr_t() const { return offset(); }
    explicit TreeElementID(AGTreeElement storage)
        : ptr((uint32_t)(uint64_t)storage) {}

    // MARK: Accessing graph data

    Subgraph *_Nullable subgraph() const {
        return reinterpret_cast<Subgraph *_Nullable>(page_ptr()->zone);
    }
};

struct Graph::TreeElement {
    const swift::metadata *_Nullable type;

    AttributeID value;
    uint32_t flags;

    Graph::TreeElementID parent;
    Graph::TreeElementID first_child;
    Graph::TreeElementID next_sibling;

    Graph::TreeValueID first_value;
};
static_assert(sizeof(Graph::TreeElement) == 0x20);

} // namespace AG

AG_ASSUME_NONNULL_END
