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

    AttributeID node;
    uint32_t flags;

    data::ptr<Graph::TreeElement> parent;
    data::ptr<Graph::TreeElement> first_child;
    data::ptr<Graph::TreeElement> next_sibling;
    
    data::ptr<TreeValue> first_value;
};
static_assert(sizeof(Graph::TreeElement) == 0x20);

class TreeElementID {
  private:
    uint32_t _value;

  public:
    TreeElementID(uint32_t value) : _value(value){};

    data::ptr<Graph::TreeElement> to_element_ptr() const { return data::ptr<Graph::TreeElement>(_value); };

    Subgraph *_Nullable subgraph() const { return reinterpret_cast<Subgraph *_Nullable>(page_ptr()->zone); }
    data::ptr<data::page> page_ptr() const { return data::ptr<void>(_value).page_ptr(); };
};

struct Graph::TreeValue {
    const swift::metadata *_Nonnull type;
    AttributeID value;
    uint32_t key_id;
    uint32_t flags;
    data::ptr<TreeValue> next;
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
