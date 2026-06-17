#include <stdatomic.h>

#include <ComputeCxx/IAGUniqueID.h>

IAGUniqueID IAGMakeUniqueID() {
    static atomic_ulong counter = 1;
    return atomic_fetch_add_explicit(&counter, 1, memory_order_relaxed);
}
