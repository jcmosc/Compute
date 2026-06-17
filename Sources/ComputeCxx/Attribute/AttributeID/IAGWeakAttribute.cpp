#include "ComputeCxx/IAGWeakAttribute.h"
#include "AttributeID.h"
#include "Data/Zone.h"
#include "Subgraph/Subgraph.h"
#include "WeakAttributeID.h"

IAGWeakAttribute IAGCreateWeakAttribute(IAGAttribute attribute) {
    auto attribute_id = IAG::AttributeID(attribute);
    if (!attribute_id || attribute_id.is_nil()) {
        return IAGWeakAttribute({attribute_id, 0});
    }

    auto subgraph_id = attribute_id.subgraph()->subgraph_id();
    return IAGWeakAttribute({attribute_id, uint32_t(subgraph_id)});
}

// TODO: add test that evaluate checks deleted bit in zone id
IAGAttribute IAGWeakAttributeGetAttribute(IAGWeakAttribute weak_attribute) {
    auto weak_attribute_id = IAG::WeakAttributeID(weak_attribute);
    return weak_attribute_id.evaluate();
}
