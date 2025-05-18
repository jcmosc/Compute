#pragma once

#include <CoreFoundation/CFBase.h>
#include <cstddef>
#include <utility>
#include <objc/runtime.h>

CF_ASSUME_NONNULL_BEGIN

// Redeclare APIs from the Objective-C runtime.
// These functions are not available through public headers, but are guaranteed
// to exist on OS X >= 10.9 and iOS >= 7.0.
OBJC_EXPORT id objc_retain(id obj);
OBJC_EXPORT void objc_release(id obj);

namespace util {

template <typename T> class objc_ptr {
  private:
    id _storage;

    static inline id to_storage(T obj) { return (id)(obj); }
    static inline T from_storage(id storage) { return (T)storage; }

  public:
    constexpr objc_ptr() noexcept : _storage(nullptr) {}
    constexpr objc_ptr(std::nullptr_t) noexcept : _storage(nullptr) {}

    explicit objc_ptr(T obj) : _storage(to_storage(obj)) {
        if (_storage) {
            objc_retain(_storage);
        }
    }

    ~objc_ptr() {
        if (_storage) {
            objc_release(_storage);
        }
    }

    // Copy and move constructors

    objc_ptr(const objc_ptr &other) noexcept : _storage(other._storage) {
        if (_storage) {
            objc_retain(_storage);
        }
    }

    objc_ptr(objc_ptr &&other) noexcept : _storage(std::exchange(other._storage, nullptr)) {};

    // Copy and move assignment operators

    objc_ptr &operator=(const objc_ptr &other) noexcept {
        if (this != &other) {
            if (_storage) {
                objc_release(_storage);
            }
            _storage = other._storage;
            if (_storage) {
                objc_retain(_storage);
            }
        }
        return *this;
    }

    objc_ptr &operator=(objc_ptr &&other) noexcept {
        if (this != &other) {
            if (_storage) {
                objc_release(_storage);
            }
            _storage = other._storage;
            other._storage = nullptr;
        }
        return *this;
    }

    // Modifiers

    void reset() noexcept { reset(nullptr); }

    void reset(T obj = nullptr) noexcept {
        if (_storage != obj) {
            if (_storage) {
                objc_release(_storage);
            }
            _storage = obj;
            if (_storage) {
                objc_retain(_storage);
            }
        }
    }

    // Observers

    T get() const noexcept { return from_storage(_storage); };

    explicit operator bool() const noexcept { return _storage != nullptr; }
};

} // namespace util

CF_ASSUME_NONNULL_END
