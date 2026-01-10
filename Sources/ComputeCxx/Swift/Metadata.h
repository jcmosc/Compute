#pragma once

#include <CoreFoundation/CFBase.h>
#include <swift/Demangling/ManglingMacros.h>
#include <swift/Runtime/Metadata.h>

#include "Comparison/LayoutDescriptor.h"

CF_ASSUME_NONNULL_BEGIN

CF_EXPORT
const void *PROTOCOL_DESCR_SYM(SQ);

namespace AG {
namespace swift {

static constexpr auto &EquatableProtocolDescriptor = PROTOCOL_DESCR_SYM(SQ);

struct equatable_witness_table;

using opaque_value = ::swift::OpaqueValue;

class metadata_visitor;
class context_descriptor;
class type_context_descriptor;

class metadata : public ::swift::Metadata {
  private:
    const ::swift::Metadata *base() const { return this; };
    static const metadata *from_base(const ::swift::Metadata *base) { return static_cast<const metadata *>(base); };

    bool is_type_metadata() const {
        if (::swift::ClassMetadata::classof(this)) {
            const auto class_type = static_cast<const ::swift::ClassMetadata *>(base());
            // Depending on the deployment target a binary was compiled for,
            // statically emitted metadata templates may have a different bit set
            // from the one that this runtime canonically considers the "is Swift" bit.
            // See SWIFT_CLASS_IS_SWIFT_MASK in
            // https://github.com/swiftlang/swift/blob/main/include/swift/ABI/Metadata.h
            return class_type->Data & 0x3;
        }
        return true;
    }

  public:
    const char *name(bool qualified) const;

    const context_descriptor *_Nullable descriptor() const;
    const type_context_descriptor *_Nullable nominal_descriptor() const;

    void append_description(CFMutableStringRef description) const;
    const void *signature() const;

    const equatable_witness_table *_Nullable equatable() const;

    // Mutating objects

    void copy_on_write_heap_object(void *_Nonnull *_Nonnull object_ref) const;

    // Looking up types by name

    enum class ref_kind {
        strong = 0,
        weak = 1,
        unowned = 2,
        unmanaged = 3,
    };

    const metadata *_Nullable mangled_type_name_ref(const char *type_name, bool fault_if_null,
                                                    ref_kind *_Nullable kind_out) const;
    const metadata *_Nullable mangled_type_name_ref_cached(const char *type_name, ref_kind *_Nullable kind_out) const;

    // Visiting

    bool visit(metadata_visitor &visitor) const;
    bool visit_heap(metadata_visitor &visitor, LayoutDescriptor::HeapMode heap_mode) const;

  private:
    bool visit_heap_class(metadata_visitor &visitor) const;
    bool visit_heap_locals(metadata_visitor &visitor) const;
};

class any_class_type_metadata : public ::swift::AnyClassMetadata {};

class function_type_metadata : public ::swift::FunctionTypeMetadata {};

class existential_type_metadata : public ::swift::ExistentialTypeMetadata {
  public:
    // We need to reimplement these to avoid errors when linking against the Swift runtime library
    ::swift::ExistentialTypeRepresentation representation(void) const;
    const void *project_value(void *container) const;
    const metadata *dynamic_type(void *container) const;
};

} // namespace swift
} // namespace AG

CF_ASSUME_NONNULL_END
