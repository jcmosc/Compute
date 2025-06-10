#pragma once

#include <CoreFoundation/CFBase.h>

#include "Attribute/AttributeData/Edge/OutputEdge.h"
#include "Attribute/AttributeID/AttributeID.h"
#include "Attribute/AttributeID/RelativeAttributeID.h"
#include "Attribute/AttributeID/WeakAttributeID.h"
#include "Data/Vector.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class MutableIndirectNode;

class IndirectNode {
  private:
    static constexpr uint16_t MaximumOffset = 0x3ffffffe; // 30 bits - 1
    static constexpr uint32_t InvalidSize = 0xffff;

    WeakAttributeID _source;
    unsigned int _mutable : 1;
    unsigned int _traverses_contexts : 1;
    unsigned int _offset : 30;

    uint16_t _size;
    RelativeAttributeID _next_attribute;

  public:
    // Non-copyable
    IndirectNode(const IndirectNode &) = delete;
    IndirectNode &operator=(const IndirectNode &) = delete;

    // Non-movabe
    IndirectNode(IndirectNode &&) = delete;
    IndirectNode &operator=(IndirectNode &&) = delete;

    const WeakAttributeID &source() const { return _source; };

    bool is_mutable() const { return _mutable; };
    MutableIndirectNode &to_mutable();
    const MutableIndirectNode &to_mutable() const;

    uint32_t offset() const { return _offset; };
    std::optional<size_t> size() const {
        return _size != InvalidSize ? std::optional(size_t(_size)) : std::optional<size_t>();
    };

    const RelativeAttributeID next_attribute() const { return _next_attribute; }
    void set_next_attribute(RelativeAttributeID next_attribute) { _next_attribute = next_attribute; }

    void modify(WeakAttributeID source, size_t size);
};

class MutableIndirectNode : public IndirectNode {
  private:
    AttributeID _dependency;
    data::vector<OutputEdge> _output_edges;

    WeakAttributeID _initial_source;
    uint32_t _initial_offset;

  public:
    const AttributeID &dependency() const { return _dependency; };
    void set_dependency(const AttributeID &dependency) { _dependency = dependency; };

    WeakAttributeID initial_source() { return _initial_source; };
    uint32_t initial_offset() { return _initial_offset; };

    const data::vector<OutputEdge> &output_edges() const { return _output_edges; };
};

} // namespace AG

CF_ASSUME_NONNULL_END
