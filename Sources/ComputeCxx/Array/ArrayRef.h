#pragma once

#include <iterator>

#include <ComputeCxx/AGBase.h>

AG_ASSUME_NONNULL_BEGIN

namespace AG {

template <typename T> class ArrayRef {
  public:
    using value_type = T;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = value_type *_Nullable;
    using const_iterator = const value_type *_Nullable;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = size_t;

  private:
    const T *_Nullable _data = nullptr;
    size_type _size = 0;

  public:
    ArrayRef() = default;
    ArrayRef(T *_Nullable data, size_t size) : _data(data), _size(size) {};

    // Element access

    reference operator[](size_type pos) { return _data[pos]; };
    const_reference operator[](size_type pos) const { return _data[pos]; };

    reference front() { return &_data[0]; };
    const_reference front() const { return *&_data[0]; };
    reference back() { return *&_data[_size - 1]; };
    const_reference back() const { return *&_data[_size - 1]; };

    const T *_Nullable data() const { return _data; };

    // Iterators

    iterator begin() { return _data; };
    iterator end() { return _data + _size; };
    const_iterator cbegin() const { return _data; };
    const_iterator cend() const { return _data + _size; };
    const_iterator begin() const { return cbegin(); };
    const_iterator end() const { return cend(); };

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

AG_ASSUME_NONNULL_END
