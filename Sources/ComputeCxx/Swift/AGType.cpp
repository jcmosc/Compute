#include "AGType.h"

#include <CoreFoundation/CFString.h>

#include "Metadata.h"

CFStringRef AGTypeDescription(AGTypeID typeID) {
    CFMutableStringRef description = CFStringCreateMutable(kCFAllocatorDefault, 0);
    auto type = reinterpret_cast<const AG::swift::metadata *>(typeID);
    type->append_description(description);
    CFAutorelease(description);
    return description;
}
