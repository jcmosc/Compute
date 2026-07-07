#include "ComputeCxx/IAGClosure.h"

#include "Swift/HeapObject.h"

IAGClosureStorage IAGRetainClosure(const void *thunk, const void *_Nullable context) {
    const void *retained_context = context ? IAG::swift::retain(context) : nullptr;
    return IAGClosureStorage((void *)thunk, retained_context);
}

void IAGReleaseClosure(IAGClosureStorage closure) {
    if (closure.context) {
        IAG::swift::release(closure.context);
    }
}
