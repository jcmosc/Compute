#pragma once

#include <CoreFoundation/CFBase.h>

#include "Data/Pointer.h"
#include "Data/Vector.h"
#include "Edge.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

namespace data {
class zone;
}
class AttributeType;
class Graph;

class NodeFlags {
  public:
    enum Flags3 : uint8_t {};
    enum Flags4 : uint8_t {
        HasIndirectSelf = 1 << 0,  // 0x01
        HasIndirectValue = 1 << 1, // 0x02

        InputsTraverseGraphContexts = 1 << 2, // 0x04
        InputsUnsorted = 1 << 3,              // 0x08
        Cacheable = 1 << 4,                   // 0x10

        Unknown0x20 = 1 << 5, // 0x20 - initial  value
        Unknown0x40 = 1 << 6, // 0x40 - didn't call mark_changed
    };

  private:
    uint16_t _relative_offset;
    uint8_t _value3;
    uint8_t _value4;

  public:
    NodeFlags(uint8_t value4 = 0) : _value4(value4){};

    uint16_t relative_offset() const { return _relative_offset; };
    void set_relative_offset(uint16_t relative_offset) { _relative_offset = relative_offset; };

    // Flags 3
    uint8_t value3() const { return _value3; };
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

    bool inputs_traverse_graph_contexts() { return _value4 & Flags4::InputsTraverseGraphContexts; };
    void set_inputs_traverse_graph_contexts(bool value) {
        _value4 = (_value4 & ~Flags4::InputsTraverseGraphContexts) | (value ? Flags4::InputsTraverseGraphContexts : 0);
    };
    bool inputs_unsorted() { return _value4 & Flags4::InputsUnsorted; };
    void set_inputs_unsorted(bool value) {
        _value4 = (_value4 & ~Flags4::InputsUnsorted) | (value ? Flags4::InputsUnsorted : 0);
    };
    bool cacheable() const { return _value4 & Flags4::Cacheable; };
    void set_cacheable(bool value) { _value4 = (_value4 & ~Flags4::Cacheable) | (value ? Flags4::Cacheable : 0); };
    bool value4_unknown0x20() const { return _value4 & Flags4::Unknown0x20; };
    void set_value4_unknown0x20(bool value) {
        _value4 = (_value4 & ~Flags4::Unknown0x20) | (value ? Flags4::Unknown0x20 : 0);
    };
    bool value4_unknown0x40() const { return _value4 & Flags4::Unknown0x40; };
    void set_value4_unknown0x40(bool value) {
        _value4 = (_value4 & ~Flags4::Unknown0x40) | (value ? Flags4::Unknown0x40 : 0);
    };
};

class Node {
  public:
    class State {
      public:
        enum : uint8_t {
            Dirty = 1 << 0,          // 0x01 // Unknown0 = 1 << 0,
            Pending = 1 << 1,        // 0x02 // Unknown1 = 1 << 1,
            MainThread = 1 << 2,     // 0x04 set from attribute type flags & 8
            MainThreadOnly = 1 << 3, // 0x08 set from attribute type flags & 8

            ValueInitialized = 1 << 4, // 0x10
            SelfInitialized = 1 << 5,  // 0x20
            Updating = 1 << 6,         // 0x40
            UpdatingCyclic = 1 << 7,   // 0x80

        };

      private:
        uint8_t _data;

      public:
        explicit constexpr State(uint8_t data = 0) : _data(data){};
        uint8_t data() { return _data; };

        bool is_dirty() { return _data & Dirty; }
        State with_dirty(bool value) const { return State((_data & ~Dirty) | (value ? Dirty : 0)); };

        bool is_pending() { return _data & Pending; }
        State with_pending(bool value) const { return State((_data & ~Pending) | (value ? Pending : 0)); };

        bool is_main_thread() { return _data & MainThread; }
        State with_main_thread(bool value) const { return State((_data & ~MainThread) | (value ? MainThread : 0)); };

        bool is_main_thread_only() { return _data & MainThreadOnly; }
        State with_main_thread_only(bool value) const {
            return State((_data & ~MainThreadOnly) | (value ? MainThreadOnly : 0));
        };

        bool is_value_initialized() { return _data & ValueInitialized; };
        State with_value_initialized(bool value) const {
            return State((_data & ~ValueInitialized) | (value ? ValueInitialized : 0));
        };

        bool is_self_initialized() { return _data & SelfInitialized; };
        State with_self_initialized(bool value) const {
            return State((_data & ~SelfInitialized) | (value ? SelfInitialized : 0));
        };

        State with_updating(bool value) const { return State((_data & ~Updating) | (value ? Updating : 0)); };

        bool is_updating() { return _data & (Updating | UpdatingCyclic); }
        bool is_updating_cyclic() { return (_data & (Updating | UpdatingCyclic)) == (Updating | UpdatingCyclic); }
        uint8_t update_count() const {
            return _data >> 6;
        }
    };

  private:
    struct Info {
        unsigned int state : 8;
        unsigned int type_id : 24;
    };
    static_assert(sizeof(Info) == 4);

    Info _info;
    NodeFlags _flags;
    data::ptr<void> _value;

    data::vector<InputEdge> _inputs;
    data::vector<OutputEdge> _outputs;

  public:
    Node(State state, uint32_t type_id, uint8_t flags4) {
        _info.state = state.data();
        _info.type_id = type_id;
        _flags = NodeFlags(flags4);
    };

    State state() const { return State(_info.state); };
    void set_state(State state) { _info.state = state.data(); };
    uint32_t type_id() const { return uint32_t(_info.type_id); };

    NodeFlags &flags() { return _flags; };
    const NodeFlags &flags() const { return _flags; };

    void sort_inputs_if_needed() {
        if (_flags.inputs_unsorted()) {
            _flags.set_inputs_unsorted(false);
            std::sort(_inputs.begin(), _inputs.end());
        }
    }

    void *get_self(const AttributeType &type) const; // TODO: inline
    void update_self(const Graph &graph, void *new_self);
    void destroy_self(const Graph &graph);

    void *get_value() const;
    void allocate_value(Graph &graph, data::zone &zone);
    void destroy_value(Graph &graph);

    void destroy(Graph &graph);

    data::vector<InputEdge> inputs() const { return _inputs; };
    data::vector<OutputEdge> outputs() const { return _outputs; };
};

static_assert(sizeof(Node) == 0x1c);

} // namespace AG

CF_ASSUME_NONNULL_END
