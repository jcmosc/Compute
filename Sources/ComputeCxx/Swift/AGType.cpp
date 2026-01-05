#include "ComputeCxx/AGType.h"

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
CFStringRef AGTypeDescription(AGTypeID typeID) {
    auto type = reinterpret_cast<const AG::swift::metadata *>(typeID);
    CFMutableStringRef description =
        CFStringCreateMutable(kCFAllocatorDefault, 0);
    type->append_description(description);
    CFAutorelease(description);
    return description;
}
#else
CFStringRef AGTypeCopyDescription(AGTypeID typeID) {
    auto type = reinterpret_cast<const AG::swift::metadata *>(typeID);
    CFMutableStringRef description =
        CFStringCreateMutable(kCFAllocatorDefault, 0);
    type->append_description(description);
    return description;
}
#endif

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
    const uint8_t *data = static_cast<const uint8_t *>(type->signature());
    if (!data) {
        return AGTypeSignature();
    }
    AGTypeSignature signature = {};
    for (size_t i = 0; i < 20; ++i) {
        signature.bytes[i] = data[i];
    }
    return signature;
}

const void *AGTypeGetDescriptor(AGTypeID typeID) {
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

void AGTypeApplyFields(AGTypeID typeID,
                       void (*apply)(const void *context AG_SWIFT_CONTEXT,
                                     const char *field_name,
                                     size_t field_offset, AGTypeID field_type)
                           AG_SWIFT_CC(swift),
                       const void *apply_context) {
    class Visitor : public AG::swift::metadata_visitor {
      private:
        void (*_body)(const void *context AG_SWIFT_CONTEXT, const char *field_name,
                      size_t field_offset, AGTypeID field_type)
            AG_SWIFT_CC(swift);
        const void *_body_context;

      public:
        Visitor(void (*body)(const void *context AG_SWIFT_CONTEXT,
                             const char *field_name, size_t field_offset,
                             AGTypeID field_type) AG_SWIFT_CC(swift),
                const void *body_context)
            : _body(body), _body_context(body_context) {}

        bool unknown_result() override { return true; }
        bool visit_field(const AG::swift::metadata &type,
                         const AG::swift::field_record &field,
                         size_t field_offset, size_t field_size) override {
            auto mangled_name = field.MangledTypeName.get();
            auto field_type =
                type.mangled_type_name_ref(mangled_name, true, nullptr);
            if (field_type) {
                auto field_name = field.FieldName.get();
                _body(_body_context, field_name, field_offset,
                      AGTypeID(field_type));
            }
            return true;
        }
    };

    Visitor visitor = Visitor(apply, apply_context);

    auto type = reinterpret_cast<const AG::swift::metadata *>(typeID);
    type->visit(visitor);
}

bool AGTypeApplyFields2(AGTypeID typeID, AGTypeApplyOptions options,
                        bool (*apply)(const void *context AG_SWIFT_CONTEXT,
                                      const char *field_name,
                                      size_t field_offset, AGTypeID field_type)
                            AG_SWIFT_CC(swift),
                        const void *apply_context) {
    class Visitor : public AG::swift::metadata_visitor {
      private:
        AGTypeApplyOptions _options;
        AG::ClosureFunction<bool, const char *, size_t, AGTypeID> *_body;

      public:
        Visitor(AGTypeApplyOptions options,
                AG::ClosureFunction<bool, const char *, size_t, AGTypeID> *body)
            : _options(options), _body(body) {}

        bool unknown_result() override {
            return _options & AGTypeApplyOptionsContinueAfterUnknownField;
        }
        bool visit_field(const AG::swift::metadata &type,
                         const AG::swift::field_record &field,
                         size_t field_offset, size_t field_size) override {
            auto mangled_name = field.MangledTypeName.get();
            auto field_type =
                type.mangled_type_name_ref(mangled_name, true, nullptr);
            if (!field_type) {
                return unknown_result();
            }
            auto field_name = field.FieldName.get();
            bool result =
                (*_body)(field_name, field_offset, AGTypeID(field_type));
            return result != 0;
        }
        bool visit_case(const AG::swift::metadata &type,
                        const AG::swift::field_record &field,
                        uint32_t index) override {
            auto mangled_name = field.MangledTypeName.get();
            auto field_type = type.mangled_type_name_ref(
                mangled_name, true, nullptr); // TODO: _cached or not?
            if (!field_type) {
                return unknown_result();
            }
            auto field_name = field.FieldName.get();
            bool result = (*_body)(field_name, index, AGTypeID(field_type));
            return result != 0;
        }
    };

    auto closure_function =
        AG::ClosureFunction<bool, const char *, size_t, AGTypeID>(
            apply, apply_context);
    Visitor visitor = Visitor(options, &closure_function);

    auto type = reinterpret_cast<const AG::swift::metadata *>(typeID);
    switch (type->getKind()) {
    case ::swift::MetadataKind::Class:
        if (options & AGTypeApplyOptionsEnumerateClassFields) {
            return type->visit_heap(visitor,
                                    AG::LayoutDescriptor::HeapMode::Class);
        }
        return false;
    case ::swift::MetadataKind::Struct:
        if (!(options & AGTypeApplyOptionsEnumerateClassFields) &&
            !(options & AGTypeApplyOptionsEnumerateEnumCases)) {
            return type->visit(visitor);
        }
        return false;
    case ::swift::MetadataKind::Enum:
    case ::swift::MetadataKind::Optional:
        if (options & AGTypeApplyOptionsEnumerateEnumCases) {
            return type->visit(visitor);
        }
        return false;
    case ::swift::MetadataKind::Tuple:
        if (!(options & AGTypeApplyOptionsEnumerateClassFields) &&
            !(options & AGTypeApplyOptionsEnumerateEnumCases)) {
            return type->visit(visitor);
        }
        return false;
    default:
        return false;
    }
}

bool AGTypeApplyEnumData(AGTypeID typeID, void *value,
                         void (*body)(void *context AG_SWIFT_CONTEXT,
                                      uint32_t tag, AGTypeID field_type,
                                      const void *field_value)
                             AG_SWIFT_CC(swift),
                         void *context) {
    auto type = reinterpret_cast<const AG::swift::metadata *>(typeID);
    auto value_witness = type->getValueWitnesses();
    if (!value_witness || !value_witness->flags.hasEnumWitnesses()) {
        AG::precondition_failure("not an enum type: %s", type->name(false));
    }

    auto enum_value_witness = value_witness->_asEVWT();
    uint32_t tag = enum_value_witness->getEnumTag(
        reinterpret_cast<AG::swift::opaque_value *>(value), type);

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
                            reinterpret_cast<AG::swift::opaque_value *>(value),
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
                        body(context, tag, AGTypeID(field_type), field_value);

                        enum_value_witness->destructiveInjectEnumTag(
                            reinterpret_cast<AG::swift::opaque_value *>(value),
                            tag, type);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool AGTypeApplyMutableEnumData(AGTypeID typeID, void *value,
                                void (*body)(void *context AG_SWIFT_CONTEXT,
                                             uint32_t tag, AGTypeID field_type,
                                             void *field_value)
                                    AG_SWIFT_CC(swift),
                                void *context) {
    auto type = reinterpret_cast<const AG::swift::metadata *>(typeID);
    auto value_witness = type->getValueWitnesses();
    if (!value_witness || !value_witness->flags.hasEnumWitnesses()) {
        AG::precondition_failure("not an enum type: %s", type->name(false));
    }

    auto enum_value_witness = value_witness->_asEVWT();
    uint32_t tag = enum_value_witness->getEnumTag(
        reinterpret_cast<AG::swift::opaque_value *>(value), type);

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
                            reinterpret_cast<AG::swift::opaque_value *>(value),
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
                        body(context, tag, AGTypeID(field_type), field_value);

                        enum_value_witness->destructiveInjectEnumTag(
                            reinterpret_cast<AG::swift::opaque_value *>(value),
                            tag, type);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

uint64_t AGTypeGetEnumTag(AGTypeID typeID, const void *value) {
    auto type = reinterpret_cast<const AG::swift::metadata *>(typeID);
    auto value_witness = type->getValueWitnesses();
    if (!value_witness || !value_witness->flags.hasEnumWitnesses()) {
        AG::precondition_failure("not an enum type: %s", type->name(false));
    }

    auto enum_value_witness = value_witness->_asEVWT();
    return enum_value_witness->getEnumTag(
        reinterpret_cast<const AG::swift::opaque_value *>(value), type);
}

void AGTypeProjectEnumData(AGTypeID typeID, void *value) {
    auto type = reinterpret_cast<const AG::swift::metadata *>(typeID);
    auto value_witness = type->getValueWitnesses();
    if (!value_witness || !value_witness->flags.hasEnumWitnesses()) {
        AG::precondition_failure("not an enum type: %s", type->name(false));
    }

    auto enum_value_witness = value_witness->_asEVWT();
    enum_value_witness->destructiveProjectEnumData(
        reinterpret_cast<AG::swift::opaque_value *>(value), type);
}

void AGTypeInjectEnumTag(AGTypeID typeID, uint32_t tag, void *value) {
    auto type = reinterpret_cast<const AG::swift::metadata *>(typeID);
    auto value_witness = type->getValueWitnesses();
    if (!value_witness || !value_witness->flags.hasEnumWitnesses()) {
        AG::precondition_failure("not an enum type: %s", type->name(false));
    }

    auto enum_value_witness = value_witness->_asEVWT();
    enum_value_witness->destructiveInjectEnumTag(
        reinterpret_cast<AG::swift::opaque_value *>(value), tag, type);
}
