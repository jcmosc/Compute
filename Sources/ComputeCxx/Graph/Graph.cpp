#include "Graph.h"

#include "Attribute/AttributeType.h"
#include "Attribute/Node/Node.h"

namespace AG {

const AttributeType &Graph::attribute_ref(data::ptr<Node> attribute, const void *_Nullable *_Nullable ref_out) const {
    auto &type = attribute_type(attribute->type_id());
    if (ref_out) {
        void *self = ((char *)attribute.get() + type.attribute_offset());
        if (attribute->has_indirect_self()) {
            self = *(void **)self;
        }
        *ref_out = self;
    }
    return type;
}

} // namespace AG
