#pragma once

#include <CoreFoundation/CFBase.h>

CF_ASSUME_NONNULL_BEGIN

namespace util {

template <typename T> class tagged_ptr {
  private:
    enum : uintptr_t {
        mask = 0x1,
    };

    uintptr_t _value;

  public:
    tagged_ptr() : _value(0){};
    tagged_ptr(T *_Nullable value) : _value((uintptr_t)value){};
    tagged_ptr(T *_Nullable value, bool tag) : _value(((uintptr_t)value & ~0x1) | (tag ? 1 : 0)){};

    uintptr_t value() { return _value; };
    bool tag() { return static_cast<bool>(_value & 0x1); };
    tagged_ptr<T> with_tag(bool tag) { return tagged_ptr(get(), tag); };

    T *_Nullable get() { return reinterpret_cast<T *>(_value & ~mask); };
    const T *_Nullable get() const { return reinterpret_cast<const T *>(_value & ~mask); };

    bool operator==(nullptr_t) const noexcept { return _value == 0; };
    bool operator!=(nullptr_t) const noexcept { return _value != 0; };
};

} // namespace util

CF_ASSUME_NONNULL_END
