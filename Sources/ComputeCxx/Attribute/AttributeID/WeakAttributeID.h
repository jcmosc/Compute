#pragma once

#include "AttributeID.h"
#include "ComputeCxx/IAGBase.h"
#include "ComputeCxx/IAGWeakAttribute.h"

IAG_ASSUME_NONNULL_BEGIN

namespace IAG {

class WeakAttributeID {
  private:
    AttributeID _identifier;
    uint32_t _seed;

  public:
    WeakAttributeID(AttributeID identifier, uint32_t seed) : _identifier(identifier), _seed(seed) {}

    operator IAGWeakAttribute() const { return *reinterpret_cast<const IAGWeakAttribute *>(this); }
    explicit constexpr WeakAttributeID(IAGWeakAttribute storage)
        : _identifier(storage._details.identifier), _seed(storage._details.seed) {}

    // MARK: Accessing data

    const AttributeID identifier() const { return _identifier; };
    uint32_t seed() const { return _seed; }

    /// Returns true if the attribute can no longer be evaluated.
    bool is_expired() const;

    /// Returns true when the attribute's subgraph has been freed (or reused).
    ///
    /// This method may return false if the attribute's subgraph has been
    /// invalidated but not yet destroyed.
    bool is_freed() const;

    /// Returns the attribute if it has not expired, otherwise returns the nil
    /// attribute.
    const AttributeID evaluate() const;
};

} // namespace IAG

IAG_ASSUME_NONNULL_END
