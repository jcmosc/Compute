#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "AGWeakAttribute.h"
#include "AttributeID.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class WeakAttributeID {
  private:
    AttributeID _attribute;
    uint32_t _zone_id;

  public:
    WeakAttributeID(AGWeakAttribute opaque_value)
        : _attribute(opaque_value & 0xffffffff), _zone_id(opaque_value >> 0x20){};
    WeakAttributeID(AttributeID attribute, uint32_t zone_id) : _attribute(attribute), _zone_id(zone_id){};

    // TODO: rename
    uint64_t to_opaque_value() const { return _attribute.value() | ((uint64_t)_zone_id << 0x20); };

    const AttributeID &attribute() const { return _attribute; };
    uint32_t zone_id() const { return _zone_id; };

    bool expired() const;

    /// Returns the attribute it is has not expired, otherwise returns the nil attribute.
    const AttributeID &evaluate() const;
};

} // namespace AG

CF_ASSUME_NONNULL_END
