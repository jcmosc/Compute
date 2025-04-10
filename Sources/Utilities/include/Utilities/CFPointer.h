#pragma once

#include <CoreFoundation/CoreFoundation.h>
#include <utility>

CF_ASSUME_NONNULL_BEGIN

namespace util {

template <typename T>
concept AnyCFTypeRef = std::is_same_v<T, CFArrayRef> || std::is_same_v<T, CFBooleanRef> ||
                       std::is_same_v<T, CFDataRef> || std::is_same_v<T, CFDictionaryRef> ||
                       std::is_same_v<T, CFNumberRef> || std::is_same_v<T, CFStringRef> || std::is_same_v<T, CFTypeRef>;

template <AnyCFTypeRef T> class cf_ptr {
  private:
    CFTypeRef _storage;

    static inline CFTypeRef to_storage(T ref) { return (CFTypeRef)(ref); }
    static inline T from_storage(CFTypeRef storage) { return (T)storage; }

  public:
    constexpr cf_ptr() noexcept : _storage(nullptr) {}
    constexpr cf_ptr(std::nullptr_t) noexcept : _storage(nullptr) {}

    explicit cf_ptr(T ref) : _storage(to_storage(ref)) {
        if (_storage) {
            CFRetain(_storage);
        }
    }

    ~cf_ptr() {
        if (_storage) {
            CFRelease(_storage);
        }
    }

    // Copy and move constructors

    cf_ptr(const cf_ptr &other) noexcept : _storage(other._storage) {
        if (_storage) {
            CFRetain(_storage);
        }
    };

    cf_ptr(cf_ptr &&other) noexcept : _storage(std::exchange(other._storage, nullptr)) {};

    // Copy and move assignment operators

    cf_ptr &operator=(const cf_ptr &other) noexcept {
        if (this != &other) {
            if (_storage) {
                CFRelease(_storage);
            }
            _storage = other._storage;
            if (_storage) {
                CFRetain(_storage);
            }
        }
        return *this;
    };

    cf_ptr &operator=(cf_ptr &&other) noexcept {
        if (this != &other) {
            if (_storage) {
                CFRelease(_storage);
            }
            _storage = other._storage;
            other._storage = nullptr;
        }
        return *this;
    }

    // Modifiers

    void reset() noexcept { reset(nullptr); }

    void reset(T ref = nullptr) noexcept {
        if (_storage != ref) {
            if (_storage) {
                CFRelease(_storage);
            }
            _storage = to_storage(ref);
            if (_storage) {
                CFRetain(_storage);
            }
        }
    }

    // Observers

    T get() const noexcept { return from_storage(_storage); };

    explicit operator bool() const noexcept { return _storage != nullptr; }
};

} // namespace util

CF_ASSUME_NONNULL_END
