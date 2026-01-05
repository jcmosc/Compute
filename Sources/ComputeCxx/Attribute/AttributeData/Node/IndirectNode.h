#pragma once

#include "Attribute/AttributeData/Edge/OutputEdge.h"
#include "Attribute/AttributeID/AttributeID.h"
#include "Attribute/AttributeID/RelativeAttributeID.h"
#include "Attribute/AttributeID/WeakAttributeID.h"
#include "ComputeCxx/AGBase.h"
#include "Data/Vector.h"

AG_ASSUME_NONNULL_BEGIN

namespace AG {

class MutableIndirectNode;

class IndirectNode {
  private:
    static constexpr uint32_t InvalidSize = 0xffff;

    WeakAttributeID _source;
    unsigned int _mutable : 1;
    unsigned int _traverses_contexts : 1;
    unsigned int _offset : 30;

    uint16_t _size;
    RelativeAttributeID _next_attribute;

  protected:
    IndirectNode(WeakAttributeID source, bool traverses_contexts, uint32_t offset, std::optional<size_t> size,
                 bool is_mutable)
        : _source(source), _mutable(is_mutable), _traverses_contexts(traverses_contexts), _offset(offset),
          _size(size.has_value() && size.value() < InvalidSize ? uint16_t(size.value()) : InvalidSize) {}

  public:
    static constexpr uint32_t MaximumOffset = 0x3ffffffe; // 30 bits - 1

    IndirectNode(WeakAttributeID source, bool traverses_contexts, uint32_t offset, std::optional<size_t> size)
        : _source(source), _traverses_contexts(traverses_contexts), _offset(offset),
          _size(size.has_value() && size.value() < InvalidSize ? uint16_t(size.value()) : InvalidSize) {}

    // Non-copyable
    IndirectNode(const IndirectNode &) = delete;
    IndirectNode &operator=(const IndirectNode &) = delete;

    // Non-movable
    IndirectNode(IndirectNode &&) = delete;
    IndirectNode &operator=(IndirectNode &&) = delete;

    const WeakAttributeID &source() const { return _source; };

    bool is_mutable() const { return _mutable; };
    MutableIndirectNode &to_mutable();
    const MutableIndirectNode &to_mutable() const;

    bool traverses_contexts() const { return _traverses_contexts; };
    void set_traverses_contexts(bool value) { _traverses_contexts = value; }

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
    AttributeID _dependency = AttributeID(nullptr);
    data::vector<OutputEdge> _output_edges;

    WeakAttributeID _initial_source;
    uint32_t _initial_offset;

  public:
    MutableIndirectNode(WeakAttributeID source, bool traverses_contexts, uint32_t offset, std::optional<size_t> size,
                        WeakAttributeID initial_source, uint32_t initial_offset)
        : IndirectNode(source, traverses_contexts, offset, size, true), _initial_source(initial_source),
          _initial_offset(initial_offset) {}

    const AttributeID &dependency() const { return _dependency; };
    void set_dependency(const AttributeID &dependency) { _dependency = dependency; };

    WeakAttributeID initial_source() { return _initial_source; };
    uint32_t initial_offset() { return _initial_offset; };

    data::vector<OutputEdge> &output_edges() { return _output_edges; };
};

} // namespace AG

AG_ASSUME_NONNULL_END
