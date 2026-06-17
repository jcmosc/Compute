#pragma once

#include "ComputeCxx/IAGBase.h"

#include "AttributeID.h"

IAG_ASSUME_NONNULL_BEGIN

namespace IAG {

class OffsetAttributeID {
  private:
    AttributeID _attribute;
    uint32_t _offset;

  public:
    OffsetAttributeID(AttributeID attribute, uint32_t offset = 0) : _attribute(attribute), _offset(offset) {};

    // MARK: Accessing data

    const AttributeID attribute() const { return _attribute; };
    uint32_t offset() const { return _offset; };
};

} // namespace IAG

IAG_ASSUME_NONNULL_END
