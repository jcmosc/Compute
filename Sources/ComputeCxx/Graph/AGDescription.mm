#include "ComputeCxx/AGDescription.h"

#if TARGET_OS_MAC
const AGDescriptionOption AGDescriptionFormat = CFSTR("format");
const AGDescriptionOption AGDescriptionMaxFrames = CFSTR("max-frames");
const AGDescriptionOption AGDescriptionIncludeValues = CFSTR("include-values");
const AGDescriptionOption AGDescriptionTruncationLimit = CFSTR("truncation-limit");
#endif
