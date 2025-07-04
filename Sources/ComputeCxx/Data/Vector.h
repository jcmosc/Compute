#pragma once

#include <CoreFoundation/CFBase.h>

#include "Pointer.h"
#include "Zone.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {
namespace data {

template <typename T> class vector {
  public:
    using value_type = T;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = value_type *_Nonnull;
    using const_iterator = const value_type *_Nonnull;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = uint32_t;

  private:
    struct Metadata {
        // capacity can be calculated via 1 << capacity_exponent
        unsigned int capacity_exponent : 5 = 0;
        unsigned int size : 27 = 0;
    };

    Metadata _metadata;
    ptr<T> _data;

    void reserve_slow(zone *zone, size_type new_cap) {
        size_type new_capacity_exponent = 1;
        if (new_cap >= 2) {
            new_capacity_exponent = 32 - std::countl_zero(new_cap - 1);
        }

        size_type old_capacity = sizeof(T) * capacity();

        // alignment_mask should be 3 for OutputEdge and 0 for InputEdge, i.e. don't insert padding
        size_type alignment_mask = std::has_unique_object_representations_v<T> ? alignof(T) - 1 : 0;
        zone->realloc_bytes((ptr<void> *)&_data, old_capacity, (size_type)sizeof(T) << new_capacity_exponent,
                            alignment_mask);
        _metadata.capacity_exponent = new_capacity_exponent;
    }

  public:
    vector() {};
    ~vector() {
        for (auto i = 0; i < _metadata.size; ++i) {
            data()[i].~T();
        }
    }

    // Non-copyable

    vector(const vector &) = delete;
    vector &operator=(const vector &) = delete;

    // Move

    vector(vector &&other) noexcept;
    vector &operator=(vector &&other) noexcept;

    // Element access

    reference operator[](size_type pos) { return data()[pos]; };
    const_reference operator[](size_type pos) const { return data()[pos]; };

    reference front() { return *&data()[0]; };
    const_reference front() const { return *&data()[0]; };
    reference back() { return *&data()[_metadata.size - 1]; };
    const_reference back() const { return *&data()[_metadata.size - 1]; };

    T *_Nonnull data() { return _data.get(); };
    const T *_Nonnull data() const { return _data.get(); };

    // Iterators

    iterator begin() { return iterator(&data()[0]); };
    iterator end() { return iterator(&data()[_metadata.size]); };
    const_iterator cbegin() const { return const_iterator(&data()[0]); };
    const_iterator cend() const { return const_iterator(&data()[_metadata.size]); };
    const_iterator begin() const { return cbegin(); };
    const_iterator end() const { return cend(); };

    reverse_iterator rbegin() { return std::reverse_iterator(end()); };
    reverse_iterator rend() { return std::reverse_iterator(begin()); };
    const_reverse_iterator crbegin() const { return std::reverse_iterator(cend()); };
    const_reverse_iterator crend() const { return std::reverse_iterator(cbegin()); };
    const_reverse_iterator rbegin() const { return crbegin(); };
    const_reverse_iterator rend() const { return crend(); };

    // Capacity

    bool empty() const { return _metadata.size == 0; };
    size_type size() const { return _metadata.size; };
    void reserve(zone *zone, size_type new_cap) {
        if (new_cap <= capacity()) {
            return;
        }
        reserve_slow(zone, new_cap);
    }
    size_type capacity() const {
        if (_metadata.capacity_exponent == 0) {
            return 0;
        }
        return 1 << _metadata.capacity_exponent;
    };

    // Modifiers

    iterator insert(zone *zone, const_iterator pos, const T &value);
    iterator insert(zone *zone, const_iterator pos, T &&value);

    iterator erase(iterator pos);
    iterator erase(iterator first, iterator last);

    void push_back(zone *zone, const T &value);
    void push_back(zone *zone, T &&value);
};

template <typename T>
vector<T>::vector(vector &&other) noexcept : _metadata(other._metadata), _data(std::exchange(other._data, nullptr)) {
    other._metadata = {};
}

template <typename T> vector<T> &vector<T>::operator=(vector &&other) noexcept {
    if (this != &other) {
        for (auto i = 0; i < _metadata.size; ++i) {
            data()[i].~T();
        }
        _metadata = other._metadata;
        _data = other._data;
        other._metadata = {};
        other._data = nullptr;
    }
    return *this;
}

template <typename T> vector<T>::iterator vector<T>::insert(zone *zone, const_iterator pos, const T &value) {
    if (pos == end()) {
        push_back(zone, value);
    } else {
        reserve(zone, _metadata.size + 1);
        iterator mutable_pos = begin() + (pos - begin());
        std::move_backward(mutable_pos, end(), end() + 1);
        new (mutable_pos) value_type(value);
        _metadata.size += 1;
    }
    return end();
}

template <typename T> vector<T>::iterator vector<T>::insert(zone *zone, const_iterator pos, T &&value) {
    if (pos == end()) {
        push_back(zone, value);
    } else {
        reserve(zone, _metadata.size + 1);
        iterator mutable_pos = begin() + (pos - begin());
        std::move_backward(mutable_pos, end(), end() + 1);
        new (pos) value_type(std::move(value));
        _metadata.size += 1;
    }
    return end();
}

template <typename T> vector<T>::iterator vector<T>::erase(iterator pos) {
    if (pos == end()) {
        return end();
    }
    return erase(pos, pos + 1);
}

template <typename T> vector<T>::iterator vector<T>::erase(iterator first, iterator last) {
    auto count = last - first;
    if (count == 0) {
        return last;
    }
    for (auto iter = first; iter != last; iter++) {
        iter->~T();
    }
    for (auto iter = last, old_end = end(); iter != old_end; iter++) {
        std::swap(*(iter - count), *iter);
    }
    _metadata.size -= count;
    return end();
}

template <typename T> void vector<T>::push_back(zone *zone, const T &value) {
    reserve(zone, _metadata.size + 1);
    new (&data()[_metadata.size]) value_type(value);
    _metadata.size += 1;
}

template <typename T> void vector<T>::push_back(zone *zone, T &&value) {
    reserve(zone, _metadata.size + 1);
    new (&data()[_metadata.size]) value_type(std::move(value));
    _metadata.size += 1;
}

} // namespace data
} // namespace AG

CF_ASSUME_NONNULL_END
