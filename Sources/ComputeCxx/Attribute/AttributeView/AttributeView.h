#pragma once

#include "Attribute/AttributeData/Node/IndirectNode.h"
#include "Attribute/AttributeData/Node/Node.h"
#include "Data/Page.h"

namespace AG {

class attribute_iterator {
  private:
    data::ptr<data::page> _page;
    RelativeAttributeID _next;

  public:
    attribute_iterator(data::ptr<data::page> page, RelativeAttributeID next) : _page(page), _next(next) {}

    bool operator==(const attribute_iterator &other) const { return _page == other._page && _next == other._next; }
    bool operator!=(const attribute_iterator &other) const { return _page != other._page || _next != other._next; }

    AttributeID operator*() {
        assert(_page);
        return _next.resolve(_page);
    }

    attribute_iterator &operator++() {
        assert(_page);

        AttributeID attribute_id = _next.resolve(_page);
        if (auto node = attribute_id.get_node()) {
            _next = node->next_attribute();
        } else if (auto indirect_node = attribute_id.get_indirect_node()) {
            _next = indirect_node->next_attribute();
        } else {
            _next = RelativeAttributeID(nullptr); // end
        }
        return *this;
    }
};

class attribute_view {
  private:
    data::ptr<data::page> _page;

  public:
    attribute_view(data::ptr<data::page> page) : _page(page) {};

    attribute_iterator begin() { return attribute_iterator(_page, RelativeAttributeID(_page->bytes_list)); };
    attribute_iterator end() { return attribute_iterator(_page, RelativeAttributeID(nullptr)); };
};

class const_attribute_view {
  private:
    data::ptr<data::page> _page;

  public:
    const_attribute_view(data::ptr<data::page> page) : _page(page) {};

    attribute_iterator begin() { return attribute_iterator(_page, RelativeAttributeID(_page->const_bytes_list)); };
    attribute_iterator end() { return attribute_iterator(_page, RelativeAttributeID(nullptr)); };
};

} // namespace AG
