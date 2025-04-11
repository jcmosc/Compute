#include "AGWeakAttribute.h"

#include "Attribute/AttributeID.h"
#include "Attribute/WeakAttributeID.h"
#include "Data/Zone.h"

AGWeakAttribute AGCreateWeakAttribute(AGAttribute attribute) {
    auto attribute_id = AG::AttributeID::from_storage(attribute);
    if (!attribute_id.has_value()) {
        return AG::WeakAttributeID(attribute_id, 0).to_cf();
    }

    auto zone_id = attribute_id.page().zone->info().zone_id();
    return AG::WeakAttributeID(attribute_id, zone_id).to_cf();
}

AGAttribute AGWeakAttributeGetAttribute(AGWeakAttribute attribute) {
    auto attribute_id = AG::WeakAttributeID::from_cf(attribute);
    return attribute_id.evaluate().to_storage();
}
