#include "Vector.h"

#include "Errors/Errors.h"

namespace AG {

#pragma mark - Base implementation

namespace details {

template <typename size_type, unsigned int element_size_bytes>
    requires std::unsigned_integral<size_type>
void *realloc_vector(void *buffer, void *stack_buffer, size_type stack_size, size_type *size,
                     size_type preferred_new_size) {
    // copy data from heap buffer buffer into stack buffer if possible
    if (preferred_new_size <= stack_size) {
        if (buffer) {
            memcpy(stack_buffer, buffer, preferred_new_size * element_size_bytes);
            free(buffer);
            *size = stack_size;
        }
        return nullptr;
    }

    size_t new_size_bytes = malloc_good_size(preferred_new_size * element_size_bytes);
    size_type new_size = new_size_bytes / element_size_bytes;
    if (new_size == *size) {
        // nothing to do
        return buffer;
    }

    void *new_buffer = realloc(buffer, new_size_bytes);
    if (!new_buffer) {
        precondition_failure("allocation failure");
    }

    // copy data from stack buffer into heap buffer
    if (!buffer) {
        memcpy(new_buffer, stack_buffer, (*size) * element_size_bytes);
    }

    *size = new_size;
    return new_buffer;
}

} // namespace details

template <typename T, unsigned int _stack_size, typename size_type>
    requires std::unsigned_integral<size_type>
vector<T, _stack_size, size_type>::~vector() {
    for (auto i = 0; i < _size; i++) {
        delete this[i];
    }
    if (_buffer) {
        free(_buffer);
    }
}

template <typename T, unsigned int _stack_size, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _stack_size, size_type>::reserve_slow(size_type new_cap) {
    size_type effective_new_cap = max(capacity() * 1.5, new_cap);
    _buffer = details::realloc_vector(_buffer, _stack_buffer, _stack_size, &_capacity, effective_new_cap);
}

template <typename T, unsigned int _stack_size, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _stack_size, size_type>::reserve(size_type new_cap) {
    if (new_cap <= capacity()) {
        return;
    }
    reserve_slow(new_cap);
}

template <typename T, unsigned int _stack_size, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _stack_size, size_type>::clear() {
    for (auto i = 0; i < _size; i++) {
        delete this[i];
    }
    _size = 0;
}

template <typename T, unsigned int _stack_size, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _stack_size, size_type>::push_back(const T &value) {
    reserve(_size + 1);
    new (this[_size]) value_type(value);
    _size += 1;
}

template <typename T, unsigned int _stack_size, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _stack_size, size_type>::push_back(T &&value) {
    reserve(_size + 1);
    new (this[_size]) value_type(std::move(value));
    _size += 1;
}

template <typename T, unsigned int _stack_size, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _stack_size, size_type>::pop_back() {
    assert(size() > 0);
    delete this[_size - 1];
    _size -= 1;
}

template <typename T, unsigned int _stack_size, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _stack_size, size_type>::resize(size_type count) {
    reserve(count);
    if (count < _size) {
        for (auto i = count; i < _size; i++) {
            delete this[i];
        }
    } else if (count > _size) {
        for (auto i = _size; i < count; i++) {
            new (this[i]) value_type();
        }
    }
    _size = count;
}

template <typename T, unsigned int _stack_size, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, _stack_size, size_type>::resize(size_type count, const value_type &value) {
    reserve(count);
    if (count < _size) {
        for (auto i = count; i < _size; i++) {
            delete this[i];
        }
    } else if (count > _size) {
        for (auto i = _size; i < count; i++) {
            new (this[i]) value_type(value);
        }
    }
    _size = count;
}

#pragma mark - Specialization for empty stack buffer

namespace details {

template <typename size_type, unsigned int element_size>
    requires std::unsigned_integral<size_type>
void *realloc_vector(void *buffer, size_type *size, size_type preferred_new_size) {
    if (preferred_new_size == 0) {
        *size = 0;
        free(buffer);
        return nullptr;
    }

    size_t new_size_bytes = malloc_good_size(preferred_new_size * element_size);
    size_type new_size = new_size_bytes / element_size;
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
        delete this[i];
    }
    if (_buffer) {
        free(_buffer);
    }
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::reserve_slow(size_type new_cap) {
    size_type effective_new_cap = max(capacity() * 1.5, new_cap);
    _buffer = details::realloc_vector(_buffer, &_capacity, effective_new_cap);
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
void vector<T, 0, size_type>::clear() {
    for (auto i = 0; i < _size; i++) {
        delete this[i];
    }
    _size = 0;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::push_back(const T &value) {
    reserve(_size + 1);
    new (this[_size]) value_type(value);
    _size += 1;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::push_back(T &&value) {
    reserve(_size + 1);
    new (this[_size]) value_type(std::move(value));
    _size += 1;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::pop_back() {
    assert(size() > 0);
    delete this[_size - 1];
    _size -= 1;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void vector<T, 0, size_type>::resize(size_type count) {
    reserve(count);
    if (count < _size) {
        for (auto i = count; i < _size; i++) {
            delete this[i];
        }
    } else if (count > _size) {
        for (auto i = _size; i < count; i++) {
            new (this[i]) value_type();
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
            delete this[i];
        }
    } else if (count > _size) {
        for (auto i = _size; i < count; i++) {
            new (this[i]) value_type(value);
        }
    }
    _size = count;
}

} // namespace AG
