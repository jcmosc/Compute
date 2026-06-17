#include "ComputeCxx/IAGDescription.h"

#if TARGET_OS_MAC
const IAGDescriptionOption IAGDescriptionFormat = CFSTR("format");
const IAGDescriptionOption IAGDescriptionMaxFrames = CFSTR("max-frames");
const IAGDescriptionOption IAGDescriptionIncludeValues = CFSTR("include-values");
const IAGDescriptionOption IAGDescriptionTruncationLimit = CFSTR("truncation-limit");
#endif
