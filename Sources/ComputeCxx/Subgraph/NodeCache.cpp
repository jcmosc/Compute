#include "NodeCache.h"

#include "Attribute/AttributeType.h"
#include "Swift/EquatableSupport.h"

namespace AG {

Subgraph::NodeCache::NodeCache() noexcept
    : _heap(nullptr, 0, 0), _types(nullptr, nullptr, nullptr, nullptr, &_heap),
      _table2([](const ItemKey *item) -> uint64_t { return item->field_0x00 >> 8; },
              [](const ItemKey *a, const ItemKey *b) -> bool {
                  if (a == b) {
                      return true;
                  }

                  if (a->equals_item != b->equals_item || (a->field_0x00 ^ b->field_0x00) > 0xff) {
                      return false;
                  }

                  void *a_body = nullptr;
                  if (a->node) {
                      data::ptr<Node> a_node = a->node;
                      auto a_type = AttributeID(a->node).subgraph()->graph()->attribute_type(a_node->type_id());
                      a_body = a_node->get_self(a_type);
                  } else {
                      a_body = a->body; // next or body?
                  }

                  void *b_body = nullptr;
                  if (b->node) {
                      data::ptr<Node> b_node = b->node;
                      auto b_type = AttributeID(b->node).subgraph()->graph()->attribute_type(b_node->type_id());
                      b_body = b_node->get_self(b_type);
                  } else {
                      b_body = b->body; // next or body?
                  }

                  return AGDispatchEquatable(a_body, b_body, a->equals_item->type, a->equals_item->equatable);
              },
              nullptr, nullptr, &_heap),
      _items(nullptr, nullptr, nullptr, nullptr, &_heap) {}

Subgraph::NodeCache::~NodeCache() noexcept {
    _items.for_each(
        [](data::ptr<Node> node, NodeCache::Item *item, void *context) {
            if (item) {
                delete item;
            }
        },
        nullptr);
}

} // namespace AG
