#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "Pointer.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {
namespace data {

class zone;

struct page {
    zone *zone;
    ptr<page> previous;
    uint32_t total;
    uint32_t in_use;
};

} // namespace data
} // namespace AG

CF_ASSUME_NONNULL_END
