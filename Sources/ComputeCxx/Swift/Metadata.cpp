#include "Metadata.h"

#include <CommonCrypto/CommonDigest.h>
#include <CoreFoundation/CFString.h>
#include <SwiftEquatableSupport.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach-o/dyld_images.h>
#include <os/lock.h>
#include <swift/Runtime/Casting.h>
#include <swift/Runtime/HeapObject.h>
#include <uuid/uuid.h>

#include "ContextDescriptor.h"
#include "Errors/Errors.h"
#include "MetadataVisitor.h"
#include "Swift/mach-o/dyld.h"
#include "Util/HashTable.h"
#include "Util/Heap.h"
#include "Vector/Vector.h"
#include "_SwiftStdlibCxxOverlay.h"

namespace AG {
namespace swift {

const char *metadata::name(bool qualified) const {
    auto result = swift_getTypeName(this, qualified);
    return result.data;
};

const context_descriptor *_Nullable metadata::descriptor() const {
    switch (getKind()) {
    case ::swift::MetadataKind::Class: {
        const auto cls = static_cast<const ::swift::ClassMetadata *>(base());
        // Depending on the deployment target a binary was compiled for,
        // statically emitted metadata templates may have a different bit set
        // from the one that this runtime canonically considers the "is Swift" bit.
        if (cls->Data & 3) {
            return reinterpret_cast<const context_descriptor *>(cls->getDescription());
        }
        return nullptr;
    }
    case ::swift::MetadataKind::Struct:
    case ::swift::MetadataKind::Enum:
    case ::swift::MetadataKind::Optional: {
        auto desc = static_cast<const ::swift::TargetValueMetadata<::swift::InProcess> *>(base())->Description;
        return reinterpret_cast<const context_descriptor *>(desc);
    }
    default:
        return nullptr;
    }
}

const context_descriptor *_Nullable metadata::nominal_descriptor() const {
    auto desc = descriptor();
    if (!desc) {
        return nullptr;
    }

    switch (desc->getKind()) {
    case ::swift::ContextDescriptorKind::Struct:
    case ::swift::ContextDescriptorKind::Enum:
        return desc;
    default:
        return nullptr;
    }
};

const witness_table *metadata::equatable() const {

    switch (getKind()) {
    case ::swift::MetadataKind::Class: {
        static const witness_table *nsobject_conformance = []() -> const witness_table * {
            Class cls = objc_getClass("NSObject");
            if (!cls) {
                return nullptr;
            }
            auto metadata = swift_getObjCClassMetadata(reinterpret_cast<::swift::ClassMetadata *>(cls));
            if (!metadata) {
                return nullptr;
            }
            return swift_conformsToProtocol(metadata, &::swift::equatable_support::EquatableProtocolDescriptor);
        }();
        const witness_table *conformance =
            swift_conformsToProtocol(this, &::swift::equatable_support::EquatableProtocolDescriptor);
        if (conformance == nsobject_conformance) {
            return nullptr;
        }
        return conformance;
    }
    case ::swift::MetadataKind::Struct:
    case ::swift::MetadataKind::Enum:
    case ::swift::MetadataKind::Optional:
    case ::swift::MetadataKind::Tuple:
        return swift_conformsToProtocol(this, &::swift::equatable_support::EquatableProtocolDescriptor);
    default:
        break;
    }
}

const metadata *metadata::mangled_type_name_ref(const char *type_name, bool fault_if_null, ref_kind *kind_out) const {
    if (!type_name) {
        return nullptr;
    }

    auto context = descriptor();

    if (context && context->isGeneric()) {
        switch (context->getKind()) {
        case ::swift::ContextDescriptorKind::Class:

            break;
        case ::swift::ContextDescriptorKind::Struct:
        case ::swift::ContextDescriptorKind::Enum:
        default:
            break;
        }
    }

    // TODO: need this or can use getGenericArgs()?

    //    v9 = v8;
    //    if ( !v8 || (*(_DWORD *)v8 & 0x80) == 0 )
    //        goto LABEL_9;
    //    v10 = *(_DWORD *)v8 & 0x1F;
    //    if ( (unsigned int)(v10 - 17) < 2 )
    //    {
    //        v11 = (char *)a1 + 16;
    //        goto LABEL_10;
    //    }
    //    if ( v10 == 16 )
    //        v11 = (char *)a1 + 8 * AG::swift::class_type_descriptor::immediate_members_offset(v8);
    //    else
    //        LABEL_9:
    //        v11 = 0LL;
    //    if (context && context->isGeneric()) {
    //        switch (context->getKind()) {
    //        case ::swift::ContextDescriptorKind::Class:
    //            const auto cls = static_cast<const ::swift::ClassMetadata *>(base());
    //            cls->getGenericArgs()
    //                //                offset = desc->immediate_members_offset();
    //                offset = 0;
    //            break;
    //        case ::swift::ContextDescriptorKind::Struct:
    //        case ::swift::ContextDescriptorKind::Enum:
    //            offset = 2;
    //            break;
    //        default:
    //            offset = 0;
    //            break;
    //        }
    //    }

    const void *const *generic_args = reinterpret_cast<const void *const *>(getGenericArgs());

    // See https://github.com/swiftlang/swift/blob/main/docs/ABI/Mangling.rst#symbolic-references
    // TODO:
    // https://github.com/swiftlang/swift/blob/217311fa7f3087de99542e3d3924adb1647eb687/lib/Demangling/Demangler.cpp#L165
    const char *c = type_name;
    while (*c) {
        if (*c < 0x18) {
            // relative symbolic reference
            c += 5;
            break;
        }
        if (*c < 0x20) {
            // absolute symbolic reference
            c += 1 + sizeof(void *);
            break;
        }
        c += 1;
    }
    size_t type_name_length = c - type_name;

    const metadata *_Nullable result =
        metadata::from_base(swift_getTypeByMangledNameInContext(type_name, type_name_length, context, generic_args));

    if (!result && fault_if_null) {
        std::string s = "";
        const char *c = type_name;
        for (int i = 0; i < type_name_length; i++) {
            if (*c == '\\') {
                s.append("\\\\", 2);
            } else {
                if (*c > 126) {
                    // char is outside of ASCII range, encode as "\ABC" where ABC are digits between 0 and 7.
                    s.push_back('\\');
                    s.push_back((*c >> 6) | 0x30);
                    s.push_back(((*c >> 3) & 7) | 0x30);
                    s.push_back((uint8_t)((*c & 7) | 0x30));
                } else {
                    s.push_back(*c);
                }
            }
        }

        non_fatal_precondition_failure("Swift metadata failure: \"%s\", context %s (%p), args %p", s.c_str(),
                                       name(false), context, generic_args);
    }

    if (kind_out && result) {
        *kind_out = ref_kind::none;
        if (type_name_length >= 3 && type_name[type_name_length - 2] == 'X') {
            switch (type_name[type_name_length - 1]) {
            case 'o':
                *kind_out = ref_kind::unowned;
                break;
            case 'w':
                *kind_out = ref_kind::weak;
                break;
            case 'u':
                *kind_out = ref_kind::unowned_unsafe;
                break;
            }
        }
    }

    return result;
}

namespace {

class TypeCache {
  private:
    os_unfair_lock _lock;
    util::Heap _heap;
    char _heap_buffer[4096];
    util::UntypedTable _table;

  public:
    using key_info = std::pair<const metadata *, const char *>;
    using value_info = std::pair<const metadata *, metadata::ref_kind>;

    TypeCache()
        : _lock(OS_UNFAIR_LOCK_INIT), _heap(_heap_buffer, 4096, 0),
          _table(
              [](const void *key) -> uint64_t {
                  auto pair = reinterpret_cast<const key_info *>(key);
                  return uintptr_t(pair->first) * 0x21 ^ uintptr_t(pair->second);
              },
              [](const void *a, const void *b) -> bool {
                  auto a_pair = reinterpret_cast<const key_info *>(a);
                  auto b_pair = reinterpret_cast<const key_info *>(b);
                  if (a_pair->first != b_pair->first) {
                      return false;
                  }
                  return a_pair->second == b_pair->second;
              },
              nullptr, nullptr, &_heap){};

    void lock() { os_unfair_lock_lock(&_lock); };
    void unlock() { os_unfair_lock_unlock(&_lock); };

    util::Heap &heap() { return _heap; };
    util::UntypedTable &table() { return _table; };
};

} // namespace

const metadata *metadata::mangled_type_name_ref_cached(const char *type_name, ref_kind *kind_out) {
    if (!type_name) {
        return nullptr;
    }

    static TypeCache *cache = new TypeCache();

    cache->lock();
    auto result = reinterpret_cast<TypeCache::value_info *_Nullable>(cache->table().lookup(this, nullptr));
    cache->unlock();

    if (!result) {
        ref_kind kind;
        const metadata *type = mangled_type_name_ref(type_name, true, &kind);

        cache->lock();

        auto key = reinterpret_cast<TypeCache::key_info *>(cache->heap().alloc_(sizeof(TypeCache::key_info)));
        key->first = this;
        key->second = type_name;

        auto value = reinterpret_cast<TypeCache::value_info *>(cache->heap().alloc_(sizeof(TypeCache::value_info)));
        value->first = type;
        value->second = kind;

        cache->table().insert(key, value);

        cache->unlock();

        result->first = type;
        result->second = kind;
    }

    if (kind_out) {
        *kind_out = result->second;
    }
    return result->first;
}

void metadata::copy_on_write_heap_object(void **object_ref) {
    if (::swift::swift_isUniquelyReferencedNonObjC(*object_ref)) {
        return;
    }

    assert(::swift::isHeapMetadataKind(getKind()));
    auto heap_metadata = reinterpret_cast<::swift::HeapMetadata *>(this);

    ::swift::HeapObject *copy = ::swift::swift_allocObject(heap_metadata, vw_size(), vw_alignment());
    vw_initializeWithCopy(reinterpret_cast<opaque_value *>(copy), reinterpret_cast<opaque_value *>(*object_ref));
    ::swift::swift_release(reinterpret_cast<::swift::HeapObject *>(*object_ref));
    *object_ref = copy;
}

struct ancestor_info {
    const char *name;
    uint64_t generic_args_end;
};

void metadata::append_description(CFMutableStringRef description) const {

    auto type = this;
    while (true) {
        if (type->getKind() != ::swift::MetadataKind::Function) {
            break;
        }
        auto function_type = reinterpret_cast<const ::swift::FunctionTypeMetadata *>(type);

        CFStringAppendCString(description, "(", kCFStringEncodingUTF8);
        size_t num_parameters = function_type->getNumParameters();
        for (int i = 0; i < num_parameters; i++) {
            if (i > 0) {
                CFStringAppendCString(description, ", ", kCFStringEncodingUTF8);
            }
            auto parameter = function_type->getParameter(i);
            if (parameter) {
                reinterpret_cast<const metadata *>(parameter)->append_description(description);
            } else {
                CFStringAppendCString(description, "nil", kCFStringEncodingUTF8);
            }
        }
        CFStringAppendCString(description, ")", kCFStringEncodingUTF8);

        CFStringAppendCString(description, " -> ", kCFStringEncodingUTF8);

        type = metadata::from_base(function_type->ResultType);
        if (!type) {
            CFStringAppendCString(description, "nil", kCFStringEncodingUTF8);
            return;
        }
    }

    if (type->getKind() == ::swift::MetadataKind::Tuple) {
        auto tuple_type = reinterpret_cast<const ::swift::TupleTypeMetadata *>(type);
        size_t num_elements = tuple_type->NumElements;
        if (num_elements > 0) {
            CFStringAppendCString(description, "(", kCFStringEncodingUTF8);
            for (int i = 0; i < num_elements; i++) {
                if (i > 0) {
                    CFStringAppendCString(description, ", ", kCFStringEncodingUTF8);
                }
                auto element = tuple_type->getElement(i);
                if (element.Type) {
                    reinterpret_cast<const metadata *>(element.Type)->append_description(description);
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

    vector<ancestor_info, 8, uint64_t> ancestors = {};
    for (auto ancestor = type->descriptor(); ancestor != nullptr; ancestor = ancestor->parent()) {
        switch (ancestor->getKind()) {
        case ::swift::ContextDescriptorKind::Class:
        case ::swift::ContextDescriptorKind::Struct:
        case ::swift::ContextDescriptorKind::Enum: {
            auto nominal_type = reinterpret_cast<const ::swift::TypeContextDescriptor *>(type);
            if (nominal_type->Name) {
                const char *name = nominal_type->Name.get();
                uint64_t num_generic_args = ancestor->count_generic_args();
                ancestors.push_back({name, num_generic_args});
            }
            break;
        }
        default:
            continue;
        }
    }
    if (ancestors.empty()) {
        const char *name = type->name(true);
        if (name) {
            ancestors.push_back({name, 0});
        }
    }

    vector<context_descriptor::generic_arg, 8, uint64_t> generic_args = {};
    auto desc = type->descriptor();
    if (desc) {
        desc->push_generic_args(*this, generic_args);
    }
    if (ancestors.size()) {
        uint64_t generic_args_start = 0;
        for (auto ancestor = ancestors.rbegin(); ancestor != ancestors.rend(); ancestor++) {
            CFStringAppendCString(description, ancestor->name, kCFStringEncodingUTF8);
            if (ancestor->generic_args_end != generic_args_start) {
                CFStringAppendCString(description, "<", kCFStringEncodingUTF8);
                for (uint64_t i = generic_args_start; i < ancestor->generic_args_end; i++) {
                    if (i > 0) {
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
            if (ancestor < ancestors.rend()) {
                CFStringAppendCString(description, ".", kCFStringEncodingUTF8);
            }
        }
    }
}

namespace {

class TypeSignatureCache {
  private:
    os_unfair_lock _lock;
    util::UntypedTable _signatures_by_type;

    void lock() { os_unfair_lock_lock(&_lock); };
    void unlock() { os_unfair_lock_unlock(&_lock); };

  public:
    TypeSignatureCache() : _lock(OS_UNFAIR_LOCK_INIT), _signatures_by_type(){};

    util::UntypedTable &signatures_by_type() { return _signatures_by_type; };

    unsigned char *signature_for_type(const metadata *type, const void **_Nullable found) {
        lock();
        return (unsigned char *)_signatures_by_type.lookup(reinterpret_cast<const void *>(type), found);
        unlock();
    }

    unsigned char *insert_signature(const metadata *type, unsigned char *signature) {
        lock();
        _signatures_by_type.insert(reinterpret_cast<const void *>(type), signature);
        unlock();
    }
};

} // namespace

void *metadata::signature() const {
    static TypeSignatureCache *cache = new TypeSignatureCache();

    const void *found = nullptr;
    unsigned char *signature = cache->signature_for_type(this, &found);

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

        unsigned char *digest = (unsigned char *)malloc(CC_SHA1_DIGEST_LENGTH);
        CC_SHA1_Final(digest, &context);

        signature = digest;
    } else {
        signature = nullptr;
    }

    cache->insert_signature(this, signature);

    return signature;
}

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
                if (!visitor.visit_element(*metadata::from_base(element.Type), metadata::ref_kind::none, element.Offset,
                                           element_size)) {
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
            return visitor.visit_element(reinterpret_cast<const metadata &>(*element_type), metadata::ref_kind::none,
                                         offset, element_type->vw_size());
        }
        return visitor.unknown_result();
    }
    default:
        return visitor.unknown_result();
    }
}

bool metadata::visit_heap_class(metadata_visitor &visitor) const {
    if (!this) {
        return visitor.unknown_result();
    }

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
        return reinterpret_cast<const metadata &>(*class_type->Superclass).visit_heap_class(visitor);
    }

    if (!class_context->Fields) {
        return true;
    }

    auto fields = class_context->Fields.get();
    if (!fields) {
        return true;
    }

    if (!class_context->Fields->NumFields) {
        return true;
    }

    if (class_context->Fields->NumFields != class_context->NumFields) {
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
            // TODO: check this is same as above
            // TODO: check don't need to call immediate_members_offset
            auto field_offsets = class_type->getFieldOffsets();
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
            static const metadata *pointer_type = mangled_type_name_ref("Bp", true, nullptr);
            if (pointer_type == nullptr) {
                return visitor.unknown_result();
            }
            visitor.visit_element(*pointer_type, metadata::ref_kind::unowned_unsafe, offset, vw_size());
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
        metadata::ref_kind kind = metadata::ref_kind::none;
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

::swift::ExistentialTypeRepresentation existential_type_metadata::representation() { return getRepresentation(); }

void *existential_type_metadata::project_value(void *container) {
    if (getRepresentation() == ::swift::ExistentialTypeRepresentation::Error) {
        return nullptr;
    }
    return projectValue(static_cast<::swift::OpaqueValue *>(container));
}

const metadata *existential_type_metadata::dynamic_type(void *container) {
    if (getRepresentation() == ::swift::ExistentialTypeRepresentation::Error) {
        return nullptr;
    }
    auto result = getDynamicType(static_cast<::swift::OpaqueValue *>(container));
    return static_cast<const metadata *>(result);
}

} // namespace swift
} // namespace AG
