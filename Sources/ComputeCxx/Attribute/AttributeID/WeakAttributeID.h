#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "AttributeID.h"
#include "ComputeCxx/AGWeakAttribute.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class WeakAttributeID {
  private:
    AttributeID _attribute;
    uint32_t _subgraph_id;

  public:
    WeakAttributeID(AttributeID attribute, uint32_t subgraph_id) : _attribute(attribute), _subgraph_id(subgraph_id) {}

    operator AGWeakAttribute() const { return AGWeakAttribute(_attribute, _subgraph_id); }
    explicit constexpr WeakAttributeID(AGWeakAttribute storage)
        : _attribute(storage.attribute), _subgraph_id(storage.subgraph_id) {}

    // MARK: Accessing data

    const AttributeID attribute() const { return _attribute; };
    uint32_t subgraph_id() const { return _subgraph_id; }

    bool expired() const;

    /// Returns the attribute it is has not expired, otherwise returns the nil attribute.
    const AttributeID evaluate() const;
};

} // namespace AG

CF_ASSUME_NONNULL_END
