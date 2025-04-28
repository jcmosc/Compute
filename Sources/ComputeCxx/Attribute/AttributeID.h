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
    friend RelativeAttributeID;

  private:
    static constexpr uint32_t KindMask = 0x3;

    uint32_t _value;

    explicit constexpr AttributeID(uint32_t value) : _value(value) {};

  public:
    enum Kind : uint32_t {
        Direct = 0,
        Indirect = 1 << 0,
        NilAttribute = 1 << 1,
    };

    explicit constexpr AttributeID() : _value(0) {};
    explicit AttributeID(data::ptr<Node> node) : _value(node.offset() | Kind::Direct) {};
    explicit AttributeID(data::ptr<IndirectNode> indirect_node) : _value(indirect_node.offset() | Kind::Indirect) {};
    explicit AttributeID(data::ptr<MutableIndirectNode> indirect_node)
        : _value(indirect_node.offset() | Kind::Indirect) {};

    operator AGAttribute() const { return _value; }
    static constexpr AttributeID from_storage(uint32_t value) { return AttributeID(value); }

    //

    // MARK: Operators

    bool operator==(const AttributeID &other) const { return _value == other._value; }
    bool operator!=(const AttributeID &other) const { return _value != other._value; }

    bool operator<(const AttributeID &other) const { return _value < other._value; }

    explicit operator bool() const { return _value != 0; }

    // MARK: Accessing zone data

    //    data::ptr<void> as_ptr() const { return data::ptr<void>(_value); };

    //    uint32_t value() const { return _value; }
    //    explicit operator bool() const { return _value == 0; };

    data::page &page() const {
        assert(_value);
        return *data::ptr<void>(_value).page_ptr();
    };

    data::ptr<data::page> page_ptr() const { return data::ptr<void>(_value).page_ptr(); };

    void validate_data_offset() const { data::ptr<void>(_value).assert_valid(); };

    // MARK: Relative

    RelativeAttributeID to_relative() const;

    // MARK: Accessing graph data

    Kind kind() const { return Kind(_value & KindMask); };
    AttributeID with_kind(Kind kind) const { return AttributeID((_value & ~KindMask) | kind); };

    bool is_direct() const { return kind() == Kind::Direct; };
    bool is_indirect() const { return kind() == Kind::Indirect; };
    bool is_nil() const { return kind() == Kind::NilAttribute; }; // TODO: return true if whole thing is zero?

    bool has_value() const { return (_value & ~KindMask) != 0; }

    template <typename T> data::ptr<T> to_ptr() const { return data::ptr<T>(_value & ~KindMask); }
    template <> data::ptr<Node> to_ptr() const {
        assert(is_direct());
        return data::ptr<Node>(_value & ~KindMask);
    }
    template <> data::ptr<IndirectNode> to_ptr() const {
        assert(is_indirect());
        return data::ptr<IndirectNode>(_value & ~KindMask);
    }
    template <> data::ptr<MutableIndirectNode> to_ptr() const {
        assert(is_indirect());
        return data::ptr<MutableIndirectNode>(_value & ~KindMask);
    }

    Node &to_node() const { return *to_ptr<Node>(); };
    IndirectNode &to_indirect_node() const { return *to_ptr<IndirectNode>(); };

    Subgraph *_Nullable subgraph() const { return reinterpret_cast<Subgraph *_Nullable>(page().zone); }

    // MARK: Value metdata

    std::optional<size_t> size() const;

    // MARK: Graph traversal

    bool traverses(AttributeID other, TraversalOptions options) const;

    OffsetAttributeID resolve(TraversalOptions options) const;
    OffsetAttributeID resolve_slow(TraversalOptions options) const;
};

class RelativeAttributeID {
  private:
    uint16_t _value;

  public:
    constexpr RelativeAttributeID() : _value(0) {};
    constexpr RelativeAttributeID(nullptr_t) : _value(0) {};
    constexpr RelativeAttributeID(uint16_t value) : _value(value) {};

    uint16_t value() const { return _value; }

    bool operator==(const RelativeAttributeID &other) const { return _value == other._value; }
    bool operator!=(const RelativeAttributeID &other) const { return _value != other._value; }

    AttributeID resolve(data::ptr<data::page> page_ptr) { return AttributeID(page_ptr.offset() + _value); }
};

extern AttributeID AttributeIDNil;

} // namespace AG

CF_ASSUME_NONNULL_END
