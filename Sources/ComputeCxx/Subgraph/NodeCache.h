#pragma once

#include <CoreFoundation/CFBase.h>

#include "Subgraph.h"
#include "Swift/Metadata.h"
#include "Utilities/HashTable.h"
#include "Utilities/Heap.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Subgraph::NodeCache {
  public:
    struct Item;
    struct Type {
        const swift::metadata *type;
        const swift::equatable_witness_table *equatable;

        // doubly-linked list
        Item *last_item;
        Item *first_item;

        uint32_t type_id;
    };
    static_assert(sizeof(Type) == 0x28);
    struct Item {
        uint64_t field_0x00;
        data::ptr<Type> equals_item;
        data::ptr<Node> node;
        Item *prev;
        Item *next;
    };
    static_assert(sizeof(Item) == 0x20);
    struct ItemKey {
        uint64_t field_0x00;
        data::ptr<Type> equals_item;
        data::ptr<Node> node;
        void *body;
    };

  private:
    os_unfair_lock _lock; // can't find if this is used
    util::Heap _heap;
    util::Table<const swift::metadata *, data::ptr<Type>> _types;
    util::Table<const ItemKey *, Item *> _table2;
    util::Table<data::ptr<Node>, Item *> _items;

  public:
    NodeCache() noexcept;
    ~NodeCache() noexcept;

    util::Table<const swift::metadata *, data::ptr<Type>> &types() { return _types; };
    util::Table<const ItemKey *, Item *> &table2() { return _table2; };
    util::Table<data::ptr<Node>, Item *> &items() { return _items; };
};

}; // namespace AG

CF_ASSUME_NONNULL_END
