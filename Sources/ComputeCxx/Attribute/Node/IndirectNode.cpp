#include "IndirectNode.h"

#include <cassert>

namespace AG {

const MutableIndirectNode &IndirectNode::to_mutable() const {
    assert(is_mutable());
    return static_cast<const MutableIndirectNode &>(*this);
}

void IndirectNode::modify(WeakAttributeID source, size_t size) {
    _source = source;
    _info.size = uint32_t(size);
}

} // namespace AG
