#pragma once

#include <CoreFoundation/CFBase.h>
#include <concepts>
#include <iterator>
#include <memory>

CF_ASSUME_NONNULL_BEGIN

namespace AG {

template <typename T, unsigned int _stack_size = 0, typename _size_type = std::size_t>
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
    T _stack_buffer[_stack_size];
    T *_Nullable _buffer = nullptr;
    size_type _size = 0;
    size_type _capacity = _stack_size;

    void reserve_slow(size_type new_cap);

  public:
    ~vector();

    // Element access

    reference operator[](size_type pos) { return data()[pos]; };
    const_reference operator[](size_type pos) const { return data()[pos]; };

    reference front() { return &data()[0]; };
    const_reference front() const { return *&data()[0]; };
    reference back() { return *&data()[_size - 1]; };
    const_reference back() const { return *&data()[_size - 1]; };

    T *_Nonnull data() { return _buffer != nullptr ? _buffer : _stack_buffer; };
    const T *_Nonnull data() const { return _buffer != nullptr ? _buffer : _stack_buffer; };

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
    ~vector();

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
    std::unique_ptr<T, deleter_type> *_Nonnull _buffer = nullptr;
    size_type _size = 0;
    size_type _capacity = 0;

    void reserve_slow(size_type new_cap);

  public:
    ~vector();

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

} // namespace AG

CF_ASSUME_NONNULL_END

#include "Vector.tpp"
