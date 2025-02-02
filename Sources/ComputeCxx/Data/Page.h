#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

#include "Pointer.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {
namespace data {

class zone;

struct page {
    zone *zone; // 0
    ptr<page> previous; // 8
    uint32_t total; // 12
    uint32_t in_use; // 16
    uint16_t relative_offset_1; // 0x14/20
    uint16_t relative_offset_2; // 0x16/22
};
static_assert(sizeof(page) == 0x18);

} // namespace data
} // namespace AG

CF_ASSUME_NONNULL_END
