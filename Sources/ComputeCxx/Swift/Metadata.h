#pragma once

#include <CoreFoundation/CFBase.h>
#include <swift/Runtime/Metadata.h>

CF_ASSUME_NONNULL_BEGIN

namespace AG {
namespace swift {

using opaque_value = ::swift::OpaqueValue;
using witness_table = ::swift::WitnessTable;

class metadata_visitor;
class context_descriptor;

class metadata : public ::swift::Metadata {
  private:
    const ::swift::Metadata *base() const { return this; };
    static const metadata *from_base(const ::swift::Metadata *base) { return static_cast<const metadata *>(base); };

  public:
    // TODO: is this closure capture list
    enum class ref_kind {
        none = 0, // TODO: maybe strong
        weak = 1,
        unowned = 2,
        unowned_unsafe = 3,
    };

    const char *name(bool qualified) const;

    const context_descriptor *_Nullable descriptor() const;
    const context_descriptor *_Nullable nominal_descriptor() const;

    const witness_table *_Nullable equatable() const;

    const metadata *_Nullable mangled_type_name_ref(const char *type_name, bool fault_if_null,
                                                    ref_kind *_Nullable kind_out) const;
    const metadata *_Nullable mangled_type_name_ref_cached(const char *type_name, ref_kind *_Nullable kind_out);

    void copy_on_write_heap_object(void *_Nonnull *_Nonnull object_ref);

    void append_description(CFMutableStringRef description) const;
    void *signature() const;

    // Visiting
    
    enum visit_options {
        heap_class = 1 << 0,
        heap_locals = 1 << 1,
        heap_generic_locals = 1 << 2,
    };

    bool visit(metadata_visitor &visitor) const;
    bool visit_heap(metadata_visitor &visitor, visit_options options) const;
    bool visit_heap_class(metadata_visitor &visitor) const;
    bool visit_heap_locals(metadata_visitor &visitor) const;
};

class any_class_type_metadata : public ::swift::AnyClassMetadata {};

class function_type_metadata : public ::swift::FunctionTypeMetadata {};

class existential_type_metadata : public ::swift::ExistentialTypeMetadata {
  public:
    ::swift::ExistentialTypeRepresentation representation(void);
    void *project_value(void *container);
    const metadata *dynamic_type(void *container);
};

} // namespace swift
} // namespace AG

CF_ASSUME_NONNULL_END
