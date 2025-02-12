#pragma once

#include <CoreFoundation/CFBase.h>

#include "Attribute/AttributeID.h"
#include "Attribute/WeakAttributeID.h"
#include "Data/Vector.h"
#include "Edge.h"

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
    IndirectNode(WeakAttributeID source, bool traverses_graph_contexts, uint32_t offset, uint16_t size)
        : _source(source) {
        _info.is_mutable = false;
        _info.traverses_graph_contexts = traverses_graph_contexts;
        _info.offset = offset;
        _size = size;
        _relative_offset = 0;
    }

    const WeakAttributeID &source() const { return _source; };

    bool is_mutable() const { return _info.is_mutable; };
    MutableIndirectNode &to_mutable();

    void set_traverses_graph_contexts(bool value) { _info.traverses_graph_contexts = value; };
    bool traverses_graph_contexts() const { return _info.traverses_graph_contexts; };

    uint32_t offset() const { return _info.offset; };
    bool has_size() const { return _size != InvalidSize; };
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
    data::vector<OutputEdge> _outputs;
    WeakAttributeID _initial_source;
    uint32_t _initial_offset;

  public:
    MutableIndirectNode(WeakAttributeID source, bool traverses_graph_contexts, uint32_t offset, uint16_t size,
                        WeakAttributeID initial_source, uint32_t initial_offset)
        : IndirectNode(source, traverses_graph_contexts, offset, size), _dependency(0), _initial_source(initial_source),
          _initial_offset(initial_offset){

          };

    const AttributeID &dependency() const { return _dependency; };
    void set_dependency(const AttributeID &dependency) { _dependency = dependency; };

    WeakAttributeID initial_source() { return _initial_source; };
    uint32_t initial_offset() { return _initial_offset; };

    data::vector<OutputEdge> outputs() { return _outputs; };
};

static_assert(sizeof(MutableIndirectNode) == 0x28);

} // namespace AG

CF_ASSUME_NONNULL_END
