#include "ComputeCxx/AGClosure.h"

#include <swift/Runtime/HeapObject.h>

AGClosureStorage AGRetainClosure(const void *thunk, const void *_Nullable context) {
    void *mutable_context = const_cast<void *>(context);
    const void *retained_context = ::swift::swift_retain(reinterpret_cast<::swift::HeapObject *>(mutable_context));
    return AGClosureStorage((void *)thunk, retained_context);
}

void AGReleaseClosure(AGClosureStorage closure) {
    void *context = const_cast<void *>(closure.context);
    ::swift::swift_release(reinterpret_cast<::swift::HeapObject *>(context));
}
