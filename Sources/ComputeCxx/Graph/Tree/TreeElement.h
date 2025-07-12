#pragma once

#include <stdint.h>

#include "ComputeCxx/AGTreeElement.h"
#include "Data/Pointer.h"
#include "Graph/Graph.h"
#include "TreeValue.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

namespace swift {
class metadata;
}

class Graph::TreeElementID : public data::ptr<Graph::TreeElement> {
  public:
    explicit constexpr TreeElementID() : ptr() {}
    explicit TreeElementID(data::ptr<Graph::TreeElement> tree_element) : ptr(tree_element.offset()) {};

    operator AGTreeElement() const { return offset(); }
    explicit constexpr TreeElementID(AGTreeElement storage) : ptr(storage) {}

    // MARK: Accessing graph data

    Subgraph *_Nullable subgraph() const { return reinterpret_cast<Subgraph *_Nullable>(page_ptr()->zone); }
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

CF_ASSUME_NONNULL_END
