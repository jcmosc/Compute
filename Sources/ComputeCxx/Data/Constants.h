#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

CF_ASSUME_NONNULL_BEGIN

namespace AG {
namespace data {

constexpr uint32_t page_size = 0x200;
constexpr uint32_t page_alignment_mask = page_size - 1;

} // namespace data
} // namespace AG

CF_ASSUME_NONNULL_END
