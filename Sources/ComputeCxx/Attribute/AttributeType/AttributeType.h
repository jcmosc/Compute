#pragma once

#include "Attribute/AttributeData/Node/Node.h"
#include "Comparison/LayoutDescriptor.h"
#include "ComputeCxx/AGBase.h"
#include "ComputeCxx/AGAttribute.h"
#include "ComputeCxx/AGAttributeType.h"
#include "ComputeCxx/AGComparison.h"
#include "Swift/Metadata.h"

AG_ASSUME_NONNULL_BEGIN

namespace AG {

class AttributeType {
  private:
    swift::metadata *_body_metadata;
    swift::metadata *_value_metadata;
    void (*_update)(void *context AG_SWIFT_CONTEXT, void *body, AGAttribute attribute) AG_SWIFT_CC(swift);
    void *_update_context;
    const AGAttributeVTable *_vtable;
    AGAttributeTypeFlags _flags;

    // set after construction
    uint32_t _body_offset;
    ValueLayout _layout;

    swift::metadata *_attribute_body_type;
    const void *_attribute_body_witness_table;

  public:
    class deleter {
      public:
        void operator()(AttributeType *ptr) {
            if (ptr != nullptr) {
                ptr->vtable().type_destroy(reinterpret_cast<AGAttributeType *>(ptr));
            }
        }
    };

    const swift::metadata &body_metadata() const { return *_body_metadata; };
    const swift::metadata &value_metadata() const { return *_value_metadata; };

    void update(void *body, AGAttribute attribute) const { _update(_update_context, body, attribute); };

    const AGAttributeVTable &vtable() const { return *_vtable; }
    AGAttributeTypeFlags flags() const { return _flags; };

    /// Returns the offset in bytes from a Node to the attribute body,
    /// aligned to the body's alignment.
    uint32_t body_offset() const { return _body_offset; };
    void init_body_offset() {
        uint32_t alignment_mask = uint32_t(_body_metadata->getValueWitnesses()->getAlignmentMask());
        _body_offset = (sizeof(Node) + alignment_mask) & ~alignment_mask;
    }

    void fetch_layout() {
        AGComparisonMode comparison_mode = AGComparisonMode(_flags & AGAttributeTypeFlagsComparisonModeMask);
        _layout = LayoutDescriptor::fetch(value_metadata(), AGComparisonOptions(comparison_mode), 1);
    };

    bool compare_values(const void *lhs, const void *rhs) {
        AGComparisonOptions comparison_options = AGComparisonOptions(_flags & AGAttributeTypeFlagsComparisonModeMask) |
                                                 AGComparisonOptionsCopyOnWrite | AGComparisonOptionsTraceCompareFailed;
        if (_layout == nullptr) {
            _layout = LayoutDescriptor::fetch(value_metadata(), comparison_options, 0);
        }

        ValueLayout layout = _layout == ValueLayoutTrivial ? nullptr : _layout;
        return LayoutDescriptor::compare(layout, (const unsigned char *)lhs, (const unsigned char *)rhs,
                                         value_metadata().vw_size(), comparison_options);
    }

    bool compare_values_partial(const void *lhs, const void *rhs, size_t offset, size_t size) {
        AGComparisonOptions comparison_options =
            AGComparisonOptions(_flags & AGAttributeTypeFlagsComparisonModeMask) | AGComparisonOptionsCopyOnWrite;
        if (_layout == nullptr) {
            _layout = LayoutDescriptor::fetch(value_metadata(), comparison_options, 0);
        }

        ValueLayout layout = _layout == ValueLayoutTrivial ? nullptr : _layout;
        return LayoutDescriptor::compare_partial(layout, (const unsigned char *)lhs + offset,
                                                 (const unsigned char *)rhs + offset, offset, size, comparison_options);
    }

    void destroy_self(Node &node) const {
        if (_flags & AGAttributeTypeFlagsHasDestroySelf) {
            void *body = node.get_self(*this);
            vtable().self_destroy(reinterpret_cast<const AGAttributeType *>(this), body);
        }
    }

    void destroy(Node &node) {
        void *body = node.get_self(*this);
        body_metadata().vw_destroy(static_cast<swift::opaque_value *>(body));
    }

    CFStringRef _Nullable self_description(void *body) const {
        if (auto self_description = vtable().self_description) {
            return self_description(reinterpret_cast<const AGAttributeType *>(this), body);
        }
        return nullptr;
    }

    CFStringRef _Nullable value_description(void *value) const {
        if (auto value_description = vtable().value_description) {
            return value_description(reinterpret_cast<const AGAttributeType *>(this), value);
        }
        return nullptr;
    }
};

} // namespace AG

AG_ASSUME_NONNULL_END
