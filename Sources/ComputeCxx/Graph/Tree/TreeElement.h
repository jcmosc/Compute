#pragma once

#include <stdint.h>

#include "Data/Pointer.h"
#include "Graph/Graph.h"
#include "Swift/Metadata.h"
#include "Vector/Vector.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

struct Graph::TreeElement {
    const swift::metadata *type;

    AttributeID node; // TODO: check named value from AGSubgraphBeginTreeElement
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

    explicit constexpr TreeElementID(uint32_t value) : _value(value) {};

  public:
    explicit TreeElementID(data::ptr<Graph::TreeElement> tree_element) : _value(tree_element.offset()) {};

    constexpr uint32_t to_storage() const { return _value; }
    static constexpr TreeElementID from_storage(uint32_t value) { return TreeElementID(value); }

    data::ptr<Graph::TreeElement> to_ptr() const { return data::ptr<Graph::TreeElement>(_value); };

    Subgraph *_Nullable subgraph() const { return reinterpret_cast<Subgraph *_Nullable>(page_ptr()->zone); }
    data::ptr<data::page> page_ptr() const { return data::ptr<void>(_value).page_ptr(); };

    bool has_value() const { return _value != 0; }
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

    explicit constexpr TreeValueID(uint32_t value) : _value(value) {};

  public:
    explicit TreeValueID(data::ptr<Graph::TreeValue> tree_value) : _value(tree_value.offset()) {};

    constexpr uint32_t to_storage() const { return _value; }
    static constexpr TreeValueID from_storage(uint32_t value) { return TreeValueID(value); }

    const Graph::TreeValue &to_tree_value() const { return *data::ptr<Graph::TreeValue>(_value); };

    Subgraph *_Nullable subgraph() const { return reinterpret_cast<Subgraph *_Nullable>(page_ptr()->zone); }
    data::ptr<data::page> page_ptr() const { return data::ptr<void>(_value).page_ptr(); };
};

} // namespace AG

CF_ASSUME_NONNULL_END
