#pragma once

#include "ComputeCxx/IAGBase.h"

namespace IAG {
namespace swift {

IAG_NOINLINE
IAG_OPTNONE void *_Nullable retain(const void *_Nullable object) noexcept;

IAG_NOINLINE IAG_OPTNONE void release(const void *_Nullable object) noexcept;

} // namespace swift
} // namespace IAG
