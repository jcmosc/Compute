#pragma once

#include <CoreFoundation/CFBase.h>

#include "Layout/LayoutDescriptor.h"
#include "Swift/Metadata.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class AttributeID;
class AttributeType;

class AttributeVTable {
  public:
    using Callback = void (*)(const AttributeType *attribute_type, void *body);
    Callback _unknown_0x00;
    Callback _unknown_0x08;
    Callback destroy_self;
    Callback _unknown_0x18;
    Callback _unknown_0x20;
    Callback _update_stack_callback; // maybe initialize value
};

class AttributeType {
  public:
    enum Flags : uint32_t {
        ComparisonModeMask = 0x3,

        HasDestroySelf = 1 << 2,         // 0x04
        MainThread = 1 << 3,             // 0x08
        UseGraphAsInitialValue = 1 << 4, // 0x10
        Unknown0x20 = 1 << 5,            // 0x20 // used in update_main_refs
    };

    using UpdateFunction = void (*)(void *context, void *body);

  private:
    swift::metadata *_self_metadata;
    swift::metadata *_value_metadata;
    UpdateFunction _update_function;
    void *_update_context;
    AttributeVTable *_vtable;
    Flags _flags;
    uint32_t _attribute_offset;
    ValueLayout _layout;

  public:
    class deleter {};

    const swift::metadata &self_metadata() const { return *_self_metadata; };
    const swift::metadata &value_metadata() const { return *_value_metadata; };

    bool main_thread() const { return _flags & Flags::MainThread; };
    bool use_graph_as_initial_value() const { return _flags & Flags::UseGraphAsInitialValue; };
    bool unknown_0x20() const { return _flags & Flags::Unknown0x20; };

    /// Returns the offset in bytes from a Node to the attribute body,
    /// aligned to the body's alignment.
    uint32_t attribute_offset() const { return _attribute_offset; };
    void update_attribute_offset();

    ValueLayout layout() const { return _layout; };
    void set_layout(ValueLayout layout) { _layout = layout; };
    LayoutDescriptor::ComparisonMode comparison_mode() const {
        return LayoutDescriptor::ComparisonMode(_flags & Flags::ComparisonModeMask);
    };
    void update_layout() {
        if (!_layout) {
            auto comparison_mode = LayoutDescriptor::ComparisonMode(_flags & Flags::ComparisonModeMask);
            _layout = LayoutDescriptor::fetch(value_metadata(), comparison_mode, 1);
        }
    };

    void perform_update(void *body) const { _update_function(_update_context, body); };

    // V table methods
    void vt_destroy_self(void *body) {
        if (_flags & Flags::HasDestroySelf) {
            _vtable->destroy_self(this, body);
        }
    }

    AttributeVTable::Callback vt_get_update_stack_callback() const { return _vtable->_update_stack_callback; }
};

} // namespace AG

CF_ASSUME_NONNULL_END
