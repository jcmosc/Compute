#include "LayoutDescriptor.h"

#include <os/lock.h>
#include <variant>

#include "Builder.h"
#include "Compare.h"
#include "Controls.h"
#include "Graph/Graph.h"
#include "Graph/UpdateStack.h"
#include "Swift/Metadata.h"
#include "Time/Time.h"
#include "Trace/Trace.h"
#include "Utilities/HashTable.h"

namespace AG {

namespace {

unsigned int print_layouts() {
    static unsigned int print_layouts = print_layouts = []() -> bool {
        char *result = getenv("AG_PRINT_LAYOUTS");
        if (result) {
            return atoi(result) != 0;
        }
        return false;
    }();
    return print_layouts;
}

} // namespace

#pragma mark - TypeDescriptorCache

namespace {

class TypeDescriptorCache {
  public:
    struct QueueEntry {
        const swift::metadata *type;
        LayoutDescriptor::ComparisonMode comparison_mode;
        LayoutDescriptor::HeapMode heap_mode;
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

    static void *make_key(const swift::metadata *type, LayoutDescriptor::ComparisonMode comparison_mode,
                          LayoutDescriptor::HeapMode heap_mode);

    ValueLayout fetch(const swift::metadata &type, LayoutDescriptor::ComparisonOptions options,
                      LayoutDescriptor::HeapMode heap_mode, uint32_t priority);
    static void drain_queue(void *cache);
};

TypeDescriptorCache *TypeDescriptorCache::_shared_cache = nullptr;
dispatch_once_t TypeDescriptorCache::_shared_once = 0;

void *TypeDescriptorCache::make_key(const swift::metadata *type, LayoutDescriptor::ComparisonMode comparison_mode,
                                    LayoutDescriptor::HeapMode heap_mode) {
    uintptr_t type_address = (uintptr_t)type;
    uintptr_t result =
        heap_mode != LayoutDescriptor::HeapMode::Option0 ? (~type_address & 0xfffffffffffffffc) : type_address;
    result = result | comparison_mode;
    return (void *)result;
}

ValueLayout TypeDescriptorCache::fetch(const swift::metadata &type, LayoutDescriptor::ComparisonOptions options,
                                       LayoutDescriptor::HeapMode heap_mode, uint32_t priority) {
    LayoutDescriptor::ComparisonMode comparison_mode = options.comparision_mode();

    void *key = make_key(&type, comparison_mode, heap_mode);

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

    if (options.fetch_layouts_synchronously() || !async_layouts) {
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

    // insert layout asynchronously
    lock();
    _table.insert((void *)key, nullptr);

    _async_queue.push_back({
        &type,
        comparison_mode,
        heap_mode,
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

void TypeDescriptorCache::drain_queue(void *context) {
    TypeDescriptorCache *cache = (TypeDescriptorCache *)context;

    double start_time = current_time();
    cache->lock();

    uint64_t created_count = 0;
    while (cache->_async_queue.size() > 0) {
        std::pop_heap(cache->_async_queue.begin(), cache->_async_queue.end());
        auto entry = cache->_async_queue.back();

        void *key = make_key(entry.type, entry.comparison_mode, entry.heap_mode);

        const void *found = nullptr;
        ValueLayout layout = (ValueLayout)cache->_table.lookup(key, &found);

        if (layout == nullptr && found != nullptr) {
            cache->unlock();
            layout = LayoutDescriptor::make_layout(*entry.type, entry.comparison_mode, entry.heap_mode);
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

namespace LayoutDescriptor {

uintptr_t base_address = 0;

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
            c += Controls::IndirectItemLayoutPointerSize;
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
    if (options.report_failures() && !result) {
        for (auto update = Graph::current_update(); update != nullptr; update = update.get()->previous()) {
            auto graph = update.get()->graph();
            auto attribute = update.get()->frames().back().attribute;
            graph->foreach_trace([&attribute, &lhs, &rhs, &failure_location](Trace &trace) {
                trace.compare_failed(attribute, lhs, rhs, failure_location, 1, nullptr);
            });
        }
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
        ComparisonOptions(options.comparision_mode()); // this has the effect of allowing async fetch
    ValueLayout layout = TypeDescriptorCache::shared_cache().fetch(*lhs_type, fetch_options, heap_mode, 1);

    if (layout > ValueLayoutEmpty) {
        return compare(layout, lhs, rhs, -1, options.without_copying_on_write());
    }

    return false;
}

// https://www.swift.org/blog/how-mirror-works/
bool compare_indirect(ValueLayout *layout_ref, const swift::metadata &enum_type, const swift::metadata &layout_type,
                      ComparisonOptions options, const unsigned char *lhs, const unsigned char *rhs) {

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

    enum_type.vw_initializeWithCopy((swift::opaque_value *)lhs_copy, (swift::opaque_value *)lhs);
    enum_type.vw_initializeWithCopy((swift::opaque_value *)rhs_copy, (swift::opaque_value *)rhs);

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
            *layout_ref = fetch(layout_type, options.without_copying_on_write(), 0);
        }

        ValueLayout layout = *layout_ref == ValueLayoutEmpty ? nullptr : *layout_ref;

        static_assert(sizeof(::swift::HeapObject) == 0x10);
        size_t alignment_mask = layout_type.getValueWitnesses()->getAlignmentMask();
        size_t offset = (sizeof(::swift::HeapObject) + alignment_mask) & ~alignment_mask;
        unsigned char *lhs_value = (unsigned char *)(*lhs_copy + offset);
        unsigned char *rhs_value = (unsigned char *)(*rhs_copy + offset);

        result = compare(layout, lhs_value, rhs_value, layout_type.vw_size(), options.without_copying_on_write());
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
                    options = options.without_copying_on_write();
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
        if (partial.layout != nullptr) {
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
            c += Controls::IndirectItemLayoutPointerSize;
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

void print(std::string &output, ValueLayout layout) {
    auto print_format = [&output](const char *format, ...) {
        char *message = nullptr;

        va_list args;
        va_start(args, format);
        vasprintf(&message, format, args);
        va_end(args);

        if (message != nullptr) {
            output.append(message);
            free(message);
        }
    };

    print_format("(layout #:length %d #:address %p", length(layout), layout);

    unsigned int indent = 3;
    const unsigned char *c = layout;
    while (true) {
        if (*c == '\0') {
            return;
        }

        if (*c >= 0x40 && *c < 0x80) {
            size_t length = *c & 0x3f + 1; // Convert 0-63 to 1-64
            c += 1;
            output.push_back('\n');
            output.append(indent * 2, ' ');
            print_format("(skip %u)", length);
            continue;
        }

        if (*c >= 0x80) {
            size_t length = *c & 0x7f + 1; // Convert 0-127 to 1-128
            c += 1;
            output.push_back('\n');
            output.append(indent * 2, ' ');
            print_format("(read %u)", length);
            continue;
        }

        switch (*c) {
        case Controls::EqualsItemBegin: {
            c += 1;

            auto type = reinterpret_cast<const swift::metadata *>(c);
            c += Controls::EqualsItemTypePointerSize;
            c += Controls::EqualsItemEquatablePointerSize;

            output.push_back('\n');
            output.append(indent * 2, ' ');
            print_format("(== #:size %d #:type %s)", type->vw_size(), type);
            continue;
        }
        case Controls::IndirectItemBegin: {
            c += 1;

            auto type = reinterpret_cast<const swift::metadata *>(c);
            c += Controls::IndirectItemTypePointerSize;
            c += Controls::IndirectItemLayoutPointerSize;

            output.push_back('\n');
            output.append(indent * 2, ' ');
            print_format("(indirect #:size %d #:type %s)", type->vw_size(), type);
            continue;
        }
        case Controls::ExistentialItemBegin: {
            c += 1;

            auto type = reinterpret_cast<const swift::metadata *>(c);
            c += Controls::ExistentialItemTypePointerSize;

            output.push_back('\n');
            output.append(indent * 2, ' ');
            print_format("(existential #:size %d #:type %s)", type->vw_size(), type);
            continue;
        }
        case Controls::HeapRefItemBegin:
        case Controls::FunctionItemBegin: {
            bool is_heap_ref = *c == Controls::HeapRefItemBegin;
            c += 1;

            output.push_back('\n');
            output.append(indent * 2, ' ');
            print_format("(%s)", is_heap_ref ? "heap-ref" : "capture-ref");
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

            output.push_back('\n');
            output.append(indent * 2, ' ');
            print_format("(nested%s #:size %d #:layout %p)", item_size, item_layout);
            continue;
        }
        case Controls::CompactNestedItemBegin: {
            c += 1;

            auto item_layout = reinterpret_cast<ValueLayout>(base_address + *(uint32_t *)c);
            c += Controls::CompactNestedItemLayoutRelativePointerSize;

            size_t item_size = *(uint16_t *)(c);
            c += Controls::CompactNestedItemLayoutSize;

            output.push_back('\n');
            output.append(indent * 2, ' ');
            print_format("(nested%s #:size %d #:layout %p)", item_size, item_layout);
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

            output.push_back('\n');
            output.append(indent * 2 + 4, ' ');
            print_format("(enum #:size %d #:type %s", type->vw_size(), type);

            output.push_back('\n');
            output.append(indent * 2 + 4, ' ');
            print_format("(case %d", enum_tag);

            indent += 4;
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

            output.push_back('\n');
            output.append(indent * 2 - 4, ' ');
            print_format("(case %d", enum_tag);
            continue;
        }
        case Controls::EnumItemEnd: {
            c += 1;
            indent -= 4;
            output.push_back(')');

            continue;
        }
        }
    }
}

#pragma mark - Builder

os_unfair_lock Builder::_lock = OS_UNFAIR_LOCK_INIT;
size_t Builder::_avail = 0;
unsigned char *Builder::_buffer = nullptr;

ValueLayout Builder::commit(const swift::metadata &type) {
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
        result = _buffer;
        _avail -= layout_data.size();
        _buffer += layout_data.size();
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

void Builder::add_field(size_t field_size) {
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

bool Builder::should_visit_fields(const swift::metadata &type, bool no_fetch) {
    if (!no_fetch) {
        if (auto layout = fetch(type,
                                ComparisonOptions(_current_comparison_mode) | ComparisonOptions::ReportFailures |
                                    ComparisonOptions::FetchLayoutsSynchronously,
                                true)) {
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

void Builder::revert(const RevertItemsInfo &info) {
    auto items = get_items();
    while (items.size() > info.item_index) {
        items.pop_back();
    }
    if (items.size() > 0 && info.data_item.offset != -1) {
        items[items.size() - 1] = info.data_item;
    }
}

bool Builder::visit_element(const swift::metadata &type, const swift::metadata::ref_kind kind, size_t element_offset,
                            size_t element_size) {
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

bool Builder::visit_case(const swift::metadata &type, const swift::field_record &field, uint32_t index) {
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
        auto mangled_name = field.FieldName.get(); // TODO: check this is FieldName of MangledTypeName
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

bool Builder::visit_existential(const swift::existential_type_metadata &type) {
    if (_current_comparison_mode == 0 ||
        (_current_comparison_mode == 1 && type.representation() == ::swift::ExistentialTypeRepresentation::Class)) {
        return false;
    }

    get_items().push_back(ExistentialItem(RangeItem(_current_offset, type.vw_size()), &type));
    return true;
}

bool Builder::visit_function(const swift::function_type_metadata &type) {
    if (_current_comparison_mode == 0 || type.getConvention() != ::swift::FunctionMetadataConvention::Swift) {
        return false;
    }

    add_field(sizeof(void *));
    get_items().push_back(HeapRefItem(RangeItem(_current_offset + sizeof(void *), sizeof(void *)), true));
    return true;
}

bool Builder::visit_native_object(const swift::metadata &type) {
    if (_heap_mode != HeapMode::Option2) {
        return false;
    }

    get_items().push_back(HeapRefItem(RangeItem(_current_offset, sizeof(void *)), false));
    return true;
}

#pragma mark - Builder::Emitter

void Builder::Emitter<vector<unsigned char, 512, uint64_t>>::operator()(const DataItem &item) {
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

void Builder::Emitter<vector<unsigned char, 512, uint64_t>>::operator()(const EqualsItem &item) {
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

void Builder::Emitter<vector<unsigned char, 512, uint64_t>>::operator()(const IndirectItem &item) {
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

void Builder::Emitter<vector<unsigned char, 512, uint64_t>>::operator()(const ExistentialItem &item) {
    enter(item);
    _data->push_back(Controls::ExistentialItemBegin);
    _data->reserve(_data->size() + 8);
    for (int i = 0; i < sizeof(void *); ++i) {
        _data->push_back(*((unsigned char *)item.type + i));
    }
    _emitted_size += item.size;
}

void Builder::Emitter<vector<unsigned char, 512, uint64_t>>::operator()(const HeapRefItem &item) {
    enter(item);
    _data->push_back(item.is_function ? Controls::FunctionItemBegin : Controls::HeapRefItemBegin);
    _emitted_size += item.size;
}

void Builder::Emitter<vector<unsigned char, 512, uint64_t>>::operator()(const NestedItem &item) {
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

void Builder::Emitter<vector<unsigned char, 512, uint64_t>>::operator()(const EnumItem &item) {
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

void Builder::Emitter<vector<unsigned char, 512, uint64_t>>::enter(const RangeItem &item) {
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

void Builder::Emitter<vector<unsigned char, 512, uint64_t>>::finish() {
    _data->push_back('\0'); // NULL terminating char
}

} // namespace LayoutDescriptor

} // namespace AG
