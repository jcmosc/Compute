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
    uint32_t _subgraph_id;

  public:
    WeakAttributeID(AttributeID attribute, uint32_t subgraph_id) : _attribute(attribute), _subgraph_id(subgraph_id) {};

    AGWeakAttribute to_cf() const { return AGWeakAttribute(_attribute, _subgraph_id); };
    static WeakAttributeID from_cf(AGWeakAttribute data) {
        return WeakAttributeID(AttributeID::from_storage(data.attribute), data.subgraph_id);
    };

    const AttributeID &attribute() const { return _attribute; };
    uint32_t subgraph_id() const { return _subgraph_id; };

    bool expired() const;

    /// Returns the attribute it is has not expired, otherwise returns the nil attribute.
    const AttributeID &evaluate() const;
};

} // namespace AG

CF_ASSUME_NONNULL_END
