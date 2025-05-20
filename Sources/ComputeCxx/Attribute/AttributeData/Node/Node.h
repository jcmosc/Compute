#pragma once

#include <CoreFoundation/CFBase.h>

#include "Data/Pointer.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

namespace data {
class zone;
}
class AttributeType;
class Graph;

class Node {
  private:
    class State {
      public:
        enum : uint8_t {
            ValueInitialized = 1 << 4,
            SelfInitialized = 1 << 5,
        };

      private:
        uint8_t _data;
        explicit constexpr State(uint8_t data) : _data(data) {};

      public:
        bool is_value_initialized() { return _data & ValueInitialized; };
        State with_value_initialized(bool value) const {
            return State((_data & ~ValueInitialized) | (value ? ValueInitialized : 0));
        };

        bool is_self_initialized() { return _data & SelfInitialized; };
        State with_self_initialized(bool value) const {
            return State((_data & ~SelfInitialized) | (value ? SelfInitialized : 0));
        };
    };

    enum Flags : uint8_t {
        HasIndirectSelf = 1 << 0,
        HasIndirectValue = 1 << 1,
    };

    State _state;
    unsigned int _type_id : 24;
    uint16_t _relative_offset;
    uint8_t _subgraph_flags;
    Flags _flags;
    data::ptr<void> _value;

    uint32_t _padding1;
    uint16_t _padding2;
    uint32_t _padding4;
    uint16_t _padding5;

  public:
    uint32_t type_id() const { return _type_id; };

    void *get_self(const AttributeType &type);
    void update_self(const Graph &graph, void *new_self);
    void destroy_self(const Graph &graph);

    void *get_value();
    void allocate_value(Graph &graph, data::zone &zone);
    void destroy_value(Graph &graph);

    void destroy(Graph &graph);
};

static_assert(sizeof(Node) == 0x1c);

} // namespace AG

CF_ASSUME_NONNULL_END
