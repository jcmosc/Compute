#include "AGClosure.h"

#include <swift/Runtime/HeapObject.h>

struct AGClosureStorage {
    const void * _Nullable function;
    const void * _Nullable context;
};

AGClosureRef AGRetainClosure(AGClosureRef closure) {
    ::swift::swift_retain((::swift::HeapObject *)closure->context);
    return closure;
}

void AGReleaseClosure(AGClosureRef closure) {
    ::swift::swift_release((::swift::HeapObject *)closure->context);
}
