#pragma once

#include <CoreFoundation/CFBase.h>

#include "Attribute/AttributeID/AttributeID.h"
#include "Attribute/AttributeID/WeakAttributeID.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class MutableIndirectNode;

class IndirectNode {
  private:
    struct Info {
        unsigned int is_mutable : 1;
        unsigned int traverses_graph_contexts : 1;
        unsigned int offset : 30;
        unsigned int size : 32;
    };
    static_assert(sizeof(Info) == 8);
    static constexpr uint32_t InvalidSize = 0xffff;

    WeakAttributeID _source;
    Info _info;

  public:
    bool is_mutable() const { return _info.is_mutable; };
    const MutableIndirectNode &to_mutable() const;

    bool traverses_graph_contexts() const { return _info.traverses_graph_contexts; };

    uint32_t offset() const { return _info.offset; };
    std::optional<size_t> size() const {
        return _info.size != InvalidSize ? std::optional(size_t(_info.size)) : std::optional<size_t>();
    };

    const WeakAttributeID &source() const { return _source; };

    void modify(WeakAttributeID source, size_t size);
};

class MutableIndirectNode : public IndirectNode {
  private:
    AttributeID _dependency;

  public:
    const AttributeID &dependency() const { return _dependency; };
};

} // namespace AG

CF_ASSUME_NONNULL_END
