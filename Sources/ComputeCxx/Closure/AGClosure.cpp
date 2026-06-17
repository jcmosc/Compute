#include "ComputeCxx/AGClosure.h"

#include <swift/Runtime/HeapObject.h>

AGClosureStorage AGRetainClosure(const void *thunk, const void *_Nullable context) {
    const void *retained_context = context;
    if (context) {
        void *mutable_context = const_cast<void *>(context);
        retained_context = ::swift::swift_retain(reinterpret_cast<::swift::HeapObject *>(mutable_context)); 
    }
    return AGClosureStorage((void *)thunk, retained_context);
}

void AGReleaseClosure(AGClosureStorage closure) {
    if (closure.context) {
        void *mutable_context = const_cast<void *>(closure.context);
        ::swift::swift_release(reinterpret_cast<::swift::HeapObject *>(mutable_context));
    }
}
