#include "AGWeakAttribute.h"

#include "Attribute/AttributeID.h"
#include "Attribute/WeakAttributeID.h"
#include "Data/Zone.h"

AGWeakAttribute AGCreateWeakAttribute(AGAttribute attribute) {
    auto attribute_id = AG::AttributeID(attribute);
    if (attribute_id.without_kind() == 0) {
        return AG::WeakAttributeID(attribute_id, 0).to_opaque_value();
    }

    auto zone_id = attribute_id.to_node_ptr().page_ptr()->zone->info().zone_id();
    return AG::WeakAttributeID(attribute_id, zone_id).to_opaque_value();
}

AGAttribute AGWeakAttributeGetAttribute(AGWeakAttribute attribute) {
    auto attribute_id = AG::WeakAttributeID(attribute);
    return attribute_id.evaluate();
}
