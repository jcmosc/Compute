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
    ptr<page> next;
    uint32_t total;
    uint32_t in_use;
    uint16_t relative_offset_1;
    uint16_t relative_offset_2;
};
static_assert(sizeof(page) == 0x18);

} // namespace data
} // namespace AG

CF_ASSUME_NONNULL_END
