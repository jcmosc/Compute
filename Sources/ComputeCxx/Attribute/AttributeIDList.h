#pragma once

#include "Data/Page.h"
#include "Node/IndirectNode.h"
#include "Node/Node.h"

namespace AG {

class AttributeIDIterator {
  private:
    data::ptr<data::page> _page;
    uint16_t _offset;

  public:
    AttributeIDIterator(data::ptr<data::page> page, uint16_t offset) : _page(page), _offset(offset) {}

    bool operator==(const AttributeIDIterator &other) const { return _page == other._page && _offset == other._offset; }
    bool operator!=(const AttributeIDIterator &other) const { return _page != other._page || _offset != other._offset; }

    AttributeID operator*() {
        assert(_page);
        return AttributeID(_page + _offset);
    }

    AttributeIDIterator &operator++() {
        assert(_page);

        AttributeID attribute_id = AttributeID(_page + _offset);
        if (attribute_id.is_direct()) {
            _offset = attribute_id.to_node().flags().relative_offset();
        } else if (attribute_id.is_indirect()) {
            _offset = attribute_id.to_indirect_node().relative_offset();
        } else {
            _page = nullptr;
            _offset = 0;
        }
        return *this;
    }
};

class AttributeIDList {
  public:
    virtual AttributeIDIterator begin();
    virtual AttributeIDIterator end();
};

class AttributeIDList1: public AttributeIDList {
  private:
    data::ptr<data::page> _page;

  public:
    AttributeIDList1(data::ptr<data::page> page) : _page(page) {}

    AttributeIDIterator begin() { return AttributeIDIterator(_page, _page->first_child_1); }
    AttributeIDIterator end() { return AttributeIDIterator(nullptr, 0); }
};

class AttributeIDList2: public AttributeIDList {
  private:
    data::ptr<data::page> _page;

  public:
    AttributeIDList2(data::ptr<data::page> page) : _page(page) {}

    AttributeIDIterator begin() { return AttributeIDIterator(_page, _page->first_child_2); }
    AttributeIDIterator end() { return AttributeIDIterator(nullptr, 0); }
};

} // namespace AG
