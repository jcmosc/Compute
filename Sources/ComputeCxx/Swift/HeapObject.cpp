#include "HeapObject.h"

#include <swift/Runtime/HeapObject.h>

namespace IAG {
namespace swift {

IAG_NOINLINE IAG_OPTNONE void *retain(const void *object) noexcept {
    void *mutable_object = const_cast<void *>(object);
    return ::swift::swift_retain(reinterpret_cast<::swift::HeapObject *>(mutable_object));
}

IAG_NOINLINE IAG_OPTNONE void release(const void *object) noexcept {
    void *mutable_object = const_cast<void *>(object);
    ::swift::swift_release(reinterpret_cast<::swift::HeapObject *>(mutable_object));
}

} // namespace swift
} // namespace IAG
