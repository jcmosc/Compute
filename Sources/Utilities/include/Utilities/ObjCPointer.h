#pragma once

#include <CoreFoundation/CFBase.h>
#include <concepts>
#include <objc/runtime.h>

CF_ASSUME_NONNULL_BEGIN

// Redeclare APIs from the Objective-C runtime.
// These functions are not available through public headers, but are guaranteed
// to exist on OS X >= 10.9 and iOS >= 7.0.
OBJC_EXPORT id objc_retain(id obj);
OBJC_EXPORT void objc_release(id obj);

namespace util {

template <typename T>

class objc_ptr {
  public:
    using objc_type = T;

  private:
    objc_type _ref;

  public:
    objc_ptr();
    objc_ptr(objc_type ref);
    ~objc_ptr();

    // Copy and move constructors
    objc_ptr(const objc_ptr &other);
    objc_ptr(objc_ptr &&other) noexcept;

    // Copy and move assignment operators
    objc_ptr &operator=(const objc_ptr &other);
    objc_ptr &operator=(objc_ptr &&other) noexcept;

    // Modifiers
    void reset(objc_type ref = nullptr);

    // Observers
    objc_type get() const noexcept { return _ref; };

    // Conversions
    operator objc_type() const { return _ref; }
};

template <typename T>

objc_ptr<T>::objc_ptr() : _ref(nullptr){};

template <typename T>

objc_ptr<T>::objc_ptr(objc_type ref) : _ref(ref) {
    if (_ref) {
        objc_retain((id)_ref);
    }
}

template <typename T>

objc_ptr<T>::~objc_ptr() {
    if (_ref) {
        objc_release((id)_ref);
    }
}

template <typename T>

objc_ptr<T>::objc_ptr(const objc_ptr &other) : _ref(other._ref) {
    if (_ref) {
        objc_retain((id)_ref);
    }
}

template <typename T>

objc_ptr<T>::objc_ptr(objc_ptr &&other) noexcept : _ref(other._ref) {
    other._ref = nullptr;
}

template <typename T>

objc_ptr<T> &objc_ptr<T>::operator=(const objc_ptr &other) {
    if (this != &other) {
        if (_ref) {
            objc_release((id)_ref);
        }
        _ref = other._ref;
        if (_ref) {
            objc_retain((id)_ref);
        }
    }
    return *this;
}

template <typename T>

objc_ptr<T> &objc_ptr<T>::operator=(objc_ptr &&other) noexcept {
    if (this != &other) {
        if (_ref) {
            objc_release((id)_ref);
        }
        _ref = other._ref;
        other._ref = nullptr;
    }
    return *this;
}

template <typename T>

void objc_ptr<T>::reset(objc_type ref) {
    if (_ref != ref) {
        if (_ref) {
            objc_release((id)_ref);
        }
        _ref = ref;
        if (_ref) {
            objc_retain((id)_ref);
        }
    }
}

} // namespace util

CF_ASSUME_NONNULL_END
