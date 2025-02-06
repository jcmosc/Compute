#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "AttributeID.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class WeakAttributeID {
  private:
    AttributeID _attribute;
    uint32_t _zone_id;

  public:
    WeakAttributeID(AttributeID attribute, uint32_t zone_id) : _attribute(attribute), _zone_id(zone_id){};

    bool expired() const;
    const AttributeID &attribute() const;
    
    /// Returns the attribute it is has not expired, otherwise returns the nil attribute.
    const AttributeID &evaluate() const;
};

} // namespace AG

CF_ASSUME_NONNULL_END
