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

// AGAttributeFlags
class AttributeFlags {
  private:
    uint8_t _data;

  public:
    AttributeFlags() : _data(0) {}
    AttributeFlags(uint8_t data) : _data(data) {}
    uint8_t data() const { return _data; }

    operator bool() const { return _data == 0; };
};

class Node {
  public:
    class State {
      private:
        enum : uint8_t {
            Dirty = 1 << 0,          // 0x01 // Unknown0 = 1 << 0,
            Pending = 1 << 1,        // 0x02 // Unknown1 = 1 << 1,
            MainThread = 1 << 2,     // 0x04 set from attribute type flags & 8
            MainThreadOnly = 1 << 3, // 0x08 set from attribute type flags & 8

            ValueInitialized = 1 << 4, // 0x10
            SelfInitialized = 1 << 5,  // 0x20
            Updating = 1 << 6,         // 0x40 // TODO: make this and next bit a count 0..<4
            UpdatingCyclic = 1 << 7,   // 0x80

        };
        uint8_t _data;

      public:
        explicit constexpr State(uint8_t data = 0) : _data(data) {};
        uint8_t data() const { return _data; };

        bool is_dirty() const { return _data & Dirty; }
        State with_dirty(bool value) const { return State((_data & ~Dirty) | (value ? Dirty : 0)); };

        bool is_pending() const { return _data & Pending; }
        State with_pending(bool value) const { return State((_data & ~Pending) | (value ? Pending : 0)); };

        bool is_main_thread() const { return _data & MainThread; }
        State with_main_thread(bool value) const { return State((_data & ~MainThread) | (value ? MainThread : 0)); };

        bool is_main_thread_only() const { return _data & MainThreadOnly; }
        State with_main_thread_only(bool value) const {
            return State((_data & ~MainThreadOnly) | (value ? MainThreadOnly : 0));
        };

        bool is_value_initialized() const { return _data & ValueInitialized; };
        State with_value_initialized(bool value) const {
            return State((_data & ~ValueInitialized) | (value ? ValueInitialized : 0));
        };

        bool is_self_initialized() const { return _data & SelfInitialized; };
        State with_self_initialized(bool value) const {
            return State((_data & ~SelfInitialized) | (value ? SelfInitialized : 0));
        };

        State with_updating(bool value) const { return State((_data & ~Updating) | (value ? Updating : 0)); };

        bool is_updating() const { return _data & (Updating | UpdatingCyclic); }
        bool is_updating_cyclic() const { return (_data & (Updating | UpdatingCyclic)) == (Updating | UpdatingCyclic); }
        uint8_t update_count() const { return _data >> 6; }
    };

    class Flags {
      private:
        enum : uint8_t {
            HasIndirectSelf = 1 << 0,  // 0x01
            HasIndirectValue = 1 << 1, // 0x02

            InputsTraverseContexts = 1 << 2, // 0x04
            InputsUnsorted = 1 << 3,         // 0x08
            Cacheable = 1 << 4,              // 0x10

            Unknown0x20 = 1 << 5,  // 0x20 - initial  value // see Graph::update_main_refs
            SelfModified = 1 << 6, // 0x40
        };
        uint8_t _data;

      public:
        explicit constexpr Flags(uint8_t data = 0) : _data(data) {};
        uint8_t data() const { return _data; };

        bool has_indirect_self() const { return _data & HasIndirectSelf; }
        void set_has_indirect_self(bool value) { _data = (_data & ~HasIndirectSelf) | (value ? HasIndirectSelf : 0); };

        bool has_indirect_value() const { return _data & HasIndirectValue; }
        void set_has_indirect_value(bool value) {
            _data = (_data & ~HasIndirectValue) | (value ? HasIndirectValue : 0);
        };

        bool inputs_traverse_contexts() const { return _data & InputsTraverseContexts; };
        void set_inputs_traverse_contexts(bool value) {
            _data = (_data & ~InputsTraverseContexts) | (value ? InputsTraverseContexts : 0);
        };

        bool inputs_unsorted() const { return _data & InputsUnsorted; };
        void set_inputs_unsorted(bool value) { _data = (_data & ~InputsUnsorted) | (value ? InputsUnsorted : 0); };

        bool cacheable() const { return _data & Cacheable; };
        void set_cacheable(bool value) { _data = (_data & ~Cacheable) | (value ? Cacheable : 0); };

        bool unknown0x20() const { return _data & Unknown0x20; };
        void set_unknown0x20(bool value) { _data = (_data & ~Unknown0x20) | (value ? Unknown0x20 : 0); };

        bool self_modified() const { return _data & SelfModified; };
        void set_self_modified(bool value) { _data = (_data & ~SelfModified) | (value ? SelfModified : 0); };
    };

  private:
    State _state;
    unsigned int _type_id : 24;
    RelativeAttributeID _relative_offset;
    AttributeFlags _subgraph_flags;
    Flags _flags;
    data::ptr<void> _value;

    data::vector<InputEdge> _inputs;
    data::vector<OutputEdge> _outputs;

  public:
    Node(State state, uint32_t type_id, Flags flags) : _state(state), _type_id(type_id), _flags(flags) {};

    const State &state() const { return _state; };
    void set_state(State state) { _state = state; };

    uint32_t type_id() const { return _type_id; };

    uint8_t value_state() const {
        return (state().is_dirty() ? 1 : 0) << 0 | (state().is_pending() ? 1 : 0) << 1 |
               (state().is_updating() ? 1 : 0) << 2 | (state().is_value_initialized() ? 1 : 0) << 3 |
               (state().is_main_thread() ? 1 : 0) << 4 | (flags().unknown0x20() ? 1 : 0) << 5 |
               (state().is_main_thread_only() ? 1 : 0) << 6 | (flags().self_modified() ? 1 : 0) << 7;
    };

    RelativeAttributeID relative_offset() const { return _relative_offset; };
    void set_relative_offset(RelativeAttributeID relative_offset) { _relative_offset = relative_offset; };

    AttributeFlags &subgraph_flags() { return _subgraph_flags; };
    const AttributeFlags &subgraph_flags() const { return _subgraph_flags; };
    void set_subgraph_flags(AttributeFlags subgraph_flags) { _subgraph_flags = subgraph_flags; };

    Flags &flags() { return _flags; };
    const Flags &flags() const { return _flags; };
    void set_flags(Flags flags) { _flags = flags; };

    void sort_inputs_if_needed() {
        if (_flags.inputs_unsorted()) {
            _flags.set_inputs_unsorted(false);
            std::sort(_inputs.begin(), _inputs.end());
        }
    }

    void *get_self(const AttributeType &type) const;
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
