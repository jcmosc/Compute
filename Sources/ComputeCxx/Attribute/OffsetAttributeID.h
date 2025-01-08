#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "AttributeID.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class OffsetAttributeID {
  private:
    AttributeID _attribute;
    uint32_t _offset;

  public:
    OffsetAttributeID(const AttributeID &attribute, uint32_t offset = 0) : _attribute(attribute), _offset(offset){};

    const AttributeID &attribute() { return _attribute; };
    uint32_t offset() { return _offset; };
};

} // namespace AG

CF_ASSUME_NONNULL_END
