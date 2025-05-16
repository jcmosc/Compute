#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "Pointer.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {
namespace data {

class zone;

struct page {
    zone *zone;
    ptr<page> next;
    uint32_t total;
    uint32_t in_use;

    uint16_t first_child_1;
    uint16_t first_child_2;
};
static_assert(sizeof(page) == 0x18);

class page_ptr_list {
  public:
    class iterator {
      private:
        ptr<page> _current;

      public:
        iterator(ptr<page> current) : _current(current) {}

        bool operator==(const iterator &other) const { return _current == other._current; }
        bool operator!=(const iterator &other) const { return _current != other._current; }

        ptr<page> operator*() { return _current; }

        iterator &operator++() {
            assert(_current);
            _current = _current->next;
            return *this;
        }
    };

  private:
    ptr<page> _front;

  public:
    page_ptr_list(ptr<page> front) : _front(front) {}

    iterator begin() { return iterator(_front); }
    iterator end() { return iterator(nullptr); }
};

} // namespace data
} // namespace AG

CF_ASSUME_NONNULL_END
