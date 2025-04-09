#pragma once

#include <CoreFoundation/CFBase.h>

CF_ASSUME_NONNULL_BEGIN

namespace util {

template <typename T> class cf_ptr {
public:
    using cf_type = T;
    
private:
    cf_type _ref;
    
public:
    cf_ptr() : _ref(nullptr){};
    cf_ptr(cf_type ref) : _ref(ref) {
        if (_ref) {
            CFRetain(_ref);
        }
    }
    ~cf_ptr() {
        if (_ref) {
            CFRelease(_ref);
        }
    }
    
    // Copy and move constructors
    cf_ptr(const cf_ptr &other) : _ref(other._ref) {
        if (_ref) {
            CFRetain(_ref);
        }
    }
    cf_ptr(cf_ptr &&other) noexcept : _ref(other._ref) { other._ref = nullptr; }
    
    // Copy and move assignment operators
    cf_ptr &operator=(const cf_ptr &other) {
        if (this != &other) {
            if (_ref) {
                CFRelease(_ref);
            }
            _ref = other._ref;
            if (_ref) {
                CFRetain(_ref);
            }
        }
        return *this;
    }
    cf_ptr &operator=(cf_ptr &&other) noexcept {
        if (this != &other) {
            if (_ref) {
                CFRelease(_ref);
            }
            _ref = other._ref;
            other._ref = nullptr;
        }
        return *this;
    }
    
    // Modifiers
    void reset(cf_type ref = nullptr) {
        if (_ref != ref) {
            if (_ref) {
                CFRelease(_ref);
            }
            _ref = ref;
            if (_ref) {
                CFRetain(_ref);
            }
        }
    }
    
    // Observers
    cf_type get() const noexcept { return _ref; };
    
    // Conversions
    operator cf_type() const { return _ref; }
};

} // namespace util

CF_ASSUME_NONNULL_END
