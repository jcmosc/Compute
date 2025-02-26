#include "AGComparison.h"

#include "AGComparison-Private.h"
#include "Layout/LayoutDescriptor.h"
#include "Swift/ContextDescriptor.h"
#include "Swift/Metadata.h"

const void *AGComparisonStateGetDestination(AGComparisonState state) { return state->destination; }

const void *AGComparisonStateGetSource(AGComparisonState state) { return state->source; }

AGFieldRange AGComparisonStateGetFieldRange(AGComparisonState state) { return state->field_range; }

AGTypeID AGComparisonStateGetFieldType(AGComparisonState state) { return state->field_type; }

bool AGCompareValues(const void *destination, const void *source, AGTypeID type_id, AGComparisonOptions options) {
    auto type = reinterpret_cast<const AG::swift::metadata *>(type_id);
    auto layout = AG::LayoutDescriptor::fetch(*type, options, 0);
    if (layout == AG::ValueLayoutEmpty) {
        layout = nullptr;
    }
    AG::LayoutDescriptor::compare(layout, (const unsigned char *)destination, (const unsigned char *)source,
                                  type->vw_size(), options);
}

const unsigned char *AGPrefetchCompareValues(AGTypeID type_id, AGComparisonOptions options, uint32_t priority) {
    auto type = reinterpret_cast<const AG::swift::metadata *>(type_id);
    return AG::LayoutDescriptor::fetch(*type, options, priority);
}

void AGOverrideComparisonForTypeDescriptor(void *descriptor, AGComparisonMode mode) {
    AG::LayoutDescriptor::add_type_descriptor_override(
        reinterpret_cast<const AG::swift::context_descriptor *>(descriptor),
        AG::LayoutDescriptor::ComparisonMode(mode));
}
