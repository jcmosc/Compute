#include "IAGComparison-Private.h"

#include "Comparison/LayoutDescriptor.h"
#include "Swift/ContextDescriptor.h"
#include "Swift/Metadata.h"

const void *IAGComparisonStateGetDestination(IAGComparisonState state) { return state->destination; }

const void *IAGComparisonStateGetSource(IAGComparisonState state) { return state->source; }

IAGFieldRange IAGComparisonStateGetFieldRange(IAGComparisonState state) { return state->field_range; }

IAGTypeID IAGComparisonStateGetFieldType(IAGComparisonState state) { return state->field_type; }

bool IAGCompareValues(const void *destination, const void *source, IAGTypeID type_id, IAGComparisonOptions options) {
    auto type = reinterpret_cast<const IAG::swift::metadata *>(type_id);
    auto layout = IAG::LayoutDescriptor::fetch(*type, options, 0);
    if (layout == IAG::ValueLayoutTrivial) {
        layout = nullptr;
    }
    return IAG::LayoutDescriptor::compare(layout, (const unsigned char *)destination, (const unsigned char *)source,
                                         type->vw_size(), options);
}

const unsigned char *IAGPrefetchCompareValues(IAGTypeID type_id, IAGComparisonOptions options, uint32_t priority) {
    auto type = reinterpret_cast<const IAG::swift::metadata *>(type_id);
    return IAG::LayoutDescriptor::fetch(*type, options, priority);
}

void IAGOverrideComparisonForTypeDescriptor(void *descriptor, IAGComparisonMode mode) {
    IAG::LayoutDescriptor::add_type_descriptor_override(
        reinterpret_cast<const IAG::swift::context_descriptor *>(descriptor), mode);
}
