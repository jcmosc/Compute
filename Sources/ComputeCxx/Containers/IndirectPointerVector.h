#pragma once

#include "CoreFoundation/CFBase.h"
#include <iterator>
#include <stdint.h>

#include "Vector.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

/// A  vector that efficiently stores a single element as a pointer or stores multiple elements as a vector.
template <typename T, typename _size_type = std::size_t>
    requires std::unsigned_integral<_size_type>
class indirect_pointer_vector {
  public:
    using value_type = T *_Nonnull;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = value_type *_Nonnull;
    using const_iterator = const value_type *_Nonnull;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = _size_type;

  private:
    uintptr_t _data;
    enum {
        TagMask = 0x1,
        NullElement = 0x2,
    };

    using vector_type = vector<value_type, 4, size_type>;

    bool has_vector() const { return (_data & TagMask) == 1; };
    vector_type &get_vector() {
        assert(has_vector());
        return *reinterpret_cast<vector_type *>(_data & ~TagMask);
    };
    const vector_type &get_vector() const {
        assert(has_vector());
        return *reinterpret_cast<const vector_type *>(_data & ~TagMask);
    };

  public:
    indirect_pointer_vector() = default;
    ~indirect_pointer_vector();

    // Element access

    reference operator[](size_type pos) {
        assert(pos < size());
        if (has_vector()) {
            return get_vector()[pos];
        } else {
            return reinterpret_cast<reference>(_data);
        }
    };
    const_reference operator[](size_type pos) const {
        assert(pos < size());
        if (has_vector()) {
            return get_vector()[pos];
        } else {
            return *reinterpret_cast<const value_type *>(_data);
        }
    };

    reference front() { return has_vector() ? get_vector().front() : reinterpret_cast<value_type &>(_data); };
    const_reference front() const {
        if (has_vector()) {
            return get_vector().front();
        } else {
            return *reinterpret_cast<const value_type *>(_data);
        }
    };

    // Iterators

    iterator begin() { return has_vector() ? get_vector().begin() : reinterpret_cast<value_type *>(&_data); };
    iterator end() {
        if (has_vector()) {
            return get_vector().end();
        } else {
            size_type size = _data == 0 ? 0 : 1;
            return &reinterpret_cast<value_type *>(&_data)[size];
        }
    };
    const_iterator cbegin() const {
        return has_vector() ? get_vector().cbegin() : reinterpret_cast<const value_type *>(&_data);
    };
    const_iterator cend() const {
        if (has_vector()) {
            return get_vector().cend();
        } else {
            size_type size = _data == 0 ? 0 : 1;
            return &reinterpret_cast<const value_type *>(&_data)[size];
        }
    };
    const_iterator begin() const { return cbegin(); };
    const_iterator end() const { return cend(); };

    reverse_iterator rbegin() { return std::reverse_iterator(end()); };
    reverse_iterator rend() { return std::reverse_iterator(begin()); };
    const_reverse_iterator crbegin() const { return std::reverse_iterator(cend()); };
    const_reverse_iterator crend() const { return std::reverse_iterator(cbegin()); };
    const_reverse_iterator rbegin() const { return crbegin(); };
    const_reverse_iterator rend() const { return crend(); };

    // Capacity

    bool empty() const { return has_vector() ? get_vector().empty() : _data == 0; };
    size_type size() const { return has_vector() ? get_vector().size() : _data == 0 ? 0 : 1; };

    // Modifiers

    void clear();

    iterator erase(iterator pos);
    iterator erase(iterator first, iterator last);

    void push_back(const value_type &value);
    void push_back(value_type &&value);

    void resize(size_type count);
};

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
indirect_pointer_vector<T, size_type>::~indirect_pointer_vector() {
    clear();
};

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void indirect_pointer_vector<T, size_type>::clear() {
    if (has_vector()) {
        vector_type *vector_pointer = reinterpret_cast<vector_type *>(_data & ~TagMask);
        if (vector_pointer != 0) {
            delete vector_pointer;
        }
    }
    _data = 0;
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
indirect_pointer_vector<T, size_type>::iterator indirect_pointer_vector<T, size_type>::erase(iterator pos) {
    if (pos == end()) {
        return end();
    }
    return erase(pos, pos + 1);
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
indirect_pointer_vector<T, size_type>::iterator indirect_pointer_vector<T, size_type>::erase(iterator first,
                                                                                             iterator last) {
    auto count = last - first;
    if (count == 0) {
        return last;
    }
    if (has_vector()) {
        return get_vector().erase(first, last);
    } else {
        assert(count <= 1);
        if (count == 1 && first == begin()) {
            _data = 0;
        }
        return end();
    }
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void indirect_pointer_vector<T, size_type>::push_back(const value_type &value) {
    if (has_vector()) {
        get_vector()->push_back(value);
    } else {
        if (_data == 0) {
            _data = value;
        } else {
            vector_type *vector = new vector_type();
            vector->push_back(this->get_element());
            vector->push_back(value);
            _data = vector | 1;
        }
    }
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void indirect_pointer_vector<T, size_type>::push_back(value_type &&value) {
    if (has_vector()) {
        get_vector().push_back(value);
    } else {
        if (_data == 0) {
            _data = reinterpret_cast<uintptr_t>(std::move(value));
        } else {
            vector_type *vector = new vector_type();
            vector->push_back(reinterpret_cast<value_type>(std::move(_data)));
            vector->push_back(value);
            _data = (uintptr_t)vector | 1;
        }
    }
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void indirect_pointer_vector<T, size_type>::resize(size_type count) {
    if (has_vector()) {
        get_vector().resize(count);
        return;
    }
    if (count == 1) {
        if (_data == 0) {
            _data = NullElement;
        }
        return;
    }
    if (count == 0) {
        _data = 0;
        return;
    }
    // put single element into vector
    vector_type *vector = new vector_type();
    vector->push_back(reinterpret_cast<value_type>(std::move(_data)));
    _data = vector | 1;
}

} // namespace AG

CF_ASSUME_NONNULL_END
