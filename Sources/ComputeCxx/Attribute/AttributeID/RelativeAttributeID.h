#pragma once

#include "AttributeID.h"
#include "Data/Pointer.h"

namespace AG {

class RelativeAttributeID {
  private:
    uint16_t _value;

  public:
    explicit constexpr RelativeAttributeID(std::nullptr_t = nullptr) : _value(0) {};
    explicit constexpr RelativeAttributeID(uint16_t value) : _value(value) {};
    constexpr RelativeAttributeID(AttributeID attribute)
        : _value(((attribute & ~AttributeID::KindMask) - attribute.page_ptr().offset()) | attribute.kind()) {};

    operator uint16_t() const { return _value; }

    // MARK: Operators

    bool operator==(const RelativeAttributeID &other) const { return _value == other._value; }
    bool operator!=(const RelativeAttributeID &other) const { return _value != other._value; }

    // MARK: Accessing data

    AttributeID resolve(data::ptr<data::page> page_ptr) { return AttributeID(page_ptr.offset() + _value); }
};

} // namespace AG
