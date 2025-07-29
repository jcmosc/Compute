#include "ComputeCxx/AGClosure.h"

#include <swift/Runtime/HeapObject.h>

AGClosureStorage AGRetainClosure(void (*thunk)(void *_Nullable context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                                 void *_Nullable context) {
    const void *retained_context = ::swift::swift_retain(reinterpret_cast<::swift::HeapObject *>(context));
    return AGClosureStorage((void *)thunk, retained_context);
}

void AGReleaseClosure(AGClosureStorage closure) {
    void *context = const_cast<void *>(closure.context);
    ::swift::swift_release(reinterpret_cast<::swift::HeapObject *>(context));
}
