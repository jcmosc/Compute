#include "ComputeCxx/AGUniqueID.h"

AGUniqueID AGMakeUniqueID() {
    static uint64_t counter = 0;
    return ++counter;
}
