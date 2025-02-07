#pragma once

#include <CoreFoundation/CFBase.h>
#include <llvm/ADT/ArrayRef.h>
#include <stdint.h>
#include <swift/Runtime/Metadata.h>

#include "Containers/Vector.h"
#include "Metadata.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {
namespace swift {

class context_descriptor : public ::swift::ContextDescriptor {
  public:
    struct generic_params_info {
        const ::swift::GenericContextDescriptorHeader *generic_header = nullptr;
        llvm::ArrayRef<::swift::GenericParamDescriptor> params = {};
        ::swift::GenericPackShapeHeader pack_shape_header = {};
        llvm::ArrayRef<::swift::GenericPackShapeDescriptor> pack_shape_descriptors = {};
        const metadata *_Nonnull const *_Nullable generic_args = nullptr;

        generic_params_info(const context_descriptor &context, const metadata *_Nullable type);
    };

    struct generic_arg {
        const metadata *types;
        uint64_t num_types;
        bool is_pack;
    };

    const ::swift::ContextDescriptor *base() const { return this; };
    static const context_descriptor *from_base(const ::swift::ContextDescriptor *base) {
        return static_cast<const context_descriptor *>(base);
    };

    const context_descriptor *_Nullable parent() const { return context_descriptor::from_base(Parent.get()); };

    uint64_t count_generic_args() const;
    void push_generic_args(const metadata &metadata, vector<generic_arg, 8, uint64_t> &generic_args_vector) const;
};

class type_context_descriptor : ::swift::TypeContextDescriptor {
  public:
    const char *name() const { return Name.get(); }
};

class class_type_descriptor {
  private:
    ::swift::ClassDescriptor _base;

  public:
    const ::swift::ClassDescriptor *base() const { return &_base; };
    static const class_type_descriptor *from_base(const ::swift::ClassDescriptor *base) {
        return reinterpret_cast<const class_type_descriptor *>(base);
    };

    // We need to reimplement these to avoid errors when linking against the Swift runtime library
    uint64_t immediate_members_offset(void) const;
    uint64_t field_offset_vector_offset() const;
};

} // namespace swift
} // namespace AG

CF_ASSUME_NONNULL_END
