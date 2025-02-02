#pragma once

#include <CoreFoundation/CFBase.h>
#include <cassert>
#include <stdint.h>
#include <type_traits>

#include "Constants.h"
#include "Table.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {
namespace data {

struct page;

template <typename T> class ptr {
  public:
    using element_type = T;
    using difference_type = uint32_t;

  private:
    difference_type _offset;

    template <typename U> friend class ptr;

  public:
    ptr(difference_type offset = 0) : _offset(offset){};
    ptr(nullptr_t){};

    void assert_valid() const {
        if (_offset >= table::shared().ptr_max_offset()) {
            precondition_failure("invalid data offset: %u", _offset);
        }
    }

    element_type *_Nonnull get() const noexcept {
        assert(_offset != 0);
        return reinterpret_cast<element_type *>(table::shared().ptr_base() + _offset);
    }

    ptr<page> page_ptr() const noexcept { return ptr<page>(_offset & ~page_alignment_mask); }

    difference_type offset() const noexcept { return _offset; }
    difference_type offset_from_page() const noexcept { return _offset & page_alignment_mask; }

    template <typename U> ptr<U> aligned(difference_type alignment_mask = sizeof(difference_type) - 1) const {
        return ptr<U>((_offset + alignment_mask) & ~alignment_mask);
    };

    operator bool() const noexcept { return _offset != 0; };
    std::add_lvalue_reference_t<T> operator*() const noexcept { return *get(); };
    T *_Nonnull operator->() const noexcept { return get(); };

    bool operator==(const ptr<T> &other) const noexcept { return _offset == other._offset; };
    bool operator!=(const ptr<T> &other) const noexcept { return _offset != other._offset; };
    bool operator==(nullptr_t) const noexcept { return _offset == 0; };
    bool operator!=(nullptr_t) const noexcept { return _offset != 0; };

    bool operator<(difference_type offset) const noexcept { return _offset < offset; };
    bool operator<=(difference_type offset) const noexcept { return _offset <= offset; };
    bool operator>(difference_type offset) const noexcept { return _offset > offset; };
    bool operator>=(difference_type offset) const noexcept { return _offset >= offset; };

    template <typename U> ptr<U> operator+(difference_type shift) const noexcept { return ptr(_offset + shift); };
    template <typename U> ptr<U> operator-(difference_type shift) const noexcept { return ptr(_offset - shift); };

    template <typename U> difference_type operator-(const ptr<U> &other) const noexcept {
        return _offset - other._offset;
    };
};

} // namespace data
} // namespace AG

CF_ASSUME_NONNULL_END
