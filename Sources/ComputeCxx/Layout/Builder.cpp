#include "Builder.h"

#include "Controls.h"
#include "LayoutDescriptor.h"

namespace AG {

os_unfair_lock LayoutDescriptor::Builder::_lock = OS_UNFAIR_LOCK_INIT;

ValueLayout LayoutDescriptor::Builder::commit(const swift::metadata &type) {
    if (_heap_mode == HeapMode(0)) {
        if (_items.size() == 0) {
            return ValueLayoutEmpty;
        }
        if (_items.size() == 1 && _items[0].index() == 0) {
            return ValueLayoutEmpty;
        }
    }
    if (_items.size() == 1) {
        if (auto nested_item = std::get_if<NestedItem>(&_items[0])) {
            if (nested_item->offset == 0) {
                return nested_item->layout;
            }
        }
    }

    auto emitter = Emitter<vector<unsigned char, 512, uint64_t>>();
    for (auto &&item : _items) {
        std::visit(emitter, item);
    }
    emitter.finish();

    if (_heap_mode != HeapMode(0) && emitter.layout_exceeds_object_size()) {
        return nullptr;
    }
    if (_heap_mode == HeapMode(0)) {
        emitter.set_layout_exceeds_object_size(type.vw_size() < emitter.emitted_size());
        if (emitter.layout_exceeds_object_size()) {
            return ValueLayoutEmpty;
        }
    }
    auto layout_data = emitter.data();

    unsigned char *result;

    lock();
    if (layout_data.size() < 0x400) {
        if (_avail < layout_data.size()) {
            _avail = 0x1000;
            _buffer = (unsigned char *)malloc(0x1000);
        }
        _avail -= layout_data.size();
        _buffer += layout_data.size();
        result = _buffer;
    } else {
        result = (unsigned char *)malloc(layout_data.size());
    }
    unlock();

    memcpy(result, layout_data.data(), layout_data.size());

    if (print_layouts()) {
        std::string message = {};
        print(message, result);

        if (_heap_mode == HeapMode(0)) {
            const char *name = type.name(false);
            if (name) {
                fprintf(stdout, "== %s, %d bytes ==\n%s", name, (int)layout_data.size(), message.data());
            } else {
                fprintf(stdout, "== Unknown type %p ==\n%s", &type, message.data());
            }
        } else {
            fprintf(stdout, "== Unknown heap type %p ==\n%s", &type, message.data());
        }
    }
}

void LayoutDescriptor::Builder::add_field(size_t field_size) {
    if (field_size == 0) {
        return;
    }

    auto items = get_items();
    if (auto data_item = !items.empty() ? std::get_if<DataItem>(&items.back()) : nullptr) {
        if (data_item->offset == _current_offset) {
            data_item->size += field_size;
            return;
        }
    }

    items.push_back(DataItem(RangeItem(_current_offset, field_size)));
}

bool LayoutDescriptor::Builder::should_visit_fields(const swift::metadata &type, bool no_fetch) {
    if (!no_fetch) {
        if (auto layout = LayoutDescriptor::fetch(type, ComparisonOptions(_current_comparison_mode | 0x80000200), true)) {
            if ((uintptr_t)layout == 1) {
                add_field(type.vw_size());
            } else {
                NestedItem item = {
                    _current_offset,
                    type.vw_size(),
                    layout,
                };
                get_items().push_back(item);
            }
            return false;
        }
    }

    int mode = type.getValueWitnesses()->isPOD() ? 3 : 2;
    if (_current_comparison_mode >= mode) {
        if (auto equatable = type.equatable()) {
            EqualsItem item = {
                _current_offset,
                type.vw_size(),
                &type,
                equatable,
            };
            get_items().push_back(item);
            return false;
        }
    }

    return true;
}

void LayoutDescriptor::Builder::revert(const RevertItemsInfo &info) {
    auto items = get_items();
    while (items.size() > info.item_index) {
        items.pop_back();
    }
    if (items.size() > 0 && info.data_item.offset != -1) {
        items[items.size() - 1] = info.data_item;
    }
}

bool LayoutDescriptor::Builder::visit_element(const swift::metadata &type, const swift::metadata::ref_kind kind,
                                              size_t element_offset, size_t element_size) {
    if (element_size == 0) {
        return true;
    }

    _current_offset += element_offset;

    if (kind == swift::metadata::ref_kind::strong) {
        ComparisonMode prev_comparison_mode = _current_comparison_mode;
        _current_comparison_mode = mode_for_type(&type, _current_comparison_mode);

        if (should_visit_fields(type, false)) {
            auto items = get_items();

            auto num_items = items.size();

            size_t prev_offset = -1;
            size_t prev_size = 0;
            if (auto data_item = num_items > 0 ? std::get_if<DataItem>(&items.back()) : nullptr) {
                prev_offset = data_item->offset;
                prev_size = data_item->size;
            }

            if (!type.visit(*this)) {
                // unwind items, and add single field for entire type
                revert({num_items, DataItem(RangeItem(prev_offset, prev_size))});
                add_field(element_size);
            }
        }

        _current_comparison_mode = prev_comparison_mode;
    } else {
        add_field(element_size);
    }

    _current_offset -= element_offset;
    return true;
}

bool LayoutDescriptor::Builder::visit_case(const swift::metadata &type, const swift::field_record &field,
                                           uint32_t index) {
    if (_enum_case_depth > 7) {
        return false;
    }

    auto items = get_items();

    // Add EnumItem if this is the first case
    if (index == 0) {
        EnumItem item = {
            _current_offset,
            type.vw_size(),
            &type,
            {},
        };
        items.push_back(item);
    }
    EnumItem enum_item = std::get<EnumItem>(items.back()); // throws

    // Add this case to the enum item
    EnumItem::Case enum_case = {
        index,
        _current_offset,
        {},
    };
    enum_item.cases.push_back(enum_case);

    bool result;

    _enum_case_depth += 1;
    EnumItem::Case *prev_enum_case = _current_enum_case;
    _current_enum_case = &enum_item.cases.back();

    if (field.isIndirectCase() && _current_comparison_mode == 0) {
        add_field(sizeof(void *));
        result = true;
    } else {
        auto mangled_name = field.FieldName.get();
        auto field_type = mangled_name != nullptr ? type.mangled_type_name_ref(mangled_name, false, nullptr) : nullptr;
        if (field_type == nullptr) {
            // bail out if we can't get a type for the enum case payload
            items.pop_back();
            result = false;
        } else if (field.isIndirectCase()) {
            if (auto field_size = field_type->vw_size()) {
                IndirectItem item = {
                    _current_offset,
                    type.vw_size(),
                    field_type,
                };
                _current_enum_case->children.push_back(item);
            }
            result = true;
        } else {
            ComparisonMode prev_comparison_mode = _current_comparison_mode;
            _current_comparison_mode = mode_for_type(&type, _current_comparison_mode);

            if (should_visit_fields(*field_type, false)) {

                // same as visit_element
                auto items = get_items();
                size_t prev_offset = -1;
                size_t prev_size = 0;
                if (auto data_item = items.size() > 0 ? std::get_if<DataItem>(&items.back()) : nullptr) {
                    prev_offset = data_item->offset;
                    prev_size = data_item->size;
                }

                if (!field_type->visit(*this)) {
                    RevertItemsInfo info = {
                        items.size(),
                        {
                            prev_offset,
                            prev_size,
                        },
                    };
                    revert(info);
                    _current_enum_case->children.clear();

                    if (auto size = field_type->vw_size()) {
                        DataItem item = {
                            _current_offset,
                            size,
                        };
                        _current_enum_case->children.push_back(item);
                    }
                }
            }

            _current_comparison_mode = prev_comparison_mode;

            result = true;
        }
    }

    if (_current_enum_case->children.empty()) {
        auto last_case = enum_item.cases.back();
        enum_item.cases.pop_back(); // make call destructor
    }

    _enum_case_depth -= 1;
    _current_enum_case = prev_enum_case;

    return result;
}

bool LayoutDescriptor::Builder::visit_existential(const swift::existential_type_metadata &type) {
    if (_current_comparison_mode == 0 ||
        (_current_comparison_mode == 1 && type.representation() == ::swift::ExistentialTypeRepresentation::Class)) {
        return false;
    }

    get_items().push_back(ExistentialItem(RangeItem(_current_offset, type.vw_size()), &type));
    return true;
}

bool LayoutDescriptor::Builder::visit_function(const swift::function_type_metadata &type) {
    if (_current_comparison_mode == 0 || (int)type.getConvention() != 0) {
        return false;
    }

    add_field(sizeof(void *));
    get_items().push_back(HeapRefItem(RangeItem(_current_offset + sizeof(void *), sizeof(void *)), true));
    return true;
}

bool LayoutDescriptor::Builder::visit_native_object(const swift::metadata &type) {
    if (_heap_mode != HeapMode::Option2) {
        return false;
    }

    get_items().push_back(HeapRefItem(RangeItem(_current_offset, sizeof(void *)), false));
    return true;
}

#pragma mark - Builder::Emitter

void LayoutDescriptor::Builder::Emitter<vector<unsigned char, 512, uint64_t>>::operator()(const DataItem &item) {
    enter(item);
    size_t item_size = item.size;
    while (item_size > 0x80) {
        _data->push_back(0xff);
        item_size -= 0x80;
    }
    if (item_size != 0) {
        _data->push_back((item_size - 1) | 0x80);
    }
    _emitted_size += item.size;
}

void LayoutDescriptor::Builder::Emitter<vector<unsigned char, 512, uint64_t>>::operator()(const EqualsItem &item) {
    enter(item);
    _data->push_back(Controls::EqualsItemBegin);
    _data->reserve(_data->size() + 8);
    for (int i = 0; i < sizeof(void *); ++i) {
        _data->push_back(*((unsigned char *)item.type + i));
    }
    _data->reserve(_data->size() + 8);
    for (int i = 0; i < sizeof(void *); ++i) {
        _data->push_back(*((unsigned char *)item.equatable + i));
    }
    _emitted_size += item.size;
}

void LayoutDescriptor::Builder::Emitter<vector<unsigned char, 512, uint64_t>>::operator()(const IndirectItem &item) {
    enter(item);
    _data->push_back(Controls::IndirectItemBegin);
    _data->reserve(_data->size() + 8);
    for (int i = 0; i < sizeof(void *); ++i) {
        _data->push_back(*((unsigned char *)item.type + i));
    }
    _data->reserve(_data->size() + 8);
    for (int i = 0; i < sizeof(void *); ++i) {
        _data->push_back(0);
    }
    _emitted_size += item.size;
}

void LayoutDescriptor::Builder::Emitter<vector<unsigned char, 512, uint64_t>>::operator()(const ExistentialItem &item) {
    enter(item);
    _data->push_back(Controls::ExistentialItemBegin);
    _data->reserve(_data->size() + 8);
    for (int i = 0; i < sizeof(void *); ++i) {
        _data->push_back(*((unsigned char *)item.type + i));
    }
    _emitted_size += item.size;
}

void LayoutDescriptor::Builder::Emitter<vector<unsigned char, 512, uint64_t>>::operator()(const HeapRefItem &item) {
    enter(item);
    _data->push_back(item.is_function ? Controls::FunctionItemBegin : Controls::HeapRefItemBegin);
    _emitted_size += item.size;
}

void LayoutDescriptor::Builder::Emitter<vector<unsigned char, 512, uint64_t>>::operator()(const NestedItem &item) {
    enter(item);

    size_t item_length = length(item.layout);
    if (item_length <= 0x20) {
        // append nested layout directly
        //        _data->push_back(item.layout); // append length - 1 bytes from nested layout to _data
    } else {
        uintptr_t layout_relative_address = (uintptr_t)item.layout - (uintptr_t)&base_address;
        if ((uint32_t)layout_relative_address == layout_relative_address && item.size < 0xffff) {
            _data->push_back(Controls::CompactNestedItemBegin);

            // layout addressin 4 bytes
            _data->reserve(_data->size() + 4);
            for (int i = 0; i < sizeof(uint32_t); ++i) {
                _data->push_back(*((unsigned char *)layout_relative_address + i));
            }

            // size in two bytes
            _data->push_back(*((unsigned char *)item.size + 0));
            _data->push_back(*((unsigned char *)item.size + 1));
        } else {
            _data->push_back(Controls::NestedItemBegin);

            // full pointer to layout
            _data->reserve(_data->size() + 8);
            for (int i = 0; i < sizeof(void *); ++i) {
                _data->push_back(*((unsigned char *)item.layout + i)); // TODO: how to split the pointer into 8 bytes
            }

            // Emit length 7 bits at a time, using the 8th bit as a "has more" flag
            size_t length = item.size;
            while (length) {
                _data->push_back((length & 0x7f) | (length > 0x7f ? 1 << 7 : 0));
                length = length >> 7;
            }
        }
    }

    _emitted_size += item.size;
}

void LayoutDescriptor::Builder::Emitter<vector<unsigned char, 512, uint64_t>>::operator()(const EnumItem &item) {
    enter(item);

    if (item.cases.empty()) {
        _data->push_back('\t');
        _data->reserve(_data->size() + 8);
        for (int i = 0; i < sizeof(void *); ++i) {
            _data->push_back(*((unsigned char *)item.type + i));
        }
    } else {
        bool is_first = true;
        for (auto enum_case : item.cases) {
            /*
             Case indices are encoding using the numbers 8 through 21

               - 8 through 11 encode the first case index with a payload
                 - 9, 10, 11 correspond to indices 0, 1, 2 respectively
                 - 8 indicates the index is encoded in the following bytes
               - 12 through 21 encode the first case index with a payload
                 - 13-21 correspond to indices 0-8 respectively
                 - 12 indicates the index is encoded in the following bytes
             */

            uint64_t direct_encoded_index =
                enum_case.item_index +
                (is_first ? Controls::EnumItemBeginCaseIndexFirst : Controls::EnumItemContinueCaseIndexFirst);
            uint64_t last_direct =
                is_first ? Controls::EnumItemBeginCaseIndexLast : Controls::EnumItemContinueCaseIndexLast;
            if (direct_encoded_index <= last_direct) {
                _data->push_back(direct_encoded_index);

            } else {
                _data->push_back(is_first ? Controls::EnumItemBeginVariadicCaseIndex
                                          : Controls::EnumItemContinueVariadicCaseIndex);

                // Emit case.index 7 bits at a time, using the 8th bit as a "has more" flag
                size_t number = enum_case.item_index;
                while (number) {
                    _data->push_back((number & 0x7f) | (number > 0x7f ? 1 << 7 : 0));
                    number = number >> 7;
                }
            }

            if (is_first) {
                _data->reserve(_data->size() + 8);
                for (int i = 0; i < sizeof(void *); ++i) {
                    _data->push_back(*((unsigned char *)item.type + i));
                }
            }

            for (auto &&child : enum_case.children) {
                std::visit(*this, child);
            }

            is_first = false;

            size_t new_size = _emitted_size + item.type->vw_size();
            _layout_exceeds_object_size = (_layout_exceeds_object_size || new_size < _emitted_size);
        }
    }

    _data->push_back(Controls::EnumItemEnd);

    _emitted_size += item.size;
}

void LayoutDescriptor::Builder::Emitter<vector<unsigned char, 512, uint64_t>>::enter(const RangeItem &item) {
    if (!_layout_exceeds_object_size) {
        _layout_exceeds_object_size = item.offset <= _emitted_size;
        if (!_layout_exceeds_object_size) {

            // Emit number of bytes until item offset
            // encode as a series of numbers from 0-63, indicating chunks of bytes 1-64
            size_t length = item.offset - _emitted_size;
            while (length > 0x40) {
                _data->push_back('\x7f');
                length -= 0x40;
            }
            if (length > 0) {
                _data->push_back((length - 1) | 0x40);
            }
        }
    }
    _emitted_size = item.offset;
}

void LayoutDescriptor::Builder::Emitter<vector<unsigned char, 512, uint64_t>>::finish() {
    _data->push_back('\0'); // NULL terminating char
}

} // namespace AG
