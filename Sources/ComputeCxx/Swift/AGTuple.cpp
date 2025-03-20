#include "AGTuple.h"

#include <swift/Runtime/Metadata.h>

#include "Errors/Errors.h"

AGTupleType AGNewTupleType(uint32_t count, const AGTypeID *elements) {
    if (count == 1) {
        return elements[0];
    }
    auto metadata_elements = reinterpret_cast<const ::swift::Metadata *const *>(elements);
    auto response = ::swift::swift_getTupleTypeMetadata(::swift::MetadataRequest(),
                                                        ::swift::TupleTypeFlags().withNumElements(count),
                                                        metadata_elements, nullptr, nullptr);
    if (response.State != ::swift::MetadataState::Complete) {
        AG::precondition_failure("invalid tuple type.");
    }
    return reinterpret_cast<AGTupleType>(response.Value);
}

size_t AGTupleCount(AGTupleType tuple_type) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    if (metadata->getKind() != ::swift::MetadataKind::Tuple) {
        return 1;
    }
    auto tuple_metadata = static_cast<const ::swift::TupleTypeMetadata *>(metadata);
    return static_cast<size_t>(tuple_metadata->NumElements);
}

size_t AGTupleSize(AGTupleType tuple_type) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    return metadata->vw_size();
}

AGTypeID AGTupleElementType(AGTupleType tuple_type, uint32_t index) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    if (metadata->getKind() != ::swift::MetadataKind::Tuple) {
        if (index != 0) {
            AG::precondition_failure("index out of range: %d", index);
        }
        return reinterpret_cast<AGTypeID>(metadata);
    }
    auto tuple_metadata = static_cast<const ::swift::TupleTypeMetadata *>(metadata);
    if (index >= tuple_metadata->NumElements) {
        AG::precondition_failure("index out of range: %d", index);
    }
    auto element = tuple_metadata->getElement(index);
    return reinterpret_cast<AGTypeID>(element.Type);
}

size_t AGTupleElementSize(AGTupleType tuple_type, uint32_t index) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    if (metadata->getKind() != ::swift::MetadataKind::Tuple) {
        if (index != 0) {
            AG::precondition_failure("index out of range: %d", index);
        }
        return metadata->vw_size();
    }
    auto tuple_metadata = static_cast<const ::swift::TupleTypeMetadata *>(metadata);
    if (index >= tuple_metadata->NumElements) {
        AG::precondition_failure("index out of range: %d", index);
    }
    auto element = tuple_metadata->getElement(index);
    return element.Type->vw_size();
}

size_t AGTupleElementOffset(AGTupleType tuple_type, uint32_t index) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    if (metadata->getKind() != ::swift::MetadataKind::Tuple) {
        if (index != 0) {
            AG::precondition_failure("index out of range: %d", index);
        }
        return 0;
    }
    auto tuple_metadata = static_cast<const ::swift::TupleTypeMetadata *>(metadata);
    if (index >= tuple_metadata->NumElements) {
        AG::precondition_failure("index out of range: %d", index);
    }
    auto element = tuple_metadata->getElement(index);
    return element.Offset;
}

size_t AGTupleElementOffsetChecked(AGTupleType tuple_type, uint32_t index, AGTypeID element_type) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    if (metadata->getKind() != ::swift::MetadataKind::Tuple) {
        if (index != 0) {
            AG::precondition_failure("index out of range: %d", index);
        }
        if (reinterpret_cast<AGTypeID>(metadata) != element_type) {
            AG::precondition_failure("element type mismatch");
        }
        return 0;
    }
    auto tuple_metadata = static_cast<const ::swift::TupleTypeMetadata *>(metadata);
    if (index >= tuple_metadata->NumElements) {
        AG::precondition_failure("index out of range: %d", index);
    }
    auto element = tuple_metadata->getElement(index);
    if (reinterpret_cast<AGTypeID>(element.Type) != element_type) {
        AG::precondition_failure("element type mismatch");
    }
    return element.Offset;
}

void *update(void *dest_ptr, const void *src_ptr, const ::swift::Metadata *metadata, AGTupleCopyOptions options) {
    auto dest = reinterpret_cast<::swift::OpaqueValue *>(dest_ptr);
    auto src = reinterpret_cast<::swift::OpaqueValue *>(const_cast<void *>(src_ptr));
    switch (options) {
    case AGTupleCopyOptionsAssignCopy:
        return metadata->vw_assignWithCopy(dest, src);
    case AGTupleCopyOptionsInitCopy:
        return metadata->vw_initializeWithCopy(dest, src);
    case AGTupleCopyOptionsAssignTake:
        return metadata->vw_assignWithTake(dest, src);
    case AGTupleCopyOptionsInitTake:
        return metadata->vw_initializeWithTake(dest, src);
    default:
        AG::precondition_failure("unknown copy options: %d", options);
    }
};

void *AGTupleGetElement(AGTupleType tuple_type, void *tuple_value, uint32_t index, void *element_value,
                        AGTypeID element_type, AGTupleCopyOptions options) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    if (metadata->getKind() != ::swift::MetadataKind::Tuple) {
        if (index != 0) {
            AG::precondition_failure("index out of range: %d", index);
        }
        if (reinterpret_cast<AGTypeID>(metadata) != element_type) {
            AG::precondition_failure("element type mismatch");
        }
        return update(element_value, tuple_value, metadata, options);
    }
    auto tuple_metadata = static_cast<const ::swift::TupleTypeMetadata *>(metadata);
    if (index >= tuple_metadata->NumElements) {
        AG::precondition_failure("index out of range: %d", index);
    }
    auto element = tuple_metadata->getElement(index);
    if (reinterpret_cast<AGTypeID>(element.Type) != element_type) {
        AG::precondition_failure("element type mismatch");
    }
    return update(element_value, element.findIn(reinterpret_cast<::swift::OpaqueValue *>(tuple_value)), element.Type,
                  options);
}

void *AGTupleSetElement(AGTupleType tuple_type, void *tuple_value, uint32_t index, const void *element_value,
                        AGTypeID element_type, AGTupleCopyOptions options) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    if (metadata->getKind() != ::swift::MetadataKind::Tuple) {
        if (index != 0) {
            AG::precondition_failure("index out of range: %d", index);
        }
        if (reinterpret_cast<AGTypeID>(metadata) != element_type) {
            AG::precondition_failure("element type mismatch");
        }
        return update(tuple_value, element_value, metadata, options);
    }
    auto tuple_metadata = static_cast<const ::swift::TupleTypeMetadata *>(metadata);
    if (index >= tuple_metadata->NumElements) {
        AG::precondition_failure("index out of range: %d", index);
    }
    auto element = tuple_metadata->getElement(index);
    if (reinterpret_cast<AGTypeID>(element.Type) != element_type) {
        AG::precondition_failure("element type mismatch");
    }
    return update(element.findIn(reinterpret_cast<::swift::OpaqueValue *>(tuple_value)), element_value, element.Type,
                  options);
}

void AGTupleDestroy(AGTupleType tuple_type, void *tuple_value) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    metadata->vw_destroy(reinterpret_cast<::swift::OpaqueValue *>(tuple_value));
}

void AGTupleDestroyElement(AGTupleType tuple_type, void *tuple_value, uint32_t index) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    if (metadata->getKind() != ::swift::MetadataKind::Tuple) {
        if (index != 0) {
            AG::precondition_failure("index out of range: %d", index);
        }
        metadata->vw_destroy(reinterpret_cast<::swift::OpaqueValue *>(tuple_value));
    }
    auto tuple_metadata = static_cast<const ::swift::TupleTypeMetadata *>(metadata);
    if (index >= tuple_metadata->NumElements) {
        AG::precondition_failure("index out of range: %d", index);
    }
    auto element = tuple_metadata->getElement(index);
    element.Type->vw_destroy(element.findIn(reinterpret_cast<::swift::OpaqueValue *>(tuple_value)));
}

void AGTupleWithBuffer(AGTupleType tuple_type, size_t count,
                       const void (*function)(const AGUnsafeMutableTuple mutable_tuple, const void *context),
                       const void *context) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    auto buffer_size = metadata->vw_stride() * count;
    if (buffer_size <= 0x1000) {
        char buffer[buffer_size];
        memset(&buffer, 0, buffer_size);
        //        function(tuple_type, buffer);
    } else {
        void *buffer = malloc_type_malloc(buffer_size, 0x100004077774924);
        if (buffer == nullptr) {
            AG::precondition_failure("memory allocation failure");
        }
        //        function(tuple_type, buffer);
        free(buffer);
    }
}
