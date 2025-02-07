#include "IndirectPointerVector.h"

namespace AG {

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
indirect_pointer_vector<T, size_type>::~indirect_pointer_vector() {
    clear();
};

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void indirect_pointer_vector<T, size_type>::clear() {
    if (this->has_vector()) {
        vector_type *vector = this->get_vector();
        if (vector != nullptr) {
            delete vector;
        }
    }
    _data = nullptr;
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
        return get_vector()->erase(first, last);
    } else {
        assert(count <= 1);
        if (count == 1 && first == begin()) {
            _data = nullptr;
        }
        return end();
    }
}

template <typename T, typename size_type>
    requires std::unsigned_integral<size_type>
void indirect_pointer_vector<T, size_type>::push_back(const T &value) {
    if (this->has_vector()) {
        this->get_vector()->push_back(value);
    } else {
        if (_data == nullptr) {
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
void indirect_pointer_vector<T, size_type>::push_back(T &&value) {
    if (this->has_vector()) {
        this->get_vector()->push_back(value);
    } else {
        if (_data == nullptr) {
            _data = std::move(value);
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
void indirect_pointer_vector<T, size_type>::resize(size_type count) {
    if (this->has_element()) {
        if (count == 1) {
            if (_data == nullptr) {
                _data = NullElement;
            }
            return;
        }
        if (count == 0) {
            if (_data) {
                delete _data;
            }
            _data = nullptr;
            return;
        }
        // put single element into vector
        vector_type *vector = new vector_type();
        vector->push_back(this->get_element());
        _data = vector | 1;
    }

    this->get_vector()->resize(count);
}

} // namespace AG
