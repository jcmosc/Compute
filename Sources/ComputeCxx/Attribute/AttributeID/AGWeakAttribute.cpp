#include "ComputeCxx/AGWeakAttribute.h"
#include "AttributeID.h"
#include "Data/Zone.h"
#include "Subgraph/Subgraph.h"
#include "WeakAttributeID.h"

AGWeakAttribute AGCreateWeakAttribute(AGAttribute attribute) {
    auto attribute_id = AG::AttributeID(attribute);
    if (!attribute_id || attribute_id.is_nil()) {
        return AGWeakAttribute({attribute_id, 0});
    }

    auto subgraph_id = attribute_id.subgraph()->subgraph_id();
    return AGWeakAttribute({attribute_id, uint32_t(subgraph_id)});
}

// TODO: add test that evaluate checks deleted bit in zone id
AGAttribute AGWeakAttributeGetAttribute(AGWeakAttribute weak_attribute) {
    auto weak_attribute_id = AG::WeakAttributeID(weak_attribute);
    return weak_attribute_id.evaluate();
}
