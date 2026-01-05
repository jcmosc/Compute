#pragma once

#include "AttributeID.h"
#include "ComputeCxx/AGBase.h"
#include "ComputeCxx/AGWeakAttribute.h"

AG_ASSUME_NONNULL_BEGIN

namespace AG {

class WeakAttributeID {
  private:
    AttributeID _identifier;
    uint32_t _seed;

  public:
    WeakAttributeID(AttributeID identifier, uint32_t seed) : _identifier(identifier), _seed(seed) {}

    operator AGWeakAttribute() const { return *reinterpret_cast<const AGWeakAttribute *>(this); }
    explicit constexpr WeakAttributeID(AGWeakAttribute storage)
        : _identifier(storage._details.identifier), _seed(storage._details.seed) {}

    // MARK: Accessing data

    const AttributeID identifier() const { return _identifier; };
    uint32_t seed() const { return _seed; }

    bool expired() const;

    /// Returns the attribute it is has not expired, otherwise returns the nil attribute.
    const AttributeID evaluate() const;
};

} // namespace AG

AG_ASSUME_NONNULL_END
