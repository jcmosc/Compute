#pragma once

#include "Attribute/AttributeData/Node/Node.h"
#include "Comparison/LayoutDescriptor.h"
#include "ComputeCxx/IAGBase.h"
#include "ComputeCxx/IAGAttribute.h"
#include "ComputeCxx/IAGAttributeType.h"
#include "ComputeCxx/IAGComparison.h"
#include "Swift/Metadata.h"

IAG_ASSUME_NONNULL_BEGIN

namespace IAG {

class AttributeType {
  private:
    swift::metadata *_body_metadata;
    swift::metadata *_value_metadata;
    void (*_update)(void *body, IAGAttribute attribute, void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift);
    void *_update_context;
    const IAGAttributeVTable *_vtable;
    IAGAttributeTypeFlags _flags;

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
                ptr->vtable().type_destroy(reinterpret_cast<IAGAttributeType *>(ptr));
            }
        }
    };

    const swift::metadata &body_metadata() const { return *_body_metadata; };
    const swift::metadata &value_metadata() const { return *_value_metadata; };

    void update(void *body, IAGAttribute attribute) const { _update(body, attribute, _update_context); };

    const IAGAttributeVTable &vtable() const { return *_vtable; }
    IAGAttributeTypeFlags flags() const { return _flags; };

    /// Returns the offset in bytes from a Node to the attribute body,
    /// aligned to the body's alignment.
    uint32_t body_offset() const { return _body_offset; };
    void init_body_offset() {
        uint32_t alignment_mask = uint32_t(_body_metadata->getValueWitnesses()->getAlignmentMask());
        _body_offset = (sizeof(Node) + alignment_mask) & ~alignment_mask;
    }

    void fetch_layout() {
        IAGComparisonMode comparison_mode = IAGComparisonMode(_flags & IAGAttributeTypeFlagsComparisonModeMask);
        _layout = LayoutDescriptor::fetch(value_metadata(), IAGComparisonOptions(comparison_mode), 1);
    };

    bool compare_values(const void *lhs, const void *rhs) {
        IAGComparisonOptions comparison_options = IAGComparisonOptions(_flags & IAGAttributeTypeFlagsComparisonModeMask) |
                                                 IAGComparisonOptionsCopyOnWrite | IAGComparisonOptionsTraceCompareFailed;
        if (_layout == nullptr) {
            _layout = LayoutDescriptor::fetch(value_metadata(), comparison_options, 0);
        }

        ValueLayout layout = _layout == ValueLayoutTrivial ? nullptr : _layout;
        return LayoutDescriptor::compare(layout, (const unsigned char *)lhs, (const unsigned char *)rhs,
                                         value_metadata().vw_size(), comparison_options);
    }

    bool compare_values_partial(const void *lhs, const void *rhs, size_t offset, size_t size) {
        IAGComparisonOptions comparison_options =
            IAGComparisonOptions(_flags & IAGAttributeTypeFlagsComparisonModeMask) | IAGComparisonOptionsCopyOnWrite;
        if (_layout == nullptr) {
            _layout = LayoutDescriptor::fetch(value_metadata(), comparison_options, 0);
        }

        ValueLayout layout = _layout == ValueLayoutTrivial ? nullptr : _layout;
        return LayoutDescriptor::compare_partial(layout, (const unsigned char *)lhs + offset,
                                                 (const unsigned char *)rhs + offset, offset, size, comparison_options);
    }

    void destroy_self(Node &node) const {
        if (_flags & IAGAttributeTypeFlagsHasDestroySelf) {
            void *body = node.get_self(*this);
            vtable().self_destroy(reinterpret_cast<const IAGAttributeType *>(this), body);
        }
    }

    void destroy(Node &node) {
        void *body = node.get_self(*this);
        body_metadata().vw_destroy(static_cast<swift::opaque_value *>(body));
    }

#if TARGET_OS_MAC
    CFStringRef _Nullable self_description(void *body) const {
        if (auto self_description = vtable().self_description) {
            return self_description(reinterpret_cast<const IAGAttributeType *>(this), body);
        }
        return nullptr;
    }

    CFStringRef _Nullable value_description(void *value) const {
        if (auto value_description = vtable().value_description) {
            return value_description(reinterpret_cast<const IAGAttributeType *>(this), value);
        }
        return nullptr;
    }
#else
    CFStringRef _Nullable copy_self_description(void *body) const {
        if (auto copy_self_description = vtable().copy_self_description) {
            return copy_self_description(reinterpret_cast<const IAGAttributeType *>(this), body);
        }
        return nullptr;
    }

    CFStringRef _Nullable copy_value_description(void *value) const {
        if (auto copy_value_description = vtable().copy_value_description) {
            return copy_value_description(reinterpret_cast<const IAGAttributeType *>(this), value);
        }
        return nullptr;
    }
#endif
};

} // namespace IAG

IAG_ASSUME_NONNULL_END
