#pragma once

#include <CoreFoundation/CFBase.h>
#include <swift/Runtime/Metadata.h>

CF_ASSUME_NONNULL_BEGIN

namespace AG {
namespace swift {

using opaque_value = ::swift::OpaqueValue;

class metadata : public ::swift::Metadata {};

} // namespace swift
} // namespace AG

CF_ASSUME_NONNULL_END
