#pragma once

#include <CoreFoundation/CFBase.h>

#include "AGAttributeType.h"
#include "Attribute/AttributeData/Node/Node.h"
#include "Attribute/AttributeID/AGAttribute.h"
#include "Comparison/AGComparison.h"
#include "Comparison/LayoutDescriptor.h"
#include "Swift/Metadata.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class AttributeType {

  private:
    swift::metadata *_body_metadata;
    swift::metadata *_value_metadata;
    void (*_update)(const void *context, void *body, AGAttribute attribute);
    void *_update_context;
    const AGAttributeVTable *_callbacks;
    AGAttributeTypeFlags _flags;

    // set after construction
    uint32_t _body_offset;
    ValueLayout _layout;

  public:
    class deleter {
      public:
        void operator()(AttributeType *ptr) {
            if (ptr != nullptr) {
                auto callbacks = ptr->callbacks();
                callbacks.deallocate(reinterpret_cast<AGAttributeType *>(ptr));
            }
        }
    };

    const swift::metadata &body_metadata() const { return *_body_metadata; };
    const swift::metadata &value_metadata() const { return *_value_metadata; };

    void update(void *body, AGAttribute attribute) const { _update(_update_context, body, attribute); };

    const AGAttributeVTable &callbacks() const { return *_callbacks; }
    AGAttributeTypeFlags flags() const { return _flags; };

    /// Returns the offset in bytes from a Node to the attribute body,
    /// aligned to the body's alignment.
    uint32_t body_offset() const { return _body_offset; };
    void init_body_offset() {
        uint32_t alignment_mask = uint32_t(_body_metadata->getValueWitnesses()->getAlignmentMask());
        _body_offset = (sizeof(Node) + alignment_mask) & ~alignment_mask;
    }

    void prefetch_layout() {
        AGComparisonMode comparison_mode = AGComparisonMode(_flags & AGAttributeTypeFlagsComparisonModeMask);
        _layout = LayoutDescriptor::fetch(value_metadata(), AGComparisonOptions(comparison_mode), 1);
    };

    void destroy_body(Node &node) const {
        if (_flags & AGAttributeTypeFlagsHasDestroySelf) {
            void *body = node.get_self(*this);
            callbacks().destroySelf(reinterpret_cast<const AGAttributeType *>(this), body);
        }
    }

    void destroy(Node &node) {
        void *body = node.get_self(*this);
        body_metadata().vw_destroy(static_cast<swift::opaque_value *>(body));
    }
};

} // namespace AG

CF_ASSUME_NONNULL_END
