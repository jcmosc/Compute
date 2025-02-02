#include "IndirectNode.h"

#include <cassert>

namespace AG {

MutableIndirectNode &IndirectNode::to_mutable() {
    assert(is_mutable());
    return static_cast<MutableIndirectNode &>(*this);
}

void IndirectNode::modify(WeakAttributeID source, uint32_t offset) {
    _source = source;
    _info.offset = offset;
}

} // namespace AG
