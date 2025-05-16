#pragma once

// needed by swift/RemoteInspection/Records.h
#include <cassert>
#include <type_traits>

#include <CoreFoundation/CFBase.h>
#include <swift/RemoteInspection/Records.h>

#include "Metadata.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {
namespace swift {

using field_record = ::swift::reflection::FieldRecord;

class metadata_visitor {
  public:
    virtual bool unknown_result();

    virtual bool visit_element(const metadata &type, const metadata::ref_kind kind, size_t element_offset,
                               size_t element_size);

    virtual bool visit_field(const metadata &type, const field_record &field, size_t field_offset, size_t field_size);

    virtual bool visit_case(const metadata &type, const field_record &field, uint32_t index);
    virtual bool visit_class(const any_class_type_metadata &type);
    virtual bool visit_existential(const existential_type_metadata &type);
    virtual bool visit_function(const function_type_metadata &type);
    virtual bool visit_native_object(const metadata &type);
};

} // namespace swift
} // namespace AG

CF_ASSUME_NONNULL_END
