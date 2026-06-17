#pragma once

#include "ComputeCxx/IAGBase.h"

IAG_ASSUME_NONNULL_BEGIN

namespace IAG {
namespace data {

constexpr uint32_t page_size = 0x200;
constexpr uint32_t page_alignment_mask = page_size - 1;

} // namespace data
} // namespace IAG

IAG_ASSUME_NONNULL_END
