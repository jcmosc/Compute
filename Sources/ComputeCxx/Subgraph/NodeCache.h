#pragma once

#include <ComputeCxx/AGBase.h>
#include <Utilities/HashTable.h>
#include <Utilities/Heap.h>
#include <platform/platform.h>

#include "Subgraph.h"

AG_ASSUME_NONNULL_BEGIN

namespace AG {

class Subgraph::NodeCache {
  public:
    struct Item;
    struct Type {
        const swift::metadata *type;
        const swift::equatable_witness_table *equatable;
        Item *mru;
        Item *lru;
        uint32_t type_id;
    };
    static_assert(sizeof(Type) == 40);
    struct Item {
        uint64_t hash_and_age;
        data::ptr<Type> type;
        data::ptr<Node> node;
        Item *next;
        Item *prev;
        uint8_t age() { return hash_and_age & 0xff; }
        void increment_age() { hash_and_age += 1; }
        void reset_age() { hash_and_age &= 0xffffffffffffff00; }
    };
    static_assert(sizeof(Item) == 32);
    struct ItemKey {
        uint64_t hash_and_age; // age bits are ignored
        data::ptr<Type> type;
        data::ptr<Node> node;
        const void *_Nullable body;
    };

  private:
    platform_lock _lock; // can't find how this is used..
    util::Heap _heap;
    util::Table<const swift::metadata *, data::ptr<Type>> _types;
    util::Table<const ItemKey *, Item *> _items;

    // allocated items
    util::Table<data::ptr<Node>, Item *> _items_by_node;

  public:
    NodeCache() noexcept;
    ~NodeCache() noexcept;

    // non-copyable
    NodeCache(const NodeCache &) = delete;
    NodeCache &operator=(const NodeCache &) = delete;

    // non-movable
    NodeCache(NodeCache &&) = delete;
    NodeCache &operator=(NodeCache &&) = delete;

    util::Table<const swift::metadata *, data::ptr<Type>> &types() { return _types; };
    util::Table<const ItemKey *, Item *> &items() { return _items; };
    util::Table<data::ptr<Node>, Item *> &items_by_node() { return _items_by_node; };
};

} // namespace AG

AG_ASSUME_NONNULL_END
