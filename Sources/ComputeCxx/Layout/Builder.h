#pragma once

#include <CoreFoundation/CFBase.h>

#include "Containers/Vector.h"
#include "LayoutDescriptor.h"
#include "Swift/MetadataVisitor.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {
namespace LayoutDescriptor {

class Builder : public swift::metadata_visitor {
  public:
    struct RangeItem {
        size_t offset;
        size_t size;
    };
    struct DataItem : RangeItem {};
    struct EqualsItem : RangeItem {
        const swift::metadata *type;
        const swift::equatable_witness_table *equatable;
    };
    struct IndirectItem : RangeItem {
        const swift::metadata *type;
    };
    struct ExistentialItem : RangeItem {
        const swift::existential_type_metadata *type;
    };
    struct HeapRefItem : RangeItem {
        bool is_function;
    };
    struct NestedItem : RangeItem {
        ValueLayout layout;
    };
    struct EnumItem : RangeItem {
        struct Case {
            uint64_t item_index;
            size_t offset;
            vector<std::variant<DataItem, EqualsItem, IndirectItem, ExistentialItem, HeapRefItem, NestedItem, EnumItem>,
                   0, uint64_t>
                children;
        };

        const swift::metadata *type;
        vector<Case, 0, uint64_t> cases;
    };
    using Item = std::variant<DataItem, EqualsItem, IndirectItem, ExistentialItem, HeapRefItem, NestedItem, EnumItem>;

    // Emitter

    template <typename T> class Emitter {
      private:
        T *_Nonnull _data;
        size_t _emitted_size = 0;
        bool _invalid = false;

      public:
        void operator()(const DataItem &item);
        void operator()(const EqualsItem &item);
        void operator()(const IndirectItem &item);
        void operator()(const ExistentialItem &item);
        void operator()(const HeapRefItem &item);
        void operator()(const NestedItem &item);
        void operator()(const EnumItem &item);
        void enter(const RangeItem &range_item);

        const vector<unsigned char, 512, uint64_t> &data() const { return *_data; };
        size_t emitted_size() const { return _emitted_size; };
        bool is_invalid() const { return _invalid; };
        void set_invalid(bool invalid) { _invalid = invalid; };

        void finish();
    };

    template <> class Emitter<vector<unsigned char, 512, uint64_t>> {
      private:
        vector<unsigned char, 512, uint64_t> *_Nonnull _data; // TODO: why didn't compiler check this was uninitialized?
        size_t _emitted_size = 0;
        bool _invalid = false;

      public:
        Emitter(vector<unsigned char, 512, uint64_t> *data);

        void operator()(const DataItem &item);
        void operator()(const EqualsItem &item);
        void operator()(const IndirectItem &item);
        void operator()(const ExistentialItem &item);
        void operator()(const HeapRefItem &item);
        void operator()(const NestedItem &item);
        void operator()(const EnumItem &item);
        void enter(const RangeItem &range_item);

        const vector<unsigned char, 512, uint64_t> &data() const { return *_data; };
        size_t emitted_size() const { return _emitted_size; };
        bool is_invalid() const { return _invalid; };
        void set_invalid(bool invalid) { _invalid = invalid; };

        void finish();

        template <typename value_t> void emit_value(value_t value) {
            // TODO: make not set to zero before overwriting
            uint64_t size = _data->size();
            _data->resize(size + sizeof(value_t), 0);
            unsigned char *type_pointer = &(*_data)[size];
            *(value_t *)type_pointer = value;
        }
    };

  private:
    static os_unfair_lock _lock;
    static size_t _avail;
    static unsigned char *_buffer;
    static void lock() { os_unfair_lock_lock(&_lock); };
    static void unlock() { os_unfair_lock_unlock(&_lock); };

    ComparisonMode _current_comparison_mode;
    HeapMode _heap_mode;
    size_t _current_offset = 0;
    uint64_t _enum_case_depth = 0;
    EnumItem::Case *_current_enum_case = nullptr;
    vector<Item, 0, uint64_t> _items;

  public:
    Builder(ComparisonMode comparison_mode, HeapMode heap_mode)
        : _current_comparison_mode(comparison_mode), _heap_mode(heap_mode) {}

    size_t current_offset() { return _current_offset; };
    ComparisonMode current_comparison_mode() { return _current_comparison_mode; };
    vector<Item, 0, uint64_t> &get_items() {
        return _current_enum_case != nullptr ? _current_enum_case->children : _items;
    };

    ValueLayout commit(const swift::metadata &type);

    void add_field(size_t field_size);
    bool should_visit_fields(const swift::metadata &type, bool flag);

    struct RevertItemsInfo {
        uint64_t item_index;
        DataItem data_item;
    };

    void revert(const RevertItemsInfo &info);

    virtual bool visit_element(const swift::metadata &type, const swift::metadata::ref_kind kind, size_t element_offset,
                               size_t element_size) override;

    virtual bool visit_case(const swift::metadata &type, const swift::field_record &field, uint32_t arg) override;
    virtual bool visit_existential(const swift::existential_type_metadata &type) override;
    virtual bool visit_function(const swift::function_type_metadata &type) override;
    virtual bool visit_native_object(const swift::metadata &type) override;
};

} // namespace LayoutDescriptor
} // namespace AG

CF_ASSUME_NONNULL_END
