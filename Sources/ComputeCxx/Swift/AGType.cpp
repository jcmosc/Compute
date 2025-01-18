#include "AGType.h"

#include <CoreFoundation/CFString.h>

#include "ContextDescriptor.h"
#include "Metadata.h"

CFStringRef AGTypeDescription(AGTypeID typeID) {
    auto type = reinterpret_cast<const AG::swift::metadata *>(typeID);

    CFMutableStringRef description = CFStringCreateMutable(kCFAllocatorDefault, 0);
    type->append_description(description);
    CFAutorelease(description);
    return description;
}

AGTypeKind AGTypeGetKind(AGTypeID typeID) {
    auto type = reinterpret_cast<const AG::swift::metadata *>(typeID);
    switch (type->getKind()) {
    case swift::MetadataKind::Class:
        return AGTypeKindClass;
    case swift::MetadataKind::Struct:
        return AGTypeKindStruct;
    case swift::MetadataKind::Enum:
        return AGTypeKindEnum;
    case swift::MetadataKind::Optional:
        return AGTypeKindOptional;
    case swift::MetadataKind::Tuple:
        return AGTypeKindTuple;
    case swift::MetadataKind::Function:
        return AGTypeKindFunction;
    case swift::MetadataKind::Existential:
        return AGTypeKindExistential;
    case swift::MetadataKind::Metatype:
        return AGTypeKindMetatype;
    default:
        return AGTypeKindNone;
    }
}

const AGTypeSignature AGTypeGetSignature(AGTypeID typeID) {
    auto type = reinterpret_cast<const AG::swift::metadata *>(typeID);
    const uint32_t *data = static_cast<const uint32_t *>(type->signature());
    if (!data) {
        return AGTypeSignature();
    }
    AGTypeSignature signature = {};
    signature.data[0] = data[0];
    signature.data[1] = data[1];
    signature.data[2] = data[2];
    signature.data[3] = data[3];
    signature.data[4] = data[4];
    return signature;
}

const void *OGTypeGetDescriptor(AGTypeID typeID) {
    auto type = reinterpret_cast<const AG::swift::metadata *>(typeID);
    return type->descriptor();
}

const void *AGTypeNominalDescriptor(AGTypeID typeID) {
    auto type = reinterpret_cast<const AG::swift::metadata *>(typeID);
    return type->nominal_descriptor();
}

const char *AGTypeNominalDescriptorName(AGTypeID typeID) {
    auto type = reinterpret_cast<const AG::swift::metadata *>(typeID);
    auto nominal_descriptor = type->nominal_descriptor();
    if (!nominal_descriptor) {
        return nullptr;
    }
    return nominal_descriptor->name();
}
