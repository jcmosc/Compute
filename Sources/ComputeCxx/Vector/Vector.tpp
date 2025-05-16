#include "Vector.h"

#include <algorithm>
#include <malloc/malloc.h>
#include <memory>
#include <cassert>

#include "Errors/Errors.h"

namespace AG {

#pragma mark - Base implementation

namespace details {

template <typename size_type, unsigned int element_size_bytes>
    requires std::unsigned_integral<size_type>
void *_Nullable realloc_vector(void *_Nullable buffer, void *_Nonnull _inline_buffer, size_type inline_capacity, size_type *_Nonnull size,
                               size_type preferred_new_size) {
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
        _buffer = reinterpret_cast<T *>(
            details::realloc_vector<size_type, sizeof(T)>(_buffer, _inline_buffer, _inline_capacity, &_capacity, _size));
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
    reserve(_size + 1);
    iterator mutable_pos = begin() + (pos - begin());
    std::move_backward(mutable_pos, end(), end() + 1);
    new (mutable_pos) value_type(value);
    _size += 1;
    return end();
}

template <typename T, unsigned int _inline_capacity, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, _inline_capacity, size_type>::iterator vector<T, _inline_capacity, size_type>::insert(const_iterator pos, T &&value) {
    reserve(_size + 1);
    iterator mutable_pos = begin() + (pos - begin());
    std::move_backward(mutable_pos, end(), end() + 1);
    new (pos) value_type(std::move(value));
    _size += 1;
    return end();
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
vector<T, _inline_capacity, size_type>::iterator vector<T, _inline_capacity, size_type>::erase(iterator first, iterator last) {
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

#pragma mark - Specialization for no inline buffer

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
    reserve(_size + 1);
    iterator mutable_pos = begin() + (pos - begin());
    std::move_backward(mutable_pos, end(), end() + 1);
    new (mutable_pos) value_type(value);
    _size += 1;
    return end();
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, 0, size_type>::iterator vector<T, 0, size_type>::insert(const_iterator pos, T &&value) {
    reserve(_size + 1);
    iterator mutable_pos = begin() + (pos - begin());
    std::move_backward(mutable_pos, end(), end() + 1);
    new (mutable_pos) value_type(std::move(value));
    _size += 1;
    return end();
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

#pragma mark - Specialization for unique_ptr

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
