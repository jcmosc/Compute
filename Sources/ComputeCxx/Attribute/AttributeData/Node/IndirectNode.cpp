#include "IndirectNode.h"

#include <cassert>

namespace AG {

MutableIndirectNode &IndirectNode::to_mutable() {
    assert(is_mutable());
    return static_cast<MutableIndirectNode &>(*this);
}

const MutableIndirectNode &IndirectNode::to_mutable() const {
    assert(is_mutable());
    return static_cast<const MutableIndirectNode &>(*this);
}

void IndirectNode::modify(WeakAttributeID source, size_t size) {
    _source = source;
    _size = size;
}

} // namespace AG
