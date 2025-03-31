#include "AGUniqueID.h"

uint64_t AGMakeUniqueID() {
    static uint64_t counter = 0;
    return counter++;
}
