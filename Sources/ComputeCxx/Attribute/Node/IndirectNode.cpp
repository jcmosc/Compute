#include "IndirectNode.h"

#include <cassert>

namespace AG {

const MutableIndirectNode &IndirectNode::to_mutable() const {
    assert(is_mutable());
    return static_cast<const MutableIndirectNode &>(*this);
}

} // namespace AG
