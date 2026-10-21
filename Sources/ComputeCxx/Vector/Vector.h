#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstring>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#include <platform/malloc.h>

#include "ComputeCxx/IAGBase.h"
#include "Errors/Errors.h"

IAG_ASSUME_NONNULL_BEGIN

namespace IAG {

namespace details {

template <typename T, unsigned int capacity>
struct vector_storage {
    alignas(T) char _bytes[sizeof(T) * capacity];

    T *_Nonnull data() { return reinterpret_cast<T *>(_bytes); };
    const T *_Nonnull data() const { return reinterpret_cast<const T *>(_bytes); };
};

} // namespace details

template <typename T, unsigned int _inline_capacity = 0, typename _size_type = std::size_t>
    requires std::unsigned_integral<_size_type>
class vector {
  public:
    using value_type = T;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = value_type *_Nonnull;
    using const_iterator = const value_type *_Nonnull;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = _size_type;

  private:
    // Elements are constructed and destroyed explicitly. After relocation to
    // the heap, the inline storage must not destroy the old element copies.
    details::vector_storage<T, _inline_capacity> _inline_buffer;
    T *_Nullable _buffer = nullptr;
    size_type _size = 0;
    size_type _capacity = _inline_capacity;

    void reserve_slow(size_type new_cap);

  public:
    vector() {};
    ~vector();

    // Non-copyable

    vector(const vector &) = delete;
    vector &operator=(const vector &) = delete;

    // Move

    vector(vector &&) noexcept;
    vector &operator=(vector &&) noexcept;

    // Element access

    reference operator[](size_type pos) { return data()[pos]; };
    const_reference operator[](size_type pos) const { return data()[pos]; };

    reference front() { return data()[0]; };
    const_reference front() const { return data()[0]; };
    reference back() { return data()[_size - 1]; };
    const_reference back() const { return data()[_size - 1]; };

    T *_Nonnull data() { return _buffer != nullptr ? _buffer : _inline_buffer.data(); };
    const T *_Nonnull data() const { return _buffer != nullptr ? _buffer : _inline_buffer.data(); };

    // Iterators

    iterator begin() { return iterator(&data()[0]); };
    iterator end() { return iterator(&data()[_size]); };
    const_iterator cbegin() const { return const_iterator(&data()[0]); };
    const_iterator cend() const { return const_iterator(&data()[_size]); };
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
    void reserve(size_type new_cap);
    size_type capacity() const { return _capacity; };
    void shrink_to_fit();

    // Modifiers

    void clear();

    iterator insert(const_iterator pos, const T &value);
    iterator insert(const_iterator pos, T &&value);

    iterator erase(iterator pos);
    iterator erase(iterator first, iterator last);

    void push_back(const T &value);
    void push_back(T &&value);
    void pop_back();

    template <typename... Args>
    reference emplace_back(Args &&...args);

    void resize(size_type count);
    void resize(size_type count, const value_type &value);
};

static_assert(std::contiguous_iterator<vector<int>::iterator>);
static_assert(std::contiguous_iterator<vector<int>::const_iterator>);

namespace details {

// Relocates elements with memcpy, so only valid for trivially copyable elements.
template <typename size_type, unsigned int element_size_bytes>
    requires std::unsigned_integral<size_type>
void *_Nullable realloc_vector(void *_Nullable buffer, void *_Nonnull _inline_buffer, size_type inline_capacity,
                               size_type *_Nonnull size, size_type preferred_new_size) {
    // copy data from heap buffer into inline buffer if possible
    if (preferred_new_size <= inline_capacity) {
        if (buffer) {
            memcpy(_inline_buffer, buffer, preferred_new_size * element_size_bytes);
            free(buffer);
            *size = inline_capacity;
        }
        return nullptr;
    }

    size_t new_size_bytes = malloc_good_size(preferred_new_size * element_size_bytes);
    size_type new_size = (size_type)(new_size_bytes / element_size_bytes);
    if (new_size == *size) {
        // nothing to do
        return buffer;
    }

    void *new_buffer = realloc(buffer, new_size_bytes);
    if (!new_buffer) {
        precondition_failure("allocation failure");
    }

    // copy data from inline buffer into heap buffer
    if (!buffer) {
        memcpy(new_buffer, _inline_buffer, (*size) * element_size_bytes);
    }

    *size = new_size;
    return new_buffer;
}

// Relocates elements with realloc, so only valid for trivially copyable elements, or when buffer is null.
template <typename size_type, size_t element_size>
    requires std::unsigned_integral<size_type>
void *_Nullable realloc_vector(void *_Nullable buffer, size_type *_Nonnull size, size_type preferred_new_size) {
    if (preferred_new_size == 0) {
        *size = 0;
        free(buffer);
        return nullptr;
    }

    size_t new_size_bytes = malloc_good_size(preferred_new_size * element_size);
    size_type new_size = (size_type)(new_size_bytes / element_size);
    if (new_size == *size) {
        // nothing to do
        return buffer;
    }

    void *new_buffer = realloc(buffer, new_size_bytes);
    if (!new_buffer) {
        precondition_failure("allocation failure");
    }
    *size = new_size;
    return new_buffer;
}

} // namespace details

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, _inline_capacity, size_type>::~vector() {
    std::destroy(begin(), end());
    if (_buffer) {
        free((void *)_buffer);
    }
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, _inline_capacity, size_type>::vector(vector &&other) noexcept {
    if (other._buffer) {
        _buffer = std::exchange(other._buffer, nullptr);
        _capacity = std::exchange(other._capacity, _inline_capacity);
    } else {
        std::uninitialized_move(other.begin(), other.end(), _inline_buffer.data());
        std::destroy(other.begin(), other.end());
    }
    _size = std::exchange(other._size, 0);
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, _inline_capacity, size_type> &vector<T, _inline_capacity, size_type>::operator=(vector &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    std::destroy(begin(), end());
    if (other._buffer) {
        // steal the heap buffer
        if (_buffer) {
            free((void *)_buffer);
        }
        _buffer = std::exchange(other._buffer, nullptr);
        _capacity = std::exchange(other._capacity, _inline_capacity);
    } else {
        // inline elements always fit in our current storage
        std::uninitialized_move(other.begin(), other.end(), begin());
        std::destroy(other.begin(), other.end());
    }
    _size = std::exchange(other._size, 0);
    return *this;
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _inline_capacity, size_type>::reserve_slow(size_type new_cap) {
    constexpr size_t max_capacity = std::numeric_limits<size_type>::max();
    if (_capacity == max_capacity) {
        precondition_failure("vector capacity overflow");
    }
    size_type effective_new_cap = (size_type)std::clamp(2 * size_t(_capacity) + 1, size_t(new_cap), max_capacity);

    if constexpr (std::is_trivially_copyable_v<T>) {
        _buffer = reinterpret_cast<T *>(details::realloc_vector<size_type, sizeof(T)>(
            (void *)_buffer, (void *)_inline_buffer.data(), _inline_capacity, &_capacity, effective_new_cap));
    } else {
        size_type new_capacity = 0;
        T *new_buffer = reinterpret_cast<T *>(
            details::realloc_vector<size_type, sizeof(T)>(nullptr, &new_capacity, effective_new_cap));
        std::uninitialized_move(begin(), end(), new_buffer);
        std::destroy(begin(), end());
        if (_buffer) {
            free((void *)_buffer);
        }
        _buffer = new_buffer;
        _capacity = new_capacity;
    }
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _inline_capacity, size_type>::reserve(size_type new_cap) {
    if (new_cap <= capacity()) {
        return;
    }
    reserve_slow(new_cap);
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _inline_capacity, size_type>::shrink_to_fit() {
    if (!_buffer || capacity() == _size) {
        return;
    }

    if constexpr (std::is_trivially_copyable_v<T>) {
        _buffer = reinterpret_cast<T *>(details::realloc_vector<size_type, sizeof(T)>(
            _buffer, _inline_buffer.data(), _inline_capacity, &_capacity, _size));
    } else {
        T *new_buffer = nullptr;
        size_type new_capacity = _inline_capacity;
        if (_size > _inline_capacity) {
            new_capacity = 0;
            new_buffer =
                reinterpret_cast<T *>(details::realloc_vector<size_type, sizeof(T)>(nullptr, &new_capacity, _size));
        }
        std::uninitialized_move(begin(), end(), new_buffer ? new_buffer : _inline_buffer.data());
        std::destroy(begin(), end());
        free((void *)_buffer);
        _buffer = new_buffer;
        _capacity = new_capacity;
    }
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _inline_capacity, size_type>::clear() {
    std::destroy(begin(), end());
    _size = 0;
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, _inline_capacity, size_type>::iterator vector<T, _inline_capacity, size_type>::insert(const_iterator pos,
                                                                                                const T &value) {
    if (pos == end()) {
        push_back(value);
        return end() - 1;
    }

    // value may refer to an element of this vector
    auto offset = pos - begin();
    const T *value_ptr = &value;
    if (_size >= _capacity) {
        if (!std::less<>()(value_ptr, begin()) && std::less<>()(value_ptr, end())) {
            auto value_offset = value_ptr - begin();
            reserve(_size + 1);
            value_ptr = begin() + value_offset;
        } else {
            reserve(_size + 1);
        }
    }
    iterator mutable_pos = begin() + offset;

    new (end()) value_type(std::move(back()));
    std::move_backward(mutable_pos, end() - 1, end());
    _size += 1;

    if (!std::less<>()(value_ptr, mutable_pos) && std::less<>()(value_ptr, end())) {
        ++value_ptr;
    }
    *mutable_pos = *value_ptr;
    return mutable_pos;
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, _inline_capacity, size_type>::iterator vector<T, _inline_capacity, size_type>::insert(const_iterator pos,
                                                                                                T &&value) {
    if (pos == end()) {
        push_back(std::move(value));
        return end() - 1;
    }

    // value may refer to an element of this vector
    auto offset = pos - begin();
    T *value_ptr = &value;
    if (_size >= _capacity) {
        if (!std::less<>()(value_ptr, begin()) && std::less<>()(value_ptr, end())) {
            auto value_offset = value_ptr - begin();
            reserve(_size + 1);
            value_ptr = begin() + value_offset;
        } else {
            reserve(_size + 1);
        }
    }
    iterator mutable_pos = begin() + offset;

    new (end()) value_type(std::move(back()));
    std::move_backward(mutable_pos, end() - 1, end());
    _size += 1;

    if (!std::less<>()(value_ptr, mutable_pos) && std::less<>()(value_ptr, end())) {
        ++value_ptr;
    }
    *mutable_pos = std::move(*value_ptr);
    return mutable_pos;
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, _inline_capacity, size_type>::iterator vector<T, _inline_capacity, size_type>::erase(iterator pos) {
    if (pos == end()) {
        return end();
    }
    return erase(pos, pos + 1);
}

// Unlike std::vector, returns the new end() rather than the iterator following the erased elements.
template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, _inline_capacity, size_type>::iterator vector<T, _inline_capacity, size_type>::erase(iterator first,
                                                                                               iterator last) {
    if (first == last) {
        return last;
    }
    iterator new_end = std::move(last, end(), first);
    std::destroy(new_end, end());
    _size = (size_type)(new_end - begin());
    return end();
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _inline_capacity, size_type>::push_back(const T &value) {
    // value may refer to an element of this vector
    const T *value_ptr = &value;
    if (_size >= _capacity) {
        if (!std::less<>()(value_ptr, begin()) && std::less<>()(value_ptr, end())) {
            auto value_offset = value_ptr - begin();
            reserve(_size + 1);
            value_ptr = begin() + value_offset;
        } else {
            reserve(_size + 1);
        }
    }
    new (end()) value_type(*value_ptr);
    _size += 1;
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _inline_capacity, size_type>::push_back(T &&value) {
    // value may refer to an element of this vector
    T *value_ptr = &value;
    if (_size >= _capacity) {
        if (!std::less<>()(value_ptr, begin()) && std::less<>()(value_ptr, end())) {
            auto value_offset = value_ptr - begin();
            reserve(_size + 1);
            value_ptr = begin() + value_offset;
        } else {
            reserve(_size + 1);
        }
    }
    new (end()) value_type(std::move(*value_ptr));
    _size += 1;
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
template <typename... Args>
vector<T, _inline_capacity, size_type>::reference vector<T, _inline_capacity, size_type>::emplace_back(Args &&...args) {
    if (_size >= _capacity) {
        // construct before growing in case args refer to an element of this vector
        value_type value(std::forward<Args>(args)...);
        reserve(_size + 1);
        new (end()) value_type(std::move(value));
    } else {
        new (end()) value_type(std::forward<Args>(args)...);
    }
    _size += 1;
    return back();
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _inline_capacity, size_type>::pop_back() {
    assert(size() > 0);
    _size -= 1;
    std::destroy_at(end());
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _inline_capacity, size_type>::resize(size_type count) {
    if (count < _size) {
        std::destroy(begin() + count, end());
    } else if (count > _size) {
        reserve(count);
        std::uninitialized_value_construct(end(), begin() + count);
    }
    _size = count;
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _inline_capacity, size_type>::resize(size_type count, const value_type &value) {
    if (count < _size) {
        std::destroy(begin() + count, end());
    } else if (count > _size) {
        // value may refer to an element of this vector
        const T *value_ptr = &value;
        if (count > _capacity) {
            if (!std::less<>()(value_ptr, begin()) && std::less<>()(value_ptr, end())) {
                auto value_offset = value_ptr - begin();
                reserve(count);
                value_ptr = begin() + value_offset;
            } else {
                reserve(count);
            }
        }
        std::uninitialized_fill(end(), begin() + count, *value_ptr);
    }
    _size = count;
}

// MARK: Specialization for empty stack buffer

template <typename T, typename _size_type>
    requires std::unsigned_integral<_size_type>
class vector<T, 0, _size_type> {
  public:
    using value_type = T;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = value_type *_Nonnull;
    using const_iterator = const value_type *_Nonnull;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = _size_type;

  private:
    T *_Nullable _buffer = nullptr;
    size_type _size = 0;
    size_type _capacity = 0;

    void reserve_slow(size_type new_cap);

  public:
    vector() {};
    ~vector();

    // Non-copyable

    vector(const vector &) = delete;
    vector &operator=(const vector &) = delete;

    // Move

    vector(vector &&other) noexcept;
    vector &operator=(vector &&other) noexcept;

    // Element access

    reference operator[](size_type pos) { return data()[pos]; };
    const_reference operator[](size_type pos) const { return data()[pos]; };

    reference front() { return data()[0]; };
    const_reference front() const { return data()[0]; };
    reference back() { return data()[_size - 1]; };
    const_reference back() const { return data()[_size - 1]; };

    T *_Nonnull data() { return _buffer; };
    const T *_Nonnull data() const { return _buffer; };

    // Iterators

    iterator begin() { return iterator(&data()[0]); };
    iterator end() { return iterator(&data()[_size]); };
    const_iterator cbegin() const { return const_iterator(&data()[0]); };
    const_iterator cend() const { return const_iterator(&data()[_size]); };
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
    void reserve(size_type new_cap);
    size_type capacity() const { return _capacity; };
    void shrink_to_fit();

    // Modifiers

    void clear();

    iterator insert(const_iterator pos, const T &value);
    iterator insert(const_iterator pos, T &&value);

    iterator erase(iterator pos);
    iterator erase(iterator first, iterator last);

    void push_back(const T &value);
    void push_back(T &&value);
    void pop_back();

    void resize(size_type count);
    void resize(size_type count, const value_type &value);
};

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, 0, size_type>::~vector() {
    std::destroy(begin(), end());
    if (_buffer) {
        free((void *)_buffer);
    }
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, 0, size_type>::vector(vector &&other) noexcept
    : _buffer(std::exchange(other._buffer, nullptr)), _size(std::exchange(other._size, 0)),
      _capacity(std::exchange(other._capacity, 0)) {}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, 0, size_type> &vector<T, 0, size_type>::operator=(vector &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    std::destroy(begin(), end());
    if (_buffer) {
        free((void *)_buffer);
    }
    _buffer = std::exchange(other._buffer, nullptr);
    _size = std::exchange(other._size, 0);
    _capacity = std::exchange(other._capacity, 0);
    return *this;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::reserve_slow(size_type new_cap) {
    constexpr size_t max_capacity = std::numeric_limits<size_type>::max();
    if (_capacity == max_capacity) {
        precondition_failure("vector capacity overflow");
    }
    size_type effective_new_cap = (size_type)std::clamp(2 * size_t(_capacity) + 1, size_t(new_cap), max_capacity);

    if constexpr (std::is_trivially_copyable_v<T>) {
        _buffer = reinterpret_cast<T *>(
            details::realloc_vector<size_type, sizeof(T)>((void *)_buffer, &_capacity, effective_new_cap));
    } else {
        size_type new_capacity = 0;
        T *new_buffer = reinterpret_cast<T *>(
            details::realloc_vector<size_type, sizeof(T)>(nullptr, &new_capacity, effective_new_cap));
        std::uninitialized_move(begin(), end(), new_buffer);
        std::destroy(begin(), end());
        if (_buffer) {
            free((void *)_buffer);
        }
        _buffer = new_buffer;
        _capacity = new_capacity;
    }
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::reserve(size_type new_cap) {
    if (new_cap <= capacity()) {
        return;
    }
    reserve_slow(new_cap);
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::shrink_to_fit() {
    if (capacity() == size()) {
        return;
    }

    if constexpr (std::is_trivially_copyable_v<T>) {
        _buffer = reinterpret_cast<T *>(details::realloc_vector<size_type, sizeof(T)>(_buffer, &_capacity, _size));
    } else {
        size_type new_capacity = 0;
        T *new_buffer =
            reinterpret_cast<T *>(details::realloc_vector<size_type, sizeof(T)>(nullptr, &new_capacity, _size));
        if (new_buffer) {
            std::uninitialized_move(begin(), end(), new_buffer);
        }
        std::destroy(begin(), end());
        free((void *)_buffer);
        _buffer = new_buffer;
        _capacity = new_capacity;
    }
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::clear() {
    std::destroy(begin(), end());
    _size = 0;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, 0, size_type>::iterator vector<T, 0, size_type>::insert(const_iterator pos, const T &value) {
    if (pos == end()) {
        push_back(value);
        return end() - 1;
    }

    // value may refer to an element of this vector
    auto offset = pos - begin();
    const T *value_ptr = &value;
    if (_size >= _capacity) {
        if (!std::less<>()(value_ptr, begin()) && std::less<>()(value_ptr, end())) {
            auto value_offset = value_ptr - begin();
            reserve(_size + 1);
            value_ptr = begin() + value_offset;
        } else {
            reserve(_size + 1);
        }
    }
    iterator mutable_pos = begin() + offset;

    new (end()) value_type(std::move(back()));
    std::move_backward(mutable_pos, end() - 1, end());
    _size += 1;

    if (!std::less<>()(value_ptr, mutable_pos) && std::less<>()(value_ptr, end())) {
        ++value_ptr;
    }
    *mutable_pos = *value_ptr;
    return mutable_pos;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, 0, size_type>::iterator vector<T, 0, size_type>::insert(const_iterator pos, T &&value) {
    if (pos == end()) {
        push_back(std::move(value));
        return end() - 1;
    }

    // value may refer to an element of this vector
    auto offset = pos - begin();
    T *value_ptr = &value;
    if (_size >= _capacity) {
        if (!std::less<>()(value_ptr, begin()) && std::less<>()(value_ptr, end())) {
            auto value_offset = value_ptr - begin();
            reserve(_size + 1);
            value_ptr = begin() + value_offset;
        } else {
            reserve(_size + 1);
        }
    }
    iterator mutable_pos = begin() + offset;

    new (end()) value_type(std::move(back()));
    std::move_backward(mutable_pos, end() - 1, end());
    _size += 1;

    if (!std::less<>()(value_ptr, mutable_pos) && std::less<>()(value_ptr, end())) {
        ++value_ptr;
    }
    *mutable_pos = std::move(*value_ptr);
    return mutable_pos;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, 0, size_type>::iterator vector<T, 0, size_type>::erase(iterator pos) {
    if (pos == end()) {
        return end();
    }
    return erase(pos, pos + 1);
}

// Unlike std::vector, returns the new end() rather than the iterator following the erased elements.
template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, 0, size_type>::iterator vector<T, 0, size_type>::erase(iterator first, iterator last) {
    if (first == last) {
        return last;
    }
    iterator new_end = std::move(last, end(), first);
    std::destroy(new_end, end());
    _size = (size_type)(new_end - begin());
    return end();
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::push_back(const T &value) {
    // value may refer to an element of this vector
    const T *value_ptr = &value;
    if (_size >= _capacity) {
        if (!std::less<>()(value_ptr, begin()) && std::less<>()(value_ptr, end())) {
            auto value_offset = value_ptr - begin();
            reserve(_size + 1);
            value_ptr = begin() + value_offset;
        } else {
            reserve(_size + 1);
        }
    }
    new (end()) value_type(*value_ptr);
    _size += 1;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::push_back(T &&value) {
    // value may refer to an element of this vector
    T *value_ptr = &value;
    if (_size >= _capacity) {
        if (!std::less<>()(value_ptr, begin()) && std::less<>()(value_ptr, end())) {
            auto value_offset = value_ptr - begin();
            reserve(_size + 1);
            value_ptr = begin() + value_offset;
        } else {
            reserve(_size + 1);
        }
    }
    new (end()) value_type(std::move(*value_ptr));
    _size += 1;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::pop_back() {
    assert(size() > 0);
    _size -= 1;
    std::destroy_at(end());
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::resize(size_type count) {
    if (count < _size) {
        std::destroy(begin() + count, end());
    } else if (count > _size) {
        reserve(count);
        std::uninitialized_value_construct(end(), begin() + count);
    }
    _size = count;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::resize(size_type count, const value_type &value) {
    if (count < _size) {
        std::destroy(begin() + count, end());
    } else if (count > _size) {
        // value may refer to an element of this vector
        const T *value_ptr = &value;
        if (count > _capacity) {
            if (!std::less<>()(value_ptr, begin()) && std::less<>()(value_ptr, end())) {
                auto value_offset = value_ptr - begin();
                reserve(count);
                value_ptr = begin() + value_offset;
            } else {
                reserve(count);
            }
        }
        std::uninitialized_fill(end(), begin() + count, *value_ptr);
    }
    _size = count;
}

// MARK: Specialization for unique_ptr

template <typename T, typename deleter_type, typename _size_type>
    requires std::unsigned_integral<_size_type>
class vector<std::unique_ptr<T, deleter_type>, 0, _size_type> {
  public:
    using value_type = std::unique_ptr<T, deleter_type>;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = value_type *_Nonnull;
    using const_iterator = const value_type *_Nonnull;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = _size_type;

  private:
    std::unique_ptr<T, deleter_type> *_Nullable _buffer = nullptr;
    size_type _size = 0;
    size_type _capacity = 0;

    void reserve_slow(size_type new_cap);

  public:
    vector() {};
    ~vector();

    // Non-copyable

    vector(const vector &) = delete;
    vector &operator=(const vector &) = delete;

    // Move

    vector(vector &&) noexcept;
    vector &operator=(vector &&) noexcept;

    // Element access

    reference operator[](size_type pos) { return data()[pos]; };
    const_reference operator[](size_type pos) const { return data()[pos]; };

    reference front() { return data()[0]; };
    const_reference front() const { return data()[0]; };
    reference back() { return data()[_size - 1]; };
    const_reference back() const { return data()[_size - 1]; };

    std::unique_ptr<T, deleter_type> *_Nonnull data() { return _buffer; };
    const std::unique_ptr<T, deleter_type> *_Nonnull data() const { return _buffer; };

    // Iterators

    iterator begin() { return iterator(&data()[0]); };
    iterator end() { return iterator(&data()[_size]); };
    const_iterator cbegin() const { return const_iterator(&data()[0]); };
    const_iterator cend() const { return const_iterator(&data()[_size]); };
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
    void reserve(size_type new_cap);
    size_type capacity() const { return _capacity; };
    void shrink_to_fit();

    // Modifiers

    void clear();

    iterator insert(const_iterator pos, const std::unique_ptr<T, deleter_type> &value);
    iterator insert(const_iterator pos, std::unique_ptr<T, deleter_type> &&value);

    iterator erase(iterator pos);
    iterator erase(iterator first, iterator last);

    void push_back(const std::unique_ptr<T, deleter_type> &value) = delete;
    void push_back(std::unique_ptr<T, deleter_type> &&value);
    void pop_back();

    void resize(size_type count);
    void resize(size_type count, const value_type &value);
};

template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
vector<std::unique_ptr<T, deleter_type>, 0, size_type>::~vector() {
    std::destroy(begin(), end());
    if (_buffer) {
        free((void *)_buffer);
    }
}

template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
vector<std::unique_ptr<T, deleter_type>, 0, size_type>::vector(vector &&other) noexcept
    : _buffer(std::exchange(other._buffer, nullptr)), _size(std::exchange(other._size, 0)),
      _capacity(std::exchange(other._capacity, 0)) {}

template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
vector<std::unique_ptr<T, deleter_type>, 0, size_type> &
vector<std::unique_ptr<T, deleter_type>, 0, size_type>::operator=(vector &&other) noexcept {
    if (this == &other) {
        return *this;
    }

    std::destroy(begin(), end());
    if (_buffer) {
        free((void *)_buffer);
    }
    _buffer = std::exchange(other._buffer, nullptr);
    _size = std::exchange(other._size, 0);
    _capacity = std::exchange(other._capacity, 0);
    return *this;
}

template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<std::unique_ptr<T, deleter_type>, 0, size_type>::reserve_slow(size_type new_cap) {
    constexpr size_t max_capacity = std::numeric_limits<size_type>::max();
    if (_capacity == max_capacity) {
        precondition_failure("vector capacity overflow");
    }
    size_type effective_new_cap = (size_type)std::clamp(2 * size_t(_capacity) + 1, size_t(new_cap), max_capacity);

    size_type new_capacity = 0;
    auto new_buffer = reinterpret_cast<std::unique_ptr<T, deleter_type> *>(
        details::realloc_vector<size_type, sizeof(std::unique_ptr<T, deleter_type>)>(nullptr, &new_capacity,
                                                                                     effective_new_cap));
    std::uninitialized_move(begin(), end(), new_buffer);
    std::destroy(begin(), end());
    if (_buffer) {
        free((void *)_buffer);
    }
    _buffer = new_buffer;
    _capacity = new_capacity;
}

template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<std::unique_ptr<T, deleter_type>, 0, size_type>::reserve(size_type new_cap) {
    if (new_cap <= capacity()) {
        return;
    }
    reserve_slow(new_cap);
}

template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<std::unique_ptr<T, deleter_type>, 0, size_type>::shrink_to_fit() {
    if (capacity() == size()) {
        return;
    }

    size_type new_capacity = 0;
    auto new_buffer = reinterpret_cast<std::unique_ptr<T, deleter_type> *>(
        details::realloc_vector<size_type, sizeof(std::unique_ptr<T, deleter_type>)>(nullptr, &new_capacity, _size));
    if (new_buffer) {
        std::uninitialized_move(begin(), end(), new_buffer);
    }
    std::destroy(begin(), end());
    free((void *)_buffer);
    _buffer = new_buffer;
    _capacity = new_capacity;
}

template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<std::unique_ptr<T, deleter_type>, 0, size_type>::clear() {
    std::destroy(begin(), end());
    _size = 0;
}

template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
vector<std::unique_ptr<T, deleter_type>, 0, size_type>::iterator
vector<std::unique_ptr<T, deleter_type>, 0, size_type>::insert(const_iterator pos,
                                                               std::unique_ptr<T, deleter_type> &&value) {
    if (pos == end()) {
        push_back(std::move(value));
        return end() - 1;
    }

    // value may refer to an element of this vector
    auto offset = pos - begin();
    auto value_ptr = &value;
    if (_size >= _capacity) {
        if (!std::less<>()(value_ptr, begin()) && std::less<>()(value_ptr, end())) {
            auto value_offset = value_ptr - begin();
            reserve(_size + 1);
            value_ptr = begin() + value_offset;
        } else {
            reserve(_size + 1);
        }
    }
    iterator mutable_pos = begin() + offset;

    new (end()) value_type(std::move(back()));
    std::move_backward(mutable_pos, end() - 1, end());
    _size += 1;

    if (!std::less<>()(value_ptr, mutable_pos) && std::less<>()(value_ptr, end())) {
        ++value_ptr;
    }
    *mutable_pos = std::move(*value_ptr);
    return mutable_pos;
}

template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
vector<std::unique_ptr<T, deleter_type>, 0, size_type>::iterator
vector<std::unique_ptr<T, deleter_type>, 0, size_type>::erase(iterator pos) {
    if (pos == end()) {
        return end();
    }
    return erase(pos, pos + 1);
}

// Unlike std::vector, returns the new end() rather than the iterator following the erased elements.
template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
vector<std::unique_ptr<T, deleter_type>, 0, size_type>::iterator
vector<std::unique_ptr<T, deleter_type>, 0, size_type>::erase(iterator first, iterator last) {
    if (first == last) {
        return last;
    }
    iterator new_end = std::move(last, end(), first);
    std::destroy(new_end, end());
    _size = (size_type)(new_end - begin());
    return end();
}

template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<std::unique_ptr<T, deleter_type>, 0, size_type>::push_back(std::unique_ptr<T, deleter_type> &&value) {
    // value may refer to an element of this vector
    auto value_ptr = &value;
    if (_size >= _capacity) {
        if (!std::less<>()(value_ptr, begin()) && std::less<>()(value_ptr, end())) {
            auto value_offset = value_ptr - begin();
            reserve(_size + 1);
            value_ptr = begin() + value_offset;
        } else {
            reserve(_size + 1);
        }
    }
    new (end()) value_type(std::move(*value_ptr));
    _size += 1;
}

template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<std::unique_ptr<T, deleter_type>, 0, size_type>::pop_back() {
    assert(size() > 0);
    _size -= 1;
    std::destroy_at(end());
}

template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<std::unique_ptr<T, deleter_type>, 0, size_type>::resize(size_type count) {
    if (count < _size) {
        std::destroy(begin() + count, end());
    } else if (count > _size) {
        reserve(count);
        std::uninitialized_value_construct(end(), begin() + count);
    }
    _size = count;
}

} // namespace IAG

IAG_ASSUME_NONNULL_END
