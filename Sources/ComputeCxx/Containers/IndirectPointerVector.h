#pragma once

#include "CoreFoundation/CFBase.h"
#include <iterator>
#include <stdint.h>

#include "Vector.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

/// A  vector that efficiently stores a single element as a pointer or stores multiple elements as a vector.
template <typename T, typename size_type = std::size_t>
    requires std::unsigned_integral<size_type>
class indirect_pointer_vector {
  public:
    using value_type = T;
    using pointer = value_type *_Nonnull;
    using const_pointer = const value_type *_Nonnull;
    using reference = value_type &;
    using const_reference = const value_type &;
    using vector_type = vector<pointer, 4, size_type>;
    using iterator = pointer *_Nonnull;
    using const_iterator = const pointer *_Nonnull;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  private:
    uintptr_t _data;
    enum {
        TagMask = 0x1,
        NullElement = 0x2,
    };

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
    indirect_pointer_vector();
    ~indirect_pointer_vector();

    // Element access

    reference front() { return has_vector() ? *get_vector().front() : *reinterpret_cast<pointer>(_data); };
    const_reference front() const {
        return has_vector() ? *get_vector().front() : *reinterpret_cast<const_pointer>(_data);
    };

    // Iterators

    iterator begin() { return has_vector() ? get_vector().begin() : reinterpret_cast<pointer *>(&_data); };
    iterator end() {
        if (has_vector()) {
            return get_vector().end();
        } else {
            size_type size = reinterpret_cast<pointer>(_data) == nullptr ? 0 : 1;
            return &reinterpret_cast<pointer *>(&_data)[size];
        }
    };
    const_iterator cbegin() const {
        return has_vector() ? get_vector().cbegin() : reinterpret_cast<const pointer *>(&_data);
    };
    const_iterator cend() const {
        if (has_vector()) {
            return get_vector().cend();
        } else {
            size_type size = reinterpret_cast<pointer>(_data) == nullptr ? 0 : 1;
            return &reinterpret_cast<const pointer *>(&_data)[(T *)_data != nullptr ? 1 : 0];
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

    bool empty() const { return has_vector() ? get_vector().empty() : reinterpret_cast<pointer>(_data) == nullptr; };
    size_type size() const { return has_vector() ? get_vector()->size() : _data == nullptr ? 0 : 1; };

    // Modifiers

    void clear();

    iterator erase(iterator pos);
    iterator erase(iterator first, iterator last);

    void push_back(const T &value);
    void push_back(T &&value);

    void resize(size_type count);
};

} // namespace AG

CF_ASSUME_NONNULL_END
