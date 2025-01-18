#include "MetadataVisitor.h"

#include <algorithm>

namespace AG {
namespace swift {

bool metadata_visitor::unknown_result() const { return false; }

bool metadata_visitor::visit_element(const metadata &type, const metadata::ref_kind kind, size_t element_offset,
                                     size_t element_size) const {
    return unknown_result();
}

bool metadata_visitor::visit_field(const metadata &type, const field_record &field, size_t field_offset,
                                   size_t field_size) const {
    const char *mangled_type_name = field.MangledTypeName ? field.MangledTypeName.get() : nullptr;
    if (mangled_type_name) {
        metadata::ref_kind ref_kind = metadata::ref_kind::strong;
        const metadata *element_type = type.mangled_type_name_ref(mangled_type_name, false, &ref_kind);
        if (element_type) {
            size_t element_size = element_type->vw_size();
            if (element_type->getKind() == ::swift::MetadataKind::Metatype) {
                element_size = std::min(element_size, field_size);
            }
            return visit_element(*element_type, ref_kind, field_offset, element_size);
        }
    }
    return unknown_result();
}

bool metadata_visitor::visit_case(const metadata &type, const field_record &field, uint32_t arg) const {
    return unknown_result();
}

bool metadata_visitor::metadata_visitor::visit_class(const any_class_type_metadata &type) const {
    return unknown_result();
}

bool metadata_visitor::visit_existential(const existential_type_metadata &type) const { return unknown_result(); }

bool metadata_visitor::visit_function(const function_type_metadata &type) const { return unknown_result(); }

bool metadata_visitor::visit_native_object(const metadata &type) const { return unknown_result(); }

} // namespace swift
} // namespace AG
