#include "AttributeID.h"

#include "Errors/Errors.h"

#include "AttributeType.h"
#include "Graph/Graph.h"
#include "Node/IndirectNode.h"
#include "Node/Node.h"
#include "OffsetAttributeID.h"
#include "Subgraph/Subgraph.h"

namespace AG {

AttributeID AttributeIDNil = AttributeID::make_nil();

std::optional<size_t> AttributeID::size() const {
    if (is_direct()) {
        const AttributeType &attribute_type = subgraph()->graph()->attribute_type(to_node().type_id());
        size_t size = attribute_type.value_metadata().vw_size();
        return std::optional<size_t>(size);
    }
    if (is_indirect()) {
        return to_indirect_node().size();
    }
    return std::optional<size_t>();
}

bool AttributeID::traverses(AttributeID other, TraversalOptions options) const {
    if (!is_indirect()) {
        return *this == other;
    }

    if (with_kind(Kind::Indirect) == other) {
        return true;
    }

    auto indirect_node = to_indirect_node();
    if (options & TraversalOptions::SkipMutableReference && indirect_node.is_mutable()) {
        return *this == other;
    }

    return indirect_node.source().attribute().traverses(other, options);
}

OffsetAttributeID AttributeID::resolve(TraversalOptions options) const {
    if (is_direct()) {
        return OffsetAttributeID(*this);
    }
    return resolve_slow(options);
}

OffsetAttributeID AttributeID::resolve_slow(TraversalOptions options) const {
    AttributeID result = *this;
    uint32_t offset = 0;
    while (result.is_indirect()) {
        if (offset == 0 && options & TraversalOptions::ReportIndirectionInOffset) {
            offset = 1;
        }

        auto indirect_node = to_indirect_node();

        if (indirect_node.is_mutable()) {
            if (options & TraversalOptions::SkipMutableReference) {
                return OffsetAttributeID(result, offset);
            }

            if (options & TraversalOptions::UpdateDependencies) {
                AttributeID dependency = indirect_node.to_mutable().dependency();
                if (dependency) {
                    auto subgraph = dependency.subgraph();
                    if (subgraph) {
                        subgraph->graph()->update_attribute(dependency, false);
                    }
                }
            }
        }

        if (options && TraversalOptions::EvaluateWeakReferences) {
            if (indirect_node.source().expired()) {
                if (options & TraversalOptions::AssertNotNil) {
                    precondition_failure("invalid indirect ref: %u", _value);
                }
                return OffsetAttributeID(AttributeID::make_nil());
            }
        }

        offset += indirect_node.offset();
        result = indirect_node.source().attribute();
    }

    if (options & TraversalOptions::AssertNotNil && !is_direct()) {
        precondition_failure("invalid attribute id: %u", _value);
    }

    return OffsetAttributeID(result, offset);
}

} // namespace AG
