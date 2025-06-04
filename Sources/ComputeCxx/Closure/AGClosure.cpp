#include "AGClosure.h"

#include <swift/Runtime/HeapObject.h>

AGClosureStorage AGRetainClosure(AGClosureStorage closure) {
    ::swift::swift_retain(reinterpret_cast<::swift::HeapObject *>(closure.context));
}

void AGReleaseClosure(AGClosureStorage closure) {
    ::swift::swift_release(reinterpret_cast<::swift::HeapObject *>(closure.context));
}
