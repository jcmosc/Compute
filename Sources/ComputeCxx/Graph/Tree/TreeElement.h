#pragma once

#include <stdint.h>

#include "Containers/Vector.h"
#include "Data/Pointer.h"
#include "Graph/Graph.h"
#include "Swift/Metadata.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

struct Graph::TreeElement {
    const swift::metadata *type;

    AttributeID owner;
    uint32_t flags;

    data::ptr<Graph::TreeElement> parent;
    data::ptr<Graph::TreeElement> next;
    data::ptr<Graph::TreeElement> old_parent;
    data::ptr<TreeValue> last_value;
};
static_assert(sizeof(Graph::TreeElement) == 0x20);

struct Graph::TreeValue {
    const swift::metadata *_Nonnull type;
    AttributeID value;
    uint32_t key_id;
    uint32_t flags;
    data::ptr<TreeValue> previous_sibling;
};
static_assert(sizeof(Graph::TreeValue) == 0x18);

class TreeValueID {
  private:
    uint32_t _value;

  public:
    TreeValueID(uint32_t value) : _value(value){};

    const Graph::TreeValue &to_tree_value() const { return *data::ptr<Graph::TreeValue>(_value); };

    Subgraph *_Nullable subgraph() const { return reinterpret_cast<Subgraph *_Nullable>(page_ptr()->zone); }
    data::ptr<data::page> page_ptr() const { return data::ptr<void>(_value).page_ptr(); };
};

} // namespace AG

CF_ASSUME_NONNULL_END
