#pragma once

#include "Data/Page.h"
#include "Node/IndirectNode.h"
#include "Node/Node.h"

namespace AG {

class AttributeIDIterator {
  private:
    data::ptr<data::page> _page;
    RelativeAttributeID _current;

  public:
    AttributeIDIterator(data::ptr<data::page> page, RelativeAttributeID current) : _page(page), _current(current) {}

    bool operator==(const AttributeIDIterator &other) const {
        return _page == other._page && _current == other._current;
    }
    bool operator!=(const AttributeIDIterator &other) const {
        return _page != other._page || _current != other._current;
    }

    AttributeID operator*() {
        assert(_page);
        return _current.resolve(_page);
    }

    AttributeIDIterator &operator++() {
        assert(_page);

        AttributeID attribute_id = _current.resolve(_page);
        if (attribute_id.is_direct()) {
            _current = attribute_id.to_node().relative_offset();
        } else if (attribute_id.is_indirect()) {
            _current = attribute_id.to_indirect_node().relative_offset();
        } else {
            _page = nullptr;
            _current = nullptr;
        }
        return *this;
    }
};

class AttributeIDList {
  public:
    virtual AttributeIDIterator begin() {};
    virtual AttributeIDIterator end() {};
};

class AttributeIDList1 : public AttributeIDList {
  private:
    data::ptr<data::page> _page;

  public:
    AttributeIDList1(data::ptr<data::page> page) : _page(page) {}

    AttributeIDIterator begin() { return AttributeIDIterator(_page, RelativeAttributeID(_page->first_child_1)); }
    AttributeIDIterator end() { return AttributeIDIterator(nullptr, nullptr); }
};

class AttributeIDList2 : public AttributeIDList {
  private:
    data::ptr<data::page> _page;

  public:
    AttributeIDList2(data::ptr<data::page> page) : _page(page) {}

    AttributeIDIterator begin() { return AttributeIDIterator(_page, RelativeAttributeID(_page->first_child_2)); }
    AttributeIDIterator end() { return AttributeIDIterator(nullptr, nullptr); }
};

} // namespace AG
