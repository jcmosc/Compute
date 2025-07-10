#include "Compare.h"

#include "Graph/Graph.h"
#include "Swift/Metadata.h"
#include "Swift/SwiftShims.h"
#include "ValueLayout.h"

namespace AG {
namespace LayoutDescriptor {

Compare::Enum::Enum(const swift::metadata *type, Mode mode, unsigned int enum_tag, size_t offset,
                    const unsigned char *lhs, const unsigned char *lhs_copy, const unsigned char *rhs,
                    const unsigned char *rhs_copy, bool owns_copies) {
    this->type = type;
    this->mode = mode;
    this->enum_tag = enum_tag;
    this->offset = offset;
    this->lhs = lhs;
    this->lhs_copy = lhs_copy;
    this->rhs = rhs;
    this->rhs_copy = rhs_copy;
    this->owns_copies = owns_copies;

    if (type) {
        if (mode == Mode::Managed) {
            type->vw_initializeWithCopy((swift::opaque_value *)lhs_copy, (swift::opaque_value *)lhs);
            type->vw_initializeWithCopy((swift::opaque_value *)rhs_copy, (swift::opaque_value *)rhs);
        }
        type->vw_destructiveProjectEnumData((swift::opaque_value *)lhs_copy);
        type->vw_destructiveProjectEnumData((swift::opaque_value *)rhs_copy);
    }
}

Compare::Enum::~Enum() {
    if (type) {
        type->vw_destructiveInjectEnumTag((swift::opaque_value *)lhs_copy, enum_tag);
        type->vw_destructiveInjectEnumTag((swift::opaque_value *)rhs_copy, enum_tag);
        if (mode == Mode::Managed) {
            type->vw_destroy((swift::opaque_value *)lhs_copy);
            type->vw_destroy((swift::opaque_value *)rhs_copy);
        }
    }
    if (owns_copies) {
        free((void *)lhs_copy);
        free((void *)rhs_copy);
    }
}

Compare::Frame::~Frame() {
    while (_enums->size() > _start) {
        _enums->pop_back();
    }
}

bool Compare::operator()(ValueLayout layout, const unsigned char *lhs, const unsigned char *rhs, size_t offset,
                         size_t size, AGComparisonOptions options) {
    size_t end = size < 0 ? ~0 : offset + size;

    ValueLayoutReader reader = ValueLayoutReader(layout);
    while (true) {
        if (offset >= end) {
            return true;
        }
        auto kind = reader.peek_kind();
        if (kind == ValueLayoutEntryKind::End) {
            return true;
        }

        size_t remaining_size = end - offset;

        // skip over unused layout
        if ((uint8_t)kind >= 0x40 && (uint8_t)kind < 0x80) {
            uint8_t skip = reader.read_bytes<uint8_t>();
            skip = (skip & 0x3f) + 1; // Convert 0-63 to 1-64
            offset += skip;
            continue;
        }

        // compare data as bytes
        if ((uint8_t)kind >= 0x80) {
            uint8_t data_size = reader.read_bytes<uint8_t>();
            data_size = (data_size & 0x7f) + 1; // Convert 0-127 to 1-128

            size_t smaller_size = remaining_size < data_size ? remaining_size : data_size;
            size_t failure_location = 0;
            if (!compare_bytes(lhs + offset, rhs + offset, smaller_size, &failure_location)) {
                failed(options, lhs, rhs, offset, data_size, nullptr);
                return false;
            }

            offset += data_size;
            continue;
        }

        switch (reader.read_kind()) {
        case ValueLayoutEntryKind::End:
            return true;
        case ValueLayoutEntryKind::Equals: {
            auto type = reader.read_bytes<const swift::metadata *>();
            auto equatable = reader.read_bytes<const swift::equatable_witness_table *>();

            size_t item_size = type->vw_size();
            size_t item_end = offset + item_size;

            if (end < item_end) {
                if (!compare_bytes(lhs + offset, rhs + offset, end - offset, nullptr)) {
                    failed(options, lhs, rhs, offset, item_size, type);
                    return false;
                }
            } else {
                if (!AGDispatchEquatable((const void *)(lhs + offset), (const void *)(rhs + offset), type, equatable)) {
                    failed(options, lhs, rhs, offset, item_size, type);
                    return false;
                }
            }

            offset = item_end;
            continue;
        }
        case ValueLayoutEntryKind::Indirect: {
            auto type = reader.read_bytes<const swift::metadata *>();
            auto indirect_layout = reader.read_bytes<ValueLayout>();

            size_t item_size = type->vw_size();
            size_t item_end = offset + item_size;

            if (!compare_indirect(&indirect_layout, *_enums.back().type, *type,
                                  options & ~AGComparisonOptionsTraceCompareFailed, lhs + offset, rhs + offset)) {
                failed(options, lhs, rhs, offset, item_size, type);
                return false;
            }

            offset = item_end;
            continue;
        }
        case ValueLayoutEntryKind::Existential: {
            auto type = reader.read_bytes<const swift::metadata *>();

            size_t item_size = type->vw_size();
            size_t item_end = offset + item_size;

            if (!compare_existential_values(*reinterpret_cast<const swift::existential_type_metadata *>(type),
                                            lhs + offset, rhs + offset,
                                            options & ~AGComparisonOptionsTraceCompareFailed)) {
                failed(options, lhs, rhs, offset, item_size, type);
                return false;
            }

            offset = item_end;
            continue;
        }
        case ValueLayoutEntryKind::HeapRef:
        case ValueLayoutEntryKind::Function: {
            bool is_function = kind == ValueLayoutEntryKind::Function;

            size_t item_end = offset + 8;

            if (lhs + offset != rhs + offset) {
                if (!compare_heap_objects(lhs + offset, rhs + offset, options & ~AGComparisonOptionsTraceCompareFailed,
                                          is_function)) {
                    failed(options, lhs, rhs, offset, 8, nullptr);
                    return false;
                }
            }

            offset = item_end;
            continue;
        }
        case ValueLayoutEntryKind::Nested: {
            auto nested_layout = reader.read_bytes<ValueLayout>();
            size_t nested_size = reader.read_varint();

            size_t item_end = offset + nested_size;

            size_t smaller_size = remaining_size < nested_size ? remaining_size : nested_size;
            if (!this->operator()(nested_layout, lhs, rhs, offset, smaller_size, options)) {
                // don't call failed() again
                return false;
            }

            offset = item_end;
            continue;
        }
        case ValueLayoutEntryKind::CompactNested: {
            uint32_t nested_layout_relative_pointer = reader.read_bytes<uint32_t>();
            ValueLayout nested_layout =
                reinterpret_cast<ValueLayout>(/* &base_address */ 0x1e3e6ab60 + nested_layout_relative_pointer);

            uint16_t nested_size = reader.read_bytes<uint16_t>();

            size_t item_end = offset + nested_size;

            size_t smaller_size = remaining_size < nested_size ? remaining_size : nested_size;
            if (!this->operator()(nested_layout, lhs, rhs, offset, smaller_size, options)) {
                // don't call failed() again
                return false;
            }

            offset = item_end;
            continue;
        }
        case ValueLayoutEntryKind::EnumStartVariadic:
        case ValueLayoutEntryKind::EnumStart0:
        case ValueLayoutEntryKind::EnumStart1:
        case ValueLayoutEntryKind::EnumStart2: {
            uint64_t enum_tag;
            const swift::metadata *type;
            if (kind == ValueLayoutEntryKind::EnumStartVariadic) {
                enum_tag = reader.read_varint();
                type = reader.read_bytes<const swift::metadata *>();
            } else {
                enum_tag = (uint64_t)kind - (uint64_t)ValueLayoutEntryKind::EnumStart0;
                type = reader.read_bytes<const swift::metadata *>();
            }

            unsigned int lhs_tag = type->vw_getEnumTag((swift::opaque_value *)(lhs + offset));
            unsigned int rhs_tag = type->vw_getEnumTag((swift::opaque_value *)(rhs + offset));
            if (lhs_tag != rhs_tag) {
                failed(options, lhs, rhs, offset, type->vw_size(), type);
                return false;
            }

            // Push enum

            bool is_copy = options & AGComparisonOptionsCopyOnWrite;
            const unsigned char *_Nonnull lhs_enum;
            const unsigned char *_Nonnull rhs_enum;
            bool owns_copies = false;
            if (is_copy) {
                // Copy the enum itself so that we can project the data without destroying the original.
                size_t enum_size = type->vw_size();
                bool large_allocation = enum_size > 0x1000;
                if (large_allocation) {
                    lhs_enum = (unsigned char *)malloc(enum_size);
                    rhs_enum = (unsigned char *)malloc(enum_size);
                    owns_copies = true;
                } else {
                    lhs_enum = (unsigned char *)alloca(enum_size);
                    rhs_enum = (unsigned char *)alloca(enum_size);
                    bzero((void *)lhs_enum, enum_size);
                    bzero((void *)rhs_enum, enum_size);
                }
            } else {
                lhs_enum = lhs + offset;
                rhs_enum = rhs + offset;
            }

            Enum enum_value = Enum(type, is_copy ? Enum::Mode::Managed : Enum::Mode::Unmanaged, lhs_tag, offset,
                                   lhs + offset, lhs_enum, rhs + offset, rhs_enum, owns_copies);
            _enums.push_back(enum_value);

            // Pretend the copies of the enum data are part the entire data
            // until we get to the end of the enum
            if (is_copy) {
                lhs = _enums.back().lhs_copy - offset;
                rhs = _enums.back().rhs_copy - offset;
            }

            // Advance over this enum case if it doesn't apply to the data
            if (enum_tag != _enums.back().enum_tag) {
                reader.skip(length(reader.layout));
            }
            continue;
        }
        case ValueLayoutEntryKind::EnumContinueVariadic:
        case ValueLayoutEntryKind::EnumContinue0:
        case ValueLayoutEntryKind::EnumContinue1:
        case ValueLayoutEntryKind::EnumContinue2:
        case ValueLayoutEntryKind::EnumContinue3:
        case ValueLayoutEntryKind::EnumContinue4:
        case ValueLayoutEntryKind::EnumContinue5:
        case ValueLayoutEntryKind::EnumContinue6:
        case ValueLayoutEntryKind::EnumContinue7:
        case ValueLayoutEntryKind::EnumContinue8: {
            uint64_t enum_tag;
            if (kind == ValueLayoutEntryKind::EnumContinueVariadic) {
                enum_tag = reader.read_varint();
            } else {
                enum_tag = (uint64_t)kind - (uint64_t)ValueLayoutEntryKind::EnumContinue0;
            }

            // Advance over this enum case if it doesn't apply to the data
            if (enum_tag != _enums.back().enum_tag) {
                reader.skip(length(reader.layout));
            }
            continue;
        }
        case ValueLayoutEntryKind::EnumEnd: {

            // Pop enum
            Enum &enum_item = _enums.back();

            // Restore actual data
            if (enum_item.mode) {
                lhs = enum_item.lhs - offset;
                rhs = enum_item.rhs - offset;
            }

            offset = enum_item.offset + enum_item.type->vw_size();

            _enums.pop_back();
            continue;
        }
        }
    }
}

void Compare::failed(AGComparisonOptions options, const unsigned char *lhs, const unsigned char *rhs, size_t offset,
                     size_t size, const swift::metadata *type) {
    if (options & AGComparisonOptionsTraceCompareFailed) {
        Graph::compare_failed(lhs, rhs, offset, size, type);
    }
}

} // namespace LayoutDescriptor
} // namespace AG
