#include "NodeCache.h"

#include "Swift/SwiftShims.h"

namespace AG {

Subgraph::NodeCache::NodeCache() noexcept
    : _heap(nullptr, 0, 0), _types(nullptr, nullptr, nullptr, nullptr, &_heap),
      _items([](const ItemKey *item_key) -> uint64_t { return item_key->hash_and_age >> 8; },
             [](const ItemKey *a, const ItemKey *b) -> bool {
                 if (a == b) {
                     return true;
                 }

                 if (a->type != b->type || (a->hash_and_age ^ b->hash_and_age) > 0xff) {
                     return false;
                 }

                 const void *a_body = nullptr;
                 if (a->node) {
                     data::ptr<Node> a_node = a->node;
                     auto a_attribute_type = AttributeID(a_node).subgraph()->graph()->attribute_type(a_node->type_id());
                     a_body = a->node->get_self(a_attribute_type);
                 } else {
                     a_body = a->body;
                 }

                 const void *b_body = nullptr;
                 if (b->node) {
                     data::ptr<Node> b_node = b->node;
                     auto b_attribute_type = AttributeID(b_node).subgraph()->graph()->attribute_type(b_node->type_id());
                     b_body = b->node->get_self(b_attribute_type);
                 } else {
                     b_body = b->body;
                 }

                 return AGDispatchEquatable(a_body, b_body, a->type->type, a->type->equatable);
             },
             nullptr, nullptr, &_heap),
      _items_by_node(nullptr, nullptr, nullptr, nullptr, &_heap) {}

Subgraph::NodeCache::~NodeCache() noexcept {
    _items_by_node.for_each(
        [](data::ptr<Node> node, Item *item, void *context) {
            if (item) {
                delete item;
            }
        },
        nullptr);
}

} // namespace AG
