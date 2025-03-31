#pragma once

#include <CoreFoundation/CFBase.h>
#include <cassert>
#include <optional>
#include <stdint.h>

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

inline TraversalOptions operator|(TraversalOptions lhs, TraversalOptions rhs) {
    return static_cast<TraversalOptions>(lhs | rhs);
}

class AttributeID {
  private:
    static constexpr uint32_t KindMask = 0x3;

    uint32_t _value;

  public:
    enum Kind : uint32_t {
        Direct = 0,
        Indirect = 1 << 0,
        NilAttribute = 1 << 1,
    };

    explicit AttributeID(uint32_t value) : _value(value){};
    AttributeID(data::ptr<Node> node) : _value(node | Kind::Direct){};
    AttributeID(data::ptr<IndirectNode> indirect_node) : _value(indirect_node | Kind::Indirect){};
    static AttributeID make_nil() { return AttributeID(Kind::NilAttribute); };

    operator bool() const { return _value == 0; };

    uint32_t value() const { return _value; };

    Kind kind() const { return Kind(_value & KindMask); };
    AttributeID with_kind(Kind kind) const { return AttributeID((_value & ~KindMask) | kind); };
    AttributeID without_kind() const { return AttributeID((_value & ~KindMask)); };

    bool is_direct() const { return kind() == Kind::Direct; };
    bool is_indirect() const { return kind() == Kind::Indirect; };
    bool is_nil() const { return kind() == Kind::NilAttribute; }; // TODO: return true if whole thing is zero?

    // TODO: make these data::ptr<>
    Node &to_node() const {
        assert(is_direct());
        return *data::ptr<Node>(_value & ~KindMask);
    };
    data::ptr<Node> to_node_ptr() const {
        assert(is_direct());
        return data::ptr<Node>(_value & ~KindMask);
    };

    IndirectNode &to_indirect_node() const {
        assert(is_indirect());
        return *data::ptr<IndirectNode>(_value & ~KindMask);
    };
    data::ptr<IndirectNode> to_indirect_node_ptr() const {
        assert(is_indirect());
        return data::ptr<IndirectNode>(_value & ~KindMask);
    };
    data::ptr<MutableIndirectNode> to_mutable_indirect_node_ptr() const {
        assert(is_indirect()); // XXX: no assertion that it is mutable
        return data::ptr<MutableIndirectNode>(_value & ~KindMask);
    };

    Subgraph *_Nullable subgraph() const { return reinterpret_cast<Subgraph *_Nullable>(page_ptr()->zone); }

    data::ptr<data::page> page_ptr() const { return data::ptr<void>(_value).page_ptr(); };

    // Value metadata
    std::optional<size_t> size() const;

    // Graph traversal
    bool traverses(AttributeID other, TraversalOptions options) const;
    OffsetAttributeID resolve(TraversalOptions options) const;
    OffsetAttributeID resolve_slow(TraversalOptions options) const;
};

class RelativeAttributeID {
  private:
    uint16_t _value;
};

extern AttributeID AttributeIDNil;

} // namespace AG

CF_ASSUME_NONNULL_END
