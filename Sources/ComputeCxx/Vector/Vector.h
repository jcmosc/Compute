#pragma once

#include <CoreFoundation/CFBase.h>
#include <algorithm>
#include <cassert>
#include <concepts>
#include <iterator>
#include <malloc/malloc.h>
#include <memory>

#include "Errors/Errors.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

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
    T _inline_buffer[_inline_capacity];
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

    vector(vector &&);
    vector &operator=(vector &&);

    // Element access

    reference operator[](size_type pos) { return data()[pos]; };
    const_reference operator[](size_type pos) const { return data()[pos]; };

    reference front() { return &data()[0]; };
    const_reference front() const { return *&data()[0]; };
    reference back() { return *&data()[_size - 1]; };
    const_reference back() const { return *&data()[_size - 1]; };

    T *_Nonnull data() { return _buffer != nullptr ? _buffer : _inline_buffer; };
    const T *_Nonnull data() const { return _buffer != nullptr ? _buffer : _inline_buffer; };

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

static_assert(std::contiguous_iterator<vector<int>::iterator>);
static_assert(std::contiguous_iterator<vector<int>::const_iterator>);

namespace details {

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

} // namespace details

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, _inline_capacity, size_type>::~vector() {
    for (auto i = 0; i < _size; i++) {
        data()[i].~T();
    }
    if (_buffer) {
        free((void *)_buffer);
    }
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _inline_capacity, size_type>::reserve_slow(size_type new_cap) {
    size_type effective_new_cap = std::max(capacity() * 1.5, new_cap * 1.0);
    _buffer = reinterpret_cast<T *>(details::realloc_vector<size_type, sizeof(T)>(
        (void *)_buffer, (void *)_inline_buffer, _inline_capacity, &_capacity, effective_new_cap));
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
    if (capacity() > _size) {
        _buffer = reinterpret_cast<T *>(details::realloc_vector<size_type, sizeof(T)>(
            _buffer, _inline_buffer, _inline_capacity, &_capacity, _size));
    }
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _inline_capacity, size_type>::clear() {
    for (auto i = 0; i < _size; i++) {
        data()[i].~T();
    }
    _size = 0;
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, _inline_capacity, size_type>::iterator vector<T, _inline_capacity, size_type>::insert(const_iterator pos,
                                                                                                const T &value) {
    auto offset = pos - begin();
    reserve(_size + 1);
    iterator mutable_pos = begin() + offset;
    std::move_backward(mutable_pos, end(), end() + 1);
    new (mutable_pos) value_type(value);
    _size += 1;
    return mutable_pos;
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, _inline_capacity, size_type>::iterator vector<T, _inline_capacity, size_type>::insert(const_iterator pos,
                                                                                                T &&value) {
    auto offset = pos - begin();
    reserve(_size + 1);
    iterator mutable_pos = begin() + offset;
    std::move_backward(mutable_pos, end(), end() + 1);
    new (mutable_pos) value_type(std::move(value));
    _size += 1;
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

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, _inline_capacity, size_type>::iterator vector<T, _inline_capacity, size_type>::erase(iterator first,
                                                                                               iterator last) {
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
    _size -= count;
    return end();
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _inline_capacity, size_type>::push_back(const T &value) {
    reserve(_size + 1);
    new (&data()[_size]) value_type(value);
    _size += 1;
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _inline_capacity, size_type>::push_back(T &&value) {
    reserve(_size + 1);
    new (&data()[_size]) value_type(std::move(value));
    _size += 1;
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _inline_capacity, size_type>::pop_back() {
    assert(size() > 0);
    data()[_size - 1].~T();
    _size -= 1;
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _inline_capacity, size_type>::resize(size_type count) {
    reserve(count);
    if (count < _size) {
        for (auto i = count; i < _size; i++) {
            data()[i].~T();
        }
    } else if (count > _size) {
        for (auto i = _size; i < count; i++) {
            new (&data()[i]) value_type();
        }
    }
    _size = count;
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _inline_capacity, size_type>::resize(size_type count, const value_type &value) {
    reserve(count);
    if (count < _size) {
        for (auto i = count; i < _size; i++) {
            data()[i].~T();
        }
    } else if (count > _size) {
        for (auto i = _size; i < count; i++) {
            new (&data()[i]) value_type(value);
        }
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

    reference front() { return &data()[0]; };
    const_reference front() const { return *&data()[0]; };
    reference back() { return *&data()[_size - 1]; };
    const_reference back() const { return *&data()[_size - 1]; };

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

namespace details {

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

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, 0, size_type>::~vector() {
    for (auto i = 0; i < _size; i++) {
        _buffer[i].~T();
    }
    if (_buffer) {
        free(_buffer);
    }
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, 0, size_type>::vector(vector &&other) noexcept
    : _buffer(std::exchange(other._buffer, nullptr)), _size(other._size), _capacity(other._capacity) {
    other._size = 0;
    other._capacity = 0;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, 0, size_type> &vector<T, 0, size_type>::operator=(vector &&other) noexcept {
    if (this != &other) {
        for (auto i = 0; i < _size; i++) {
            _buffer[i].~T();
        }
        if (_buffer) {
            free(_buffer);
        }
        _buffer = other._buffer;
        _size = other._size;
        _capacity = other._capacity;
        other._buffer = nullptr;
        other._size = 0;
        other._capacity = 0;
    }
    return *this;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::reserve_slow(size_type new_cap) {
    size_type effective_new_cap = std::max(capacity() * 1.5, new_cap * 1.0);
    _buffer =
        reinterpret_cast<T *>(details::realloc_vector<size_type, sizeof(T)>(_buffer, &_capacity, effective_new_cap));
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
    if (capacity() > size()) {
        _buffer = reinterpret_cast<T *>(details::realloc_vector<size_type, sizeof(T)>(_buffer, &_capacity, 0));
    }
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::clear() {
    for (auto i = 0; i < _size; i++) {
        data()[i].~T();
    }
    _size = 0;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, 0, size_type>::iterator vector<T, 0, size_type>::insert(const_iterator pos, const T &value) {
    if (pos == end()) {
        push_back(value);
        return end() - 1;
    }
    auto offset = pos - begin();
    reserve(_size + 1);
    iterator mutable_pos = begin() + offset;
    std::move_backward(mutable_pos, end(), end() + 1);
    new (mutable_pos) value_type(value);
    _size += 1;
    return mutable_pos;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, 0, size_type>::iterator vector<T, 0, size_type>::insert(const_iterator pos, T &&value) {
    if (pos == end()) {
        push_back(std::move(value));
        return end() - 1;
    }
    auto offset = pos - begin();
    reserve(_size + 1);
    iterator mutable_pos = begin() + offset;
    std::move_backward(mutable_pos, end(), end() + 1);
    new (mutable_pos) value_type(std::move(value));
    _size += 1;
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

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, 0, size_type>::iterator vector<T, 0, size_type>::erase(iterator first, iterator last) {
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
    _size -= count;
    return end();
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::push_back(const T &value) {
    reserve(_size + 1);
    new (&_buffer[_size]) value_type(value);
    _size += 1;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::push_back(T &&value) {
    reserve(_size + 1);
    new (&_buffer[_size]) value_type(std::move(value));
    _size += 1;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::pop_back() {
    assert(size() > 0);
    data()[_size - 1].~T();
    _size -= 1;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::resize(size_type count) {
    reserve(count);
    if (count < _size) {
        for (auto i = count; i < _size; i++) {
            data()[i].~T();
        }
    } else if (count > _size) {
        for (auto i = _size; i < count; i++) {
            new (&data()[i]) value_type();
        }
    }
    _size = count;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::resize(size_type count, const value_type &value) {
    reserve(count);
    if (count < _size) {
        for (auto i = count; i < _size; i++) {
            data()[i].~T();
        }
    } else if (count > _size) {
        for (auto i = _size; i < count; i++) {
            new (&data()[i]) value_type(value);
        }
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

    reference front() { return &data()[0]; };
    const_reference front() const { return *&data()[0]; };
    reference back() { return *&data()[_size - 1]; };
    const_reference back() const { return *&data()[_size - 1]; };

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
    for (auto i = 0; i < _size; i++) {
        _buffer[i].reset();
    }
    if (_buffer) {
        free(_buffer);
    }
}

template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
vector<std::unique_ptr<T, deleter_type>, 0, size_type>::vector(vector &&other) noexcept
    : _buffer(std::exchange(other._buffer, nullptr)), _size(other._size), _capacity(other._capacity) {
    other._size = 0;
    other._capacity = 0;
}

template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
vector<std::unique_ptr<T, deleter_type>, 0, size_type> &
vector<std::unique_ptr<T, deleter_type>, 0, size_type>::operator=(vector &&other) noexcept {
    if (this != &other) {
        for (auto i = 0; i < _size; i++) {
            _buffer[i].~T();
        }
        if (_buffer) {
            free(_buffer);
        }
        _buffer = other._buffer;
        _size = other._size;
        _capacity = other._capacity;
        other._buffer = nullptr;
        other._size = 0;
        other._capacity = 0;
    }
    return *this;
}

template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<std::unique_ptr<T, deleter_type>, 0, size_type>::reserve_slow(size_type new_cap) {
    size_type effective_new_cap = std::max(capacity() * 1.5, new_cap * 1.0);
    _buffer = reinterpret_cast<std::unique_ptr<T, deleter_type> *>(
        details::realloc_vector<size_type, sizeof(std::unique_ptr<T, deleter_type>)>(_buffer, &_capacity,
                                                                                     effective_new_cap));
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
vector<std::unique_ptr<T, deleter_type>, 0, size_type>::iterator
vector<std::unique_ptr<T, deleter_type>, 0, size_type>::erase(iterator pos) {
    if (pos == end()) {
        return end();
    }
    return erase(pos, pos + 1);
}

template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
vector<std::unique_ptr<T, deleter_type>, 0, size_type>::iterator
vector<std::unique_ptr<T, deleter_type>, 0, size_type>::erase(iterator first, iterator last) {
    auto count = last - first;
    if (count == 0) {
        return last;
    }
    for (auto iter = first; iter != last; iter++) {
        iter->reset();
    }
    for (auto iter = last, old_end = end(); iter != old_end; iter++) {
        std::swap(*(iter - count), *iter);
    }
    _size -= count;
    return end();
}

template <typename T, typename deleter_type, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<std::unique_ptr<T, deleter_type>, 0, size_type>::push_back(std::unique_ptr<T, deleter_type> &&value) {
    reserve(_size + 1);
    new (&_buffer[_size]) value_type(std::move(value));
    _size += 1;
}

} // namespace AG

CF_ASSUME_NONNULL_END
