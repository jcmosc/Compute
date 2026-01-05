#pragma once

#include <cassert>
#include <optional>

#include "ComputeCxx/AGAttribute.h"
#include "ComputeCxx/AGBase.h"
#include "Data/Page.h"
#include "Data/Pointer.h"
#include "Data/Zone.h"

AG_ASSUME_NONNULL_BEGIN

namespace AG {

class Subgraph;
class Node;
class IndirectNode;
class MutableIndirectNode;
class OffsetAttributeID;
class RelativeAttributeID;

enum TraversalOptions : uint32_t {
    None = 0,

    /// Updates indirect node dependencies prior to traversing.
    UpdateDependencies = 1 << 0,

    /// Guarantees the resolved attribute is not nil, otherwise traps.
    AssertNotNil = 1 << 1,

    /// When set, only statically evaluable references are traversed.
    /// The returned attribute may be a mutable indirect node.
    SkipMutableReference = 1 << 2,

    /// When set, the returned offset will be 0 if no indirection was traversed,
    /// otherwise it will be the the actual offset + 1.
    ReportIndirectionInOffset = 1 << 3,

    /// When set and `AssertNotNil` is not also set, returns the nil attribute
    /// if any weak references evaluate to nil.
    EvaluateWeakReferences = 1 << 4,
};
inline TraversalOptions &operator|=(TraversalOptions &lhs, TraversalOptions rhs) {
    lhs = TraversalOptions(uint32_t(lhs) | uint32_t(rhs));
    return lhs;
}
inline TraversalOptions operator|(TraversalOptions lhs, TraversalOptions rhs) { return (lhs |= rhs); }

class AttributeID {
  public:
    friend RelativeAttributeID;

    enum Kind : uint32_t {
        Node = 0,
        IndirectNode = 1 << 0,
        NilAttribute = 1 << 1,
    };

  private:
    uint32_t _value;

    OffsetAttributeID resolve_slow(TraversalOptions options) const;

    Kind kind() const { return Kind(_value & KindMask); };
    AttributeID with_kind(Kind kind) const { return AttributeID((_value & ~KindMask) | kind); };

  public:
    static constexpr uint32_t KindMask = 0x3;

    explicit constexpr AttributeID(nullptr_t = nullptr) : _value(0) {};
    explicit AttributeID(data::ptr<class Node> node) : _value(node.offset() | Kind::Node) {};
    explicit AttributeID(data::ptr<class IndirectNode> indirect_node)
        : _value(indirect_node.offset() | Kind::IndirectNode) {};
    explicit AttributeID(data::ptr<class MutableIndirectNode> indirect_node)
        : _value(indirect_node.offset() | Kind::IndirectNode) {};

    operator AGAttribute() const { return _value; }
    explicit constexpr AttributeID(AGAttribute storage) : _value(storage) {}

    // MARK: Operators

    bool operator==(const AttributeID &other) const { return _value == other._value; }
    bool operator!=(const AttributeID &other) const { return _value != other._value; }

    bool operator<(const AttributeID &other) const { return _value < other._value; }

    explicit operator bool() const { return _value != 0; }

    // MARK: Accessing zone data

    data::page &page() const {
        assert(_value > KindMask);
        return *data::ptr<void>(_value).page_ptr();
    };

    data::ptr<data::page> page_ptr() const { return data::ptr<void>(_value).page_ptr(); };

    void validate_data_offset() const { data::ptr<void>(_value).assert_valid(); };

    // MARK: Accessing graph data

    std::optional<size_t> size() const;

    bool is_node() const { return kind() == Kind::Node; };
    bool is_indirect_node() const { return kind() == Kind::IndirectNode; };
    bool is_nil() const { return kind() == Kind::NilAttribute; };

    data::ptr<class Node> get_node() const {
        if (!is_node()) {
            return nullptr;
        }
        return data::ptr<class Node>(_value & ~KindMask);
    };

    data::ptr<class IndirectNode> get_indirect_node() const {
        if (!is_indirect_node()) {
            return nullptr;
        }
        return data::ptr<class IndirectNode>(_value & ~KindMask);
    };

    bool has_subgraph_flags() const;
    Subgraph *_Nullable subgraph() const { return reinterpret_cast<Subgraph *_Nullable>(page().zone); }

    // MARK: Graph traversal

    bool traverses(AttributeID other, TraversalOptions options) const;
    OffsetAttributeID resolve(TraversalOptions options) const;
};

} // namespace AG

AG_ASSUME_NONNULL_END
