#include "ComputeCxx/AGClosure.h"

#include <swift/Runtime/HeapObject.h>

AGClosureStorage AGRetainClosure(void (*closure)(void *_Nullable context AG_SWIFT_CONTEXT) AG_SWIFT_CC(swift),
                                 void *_Nullable closure_context) {
    void *retained_closure_context = ::swift::swift_retain(reinterpret_cast<::swift::HeapObject *>(closure_context));
    return AGClosureStorage((void *)closure, retained_closure_context);
}

void AGReleaseClosure(AGClosureStorage closure) {
    ::swift::swift_release(reinterpret_cast<::swift::HeapObject *>(closure.context));
}
