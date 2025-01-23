#include "Compare.h"

#include "Controls.h"
#include "Swift/AGEquatable.h"
#include "Swift/Metadata.h"

namespace AG {
namespace LayoutDescriptor {

Compare::Frame::~Frame() {
    while (_compare->enums().size() > _start) {
        _compare->enums().pop_back();
    }
}

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

    if (mode == Mode::InitializingCopies) {
        type->vw_initializeWithCopy((swift::opaque_value *)lhs_copy, (swift::opaque_value *)lhs);
        type->vw_initializeWithCopy((swift::opaque_value *)rhs_copy, (swift::opaque_value *)rhs);
    }
    type->vw_getEnumTag((swift::opaque_value *)lhs_copy);
    type->vw_getEnumTag((swift::opaque_value *)rhs_copy);
}

Compare::Enum::~Enum() {
    type->vw_destructiveProjectEnumData((swift::opaque_value *)lhs_copy);
    type->vw_destructiveProjectEnumData((swift::opaque_value *)rhs_copy);
    if (mode == Mode::InitializingCopies) {
        type->vw_destroy((swift::opaque_value *)lhs_copy);
        type->vw_destroy((swift::opaque_value *)rhs_copy);
    }
    if (owns_copies) {
        free((void *)lhs_copy);
        free((void *)rhs_copy);
    }
}

bool Compare::operator()(ValueLayout layout, const unsigned char *lhs, const unsigned char *rhs, size_t offset,
                         size_t size, ComparisonOptions options) {
    size_t end = size < 0 ? ~0 : offset + size;

    const unsigned char *c = layout;
    while (true) {
        if (offset >= end) {
            return true;
        }
        if (*c == '\0') {
            return true;
        }

        size_t remaining_size = end - offset;

        // skip over unused layout
        if (*c >= 0x40 && *c < 0x80) {
            offset += *c & 0x3f + 1; // Convert 0-63 to 1-64
            c += 1;
            continue;
        }

        // compare data as bytes
        if (*c >= 0x80) {
            size_t data_size = *c & 0x7f + 1; // Convert 0-127 to 1-128
            c += 1;

            size_t smaller_size = remaining_size < data_size ? remaining_size : data_size;
            size_t failure_location = 0;
            if (!compare_bytes(lhs + offset, rhs + offset, smaller_size, &failure_location)) {
                failed(options, lhs, rhs, offset, data_size, nullptr);
                return false;
            }

            offset += data_size;
            continue;
        }

        switch (*c) {
        case Controls::EqualsItemBegin: {
            c += 1;

            auto type = reinterpret_cast<const swift::metadata *>(c);
            c += Controls::EqualsItemTypePointerSize;

            auto equatable = (const swift::equatable_witness_table *)(c);
            c += Controls::EqualsItemEquatablePointerSize;

            size_t item_size = type->vw_size();
            size_t item_end = offset + item_size;

            if (end < item_end) {
                if (!compare_bytes(lhs + offset, rhs + offset, end - offset, nullptr)) {
                    failed(options, lhs, rhs, offset, item_size, type);
                    return false;
                }
            } else {
                auto equatable = (const swift::equatable_witness_table *)(c + 9);
                if (!AGDispatchEquatable((const void *)(lhs + offset), (const void *)(rhs + offset), AGTypeID(type),
                                         equatable)) {
                    failed(options, lhs, rhs, offset, item_size, type);
                    return false;
                }
            }

            offset = item_end;
            continue;
        }
        case Controls::IndirectItemBegin: {
            c += 1;

            auto type = reinterpret_cast<const swift::metadata *>(c);
            c += Controls::IndirectItemTypePointerSize;

            ValueLayout layout_pointer = reinterpret_cast<ValueLayout>(c);
            c += Controls::IndirectItemUnusedPointerSize;

            size_t item_size = type->vw_size();
            size_t item_end = offset + item_size;

            if (!compare_indirect(&layout_pointer, *_enums.back().type, *type, options, lhs + offset, rhs + offset)) {
                failed(options, lhs, rhs, offset, item_size, type);
                return false;
            }
            // TODO: does this update layout?

            offset = item_end;
            continue;
        }
        case Controls::ExistentialItemBegin: {
            c += 1;

            auto type = reinterpret_cast<const swift::metadata *>(c);
            c += Controls::ExistentialItemTypePointerSize;

            size_t item_size = type->vw_size();
            size_t item_end = offset + item_size;

            if (!compare_existential_values(*reinterpret_cast<const swift::existential_type_metadata *>(type),
                                            lhs + offset, rhs + offset)) {
                failed(options, lhs, rhs, offset, item_size, type);
                return false;
            }

            offset = item_end;
            continue;
        }
        case Controls::HeapRefItemBegin:
        case Controls::FunctionItemBegin: {
            bool is_function = *c == Controls::FunctionItemBegin;
            c += 1;

            size_t item_end = offset + 8;

            if (lhs + offset != rhs + offset) {
                if (!compare_heap_objects(lhs + offset, rhs + offset,
                                          ComparisonOptions(options & ~ComparisonOptions::TraceFailures),
                                          is_function)) {
                    failed(options, lhs, rhs, offset, 8, nullptr);
                    return false;
                }
            }

            offset = item_end;
            continue;
        }
        case Controls::NestedItemBegin: {
            c += 1;

            auto item_layout = reinterpret_cast<ValueLayout>(c);
            c += Controls::NestedItemLayoutPointerSize;

            unsigned shift = 0;
            size_t item_size = 0;
            while (*(int *)c < 0) {
                item_size = item_size | ((*c & 0x7f) << shift);
                shift += 7;
                c += 1;
            }
            item_size = item_size | ((*c & 0x7f) << shift);
            c += 1;

            size_t item_end = offset + item_size;

            size_t smaller_size = remaining_size < item_size ? remaining_size : item_size;
            if (!this->operator()(item_layout, lhs, rhs, offset, smaller_size, options)) {
                // don't call failed() again
                return false;
            }

            offset = item_end;
            continue;
        }
        case Controls::CompactNestedItemBegin: {
            c += 1;

            auto item_layout = reinterpret_cast<ValueLayout>(base_address + *(uint32_t *)c);
            c += Controls::CompactNestedItemLayoutRelativePointerSize;

            size_t item_size = *(uint16_t *)(c);
            c += Controls::CompactNestedItemLayoutSize;

            size_t item_end = offset + item_size;

            size_t smaller_size = remaining_size < item_size ? remaining_size : item_size;
            if (!this->operator()(item_layout, lhs, rhs, offset, smaller_size, options)) {
                // don't call failed() again
                return false;
            }

            offset = item_end;
            continue;
        }
        case Controls::EnumItemBeginVariadicCaseIndex:
        case Controls::EnumItemBeginCaseIndex0:
        case Controls::EnumItemBeginCaseIndex1:
        case Controls::EnumItemBeginCaseIndex2: {
            size_t enum_tag;
            const swift::metadata *type;
            if (*c == Controls::EnumItemBeginVariadicCaseIndex) {
                c += 1;

                unsigned shift = 0;
                enum_tag = 0;
                while (*(int *)c < 0) {
                    enum_tag = enum_tag | ((*c & 0x7f) << shift);
                    shift += 7;
                    c += 1;
                }
                enum_tag = enum_tag | ((*c & 0x7f) << shift);
                c += 1;

                type = reinterpret_cast<const swift::metadata *>(c);
                c += Controls::EnumItemTypePointerSize;
            } else {
                enum_tag = *c - Controls::EnumItemBeginCaseIndexFirst;
                c += 1;

                type = reinterpret_cast<const swift::metadata *>(c);
                c += Controls::EnumItemTypePointerSize;
            }

            unsigned int lhs_tag = type->vw_getEnumTag((swift::opaque_value *)(lhs + offset));
            unsigned int rhs_tag = type->vw_getEnumTag((swift::opaque_value *)(rhs + offset));
            if (lhs_tag != rhs_tag) {
                failed(options, lhs, rhs, offset, type->vw_size(), type);
                return false;
            }

            // Push enum

            Enum::Mode mode = Enum::Mode::InitializingCopies; // options & withoutCopying
            const unsigned char *lhs_copy = nullptr;
            const unsigned char *rhs_copy = nullptr;
            bool owns_copies = true;

            Enum enum_value = Enum(type, mode, lhs_tag, offset, lhs, lhs_copy, rhs, rhs_copy, owns_copies);
            _enums.push_back(enum_value);

            if (mode) {
                lhs = _enums.back().lhs_copy - offset;
                rhs = _enums.back().rhs_copy - offset;
            }

            // Advance over this enum case if it doesn't apply to the data
            if (enum_tag != _enums.back().enum_tag) {
                c += length(c);
            }
            continue;
        }
        case Controls::EnumItemContinueVariadicCaseIndex:
        case Controls::EnumItemContinueCaseIndex0:
        case Controls::EnumItemContinueCaseIndex1:
        case Controls::EnumItemContinueCaseIndex2:
        case Controls::EnumItemContinueCaseIndex3:
        case Controls::EnumItemContinueCaseIndex4:
        case Controls::EnumItemContinueCaseIndex5:
        case Controls::EnumItemContinueCaseIndex6:
        case Controls::EnumItemContinueCaseIndex7:
        case Controls::EnumItemContinueCaseIndex8: {
            size_t enum_tag;
            if (*c == Controls::EnumItemContinueVariadicCaseIndex) {
                c += 1;

                unsigned shift = 0;
                enum_tag = 0;
                while (*(int *)c < 0) {
                    enum_tag = enum_tag | ((*c & 0x7f) << shift);
                    shift += 7;
                    c += 1;
                }
                enum_tag = enum_tag | ((*c & 0x7f) << shift);
                c += 1;
            } else {
                enum_tag = *c - Controls::EnumItemContinueCaseIndexFirst;
                c += 1;
            }

            // Advance over this enum case if it doesn't apply to the data
            if (enum_tag != _enums.back().enum_tag) {
                c += length(c);
            }
            continue;
        }
        case Controls::EnumItemEnd: {
            c += 1;

            // Pop enum
            Enum enum_item = _enums.back();
            _enums.pop_back();
        }
        }
    }
}

bool Compare::failed(ComparisonOptions options, const unsigned char *lhs, const unsigned char *rhs, size_t offset,
                     size_t size, const swift::metadata *type) {
    if (options & ComparisonOptions::TraceFailures) {
        // TODO: tracing
    }
}

} // namespace LayoutDescriptor
} // namespace AG
