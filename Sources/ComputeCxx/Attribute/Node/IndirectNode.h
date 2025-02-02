#pragma once

#include <CoreFoundation/CFBase.h>

#include "Attribute/AttributeID.h"
#include "Attribute/WeakAttributeID.h"
#include "OutputEdge.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class MutableIndirectNode;

class IndirectNode {
  private:
    struct Info {
        unsigned int is_mutable : 1;
        unsigned int traverses_graph_contexts : 1;
        unsigned int offset : 30;
    };
    static_assert(sizeof(Info) == 4);
    static constexpr uint16_t InvalidSize = 0xffff;

    WeakAttributeID _source;
    Info _info;
    uint16_t _size;
    uint16_t _relative_offset; // could be relative offset, see Subgraph::insert_attribute

  public:
    IndirectNode(WeakAttributeID source, bool traverses_graph_contexts, uint32_t offset, uint16_t size);

    const WeakAttributeID &source() const { return _source; };

    bool is_mutable() const { return _info.is_mutable; };
    MutableIndirectNode &to_mutable();

    bool traverses_graph_contexts() const { return _info.traverses_graph_contexts; };

    uint32_t offset() const { return _info.offset; };
    std::optional<size_t> size() const {
        return _size != InvalidSize ? std::optional(size_t(_size)) : std::optional<size_t>();
    };

    uint16_t relative_offset() const { return _relative_offset; };
    void set_relative_offset(uint16_t relative_offset) { _relative_offset = relative_offset; };

    void modify(WeakAttributeID source, uint32_t offset);
};

static_assert(sizeof(IndirectNode) == 0x10);

class MutableIndirectNode : public IndirectNode {
  private:
    AttributeID _dependency;
    data::ptr<OutputEdge> _output;
    uint32_t something;
    WeakAttributeID _initial_source;
    uint32_t _initial_offset;

  public:
    MutableIndirectNode(WeakAttributeID source, bool traverses_graph_contexts, uint32_t offset, uint16_t size,
                        WeakAttributeID initial_source, uint32_t initial_offset);

    const AttributeID &dependency() const { return _dependency; };
    void set_dependency(const AttributeID &dependency) { _dependency = dependency; };
};

static_assert(sizeof(MutableIndirectNode) == 0x28);

} // namespace AG

CF_ASSUME_NONNULL_END
