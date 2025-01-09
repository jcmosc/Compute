#pragma once

#include <CoreFoundation/CFBase.h>

#include "Runtime/Metadata.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class AttributeType;

class AttributeVTable {
  public:
    enum Flags : uint8_t {
        HasDestroySelf = 1 << 2,
    };

    using Callback = void (*)(AttributeType *attribute_type, void *body);
    Callback destroy_self;
};

class AttributeType {
  private:
    swift::metadata *_self_metadata;
    swift::metadata *_value_metadata;
    void *_field1;
    void *_field2;
    AttributeVTable *_v_table;
    uint8_t _v_table_flags;
    uint32_t _attribute_offset;

  public:
    const swift::metadata &self_metadata() const { return *_self_metadata; };
    const swift::metadata &value_metadata() const { return *_value_metadata; };

    /// Returns the offset in bytes from a Node to the attribute body,
    /// aligned to the body's alignment.
    uint32_t attribute_offset() const { return _attribute_offset; };

    // V table methods
    void v_destroy_self(void *body) {
        if (_v_table_flags & AttributeVTable::Flags::HasDestroySelf) {
            _v_table->destroy_self(this, body);
        }
    }
};

} // namespace AG

CF_ASSUME_NONNULL_END
