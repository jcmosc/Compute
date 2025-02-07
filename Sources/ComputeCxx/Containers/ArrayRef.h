#pragma once

#include <CoreFoundation/CFBase.h>

CF_ASSUME_NONNULL_BEGIN

namespace AG {

template <typename T> class ArrayRef {
  public:
    using value_type = T;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

  private:
    const T *_Nullable _data = nullptr;
    size_type _size = 0;

  public:
    ArrayRef() = default;
    ArrayRef(T *_Nullable data, size_t size) : _data(data), _size(size){};

    // TODO: see if this is really needed, or disable operator new in data::table managed objects...
    inline void *operator new(std::size_t n, const ArrayRef<T> *_Nonnull ptr) { return (void *)ptr; };

    // Element access

    const T &operator[](size_t pos) const {
        assert(pos < _size);
        return _data[pos];
    };

    const T &front() const {
        assert(!empty());
        return _data[0];
    };
    const T &back() const {
        assert(!empty());
        return _data[_size - 1];
    };

    const T *_Nonnull data() const { return _data; };

    // Iterators

    iterator _Nullable begin() { return _data; };
    iterator _Nullable end() { return _data + _size; };
    const_iterator _Nullable cbegin() const { return _data; };
    const_iterator _Nullable cend() const { return _data + _size; };
    const_iterator _Nullable begin() const { return cbegin(); };
    const_iterator _Nullable end() const { return cend(); };

    reverse_iterator rbegin() { return std::reverse_iterator(end()); };
    reverse_iterator rend() { return std::reverse_iterator(begin()); };
    const_reverse_iterator crbegin() const { return std::reverse_iterator(cend()); };
    const_reverse_iterator crend() const { return std::reverse_iterator(cbegin()); };
    const_reverse_iterator rbegin() const { return crbegin(); };
    const_reverse_iterator rend() const { return crend(); };

    // Capacity

    bool empty() const { return _size == 0; };
    size_type size() const { return _size; };
};

} // namespace AG

CF_ASSUME_NONNULL_END
