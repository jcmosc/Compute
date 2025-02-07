#pragma once

#include <CoreFoundation/CFBase.h>

#include "Data/Pointer.h"
#include "InputEdge.h"
#include "OutputEdge.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

namespace data {
class zone;
}
class AttributeType;
class Graph;

class NodeFlags {
  public:
    enum Flags1 : uint8_t {
        // TODO: check these
        IndirectAttribute = 1 << 0, // 0x1
        NilAttribute = 1 << 1,      // 0x2
    };
    enum Flags2 : uint8_t {};
    enum Flags3 : uint8_t {};
    enum Flags4 : uint8_t {
        HasIndirectSelf = 1 << 0,  // 0x01
        HasIndirectValue = 1 << 1, // 0x02

        Unknown0x04 = 1 << 2, // 0x04
        Unknown0x08 = 1 << 3, // 0x08
        Unknown0x10 = 1 << 4, // 0x10

        Unknown0x20 = 1 << 5, // 0x20 - initial  value
        Unknown0x40 = 1 << 6, // 0x40 - didn't call mark_changed
    };

  private:
    uint16_t _relative_offset;
    uint8_t _value3;
    uint8_t _value4;

  public:
    uint16_t relative_offset() const { return _relative_offset; };
    void set_relative_offset(uint16_t relative_offset) { _relative_offset = relative_offset; };

    // Flags 3
    uint8_t value3() { return _value3; };
    void set_value3(uint8_t value3) { _value3 = value3; };

    // Flags 4
    bool has_indirect_self() const { return _value4 & Flags4::HasIndirectSelf; }
    void set_has_indirect_self(bool value) {
        _value4 = (_value4 & ~Flags4::HasIndirectSelf) | (value ? Flags4::HasIndirectSelf : 0);
    };
    bool has_indirect_value() const { return _value4 & Flags4::HasIndirectValue; }
    void set_has_indirect_value(bool value) {
        _value4 = (_value4 & ~Flags4::HasIndirectValue) | (value ? Flags4::HasIndirectValue : 0);
    };
    bool value4_unknown0x04() { return _value4 & Flags4::Unknown0x04; };
    void set_value4_unknown0x04(bool value) {
        _value4 = (_value4 & ~Flags4::Unknown0x04) | (value ? Flags4::Unknown0x04 : 0);
    };
    bool value4_unknown0x10() { return _value4 & Flags4::Unknown0x10; };
    void set_value4_unknown0x10(bool value) {
        _value4 = (_value4 & ~Flags4::Unknown0x10) | (value ? Flags4::Unknown0x10 : 0);
    };
    bool value4_unknown0x20() { return _value4 & Flags4::Unknown0x20; };
    void set_value4_unknown0x20(bool value) {
        _value4 = (_value4 & ~Flags4::Unknown0x20) | (value ? Flags4::Unknown0x20 : 0);
    };
    bool value4_unknown0x40() { return _value4 & Flags4::Unknown0x40; };
    void set_value4_unknown0x40(bool value) {
        _value4 = (_value4 & ~Flags4::Unknown0x40) | (value ? Flags4::Unknown0x40 : 0);
    };
};

class Node {
  public:
    class State {
      public:
        enum : uint8_t {
            Dirty = 1 << 0,         // 0x01 // Unknown0 = 1 << 0,
            Pending = 1 << 1,       // 0x02 // Unknown1 = 1 << 1,
            UpdatesOnMain = 1 << 2, // 0x04 set from attribute type flags & 8
            Unknown3 = 1 << 3,      // 0x08 set from attribute type flags & 8

            ValueInitialized = 1 << 4, // 0x10
            SelfInitialized = 1 << 5,  // 0x20
            InUpdateStack = 1 << 6,    // 0x40
            Unknown7 = 1 << 7,         // 0x80

            IsEvaluating = InUpdateStack | Unknown7,
        };

      private:
        uint8_t _data;

      public:
        explicit constexpr State(uint8_t data) : _data(data){};
        uint8_t data() { return _data; };

        bool is_dirty() { return _data & Dirty; }
        State with_dirty(bool value) const { return State((_data & ~Dirty) | (value ? Dirty : 0)); };

        bool is_pending() { return _data & Pending; }
        State with_pending(bool value) const { return State((_data & ~Pending) | (value ? Pending : 0)); };

        bool updates_on_main() { return _data & UpdatesOnMain; }
        State with_updates_on_main(bool value) const { return State((_data & ~UpdatesOnMain) | (value ? UpdatesOnMain : 0)); };
        
        bool is_unknown3() { return _data & Unknown3; }
        State with_unknown3(bool value) const { return State((_data & ~Unknown3) | (value ? Unknown3 : 0)); };

        bool is_value_initialized() { return _data & ValueInitialized; };
        State with_value_initialized(bool value) const {
            return State((_data & ~ValueInitialized) | (value ? ValueInitialized : 0));
        };

        bool is_self_initialized() { return _data & SelfInitialized; };
        State with_self_initialized(bool value) const {
            return State((_data & ~SelfInitialized) | (value ? SelfInitialized : 0));
        };

        State with_in_update_stack(bool value) const {
            return State((_data & ~InUpdateStack) | (value ? InUpdateStack : 0));
        };

        bool is_evaluating() { return _data & InUpdateStack || _data & Unknown7; }
    };

  private:
    struct Info {
        unsigned int state : 8;
        unsigned int type_id : 24;
    };
    static_assert(sizeof(Info) == 4);

    struct EdgeInfo {
        unsigned int flags : 5;
        unsigned int num_edges : 11;
        unsigned int other_flag : 16;
    };
    static_assert(sizeof(EdgeInfo) == 4);

    Info _info;
    NodeFlags _flags;
    data::ptr<void> _value;

    EdgeInfo _inputs_info;
    data::ptr<InputEdge> _inputs;
    EdgeInfo _outputs_info;
    data::ptr<OutputEdge> _outputs;

  public:
    Node(State state, uint32_t type_id, uint8_t flags4);

    State state() { return State(_info.state); };
    void set_state(State state) { _info.state = state.data(); };
    uint32_t type_id() const { return uint32_t(_info.type_id); };

    NodeFlags &flags() { return _flags; };

    void *get_self(const AttributeType &type); // TODO: inline
    void update_self(const Graph &graph, void *new_self);
    void destroy_self(const Graph &graph);

    void *get_value();
    void allocate_value(Graph &graph, data::zone &zone);
    void destroy_value(Graph &graph);

    void destroy(Graph &graph);

    ConstInputEdgeArrayRef inputs() {
        return {
            _inputs.get(),
            _inputs_info.num_edges,
        };
    };

    ConstOutputEdgeArrayRef outputs() {
        return {
            _outputs.get(),
            _outputs_info.num_edges,
        };
    };
};

static_assert(sizeof(Node) == 0x1c);

} // namespace AG

CF_ASSUME_NONNULL_END
