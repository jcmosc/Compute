#include "ComputeCxx/IAGDescription.h"

#if TARGET_OS_MAC
#ifdef __OBJC__
#define IAG_BRIDGE(v) (__bridge NSString *)(v)
#else
#define IAG_BRIDGE(v) (v)
#endif
const IAGDescriptionOption IAGDescriptionFormat = IAG_BRIDGE(CFSTR("format"));
const IAGDescriptionOption IAGDescriptionMaxFrames = IAG_BRIDGE(CFSTR("max-frames"));
const IAGDescriptionOption IAGDescriptionIncludeValues = IAG_BRIDGE(CFSTR("include-values"));
const IAGDescriptionOption IAGDescriptionTruncationLimit = IAG_BRIDGE(CFSTR("truncation-limit"));
#endif
