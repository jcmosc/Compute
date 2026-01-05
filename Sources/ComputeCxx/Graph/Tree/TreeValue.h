#pragma once

#include "ComputeCxx/AGBase.h"
#include "ComputeCxx/AGTreeValue.h"
#include "Data/Pointer.h"
#include "Graph/Graph.h"

AG_ASSUME_NONNULL_BEGIN

namespace AG {

namespace swift {
class metadata;
}

class Graph::TreeValueID : public data::ptr<Graph::TreeValue> {
  public:
    explicit constexpr TreeValueID() : ptr() {}
    explicit TreeValueID(data::ptr<Graph::TreeValue> tree_value)
        : ptr(tree_value.offset()) {};

    operator uintptr_t() const { return offset(); }
    explicit TreeValueID(AGTreeValue storage)
        : ptr((uint32_t)(uint64_t)storage) {}

    // MARK: Accessing graph data

    Subgraph *_Nullable subgraph() const {
        return reinterpret_cast<Subgraph *_Nullable>(page_ptr()->zone);
    }
};

struct Graph::TreeValue {
    const swift::metadata *_Nonnull type;
    AttributeID value;
    uint32_t key_id;
    uint32_t flags;
    TreeValueID next;
};
static_assert(sizeof(Graph::TreeValue) == 0x18);

} // namespace AG

AG_ASSUME_NONNULL_END
