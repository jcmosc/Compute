#include "Pointer.h"

#include "Page.h"
#include "Table.h"

namespace AG {
namespace data {

template <typename T> void ptr<T>::assert_valid() const {
    if (_offset >= table::shared().ptr_max_offset()) {
        precondition_failure("invalid data offset: %u", _offset);
    }
}

template <typename T> ptr<T>::element_type *_Nonnull ptr<T>::get() const noexcept {
    assert(_offset != 0);
    return table::shared().ptr_base() + _offset;
};

template <typename T> ptr<page> ptr<T>::page_ptr() const noexcept { return ptr<page>(_offset & ~page_alignment_mask); };

template <typename T> ptr<T>::difference_type ptr<T>::page_relative_offset() const noexcept {
    return _offset & page_alignment_mask;
}

} // namespace data
} // namespace AG
