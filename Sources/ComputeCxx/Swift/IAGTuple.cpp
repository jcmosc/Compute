#include "ComputeCxx/IAGTuple.h"

#include <swift/Runtime/Metadata.h>

#include "Errors/Errors.h"

IAGTupleType IAGNewTupleType(size_t count, const IAGTypeID *elements) {
    if (count == 1) {
        return elements[0];
    }
    auto metadata_elements = reinterpret_cast<const ::swift::Metadata *const *>(elements);
    auto response = ::swift::swift_getTupleTypeMetadata(::swift::MetadataRequest(),
                                                        ::swift::TupleTypeFlags().withNumElements((unsigned int)count),
                                                        metadata_elements, nullptr, nullptr);
    if (response.State != ::swift::MetadataState::Complete) {
        IAG::precondition_failure("invalid tuple type.");
    }
    return reinterpret_cast<IAGTupleType>(response.Value);
}

size_t IAGTupleCount(IAGTupleType tuple_type) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    if (metadata->getKind() != ::swift::MetadataKind::Tuple) {
        return 1;
    }
    auto tuple_metadata = static_cast<const ::swift::TupleTypeMetadata *>(metadata);
    return static_cast<size_t>(tuple_metadata->NumElements);
}

size_t IAGTupleSize(IAGTupleType tuple_type) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    return metadata->vw_size();
}

IAGTypeID IAGTupleElementType(IAGTupleType tuple_type, size_t index) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    if (metadata->getKind() != ::swift::MetadataKind::Tuple) {
        if (index != 0) {
            IAG::precondition_failure("index out of range: %d", index);
        }
        return reinterpret_cast<IAGTypeID>(metadata);
    }
    auto tuple_metadata = static_cast<const ::swift::TupleTypeMetadata *>(metadata);
    if (index >= tuple_metadata->NumElements) {
        IAG::precondition_failure("index out of range: %d", index);
    }
    auto element = tuple_metadata->getElement((unsigned int)index);
    return reinterpret_cast<IAGTypeID>(element.Type);
}

size_t IAGTupleElementSize(IAGTupleType tuple_type, size_t index) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    if (metadata->getKind() != ::swift::MetadataKind::Tuple) {
        if (index != 0) {
            IAG::precondition_failure("index out of range: %d", index);
        }
        return metadata->vw_size();
    }
    auto tuple_metadata = static_cast<const ::swift::TupleTypeMetadata *>(metadata);
    if (index >= tuple_metadata->NumElements) {
        IAG::precondition_failure("index out of range: %d", index);
    }
    auto element = tuple_metadata->getElement((unsigned int)index);
    return element.Type->vw_size();
}

size_t IAGTupleElementOffset(IAGTupleType tuple_type, size_t index) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    if (metadata->getKind() != ::swift::MetadataKind::Tuple) {
        if (index != 0) {
            IAG::precondition_failure("index out of range: %d", index);
        }
        return 0;
    }
    auto tuple_metadata = static_cast<const ::swift::TupleTypeMetadata *>(metadata);
    if (index >= tuple_metadata->NumElements) {
        IAG::precondition_failure("index out of range: %d", index);
    }
    auto element = tuple_metadata->getElement((unsigned int)index);
    return element.Offset;
}

size_t IAGTupleElementOffsetChecked(IAGTupleType tuple_type, size_t index, IAGTypeID element_type) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    if (metadata->getKind() != ::swift::MetadataKind::Tuple) {
        if (index != 0) {
            IAG::precondition_failure("index out of range: %d", index);
        }
        if (reinterpret_cast<IAGTypeID>(metadata) != element_type) {
            IAG::precondition_failure("element type mismatch");
        }
        return 0;
    }
    auto tuple_metadata = static_cast<const ::swift::TupleTypeMetadata *>(metadata);
    if (index >= tuple_metadata->NumElements) {
        IAG::precondition_failure("index out of range: %d", index);
    }
    auto element = tuple_metadata->getElement((unsigned int)index);
    if (reinterpret_cast<IAGTypeID>(element.Type) != element_type) {
        IAG::precondition_failure("element type mismatch");
    }
    return element.Offset;
}

void *update(void *dest_ptr, const void *src_ptr, const ::swift::Metadata *metadata, IAGTupleCopyOptions options) {
    auto dest = reinterpret_cast<::swift::OpaqueValue *>(dest_ptr);
    auto src = reinterpret_cast<::swift::OpaqueValue *>(const_cast<void *>(src_ptr));
    switch (options) {
    case IAGTupleCopyOptionsAssignCopy:
        return metadata->vw_assignWithCopy(dest, src);
    case IAGTupleCopyOptionsInitCopy:
        return metadata->vw_initializeWithCopy(dest, src);
    case IAGTupleCopyOptionsAssignTake:
        return metadata->vw_assignWithTake(dest, src);
    case IAGTupleCopyOptionsInitTake:
        return metadata->vw_initializeWithTake(dest, src);
    default:
        IAG::precondition_failure("unknown copy options: %d", options);
    }
};

void *IAGTupleGetElement(IAGTupleType tuple_type, void *tuple_value, size_t index, void *element_value,
                        IAGTypeID element_type, IAGTupleCopyOptions options) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    if (metadata->getKind() != ::swift::MetadataKind::Tuple) {
        if (index != 0) {
            IAG::precondition_failure("index out of range: %d", index);
        }
        if (reinterpret_cast<IAGTypeID>(metadata) != element_type) {
            IAG::precondition_failure("element type mismatch");
        }
        return update(element_value, tuple_value, metadata, options);
    }
    auto tuple_metadata = static_cast<const ::swift::TupleTypeMetadata *>(metadata);
    if (index >= tuple_metadata->NumElements) {
        IAG::precondition_failure("index out of range: %d", index);
    }
    auto element = tuple_metadata->getElement((unsigned int)index);
    if (reinterpret_cast<IAGTypeID>(element.Type) != element_type) {
        IAG::precondition_failure("element type mismatch");
    }
    return update(element_value, element.findIn(reinterpret_cast<::swift::OpaqueValue *>(tuple_value)), element.Type,
                  options);
}

void *IAGTupleSetElement(IAGTupleType tuple_type, void *tuple_value, size_t index, const void *element_value,
                        IAGTypeID element_type, IAGTupleCopyOptions options) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    if (metadata->getKind() != ::swift::MetadataKind::Tuple) {
        if (index != 0) {
            IAG::precondition_failure("index out of range: %d", index);
        }
        if (reinterpret_cast<IAGTypeID>(metadata) != element_type) {
            IAG::precondition_failure("element type mismatch");
        }
        return update(tuple_value, element_value, metadata, options);
    }
    auto tuple_metadata = static_cast<const ::swift::TupleTypeMetadata *>(metadata);
    if (index >= tuple_metadata->NumElements) {
        IAG::precondition_failure("index out of range: %d", index);
    }
    auto element = tuple_metadata->getElement((unsigned int)index);
    if (reinterpret_cast<IAGTypeID>(element.Type) != element_type) {
        IAG::precondition_failure("element type mismatch");
    }
    return update(element.findIn(reinterpret_cast<::swift::OpaqueValue *>(tuple_value)), element_value, element.Type,
                  options);
}

void IAGTupleDestroy(IAGTupleType tuple_type, void *tuple_value) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    metadata->vw_destroy(reinterpret_cast<::swift::OpaqueValue *>(tuple_value));
}

void IAGTupleDestroyElement(IAGTupleType tuple_type, void *tuple_value, size_t index) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    if (metadata->getKind() != ::swift::MetadataKind::Tuple) {
        if (index != 0) {
            IAG::precondition_failure("index out of range: %d", index);
        }
        metadata->vw_destroy(reinterpret_cast<::swift::OpaqueValue *>(tuple_value));
    }
    auto tuple_metadata = static_cast<const ::swift::TupleTypeMetadata *>(metadata);
    if (index >= tuple_metadata->NumElements) {
        IAG::precondition_failure("index out of range: %d", index);
    }
    auto element = tuple_metadata->getElement((unsigned int)index);
    element.Type->vw_destroy(element.findIn(reinterpret_cast<::swift::OpaqueValue *>(tuple_value)));
}

void IAGTupleWithBuffer(IAGTupleType tuple_type, size_t count,
                       void (*function)(const IAGUnsafeMutableTuple mutable_tuple, void *context IAG_SWIFT_CONTEXT)
                           IAG_SWIFT_CC(swift),
                       void *context) {
    auto metadata = reinterpret_cast<const ::swift::Metadata *>(tuple_type);
    auto buffer_size = metadata->vw_stride() * count;
    if (buffer_size <= 0x1000) {
        void *buffer = (unsigned char *)alloca(buffer_size);
        bzero((void *)buffer, buffer_size);
        IAGUnsafeMutableTuple tuple = {tuple_type, buffer};
        function(tuple, context);
    } else {
        void *buffer = malloc(buffer_size);
        if (buffer == nullptr) {
            IAG::precondition_failure("memory allocation failure");
        }
        IAGUnsafeMutableTuple tuple = {tuple_type, buffer};
        function(tuple, context);
        free(buffer);
    }
}
