#include "Metadata.h"

#include <CommonCrypto/CommonDigest.h>
#include <CoreFoundation/CFString.h>
#include <SwiftEquatableSupport.h>
#include <swift/Runtime/Casting.h>
#include <swift/Runtime/ExistentialContainer.h>
#include <swift/Runtime/HeapObject.h>

#include "ContextDescriptor.h"
#include "Errors/Errors.h"
#include "MetadataVisitor.h"
#include "Swift/mach-o/dyld.h"
#include "Utilities/HashTable.h"
#include "Utilities/Heap.h"
#include "_SwiftStdlibCxxOverlay.h"

namespace AG {
namespace swift {

#pragma mark - Metadata

const char *metadata::name(bool qualified) const {
    auto result = swift_getTypeName(this, qualified);
    return result.data;
};

const context_descriptor *_Nullable metadata::descriptor() const {
    switch (getKind()) {
    case ::swift::MetadataKind::Class: {
        if (is_type_metadata()) {
            const auto class_type = static_cast<const ::swift::ClassMetadata *>(base());
            return reinterpret_cast<const context_descriptor *>(class_type->getDescription());
        }
        return nullptr;
    }
    case ::swift::MetadataKind::Struct:
    case ::swift::MetadataKind::Enum:
    case ::swift::MetadataKind::Optional: {
        auto descriptor = static_cast<const ::swift::TargetValueMetadata<::swift::InProcess> *>(base())->Description;
        return reinterpret_cast<const context_descriptor *>(descriptor);
    }
    default:
        return nullptr;
    }
}

const type_context_descriptor *_Nullable metadata::nominal_descriptor() const {
    auto result = descriptor();
    if (!result) {
        return nullptr;
    }
    switch (result->getKind()) {
    case ::swift::ContextDescriptorKind::Struct:
    case ::swift::ContextDescriptorKind::Enum:
        return reinterpret_cast<const type_context_descriptor *>(result);
    default:
        return nullptr;
    }
};

struct parent_info {
    const char *name;
    uint64_t generic_args_end;
};

void metadata::append_description(CFMutableStringRef description) const {
    // Append function signature
    if (auto functiom_metadata = llvm::dyn_cast<::swift::FunctionTypeMetadata>(base())) {
        CFStringAppendCString(description, "(", kCFStringEncodingUTF8);
        size_t num_parameters = functiom_metadata->getNumParameters();
        for (int i = 0; i < num_parameters; i++) {
            if (i > 0) {
                CFStringAppendCString(description, ", ", kCFStringEncodingUTF8);
            }
            auto parameter = functiom_metadata->getParameter(i);
            if (parameter) {
                metadata::from_base(parameter)->append_description(description);
            } else {
                CFStringAppendCString(description, "nil", kCFStringEncodingUTF8);
            }
        }
        CFStringAppendCString(description, ")", kCFStringEncodingUTF8);

        CFStringAppendCString(description, " -> ", kCFStringEncodingUTF8);

        if (!functiom_metadata->ResultType) {
            CFStringAppendCString(description, "nil", kCFStringEncodingUTF8);
            return;
        }

        metadata::from_base(functiom_metadata->ResultType)->append_description(description);
        return;
    }

    // Append tuple signature
    if (auto tuple_metadata = llvm::dyn_cast<::swift::TupleTypeMetadata>(base())) {
        if (tuple_metadata->NumElements) {
            CFStringAppendCString(description, "(", kCFStringEncodingUTF8);
            for (int i = 0; i < tuple_metadata->NumElements; i++) {
                if (i > 0) {
                    CFStringAppendCString(description, ", ", kCFStringEncodingUTF8);
                }
                auto element = tuple_metadata->getElement(i);
                if (element.Type) {
                    metadata::from_base(element.Type)->append_description(description);
                } else {
                    CFStringAppendCString(description, "nil", kCFStringEncodingUTF8);
                }
            }
            CFStringAppendCString(description, ")", kCFStringEncodingUTF8);
        } else {
            CFStringAppendCString(description, "Void", kCFStringEncodingUTF8);
        }
        return;
    }

    // Collect all parent types
    vector<parent_info, 8, uint64_t> all_parents = {};
    for (auto parent = descriptor(); parent != nullptr; parent = parent->parent()) {
        if (auto nominal_parent = llvm::dyn_cast<::swift::TypeContextDescriptor>(parent->base())) {
            const char *name = nominal_parent->Name.get();
            uint64_t total_generic_args = parent->count_generic_args();
            all_parents.push_back({name, total_generic_args});
        }
    }
    if (all_parents.empty()) {
        all_parents.push_back({
            name(true),
            0,
        });
    }

    // Append the type name, starting from the outermost parent type
    vector<context_descriptor::generic_arg, 8, uint64_t> generic_args = {};
    if (auto metadata_descriptor = descriptor()) {
        metadata_descriptor->push_generic_args(*this, generic_args);
    }
    if (all_parents.size()) {
        uint64_t generic_args_start = 0;
        for (auto parent = all_parents.rbegin(), end = all_parents.rend(); parent != end; parent++) {
            CFStringAppendCString(description, parent->name, kCFStringEncodingUTF8);
            if (parent->generic_args_end != generic_args_start) {
                CFStringAppendCString(description, "<", kCFStringEncodingUTF8);
                for (uint64_t i = generic_args_start; i < parent->generic_args_end; i++) {
                    if (i > generic_args_start) {
                        CFStringAppendCString(description, ", ", kCFStringEncodingUTF8);
                    }
                    context_descriptor::generic_arg arg = generic_args[i];
                    if (arg.is_pack) {
                        CFStringAppendCString(description, "Pack{", kCFStringEncodingUTF8);
                    }
                    for (int j = 0; j < arg.num_types; j++) {
                        if (j > 0) {
                            CFStringAppendCString(description, ", ", kCFStringEncodingUTF8);
                        }
                        arg.types[j].append_description(description);
                    }
                    if (arg.is_pack) {
                        CFStringAppendCString(description, "}", kCFStringEncodingUTF8);
                    }
                }
                CFStringAppendCString(description, ">", kCFStringEncodingUTF8);
            }
            if (parent + 1 < end) {
                CFStringAppendCString(description, ".", kCFStringEncodingUTF8);
            }
            generic_args_start = parent->generic_args_end;
        }
    }
}

namespace {

class TypeSignatureCache {
  private:
    os_unfair_lock _lock;
    util::Table<const metadata *, const unsigned char *> _table;

  public:
    TypeSignatureCache() : _lock(OS_UNFAIR_LOCK_INIT), _table(){};

    void lock() { os_unfair_lock_lock(&_lock); };
    void unlock() { os_unfair_lock_unlock(&_lock); };

    const unsigned char *lookup(const metadata *type, const metadata **_Nullable found) {
        return _table.lookup(type, found);
    }

    bool insert(const metadata *type, const unsigned char *signature) { return _table.insert(type, signature); }
};

} // namespace

const void *metadata::signature() const {
    static TypeSignatureCache *cache = new TypeSignatureCache();

    const metadata *found = nullptr;
    cache->lock();
    const unsigned char *signature = cache->lookup(this, &found);
    cache->unlock();
    if (found) {
        return signature;
    }

    auto metadata_queue = vector<const metadata *, 8, uint64_t>();
    auto generic_args = vector<context_descriptor::generic_arg, 8, uint64_t>();
    auto descriptors = vector<const context_descriptor *, 8, uint64_t>();

    metadata_queue.push_back(this);

    while (metadata_queue.size() > 0) {
        auto metadata = metadata_queue.back();
        metadata_queue.pop_back();

        generic_args.clear();

        auto descriptor = metadata->descriptor();
        if (descriptor) {
            descriptors.push_back(descriptor);
            descriptor->push_generic_args(*metadata, generic_args);
        }
        for (auto generic_arg : generic_args) {
            for (int i = 0; i < generic_arg.num_types; i++) {
                metadata_queue.push_back(&generic_arg.types[i]);
            }
        }
    }

    if (descriptors.size()) {
        auto context = CC_SHA1_CTX();
        CC_SHA1_Init(&context);

        const char prefix[] = "AGTypeSignature";
        CC_SHA1_Update(&context, prefix, sizeof(prefix));

        auto infos = vector<dyld_image_uuid_offset, 8, uint64_t>();
        infos.reserve(descriptors.size());

        dyld_images_for_addresses((unsigned)descriptors.size(), static_cast<const void *[]>(descriptors.data()),
                                  infos.data());

        for (auto info : infos) {
            CC_SHA1_Update(&context, info.uuid, sizeof(((dyld_image_uuid_offset *)0)->uuid));
            CC_SHA1_Update(&context, &info.offsetInImage, sizeof(((dyld_image_uuid_offset *)0)->offsetInImage));
        }

        auto digest = new unsigned char[CC_SHA1_DIGEST_LENGTH];
        CC_SHA1_Final(digest, &context);

        signature = digest;
    } else {
        signature = nullptr;
    }

    cache->lock();
    cache->insert(this, signature);
    cache->unlock();

    return signature;
}

const equatable_witness_table *metadata::equatable() const {
    switch (getKind()) {
    case ::swift::MetadataKind::Class: {
        static const equatable_witness_table *nsobject_conformance = []() -> const equatable_witness_table * {
            Class nsobject = objc_getClass("NSObject");
            if (!nsobject) {
                return nullptr;
            }
            auto nsobject_metadata = swift_getObjCClassMetadata(reinterpret_cast<::swift::ClassMetadata *>(nsobject));
            if (!nsobject_metadata) {
                return nullptr;
            }
            auto witness_table =
                swift_conformsToProtocol(nsobject_metadata, &::swift::equatable_support::EquatableProtocolDescriptor);
            return reinterpret_cast<const equatable_witness_table *>(witness_table);
        }();
        auto conformance = reinterpret_cast<const equatable_witness_table *>(
            swift_conformsToProtocol(this, &::swift::equatable_support::EquatableProtocolDescriptor));
        if (conformance == nsobject_conformance) {
            return nullptr;
        }
        return conformance;
    }
    case ::swift::MetadataKind::Struct:
    case ::swift::MetadataKind::Enum:
    case ::swift::MetadataKind::Optional:
    case ::swift::MetadataKind::Tuple: {
        auto witness_table = swift_conformsToProtocol(this, &::swift::equatable_support::EquatableProtocolDescriptor);
        return reinterpret_cast<const equatable_witness_table *>(witness_table);
    }
    default:
        return nullptr;
    }
}

#pragma mark Mutating objects

void metadata::copy_on_write_heap_object(void **object_ref) const {
    if (::swift::swift_isUniquelyReferencedNonObjC(*object_ref)) {
        return;
    }

    assert(::swift::isHeapMetadataKind(getKind()));
    auto heap_metadata = reinterpret_cast<const ::swift::HeapMetadata *>(this);

    ::swift::HeapObject *copy =
        ::swift::swift_allocObject(heap_metadata, vw_size(), getValueWitnesses()->getAlignmentMask());
    vw_initializeWithCopy(reinterpret_cast<opaque_value *>(copy), reinterpret_cast<opaque_value *>(*object_ref));
    ::swift::swift_release(reinterpret_cast<::swift::HeapObject *>(*object_ref));
    *object_ref = copy;
}

#pragma mark Looking up types by name

const metadata *metadata::mangled_type_name_ref(const char *type_name, bool fault_if_null, ref_kind *kind_out) const {
    if (!type_name) {
        return nullptr;
    }

    auto context = descriptor();

    const void *const *generic_args = nullptr;
    if (context && context->isGeneric()) {
        switch (context->getKind()) {
        case ::swift::ContextDescriptorKind::Class: {
            if (auto base = llvm::dyn_cast<::swift::ClassDescriptor>(context)) {
                auto class_descriptor = class_type_descriptor::from_base(base);

                auto asWords = reinterpret_cast<const void *const *>(this);
                generic_args = asWords + class_descriptor->immediate_members_offset();
            }
            break;
        }
        case ::swift::ContextDescriptorKind::Struct: {
            if (auto struct_descriptor = llvm::dyn_cast<::swift::StructDescriptor>(context)) {
                auto asWords = reinterpret_cast<const void *const *>(this);
                generic_args = asWords + struct_descriptor->getGenericArgumentOffset();
            }
            break;
        }
        case ::swift::ContextDescriptorKind::Enum: {
            if (auto enum_descriptor = llvm::dyn_cast<::swift::EnumDescriptor>(context)) {
                auto asWords = reinterpret_cast<const void *const *>(this);
                generic_args = asWords + enum_descriptor->getGenericArgumentOffset();
            }
        }
        default:
            break;
        }
    }

    // See https://github.com/swiftlang/swift/blob/main/docs/ABI/Mangling.rst#symbolic-references
    auto string = ::swift::Demangle::makeSymbolicMangledNameStringRef(type_name);
    auto type = swift_getTypeByMangledNameInContext(string.data(), string.size(), context, generic_args);
    if (!type) {
        if (fault_if_null) {
            std::string ascii_type_name = "";
            for (auto c = string.begin(), end = string.end(); c != end; c++) {
                if (*c == '\\') {
                    ascii_type_name.append("\\\\", 2);
                } else {
                    if (*c > 126) {
                        // char is outside of ASCII range, encode as "\ABC" where ABC are digits between 0 and 7.
                        ascii_type_name.push_back('\\');
                        ascii_type_name.push_back((*c >> 6) | 0x30);
                        ascii_type_name.push_back(((*c >> 3) & 7) | 0x30);
                        ascii_type_name.push_back((uint8_t)((*c & 7) | 0x30));
                    } else {
                        ascii_type_name.push_back(*c);
                    }
                }
            }

            non_fatal_precondition_failure("Swift metadata failure: \"%s\", context %s (%p), args %p",
                                           ascii_type_name.c_str(), name(false), context, generic_args);
        }
        return nullptr;
    }

    if (kind_out) {
        *kind_out = ref_kind::strong;
        if (string.size() >= 3 && string[string.size() - 2] == 'X') {
            switch (string[string.size() - 1]) {
            case 'o':
                *kind_out = ref_kind::unowned;
                break;
            case 'w':
                *kind_out = ref_kind::weak;
                break;
            case 'u':
                *kind_out = ref_kind::unmanaged;
                break;
            }
        }
    }

    return metadata::from_base(type);
}

namespace {

class TypeCache {
  public:
    using key_info = std::pair<const metadata *, const char *>;
    using value_info = std::pair<const metadata *, metadata::ref_kind>;

  private:
    os_unfair_lock _lock;
    util::Heap _heap;
    char _heap_buffer[4096];
    util::Table<const key_info *, const value_info *> _table;

  public:
    TypeCache()
        : _lock(OS_UNFAIR_LOCK_INIT), _heap(_heap_buffer, 4096, 0),
          _table([](const key_info *key) -> uint64_t { return uintptr_t(key->first) * 0x21 ^ uintptr_t(key->second); },
                 [](const key_info *a, const key_info *b) -> bool {
                     if (a->first != b->first) {
                         return false;
                     }
                     return a->second == b->second;
                 },
                 nullptr, nullptr, &_heap){};

    void lock() { os_unfair_lock_lock(&_lock); };
    void unlock() { os_unfair_lock_unlock(&_lock); };

    const value_info *lookup(const key_info *type, const key_info **_Nullable found) {
        return _table.lookup(type, found);
    }

    bool insert(const key_info *key, const value_info *value) { return _table.insert(key, value); };

    key_info *create_key(const metadata *type, const char *type_name) {
        auto key = _heap.alloc<TypeCache::key_info>();
        key->first = type;
        key->second = type_name;
        return key;
    };

    value_info *create_value(const metadata *type, metadata::ref_kind ref_kind) {
        auto value = _heap.alloc<TypeCache::value_info>();
        value->first = type;
        value->second = ref_kind;
        return value;
    };
};

} // namespace

const metadata *metadata::mangled_type_name_ref_cached(const char *type_name, ref_kind *kind_out) const {
    if (!type_name) {
        return nullptr;
    }

    static TypeCache *cache = new TypeCache();

    cache->lock();
    TypeCache::key_info lookup_key = {this, type_name};
    auto result = cache->lookup(&lookup_key, nullptr);
    cache->unlock();

    if (!result) {
        ref_kind kind;
        const metadata *type = mangled_type_name_ref(type_name, true, &kind);

        cache->lock();
        auto key = cache->create_key(this, type_name);
        auto value = cache->create_value(type, kind);
        cache->insert(key, value);
        cache->unlock();

        result = value;
    }

    if (kind_out) {
        *kind_out = result->second;
    }
    return result->first;
}

#pragma mark Visiting

bool metadata::visit(metadata_visitor &visitor) const {
    switch (getKind()) {
    case ::swift::MetadataKind::Class: {
        return visitor.visit_class(reinterpret_cast<const any_class_type_metadata &>(*this));
    }
    case ::swift::MetadataKind::Struct: {
        auto struct_type = reinterpret_cast<const ::swift::StructMetadata *>(this);
        auto context = descriptor();
        if (context && ::swift::StructDescriptor::classof(context)) {
            auto struct_context = reinterpret_cast<const ::swift::StructDescriptor *>(context);
            if (struct_context->Fields && struct_context->hasFieldOffsetVector()) {
                auto field_offsets = struct_type->getFieldOffsets();
                unsigned index = 0;
                for (auto &field : struct_context->Fields->getFields()) {
                    size_t offset = field_offsets[index];
                    size_t end_offset = index + 1 < struct_context->NumFields ? field_offsets[index + 1] : vw_size();
                    size_t field_size = offset <= end_offset ? end_offset - offset : -1;
                    if (!visitor.visit_field(*this, field, offset, field_size)) {
                        return false;
                    }
                    index += 1;
                }
                return true;
            }
        }
        return visitor.unknown_result();
    }
    case ::swift::MetadataKind::Enum:
    case ::swift::MetadataKind::Optional: {
        auto enum_type = reinterpret_cast<const ::swift::EnumMetadata *>(this);
        auto context = descriptor();
        if (context && ::swift::EnumDescriptor::classof(context)) {
            auto enum_context = reinterpret_cast<const ::swift::EnumDescriptor *>(context);
            if (enum_context->Fields) {
                if (enum_context->getNumPayloadCases() != 0) {
                    if (enum_context->Fields->NumFields == 0) {
                        return true;
                    }
                    unsigned index = 0;
                    for (auto &field : enum_context->Fields->getFields()) {
                        if (!visitor.visit_case(*this, field, index)) {
                            return false;
                        }
                        index += 1;
                    }
                    return true;
                }
            }
        }
        return visitor.unknown_result();
    }
    case ::swift::MetadataKind::Opaque: {
        // Builtin.NativeObject, see https://github.com/swiftlang/swift/blob/main/docs/ABI/Mangling.rst
        static const metadata *native_object = mangled_type_name_ref("Bo", true, nullptr);
        if (this == native_object) {
            return visitor.visit_native_object(*this);
        }
        return visitor.unknown_result();
    }
    case ::swift::MetadataKind::Tuple: {
        auto tuple_type = reinterpret_cast<const ::swift::TupleTypeMetadata *>(this);
        for (unsigned index = 0; index < tuple_type->NumElements; ++index) {
            const auto &element = tuple_type->getElement(index);
            if (element.Type) {
                size_t element_offset = element.Offset;
                size_t element_size = element.Type->vw_size();

                if (element.Type->getKind() == ::swift::MetadataKind::Metatype) {
                    size_t end_offset =
                        index + 1 < tuple_type->NumElements ? tuple_type->getElement(index + 1).Offset : vw_size();
                    size_t calculated_size = element_offset <= end_offset ? end_offset - element_offset : -1;
                    element_size = std::min(element_size, calculated_size);
                }
                if (!visitor.visit_element(*metadata::from_base(element.Type), metadata::ref_kind::strong,
                                           element.Offset, element_size)) {
                    return false;
                }
            }
        }
        return true;
    }
    case ::swift::MetadataKind::Function: {
        return visitor.visit_function(reinterpret_cast<const function_type_metadata &>(*this));
    }
    case ::swift::MetadataKind::Existential: {
        return visitor.visit_existential(reinterpret_cast<const existential_type_metadata &>(*this));
    }
    default:
        return visitor.unknown_result();
    }
}

bool metadata::visit_heap(metadata_visitor &visitor, visit_options options) const {
    switch (getKind()) {
    case ::swift::MetadataKind::Class: {
        if (options & visit_options::heap_class) {
            return visit_heap_class(visitor);
        }
        return visitor.unknown_result();
    }
    case ::swift::MetadataKind::HeapLocalVariable: {
        if (options & visit_options::heap_locals) {
            return visit_heap_locals(visitor);
        }
        return visitor.unknown_result();
    }
    case ::swift::MetadataKind::HeapGenericLocalVariable: {
        auto generic_heap_type = reinterpret_cast<const ::swift::GenericBoxHeapMetadata *>(this);
        if (options & visit_options::heap_generic_locals && generic_heap_type->BoxedType) {
            auto element_type = generic_heap_type->BoxedType;
            auto alignment_mask = element_type->getValueWitnesses()->getAlignmentMask();
            size_t offset = (generic_heap_type->Offset + alignment_mask) & ~alignment_mask;
            return visitor.visit_element(reinterpret_cast<const metadata &>(*element_type), metadata::ref_kind::strong,
                                         offset, element_type->vw_size());
        }
        return visitor.unknown_result();
    }
    default:
        return visitor.unknown_result();
    }
}

bool metadata::visit_heap_class(metadata_visitor &visitor) const {
    const auto class_type = static_cast<const ::swift::ClassMetadata *>(base());

    if ((class_type->Data & 3) == 0) {
        // Pure Objective-C class unsupported
        return visitor.unknown_result();
    }

    auto context = descriptor();
    if (!context) {
        return visitor.unknown_result();
    }

    auto class_context = reinterpret_cast<const ::swift::ClassDescriptor *>(context);
    if (class_context->SuperclassType && class_type->Superclass) {
        if (!class_type->Superclass->isClassObject()) {
            return visitor.unknown_result();
        }
        if (!reinterpret_cast<const metadata &>(*class_type->Superclass).visit_heap_class(visitor)) {
            return false;
        }
    }

    if (!class_context->Fields) {
        return true;
    }

    auto fields = class_context->Fields.get();
    if (!fields) {
        return true;
    }

    if (!fields->NumFields) {
        return true;
    }

    if (fields->NumFields != class_context->NumFields) {
        return visitor.unknown_result();
    }

    if ((class_type->Flags & ::swift::ClassFlags::UsesSwiftRefcounting) == 0) {
        size_t *ivar_offsets = nullptr; // TODO: use new

        unsigned int ivar_count;
        Ivar *ivar_list = class_copyIvarList(reinterpret_cast<const Class>((void *)this), &ivar_count);
        if (ivar_list) {
            if (ivar_count == fields->NumFields) {
                ivar_offsets = (size_t *)calloc(ivar_count, sizeof(size_t)); // or bzero?
                for (unsigned int ivar_index = 0; ivar_index < ivar_count; ++ivar_index) {
                    ivar_offsets[ivar_index] = ivar_getOffset(ivar_list[ivar_index]);
                }
            }
            free(ivar_list);
        }

        if (ivar_offsets && *ivar_offsets != 0) {
            unsigned index = 0;
            for (auto &field : fields->getFields()) {
                size_t offset = ivar_offsets[index];
                size_t end_offset = index + 1 < fields->NumFields ? ivar_offsets[index + 1] : -1;
                size_t field_size = offset <= end_offset ? end_offset - offset : -1;
                if (!visitor.visit_field(*this, field, offset, field_size)) {
                    if (ivar_offsets) {
                        free(ivar_offsets);
                    }
                    return false;
                }
                index += 1;
            }
            if (ivar_offsets) {
                free(ivar_offsets);
            }
            return true;
        }

        if (ivar_offsets) {
            free(ivar_offsets);
        }
        return visitor.unknown_result();
    } else {
        if (class_context->hasFieldOffsetVector()) {
            const size_t *field_offsets = nullptr;
            auto offset = reinterpret_cast<const class_type_descriptor *>(class_context)->field_offset_vector_offset();
            auto asWords = reinterpret_cast<const size_t *const *>(this);
            field_offsets = reinterpret_cast<const size_t *>(asWords + offset);

            unsigned index = 0;
            for (auto &field : class_context->Fields->getFields()) {
                size_t offset = field_offsets[index];
                size_t end_offset = index + 1 < class_context->NumFields ? field_offsets[index + 1] : vw_size();
                size_t field_size = offset <= end_offset ? end_offset - offset : -1;
                if (!visitor.visit_field(*this, field, offset, field_size)) {
                    return false;
                }
                index += 1;
            }
            return true;
        }
        return visitor.unknown_result();
    }

    return visitor.unknown_result();
}

bool metadata::visit_heap_locals(metadata_visitor &visitor) const {
    auto local_type = reinterpret_cast<const ::swift::HeapLocalVariableMetadata *>(this);
    if (!local_type->CaptureDescription || local_type->CaptureDescription[1] != '\0' ||
        local_type->OffsetToFirstCapture == 0) {
        return visitor.unknown_result();
    }

    auto descriptor = reinterpret_cast<const ::swift::reflection::CaptureDescriptor *>(local_type->CaptureDescription);

    if (descriptor->NumBindings) {
        size_t offset = local_type->OffsetToFirstCapture;
        for (unsigned i = 0; i < descriptor->NumBindings; ++i) {
            // Builtin.RawPointer, see https://github.com/swiftlang/swift/blob/main/docs/ABI/Mangling.rst
            static const metadata *pointer_type = mangled_type_name_ref("Bp", true, nullptr);
            if (pointer_type == nullptr) {
                return visitor.unknown_result();
            }
            visitor.visit_element(*pointer_type, metadata::ref_kind::unmanaged, offset, vw_size());
            offset += sizeof(void *);
        }
    }

    size_t offset = local_type->OffsetToFirstCapture;
    for (auto capture_type_record = descriptor->capture_begin(), end = descriptor->capture_end();
         capture_type_record != end; ++capture_type_record) {
        const char *mangled_name = nullptr;
        if (capture_type_record->hasMangledTypeName()) {
            mangled_name = capture_type_record->getMangledTypeName().data();
        }
        metadata::ref_kind kind = metadata::ref_kind::strong;
        const metadata *element_type = mangled_type_name_ref(mangled_name, true, &kind);
        if (!element_type) {
            return visitor.unknown_result();
        }

        size_t size = element_type->vw_size();
        size_t alignment_mask = element_type->getValueWitnesses()->getAlignmentMask();
        size_t offset = (offset + alignment_mask) & ~alignment_mask;

        if (!visitor.visit_element(*element_type, kind, offset, size)) {
            return false;
        }

        offset += size;
    }

    return true;
}

#pragma mark - Existential type metadata

::swift::ExistentialTypeRepresentation existential_type_metadata::representation() const {
    // Some existentials use special containers.
    switch (Flags.getSpecialProtocol()) {
    case ::swift::SpecialProtocol::Error:
        return ::swift::ExistentialTypeRepresentation::Error;
    case ::swift::SpecialProtocol::None:
        break;
    }
    // The layout of standard containers depends on whether the existential is
    // class-constrained.
    if (isClassBounded())
        return ::swift::ExistentialTypeRepresentation::Class;
    return ::swift::ExistentialTypeRepresentation::Opaque;
}

const void *existential_type_metadata::project_value(void *container) const {
    switch (representation()) {
    case ::swift::ExistentialTypeRepresentation::Class: {
        auto class_container = reinterpret_cast<const ::swift::ClassExistentialContainer *>(container);
        return reinterpret_cast<const void *>(&class_container->Value);
    }
    case ::swift::ExistentialTypeRepresentation::Opaque: {
        auto *opaque_container = reinterpret_cast<const ::swift::OpaqueExistentialContainer *>(container);

        auto *vwt = opaque_container->Type->getValueWitnesses();

        if (vwt->isValueInline()) {
            return reinterpret_cast<const void *>(&opaque_container->Buffer);
        }

        // Compute the byte offset of the object in the box.
        size_t alignment_mask = vwt->getAlignmentMask();
        size_t byte_offset = (sizeof(::swift::HeapObject) + alignment_mask) & ~alignment_mask;
        auto *byte_pointer = reinterpret_cast<const char *>(
            *reinterpret_cast<::swift::HeapObject *const *const>(&opaque_container->Buffer));
        return reinterpret_cast<const void *>(byte_pointer + byte_offset);
    }
    case ::swift::ExistentialTypeRepresentation::Error:
        return nullptr;
    }
}

const metadata *existential_type_metadata::dynamic_type(void *container) const {
    switch (representation()) {
    case ::swift::ExistentialTypeRepresentation::Class: {
        auto class_container = reinterpret_cast<const ::swift::ClassExistentialContainer *>(container);
        void *obj = class_container->Value;
        return static_cast<const metadata *>(swift_getObjectType(reinterpret_cast<::swift::HeapObject *>(obj)));
    }
    case ::swift::ExistentialTypeRepresentation::Opaque: {
        auto opaque_container = reinterpret_cast<const ::swift::OpaqueExistentialContainer *>(container);
        return static_cast<const metadata *>(opaque_container->Type);
    }
    case ::swift::ExistentialTypeRepresentation::Error: {
        return nullptr;
    }
    }
}

} // namespace swift
} // namespace AG
