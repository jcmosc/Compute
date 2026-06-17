#include "ComputeCxx/IAGType.h"

#if TARGET_OS_MAC
#include <CoreFoundation/CFString.h>
#else
#include <SwiftCorelibsCoreFoundation/CFString.h>
#endif

#include "Closure/ClosureFunction.h"
#include "ContextDescriptor.h"
#include "Metadata.h"
#include "MetadataVisitor.h"

#if TARGET_OS_MAC
CFStringRef IAGTypeDescription(IAGTypeID typeID) {
    auto type = reinterpret_cast<const IAG::swift::metadata *>(typeID);
    CFMutableStringRef description =
        CFStringCreateMutable(kCFAllocatorDefault, 0);
    type->append_description(description);
    CFAutorelease(description);
    return description;
}
#else
CFStringRef IAGTypeCopyDescription(IAGTypeID typeID) {
    auto type = reinterpret_cast<const IAG::swift::metadata *>(typeID);
    CFMutableStringRef description =
        CFStringCreateMutable(kCFAllocatorDefault, 0);
    type->append_description(description);
    return description;
}
#endif

IAGTypeKind IAGTypeGetKind(IAGTypeID typeID) {
    auto type = reinterpret_cast<const IAG::swift::metadata *>(typeID);
    switch (type->getKind()) {
    case swift::MetadataKind::Class:
        return IAGTypeKindClass;
    case swift::MetadataKind::Struct:
        return IAGTypeKindStruct;
    case swift::MetadataKind::Enum:
        return IAGTypeKindEnum;
    case swift::MetadataKind::Optional:
        return IAGTypeKindOptional;
    case swift::MetadataKind::Tuple:
        return IAGTypeKindTuple;
    case swift::MetadataKind::Function:
        return IAGTypeKindFunction;
    case swift::MetadataKind::Existential:
        return IAGTypeKindExistential;
    case swift::MetadataKind::Metatype:
        return IAGTypeKindMetatype;
    default:
        return IAGTypeKindNone;
    }
}

const IAGTypeSignature IAGTypeGetSignature(IAGTypeID typeID) {
    auto type = reinterpret_cast<const IAG::swift::metadata *>(typeID);
    const uint8_t *data = static_cast<const uint8_t *>(type->signature());
    if (!data) {
        return IAGTypeSignature();
    }
    IAGTypeSignature signature = {};
    for (size_t i = 0; i < 20; ++i) {
        signature.bytes[i] = data[i];
    }
    return signature;
}

const void *IAGTypeGetDescriptor(IAGTypeID typeID) {
    auto type = reinterpret_cast<const IAG::swift::metadata *>(typeID);
    return type->descriptor();
}

const void *IAGTypeNominalDescriptor(IAGTypeID typeID) {
    auto type = reinterpret_cast<const IAG::swift::metadata *>(typeID);
    return type->nominal_descriptor();
}

const char *IAGTypeNominalDescriptorName(IAGTypeID typeID) {
    auto type = reinterpret_cast<const IAG::swift::metadata *>(typeID);
    auto nominal_descriptor = type->nominal_descriptor();
    if (!nominal_descriptor) {
        return nullptr;
    }
    return nominal_descriptor->name();
}

void IAGTypeApplyFields(IAGTypeID typeID,
                       void (*apply)(const char *field_name,
                                     size_t field_offset,
                                     IAGTypeID field_type,
                                     const void *context IAG_SWIFT_CONTEXT)
                           IAG_SWIFT_CC(swift),
                       const void *apply_context) {
    class Visitor : public IAG::swift::metadata_visitor {
      private:
        void (*_body)(const char *field_name,
                      size_t field_offset,
                      IAGTypeID field_type,
                      const void *context IAG_SWIFT_CONTEXT)
            IAG_SWIFT_CC(swift);
        const void *_body_context;

      public:
        Visitor(void (*body)(const char *field_name,
                             size_t field_offset,
                             IAGTypeID field_type,
                             const void *context IAG_SWIFT_CONTEXT) IAG_SWIFT_CC(swift),
                const void *body_context)
            : _body(body), _body_context(body_context) {}

        bool unknown_result() override { return true; }
        bool visit_field(const IAG::swift::metadata &type,
                         const IAG::swift::field_record &field,
                         size_t field_offset, size_t field_size) override {
            auto mangled_name = field.MangledTypeName.get();
            auto field_type =
                type.mangled_type_name_ref(mangled_name, true, nullptr);
            if (field_type) {
                auto field_name = field.FieldName.get();
                _body(field_name, field_offset, IAGTypeID(field_type), _body_context);
            }
            return true;
        }
    };

    Visitor visitor = Visitor(apply, apply_context);

    auto type = reinterpret_cast<const IAG::swift::metadata *>(typeID);
    type->visit(visitor);
}

bool IAGTypeApplyFields2(IAGTypeID typeID, IAGTypeApplyOptions options,
                        bool (*apply)(const char *field_name,
                                      size_t field_offset,
                                      IAGTypeID field_type,
                                      const void *context IAG_SWIFT_CONTEXT)
                            IAG_SWIFT_CC(swift),
                        const void *apply_context) {
    class Visitor : public IAG::swift::metadata_visitor {
      private:
        IAGTypeApplyOptions _options;
        IAG::ClosureFunction<bool, const char *, size_t, IAGTypeID> *_body;

      public:
        Visitor(IAGTypeApplyOptions options,
                IAG::ClosureFunction<bool, const char *, size_t, IAGTypeID> *body)
            : _options(options), _body(body) {}

        bool unknown_result() override {
            return _options & IAGTypeApplyOptionsContinueAfterUnknownField;
        }
        bool visit_field(const IAG::swift::metadata &type,
                         const IAG::swift::field_record &field,
                         size_t field_offset, size_t field_size) override {
            auto mangled_name = field.MangledTypeName.get();
            auto field_type =
                type.mangled_type_name_ref(mangled_name, true, nullptr);
            if (!field_type) {
                return unknown_result();
            }
            auto field_name = field.FieldName.get();
            bool result =
                (*_body)(field_name, field_offset, IAGTypeID(field_type));
            return result != 0;
        }
        bool visit_case(const IAG::swift::metadata &type,
                        const IAG::swift::field_record &field,
                        uint32_t index) override {
            auto mangled_name = field.MangledTypeName.get();
            auto field_type = type.mangled_type_name_ref(
                mangled_name, true, nullptr); // TODO: _cached or not?
            if (!field_type) {
                return unknown_result();
            }
            auto field_name = field.FieldName.get();
            bool result = (*_body)(field_name, index, IAGTypeID(field_type));
            return result != 0;
        }
    };

    auto closure_function =
        IAG::ClosureFunction<bool, const char *, size_t, IAGTypeID>(
            apply, apply_context);
    Visitor visitor = Visitor(options, &closure_function);

    auto type = reinterpret_cast<const IAG::swift::metadata *>(typeID);
    switch (type->getKind()) {
    case ::swift::MetadataKind::Class:
        if (options & IAGTypeApplyOptionsEnumerateClassFields) {
            return type->visit_heap(visitor,
                                    IAG::LayoutDescriptor::HeapMode::Class);
        }
        return false;
    case ::swift::MetadataKind::Struct:
        if (!(options & IAGTypeApplyOptionsEnumerateClassFields) &&
            !(options & IAGTypeApplyOptionsEnumerateEnumCases)) {
            return type->visit(visitor);
        }
        return false;
    case ::swift::MetadataKind::Enum:
    case ::swift::MetadataKind::Optional:
        if (options & IAGTypeApplyOptionsEnumerateEnumCases) {
            return type->visit(visitor);
        }
        return false;
    case ::swift::MetadataKind::Tuple:
        if (!(options & IAGTypeApplyOptionsEnumerateClassFields) &&
            !(options & IAGTypeApplyOptionsEnumerateEnumCases)) {
            return type->visit(visitor);
        }
        return false;
    default:
        return false;
    }
}

bool IAGTypeApplyEnumData(IAGTypeID typeID, void *value,
                         void (*body)(uint32_t tag,
                                      IAGTypeID field_type,
                                      const void *field_value,
                                      void *context IAG_SWIFT_CONTEXT)
                             IAG_SWIFT_CC(swift),
                         void *context) {
    auto type = reinterpret_cast<const IAG::swift::metadata *>(typeID);
    auto value_witness = type->getValueWitnesses();
    if (!value_witness || !value_witness->flags.hasEnumWitnesses()) {
        IAG::precondition_failure("not an enum type: %s", type->name(false));
    }

    auto enum_value_witness = value_witness->_asEVWT();
    uint32_t tag = enum_value_witness->getEnumTag(
        reinterpret_cast<IAG::swift::opaque_value *>(value), type);

    if (auto descriptor = type->nominal_descriptor()) {
        auto enum_descriptor =
            reinterpret_cast<const ::swift::EnumDescriptor *>(descriptor);
        if (auto fields = enum_descriptor->Fields.get()) {
            auto num_payload_cases = enum_descriptor->getNumPayloadCases();
            if (tag < num_payload_cases) {
                auto &field = fields->getFields()[tag];
                if (auto mangled_name = field.MangledTypeName.get()) {
                    auto field_type = type->mangled_type_name_ref_cached(
                        mangled_name, nullptr);
                    if (field_type) {
                        enum_value_witness->destructiveProjectEnumData(
                            reinterpret_cast<IAG::swift::opaque_value *>(value),
                            type);

                        void *field_value = value;
                        if (field.isIndirectCase()) {
                            size_t alignment_mask =
                                field_type->getValueWitnesses()
                                    ->getAlignmentMask();
                            size_t offset =
                                (sizeof(::swift::HeapObject) + alignment_mask) &
                                ~alignment_mask;
                            field_value = *(char **)value + offset;
                        }
                        body(tag, IAGTypeID(field_type), field_value, context);

                        enum_value_witness->destructiveInjectEnumTag(
                            reinterpret_cast<IAG::swift::opaque_value *>(value),
                            tag, type);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool IAGTypeApplyMutableEnumData(IAGTypeID typeID, void *value,
                                void (*body)(uint32_t tag,
                                             IAGTypeID field_type,
                                             void *field_value,
                                             void *context IAG_SWIFT_CONTEXT)
                                    IAG_SWIFT_CC(swift),
                                void *context) {
    auto type = reinterpret_cast<const IAG::swift::metadata *>(typeID);
    auto value_witness = type->getValueWitnesses();
    if (!value_witness || !value_witness->flags.hasEnumWitnesses()) {
        IAG::precondition_failure("not an enum type: %s", type->name(false));
    }

    auto enum_value_witness = value_witness->_asEVWT();
    uint32_t tag = enum_value_witness->getEnumTag(
        reinterpret_cast<IAG::swift::opaque_value *>(value), type);

    if (auto descriptor = type->nominal_descriptor()) {
        auto enum_descriptor =
            reinterpret_cast<const ::swift::EnumDescriptor *>(descriptor);
        if (auto fields = enum_descriptor->Fields.get()) {
            auto num_payload_cases = enum_descriptor->getNumPayloadCases();
            if (tag < num_payload_cases) {
                auto &field = fields->getFields()[tag];
                if (auto mangled_name = field.MangledTypeName.get()) {
                    auto field_type = type->mangled_type_name_ref_cached(
                        mangled_name, nullptr);
                    if (field_type) {
                        enum_value_witness->destructiveProjectEnumData(
                            reinterpret_cast<IAG::swift::opaque_value *>(value),
                            type);

                        void *field_value = value;
                        if (field.isIndirectCase()) {
                            field_type->copy_on_write_heap_object(
                                (void **)value);
                            size_t alignment_mask =
                                field_type->getValueWitnesses()
                                    ->getAlignmentMask();
                            size_t offset =
                                (sizeof(::swift::HeapObject) + alignment_mask) &
                                ~alignment_mask;
                            field_value = *(char **)value + offset;
                        }
                        body(tag, IAGTypeID(field_type), field_value, context);

                        enum_value_witness->destructiveInjectEnumTag(
                            reinterpret_cast<IAG::swift::opaque_value *>(value),
                            tag, type);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

uint64_t IAGTypeGetEnumTag(IAGTypeID typeID, const void *value) {
    auto type = reinterpret_cast<const IAG::swift::metadata *>(typeID);
    auto value_witness = type->getValueWitnesses();
    if (!value_witness || !value_witness->flags.hasEnumWitnesses()) {
        IAG::precondition_failure("not an enum type: %s", type->name(false));
    }

    auto enum_value_witness = value_witness->_asEVWT();
    return enum_value_witness->getEnumTag(
        reinterpret_cast<const IAG::swift::opaque_value *>(value), type);
}

void IAGTypeProjectEnumData(IAGTypeID typeID, void *value) {
    auto type = reinterpret_cast<const IAG::swift::metadata *>(typeID);
    auto value_witness = type->getValueWitnesses();
    if (!value_witness || !value_witness->flags.hasEnumWitnesses()) {
        IAG::precondition_failure("not an enum type: %s", type->name(false));
    }

    auto enum_value_witness = value_witness->_asEVWT();
    enum_value_witness->destructiveProjectEnumData(
        reinterpret_cast<IAG::swift::opaque_value *>(value), type);
}

void IAGTypeInjectEnumTag(IAGTypeID typeID, uint32_t tag, void *value) {
    auto type = reinterpret_cast<const IAG::swift::metadata *>(typeID);
    auto value_witness = type->getValueWitnesses();
    if (!value_witness || !value_witness->flags.hasEnumWitnesses()) {
        IAG::precondition_failure("not an enum type: %s", type->name(false));
    }

    auto enum_value_witness = value_witness->_asEVWT();
    enum_value_witness->destructiveInjectEnumTag(
        reinterpret_cast<IAG::swift::opaque_value *>(value), tag, type);
}
