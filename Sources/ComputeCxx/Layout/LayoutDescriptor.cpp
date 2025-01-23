#include "LayoutDescriptor.h"

#include <os/lock.h>
#include <variant>

#include "Builder.h"
#include "Compare.h"
#include "Controls.h"
#include "Swift/Metadata.h"
#include "Time/Time.h"
#include "Util/HashTable.h"

namespace AG {

#pragma mark - TypeDescriptorCache

namespace {

class TypeDescriptorCache {
  public:
    struct QueueEntry {
        const swift::metadata *type;
        uint32_t comparison_and_heap_mode; // comparison in 0xffff, heap in 0x00ff0000 ?
        uint32_t priority;

        bool operator<(const QueueEntry &other) const noexcept { return priority < other.priority; };
    };

  private:
    os_unfair_lock _lock;
    util::UntypedTable _table;
    vector<QueueEntry, 8, uint64_t> _async_queue;
    void *_field_0xf0;
    uint64_t _async_queue_running;
    vector<std::pair<const swift::context_descriptor *, LayoutDescriptor::ComparisonMode>> _modes;
    void *_field_0x118;
    uint64_t _cache_hit_count;
    uint64_t _cache_miss_count;
    double _async_total_seconds;
    double _sync_total_seconds;

    static TypeDescriptorCache *_shared_cache;
    static dispatch_once_t _shared_once;

  public:
    static void init_shared_cache(void *_Nullable context) {
        _shared_cache = new TypeDescriptorCache();
        LayoutDescriptor::base_address += 1;
    };
    static TypeDescriptorCache &shared_cache() {
        dispatch_once_f(&_shared_once, nullptr, init_shared_cache);
        return *_shared_cache;
    };

    void lock() { os_unfair_lock_lock(&_lock); };
    void unlock() { os_unfair_lock_unlock(&_lock); };

    vector<std::pair<const swift::context_descriptor *, LayoutDescriptor::ComparisonMode>> modes() { return _modes; };

    ValueLayout fetch(const swift::metadata &type, LayoutDescriptor::ComparisonOptions options,
                      LayoutDescriptor::HeapMode heap_mode, uint32_t priority);
    static void drain_queue(void *cache);
};

TypeDescriptorCache *TypeDescriptorCache::_shared_cache = nullptr;
dispatch_once_t TypeDescriptorCache::_shared_once = 0;

int print_layouts() {
    static bool print_layouts = print_layouts = []() -> bool {
        char *result = getenv("AG_PRINT_LAYOUTS");
        if (result) {
            return atoi(result) != 0;
        }
        return false;
    }();
    return print_layouts;
}

ValueLayout TypeDescriptorCache::fetch(const swift::metadata &type, LayoutDescriptor::ComparisonOptions options,
                                       LayoutDescriptor::HeapMode heap_mode, uint32_t priority) {
    LayoutDescriptor::ComparisonMode comparison_mode = LayoutDescriptor::ComparisonModeFromOptions(options);

    uintptr_t type_address = (uintptr_t)&type;
    uintptr_t key =
        heap_mode != LayoutDescriptor::HeapMode::Option0 ? (~type_address & 0xfffffffffffffffc) : type_address;
    key = key | comparison_mode;

    lock();

    const void *found = nullptr;
    ValueLayout layout = (ValueLayout)_table.lookup((void *)key, &found);
    if (found) {
        _cache_hit_count += 1;
        unlock();
        return layout;
    }

    _cache_miss_count += 1;
    unlock();

    static bool async_layouts = []() {
        char *result = getenv("AG_ASYNC_LAYOUTS");
        if (result) {
            return atoi(result) != 0;
        }
        return false;
    }();

    if ((options & 0x200) == 0 && async_layouts) {
        // insert layout asynchronously
        lock();
        _table.insert((void *)key, nullptr);

        _async_queue.push_back({
            &type,
            (comparison_mode | (((uint32_t)heap_mode & 0xff) << 0x10)),
            priority,
        });
        std::push_heap(_async_queue.begin(), _async_queue.end());

        if (!_async_queue_running) {
            _async_queue_running = true;
            dispatch_queue_global_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0);
            dispatch_async_f(queue, this, drain_queue);
        }

        unlock();
        return nullptr;
    }

    // insert layout synchronously
    double start_time = current_time();
    layout = LayoutDescriptor::make_layout(type, comparison_mode, heap_mode);
    double end_time = current_time();

    if (comparison_mode < 0) {
        lock();
    } else {
        double time = end_time - start_time;
        if ((print_layouts() & 4) != 0) {
            const char *name = type.name(false);
            std::fprintf(stdout, "!! synchronous layout creation for %s: %g ms\n", name, time * 1000.0);
        }
        lock();
        _sync_total_seconds += time;
    }

    _table.insert((void *)key, layout);
    unlock();
    return layout;
}

void TypeDescriptorCache::drain_queue(void *context) {
    TypeDescriptorCache *cache = (TypeDescriptorCache *)context;

    double start_time = current_time();
    cache->lock();

    uint64_t created_count = 0;
    while (cache->_async_queue.size() > 0) {
        std::pop_heap(cache->_async_queue.begin(), cache->_async_queue.end());
        auto entry = cache->_async_queue.back();

        void *key = (void *)((uint64_t)(((entry.comparison_and_heap_mode >> 0x10) & 0xff)
                                            ? (~(uintptr_t)entry.type & 0xfffffffffffffffc)
                                            : (uintptr_t)entry.type) |
                             (uint64_t)(entry.comparison_and_heap_mode & 0xffff));

        const void *found = nullptr;
        ValueLayout layout = (ValueLayout)cache->_table.lookup(key, &found);

        if (layout == nullptr && found != nullptr) {
            cache->unlock();

            layout = LayoutDescriptor::make_layout(
                *entry.type, LayoutDescriptor::ComparisonMode(entry.comparison_and_heap_mode & 0xffff),
                LayoutDescriptor::HeapMode((entry.comparison_and_heap_mode & 0x00ff0000) >> 0x10));

            cache->lock();
            cache->_table.insert(key, layout);
        }

        cache->_async_queue.pop_back();
    }
    cache->_async_queue.shrink_to_fit();

    cache->_async_queue_running = false;

    double time = current_time() - start_time;
    if (print_layouts() & 2) {
        std::fprintf(stdout,
                     "## bg queue ran for %g ms, created %u layouts (%u extant). "
                     "Totals: %g ms async, %g ms sync. %u hits, %u misses.\n",
                     time * 1000.0, (uint)created_count, (uint)cache->_table.count(),
                     cache->_async_total_seconds * 1000.0, cache->_sync_total_seconds * 1000.0,
                     (uint)cache->_cache_hit_count, (uint)cache->_cache_miss_count);
    }

    cache->_async_total_seconds += time;
    cache->unlock();
}

} // namespace

#pragma mark - LayoutDescriptor

const ValueLayout ValueLayoutEmpty = (ValueLayout)1;

uintptr_t base_address = 0;

namespace LayoutDescriptor {

ComparisonMode ComparisonModeFromOptions(ComparisonOptions options) { return ComparisonMode(options & 0xff); };

ComparisonMode mode_for_type(const swift::metadata *type, ComparisonMode default_mode) {
    if (!type) {
        return default_mode;
    }
    auto descriptor = type->descriptor();
    if (!descriptor) {
        return default_mode;
    }

    ComparisonMode result = default_mode;
    TypeDescriptorCache::shared_cache().lock();
    auto modes = TypeDescriptorCache::shared_cache().modes();
    auto iter = std::find_if(modes.begin(), modes.end(),
                             [&](const auto &element) -> bool { return element.first == descriptor; });
    if (iter != modes.end()) {
        result = iter->second;
    }
    TypeDescriptorCache::shared_cache().unlock();
    return result;
}

void add_type_descriptor_override(const swift::context_descriptor *_Nullable type_descriptor,
                                  ComparisonMode override_mode) {
    if (!type_descriptor) {
        return;
    }

    TypeDescriptorCache::shared_cache().lock();
    auto modes = TypeDescriptorCache::shared_cache().modes();
    auto iter = std::find_if(modes.begin(), modes.end(),
                             [&](const auto &element) -> bool { return element.first == type_descriptor; });
    if (iter != modes.end()) {
        iter->second = override_mode;
    } else {
        modes.push_back({type_descriptor, override_mode});
    }
    TypeDescriptorCache::shared_cache().unlock();
}

ValueLayout _Nullable fetch(const swift::metadata &type, ComparisonOptions options, uint32_t priority) {
    return TypeDescriptorCache::shared_cache().fetch(type, options, HeapMode(0), priority);
}

ValueLayout make_layout(const swift::metadata &type, ComparisonMode default_mode, HeapMode heap_mode) {
    ComparisonMode comparison_mode = mode_for_type(&type, default_mode);

    Builder builder = Builder(comparison_mode, heap_mode);

    if (heap_mode == HeapMode::Option2) {
        if (!type.visit_heap(builder, swift::metadata::visit_options::heap_locals)) {
            return nullptr;
        }
    } else {
        if (heap_mode == HeapMode::Option1) {
            if (type.isClassObject()) {
                int mode = type.getValueWitnesses()->isPOD() ? 3 : 2; // TODO: check
                if (mode <= builder.current_comparison_mode()) {
                    if (auto equatable = type.equatable()) {
                        size_t offset = builder.current_offset();
                        size_t size = type.vw_size();
                        Builder::EqualsItem item = {offset, size, &type, equatable};
                        builder.get_items().push_back(item);
                        return builder.commit(type);
                    }
                }
            }
            if (!type.visit_heap(builder, swift::metadata::visit_options::heap_class_and_generic_locals)) {
                return nullptr;
            }
        }
        if (heap_mode != HeapMode::Option0) {
            return nullptr;
        }

        int mode = type.getValueWitnesses()->isPOD() ? 3 : 2; // TODO: check
        if (builder.current_comparison_mode() < mode) {
            if (!type.visit(builder)) {
                return nullptr;
            }
        }
        auto equatable = type.equatable();
        if (equatable == nullptr) {
            if (!type.visit(builder)) {
                return nullptr;
            }
        }

        size_t offset = builder.current_offset();
        size_t size = type.vw_size();
        Builder::EqualsItem item = {offset, size, &type, equatable};
        builder.get_items().push_back(item);
        return builder.commit(type);
    }
}

size_t length(ValueLayout layout) {
    const unsigned char *c = layout;
    unsigned int enum_depth = 0;
    while (true) {
        if (*c > LastControlCharacter) {
            c += 1;
            continue;
        }
        switch (*c) {
        case 0:
            c += 1;
            break;
        case Controls::EqualsItemBegin:
            c += 1;
            c += Controls::EqualsItemTypePointerSize;
            c += Controls::EqualsItemEquatablePointerSize;
            continue;
        case Controls::IndirectItemBegin:
            c += 1;
            c += Controls::IndirectItemTypePointerSize;
            c += Controls::IndirectItemUnusedPointerSize;
            continue;
        case Controls::ExistentialItemBegin:
            c += 1;
            c += Controls::ExistentialItemTypePointerSize;
            continue;
        case Controls::HeapRefItemBegin:
        case Controls::FunctionItemBegin:
            c += 1;
            continue;
        case Controls::NestedItemBegin: {
            c += 1;
            c += Controls::NestedItemLayoutPointerSize;
            while (*(int *)c < 0) {
                c += 1;
            }
            c += 1;
            continue;
        }
        case Controls::CompactNestedItemBegin: {
            c += 1;
            c += Controls::CompactNestedItemLayoutRelativePointerSize;
            c += Controls::CompactNestedItemLayoutSize;
            continue;
        }
        case Controls::EnumItemBeginVariadicCaseIndex: {
            enum_depth += 1;
            c += 1;
            while (*(int *)c < 0) {
                c += 1;
            }
            c += 1;
            c += Controls::EnumItemTypePointerSize;
            continue;
        }
        case Controls::EnumItemBeginCaseIndex0:
        case Controls::EnumItemBeginCaseIndex1:
        case Controls::EnumItemBeginCaseIndex2: {
            enum_depth += 1;
            c += 1;
            c += Controls::EnumItemTypePointerSize;
            continue;
        }
        case Controls::EnumItemContinueVariadicCaseIndex: {
            if (enum_depth == 0) {
                break;
            }
            c += 1;
            while (*(int *)c < 0) {
                c += 1;
            }
            c += 1;
            continue;
        }
        case Controls::EnumItemContinueCaseIndex0:
        case Controls::EnumItemContinueCaseIndex1:
        case Controls::EnumItemContinueCaseIndex2:
        case Controls::EnumItemContinueCaseIndex3:
        case Controls::EnumItemContinueCaseIndex4:
        case Controls::EnumItemContinueCaseIndex5:
        case Controls::EnumItemContinueCaseIndex6:
        case Controls::EnumItemContinueCaseIndex7:
        case Controls::EnumItemContinueCaseIndex8: {
            if (enum_depth == 0) {
                break;
            }
            c += 1;
            continue;
        }
        case Controls::EnumItemEnd: {
            if (enum_depth == 0) {
                break;
            }
            enum_depth -= 1;
            c += 1;
        }
        }
    }
    return c - layout;
}

// MARK: Comparing values

bool compare(ValueLayout layout, const unsigned char *lhs, const unsigned char *rhs, size_t size,
             ComparisonOptions options) {
    if (lhs == rhs) {
        return true;
    }
    if (!layout) {
        return compare_bytes_top_level(lhs, rhs, size, options);
    }
    auto compare_object = Compare();
    return compare_object(layout, lhs, rhs, 0, size, options);
}

bool compare_bytes_top_level(const unsigned char *lhs, const unsigned char *rhs, size_t size,
                             ComparisonOptions options) {
    size_t failure_location = 0;
    bool result = compare_bytes(lhs, rhs, size, &failure_location);
    if (options & ComparisonOptions::TraceFailures && !result) { // options.TraceCompareFailed
        // TODO: tracing
    }
    return result;
}

bool compare_bytes(const unsigned char *lhs, const unsigned char *rhs, size_t size, size_t *failure_location) {
    if (size == 0) {
        return true;
    }

    size_t location = 0;
    size_t remaining_size = size;

    // If both aligned to 8 bytes, compare 8 bytes at a time
    if ((((uintptr_t)lhs | (uintptr_t)rhs) & 7) == 0) {
        while (remaining_size >= 8) {
            if (*(uint64_t *)lhs != *(uint64_t *)rhs) {
                if (failure_location) {
                    *failure_location = location;
                }
                return false;
            }
            lhs += 8;
            rhs += 8;
            location += 8;
        }
    }

    // Compare one byte at a time
    while (remaining_size > 0) {
        if (*(uint8_t *)lhs != *(uint8_t *)rhs) {
            if (failure_location) {
                *failure_location = location;
            }
            return false;
        }
        lhs += 1;
        rhs += 1;
        location += 1;
    }

    return true;
}

bool compare_heap_objects(const unsigned char *lhs, const unsigned char *rhs, ComparisonOptions options,
                          bool is_function) {
    if (lhs == rhs) {
        return true;
    }
    if (lhs == nullptr || rhs == nullptr) {
        return false;
    }

    auto lhs_type = (const swift::metadata *)lhs;
    auto rhs_type = (const swift::metadata *)rhs;
    if (lhs_type != rhs_type) {
        return false;
    }

    HeapMode heap_mode = is_function ? HeapMode::Option2 : HeapMode::Option1;
    ComparisonOptions fetch_options =
        ComparisonOptions(ComparisonModeFromOptions(options)); // this has the effect of allowing async fetch
    ValueLayout layout = TypeDescriptorCache::shared_cache().fetch(*lhs_type, fetch_options, heap_mode, 1);

    if (layout > ValueLayoutEmpty) {
        return compare(layout, lhs, rhs, -1, ComparisonOptions(options & 0xfffffeff));
    }

    return false;
}

// https://www.swift.org/blog/how-mirror-works/
bool compare_indirect(ValueLayout *layout_ref, const swift::metadata &enum_type, const swift::metadata &layout_type,
                      ComparisonOptions options, const unsigned char *lhs, const unsigned char *rhs) {

    // TODO: do we need lhs and rhs types, or just one?

    size_t enum_size = enum_type.vw_size();
    bool large_allocation = enum_size > 0x1000;

    // Copy the enum itself so that we can project the data without destroying the original.
    unsigned char *lhs_copy = nullptr;
    unsigned char *rhs_copy = nullptr;
    if (large_allocation) {
        lhs_copy = (unsigned char *)malloc(enum_size);
        rhs_copy = (unsigned char *)malloc(enum_size);
        if (!lhs_copy || !rhs_copy) {
            if (lhs_copy) {
                free(lhs_copy);
            }
            if (rhs_copy) {
                free(rhs_copy);
            }
            return false;
        }
    } else {
        lhs_copy = (unsigned char *)alloca(enum_size);
        rhs_copy = (unsigned char *)alloca(enum_size);
        bzero(lhs_copy, enum_size);
        bzero(rhs_copy, enum_size);
    }

    // will this always be memcpy for indirect enum?
    enum_type.vw_initializeWithCopy((swift::opaque_value *)lhs_copy, (swift::opaque_value *)lhs);
    enum_type.vw_initializeWithCopy((swift::opaque_value *)rhs_copy, (swift::opaque_value *)rhs);
    //    memcpy(lhs_copy, lhs, size);
    //    memcpy(rhs_copy, rhs, size);

    // Project data
    assert(enum_type.getValueWitnesses()->flags.hasEnumWitnesses());
    enum_type.vw_destructiveProjectEnumData((swift::opaque_value *)lhs_copy);
    enum_type.vw_destructiveProjectEnumData((swift::opaque_value *)rhs_copy);

    // compare as heap objects
    bool result;
    if (*lhs_copy == *rhs_copy) {
        // projected data are referentially equal
        result = true;
    } else {
        if (*layout_ref == nullptr) {
            *layout_ref = fetch(layout_type, ComparisonOptions(options & 0xfffffeff), 0);
        }

        ValueLayout layout = *layout_ref == ValueLayoutEmpty ? nullptr : *layout_ref;

        static_assert(sizeof(::swift::HeapObject) == 0x10);
        size_t alignment_mask = layout_type.getValueWitnesses()->getAlignmentMask();
        size_t offset = (sizeof(::swift::HeapObject) + alignment_mask) & ~alignment_mask;
        unsigned char *lhs_value = (unsigned char *)(*lhs_copy + offset);
        unsigned char *rhs_value = (unsigned char *)(*rhs_copy + offset);

        result = compare(layout, lhs_value, rhs_value, layout_type.vw_size(), ComparisonOptions(options & 0xfffffeff));
    }

    if (large_allocation) {
        free(lhs_copy);
        free(rhs_copy);
    }

    return result;
}

bool compare_existential_values(const swift::existential_type_metadata &type, const unsigned char *lhs,
                                const unsigned char *rhs, ComparisonOptions options) {

    if (auto lhs_dynamic_type = type.dynamic_type((void *)lhs)) {
        if (auto rhs_dynamic_type = type.dynamic_type((void *)rhs)) {
            if (lhs_dynamic_type == rhs_dynamic_type) {
                unsigned char *lhs_value = (unsigned char *)type.project_value((void *)lhs);
                unsigned char *rhs_value = (unsigned char *)type.project_value((void *)lhs);
                if (lhs_value == rhs_value) {
                    return true;
                }

                if (lhs_value != lhs || rhs_value != rhs) {
                    // remove 0xf00 from comparison options
                    options = ComparisonOptions(options & 0xfffffeff);
                }

                ValueLayout wrapped_layout = fetch(reinterpret_cast<const swift::metadata &>(type), options, 0);
                ValueLayout layout = wrapped_layout == ValueLayoutEmpty ? nullptr : layout;

                return compare(layout, lhs_value, rhs_value, type.vw_size(), options);
            }
        }
    }
    return false;
}

bool compare_partial(ValueLayout layout, const unsigned char *lhs, const unsigned char *rhs, size_t offset, size_t size,
                     ComparisonOptions options) {
    if (lhs == rhs) {
        return true;
    }

    if (layout) {
        Partial partial = find_partial(layout, offset, size);
        if (partial.layout != nullptr) {offset
            size_t remaining_size = size;
            if (partial.location != 0) {
                if (!compare_bytes_top_level(lhs, rhs, partial.location, options)) {
                    return false;
                }
                remaining_size = size >= partial.location ? size - partial.location : 0;
            }

            Compare compare_object = Compare();
            return compare_object(partial.layout, lhs, rhs, partial.location, remaining_size, options);
        }
    }

    return compare_bytes_top_level(lhs, rhs, size, options);
}

Partial find_partial(ValueLayout layout, size_t range_location, size_t range_size) {

    if (range_location == 0) {
        return {layout, 0};
    }

    const unsigned char *c = layout;
    size_t accumulated_size = 0;

    const swift::metadata *enum_type = nullptr;
    while (accumulated_size < range_location) {

        if (*c == '\0') {
            return {nullptr, 0};
        }

        if (*c >= 0x80) {
            accumulated_size += *c & 0x7f + 1; // Convert 0-127 to 1-128
            c += 1;
            continue;
        }

        if (*c >= 0x40) {
            accumulated_size += *c & 0x3f + 1; // Convert 0-63 to 1-64
            c += 1;
            continue;
        }

        switch (*c) {
        case Controls::EqualsItemBegin: {
            c += 1;
            auto type = reinterpret_cast<const swift::metadata *>(c);
            c += Controls::EqualsItemTypePointerSize;
            c += Controls::EqualsItemEquatablePointerSize;
            accumulated_size += type->vw_size();
            continue;
        }
        case Controls::IndirectItemBegin: {
            c += 1;
            auto type = reinterpret_cast<const swift::metadata *>(c);
            c += Controls::IndirectItemTypePointerSize;
            c += Controls::IndirectItemUnusedPointerSize;
            accumulated_size += type->vw_size();
            continue;
        }
        case Controls::ExistentialItemBegin: {
            c += 1;
            auto type = reinterpret_cast<const swift::metadata *>(c);
            c += Controls::ExistentialItemTypePointerSize;
            accumulated_size += type->vw_size();
            continue;
        }
        case Controls::HeapRefItemBegin:
        case Controls::FunctionItemBegin: {
            c += 1;
            accumulated_size += sizeof(void *);
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

            if (accumulated_size + item_size > range_location &&
                accumulated_size + item_size >= range_location + range_size) {
                // offset after applying nested layout exceeds range
                // restart search from nested layout
                range_location -= accumulated_size;
                accumulated_size = 0;
                c = item_layout;
            } else {
                accumulated_size += item_size;
            }
            continue;
        }
        case Controls::CompactNestedItemBegin: {
            c += 1;

            auto item_layout = reinterpret_cast<ValueLayout>(base_address + (uint32_t *)c);
            c += Controls::CompactNestedItemLayoutRelativePointerSize;

            size_t item_size = *(uint16_t *)(c);
            c += Controls::CompactNestedItemLayoutSize;

            if (accumulated_size + item_size > range_location &&
                accumulated_size + item_size >= range_location + range_size) {
                // offset after applying nested layout exceeds range
                // restart search from nested layout
                range_location -= accumulated_size;
                accumulated_size = 0;
                c = item_layout;
            } else {
                accumulated_size += item_size;
            }
            continue;
        }
        case Controls::EnumItemBeginVariadicCaseIndex:
        case Controls::EnumItemBeginCaseIndex0:
        case Controls::EnumItemBeginCaseIndex1:
        case Controls::EnumItemBeginCaseIndex2: {
            if (*c == Controls::EnumItemBeginVariadicCaseIndex) {
                c += 1;
                while (*(int *)c < 0) {
                    c += 1;
                }
                c += 1;
                enum_type = reinterpret_cast<const swift::metadata *>(c);
                c += Controls::EnumItemTypePointerSize;
            } else {
                c += 1;
                enum_type = reinterpret_cast<const swift::metadata *>(c);
                c += Controls::EnumItemTypePointerSize;
            }

            c += length(c);
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
            if (*c == Controls::EnumItemContinueVariadicCaseIndex) {
                c += 1;
                while (*(int *)c < 0) {
                    c += 1;
                }
                c += 1;
            } else {
                c += 1;
            }

            c += length(c);
        }
        case Controls::EnumItemEnd: {
            c += 1;
            accumulated_size += enum_type->vw_size();
            enum_type = nullptr;
            continue;
        }
        default: {
            c += 1;
        }
        }
    };
    
    return {c, accumulated_size};
}

} // namespace LayoutDescriptor

} // namespace AG
