#pragma once

#include <CoreFoundation/CFBase.h>
#include <cassert>
#include <optional>
#include <stdint.h>

#include "AGAttribute.h"
#include "Data/Page.h"
#include "Data/Pointer.h"
#include "Data/Zone.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Subgraph;
class Node;
class IndirectNode;
class MutableIndirectNode;
class OffsetAttributeID;

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
    enum Kind : uint32_t {
        Node = 0,
        IndirectNode = 1 << 0,
        NilAttribute = 1 << 1,
    };

  private:
    uint32_t _value;

    OffsetAttributeID resolve_slow(TraversalOptions options) const;

  public:
    static constexpr uint32_t KindMask = 0x3;

    explicit constexpr AttributeID() : _value(0) {};
    explicit AttributeID(data::ptr<class Node> node) : _value(node.offset() | Kind::Node) {};
    explicit AttributeID(data::ptr<class IndirectNode> indirect_node)
        : _value(indirect_node.offset() | Kind::IndirectNode) {};
    explicit AttributeID(data::ptr<class MutableIndirectNode> indirect_node)
        : _value(indirect_node.offset() | Kind::IndirectNode) {};

    operator AGAttribute() const { return _value; }
    explicit constexpr AttributeID(AGAttribute storage) : _value(storage) {}

    Kind kind() const { return Kind(_value & KindMask); };
    AttributeID with_kind(Kind kind) const { return AttributeID((_value & ~KindMask) | kind); };

    // MARK: Operators

    bool operator==(const AttributeID &other) const { return _value == other._value; }
    bool operator!=(const AttributeID &other) const { return _value != other._value; }

    bool operator<(const AttributeID &other) const { return _value < other._value; }

    explicit operator bool() const { return _value != 0; }
    //    bool has_value() const { return (_value & ~KindMask) != 0; }

    // MARK: Accessing zone data

    template <typename T> data::ptr<T> to_ptr() const { return data::ptr<T>(_value & ~KindMask); }
    template <> data::ptr<class Node> to_ptr() const {
        assert(is_node());
        return data::ptr<class Node>(_value & ~KindMask);
    }
    template <> data::ptr<class IndirectNode> to_ptr() const {
        assert(is_indirect_node());
        return data::ptr<class IndirectNode>(_value & ~KindMask);
    }
    template <> data::ptr<class MutableIndirectNode> to_ptr() const {
        assert(is_indirect_node());
        return data::ptr<class MutableIndirectNode>(_value & ~KindMask);
    }

    data::page &page() const {
        assert(_value);
        return *data::ptr<void>(_value).page_ptr();
    };

    data::ptr<data::page> page_ptr() const { return data::ptr<void>(_value).page_ptr(); };

    void validate_data_offset() const { data::ptr<void>(_value).assert_valid(); };

    // MARK: Accessing graph data

    std::optional<size_t> size() const;

    bool is_node() const { return kind() == Kind::Node; };
    bool is_indirect_node() const { return kind() == Kind::IndirectNode; };
    bool is_nil() const { return kind() == Kind::NilAttribute; };

    class Node &to_node() const { return *to_ptr<class Node>(); };
    class IndirectNode &to_indirect_node() const { return *to_ptr<class IndirectNode>(); };

    Subgraph *_Nullable subgraph() const { return reinterpret_cast<Subgraph *_Nullable>(page().zone); }

    // MARK: Graph traversal

    bool traverses(AttributeID other, TraversalOptions options) const;
    OffsetAttributeID resolve(TraversalOptions options) const;
};

} // namespace AG

CF_ASSUME_NONNULL_END
