#include "AGClosure.h"

#include <swift/Runtime/HeapObject.h>

void AGRetainClosure(AGClosureStorage *closure) { ::swift::swift_retain((::swift::HeapObject *)closure->context); }

void AGReleaseClosure(AGClosureStorage *closure) { ::swift::swift_release((::swift::HeapObject *)closure->context); }
