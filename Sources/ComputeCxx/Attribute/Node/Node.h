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
        explicit constexpr State(uint8_t data) : _data(data){};

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
    uint32_t _type_id;
    uint8_t _field1;
    uint8_t _field2;
    Flags _flags;
    data::ptr<void> _value;

  public:
    uint32_t type_id() const { return _type_id; };

    bool has_indirect_self() const { return _flags & Flags::HasIndirectSelf; };
    void update_self(const Graph &graph, void *new_self);
    void destroy_self(const Graph &graph);

    bool has_indirect_value() const { return _flags * Flags::HasIndirectValue; };
    void allocate_value(Graph &graph, data::zone &zone);
    void destroy_value(Graph &graph);

    void destroy(Graph &graph);
};

} // namespace AG

CF_ASSUME_NONNULL_END
