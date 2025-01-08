#pragma once

#include <CoreFoundation/CFBase.h>

#include "Runtime/Metadata.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class AttributeType {
  private:
    swift::metadata *_self_metadata;
    swift::metadata *_value_metadata;

  public:
    const swift::metadata &self_metadata() const { return *_self_metadata; };
    const swift::metadata &value_metadata() const { return *_value_metadata; };
};

} // namespace AG

CF_ASSUME_NONNULL_END
