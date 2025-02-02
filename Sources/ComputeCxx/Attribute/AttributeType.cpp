#include "AttributeType.h"

#include "Attribute/Node/Node.h"

namespace AG {

void AttributeType::update_attribute_offset() {
    uint32_t alignment_mask = uint32_t(self_metadata().getValueWitnesses()->getAlignmentMask());
    _attribute_offset = (sizeof(Node) + alignment_mask) & ~alignment_mask;
}

static_assert(sizeof(Node) == 0x1c);

} // namespace AG
