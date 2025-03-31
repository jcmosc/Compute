#include "Table.h"

#include <algorithm>
#include <cerrno>
#include <dispatch/dispatch.h>
#include <mach/mach.h>
#include <malloc/malloc.h>
#include <os/lock.h>
#include <sys/mman.h>

#include "Errors/Errors.h"
#include "Page.h"
#include "Zone.h"

static void *AGGraphVMRegionBaseAddress;

namespace AG {
namespace data {

table _shared_table_bytes;

table &table::ensure_shared() {
    static dispatch_once_t onceToken;
    dispatch_once_f(&onceToken, nullptr, [](void *_Nullable context) { new (&_shared_table_bytes) table(); });
    return _shared_table_bytes;
}

table &table::shared() { return _shared_table_bytes; }

malloc_zone_t *table::_malloc_zone = nullptr;

std::unique_ptr<void, table::malloc_zone_deleter> table::alloc_persistent(size_t size) {
    void *buffer = malloc_zone_malloc(table::_malloc_zone, size);
    if (!buffer) {
        precondition_failure("memory allocation failure (%lu bytes)", size);
    }
    return std::unique_ptr<void, table::malloc_zone_deleter>(buffer);
}

table::table() {
    constexpr vm_size_t initial_size = 32 * pages_per_map * page_size;

    void *region = mmap(nullptr, initial_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (region == MAP_FAILED) {
        precondition_failure("memory allocation failure (%u bytes, %u)", initial_size, errno);
    }

    _vm_region_base_address = reinterpret_cast<vm_address_t>(region);
    _vm_region_size = initial_size;

    AGGraphVMRegionBaseAddress = region;

    _ptr_base = reinterpret_cast<vm_address_t>(region) - page_size;
    _ptr_max_offset = initial_size + page_size;

    if (!_malloc_zone) {
        _malloc_zone = malloc_create_zone(0, 0);
        malloc_set_zone_name(_malloc_zone, "Compute graph data");
    }
}

void table::lock() { os_unfair_lock_lock(&_lock); }

void table::unlock() { os_unfair_lock_unlock(&_lock); }

#pragma mark - Region

void table::grow_region() {
    uint64_t new_size = 4 * _vm_region_size;

    // Check size does not exceed 32 bits
    if ((uint32_t)new_size <= _vm_region_size) {
        precondition_failure("exhausted data space");
    }

    void *new_region = mmap(nullptr, new_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (new_region == MAP_FAILED) {
        precondition_failure("memory allocation failure (%u bytes, %u)", new_size, errno);
    }

    vm_prot_t cur_protection = VM_PROT_NONE;
    vm_prot_t max_protection = VM_PROT_NONE;
    kern_return_t error =
        vm_remap(mach_task_self(), reinterpret_cast<vm_address_t *>(&new_region), _vm_region_size, 0,
                 VM_FLAGS_OVERWRITE, mach_task_self(), reinterpret_cast<vm_address_t>(_vm_region_base_address), false,
                 &cur_protection, &max_protection, VM_INHERIT_NONE);
    if (error) {
        precondition_failure("vm_remap failure: 0x%x", error);
    }

    _remapped_regions.push_back({this->_vm_region_base_address, this->_vm_region_size});

    _vm_region_base_address = reinterpret_cast<vm_address_t>(new_region);
    AGGraphVMRegionBaseAddress = new_region;

    _vm_region_size = static_cast<uint32_t>(new_size);
    _ptr_base = reinterpret_cast<vm_address_t>(new_region) - page_size;
    _ptr_max_offset = _vm_region_size + page_size;
}

#pragma mark - Zones

uint32_t table::make_zone_id() {
    _num_zones += 1;
    return _num_zones;
}

#pragma mark - Pages

ptr<page> table::alloc_page(zone *zone, uint32_t needed_size) {
    lock();

    uint32_t needed_pages = (needed_size + page_alignment_mask) / page_size;

    // assume we'll have to append a new page
    uint32_t new_page_index = _page_maps.size() * pages_per_map;

    // scan for consecutive free pages
    if (!_page_maps.empty() && _num_used_pages <= _page_maps.size() * pages_per_map) {

        uint32_t start_map_index = _map_search_start;
        for (int i = 0; i < _page_maps.size(); i++) {
            int map_index = start_map_index + i;
            if (map_index >= _page_maps.size()) {
                map_index -= _page_maps.size(); // wrap around
            }

            page_map_type free_pages_map = _page_maps[map_index].flip();
            while (free_pages_map.any()) {

                int candidate_bit = std::countr_zero(static_cast<uint64_t>(free_pages_map.to_ullong()));

                // scan ahead to find enough consecutive free pages
                bool found = false;
                if (needed_pages > 1) {
                    for (int j = 1; j < needed_pages; j++) {
                        int next_page_index = (map_index * pages_per_map) + candidate_bit + j;
                        int next_map_index = next_page_index / pages_per_map;
                        if (next_map_index == _page_maps.size()) {
                            // There are not enough maps, but the trailing pages are contiguous so this page is
                            // usable
                            found = true;
                            break;
                        }
                        if (_page_maps[next_map_index].test(next_page_index % pages_per_map)) {
                            // next page is used, remove this page from free_pages_map
                            free_pages_map.reset(candidate_bit);
                            break;
                        }
                    }
                    found = true;
                } else {
                    // only need one page
                    found = true;
                }

                if (found) {
                    new_page_index = (map_index * pages_per_map) + candidate_bit;
                    _map_search_start = map_index;
                    break;
                }
            }
        }
    }

    // update maps
    for (int i = 0; i < needed_pages; i++) {
        uint32_t page_index = new_page_index + i;
        uint32_t map_index = page_index / pages_per_map;

        if (map_index == _page_maps.size()) {
            _page_maps.push_back(0);
            _page_metadata_maps.push_back(0);
        } else if (_page_maps[map_index] == 0) {
            make_pages_reusable(page_index, false);
        }

        _page_maps[map_index].set(page_index % pages_per_map);
        if (i == 0) {
            _page_metadata_maps[map_index].set(page_index % pages_per_map);
        }
    }

    _num_used_pages += needed_pages;

    uint32_t new_region_size = (new_page_index + needed_pages) * page_size;
    while (_vm_region_size < new_region_size) {
        grow_region();
    }

    // ptr offsets are "one"-based, so that we can treat 0 as null.
    ptr<page> new_page = ptr<page>((new_page_index + 1) * page_size);
    new_page->zone = zone;
    new_page->previous = nullptr;
    new_page->total = (needed_size + page_alignment_mask) & ~page_alignment_mask;
    new_page->in_use = sizeof(page);

    unlock();

    return new_page;
}

void table::dealloc_page_locked(ptr<page> page) {
    int32_t total_bytes = page->total;
    int32_t num_pages = total_bytes / page_size;

    _num_used_pages -= num_pages;

    // convert the page address (starts at 512) to an index (starts at 0)
    int32_t page_index = (page / page_size) - 1;
    for (int32_t i = 0; i != num_pages; i += 1) {

        int32_t next_page_index = page_index + i;
        int32_t next_map_index = next_page_index / pages_per_map;

        _page_maps[next_map_index].reset(next_page_index % pages_per_map);
        if (i == 0) {
            _page_metadata_maps[next_map_index].reset(next_page_index % pages_per_map);
        }

        if (_page_maps[next_map_index].none()) {
            make_pages_reusable(next_page_index, true);
        }
    }
}

void table::make_pages_reusable(uint32_t page_index, bool reusable) {
    static constexpr uint32_t mapped_pages_size = page_size * pages_per_map; // 64 * 512 = 0x8000

    void *mapped_pages_address =
        reinterpret_cast<void *>(_vm_region_base_address + ((page_index * page_size) & ~(mapped_pages_size - 1)));

    int advice = reusable ? MADV_FREE_REUSABLE : MADV_FREE_REUSE;
    madvise(mapped_pages_address, mapped_pages_size, advice);

    static bool unmap_reusable = []() -> bool {
        char *result = getenv("AG_UNMAP_REUSABLE");
        if (result) {
            return atoi(result) != 0;
        }
        return false;
    }();

    if (unmap_reusable) {
        int protection = reusable ? PROT_NONE : (PROT_READ | PROT_WRITE);
        mprotect(mapped_pages_address, mapped_pages_size, protection);
    }

    _num_reusable_bytes += reusable ? mapped_pages_size : -mapped_pages_size;
}

uint64_t table::raw_page_seed(ptr<page> page) {
    page.assert_valid();

    lock();

    uint32_t page_index = (page / page_size) - 1;
    uint32_t map_index = page_index / pages_per_map;

    uint64_t result = 0;
    if (map_index < _page_metadata_maps.size() && _page_metadata_maps[map_index].test(page_index % page_size)) {
        auto raw_zone_info = page->zone->info().to_raw_value();
        result = raw_zone_info | (1 < 8);
    }

    unlock();

    return result;
}

#pragma mark - Printing

void table::print() {
    lock();
    fprintf(stdout, "data::table %p:\n  %.2fKB allocated, %.2fKB used, %.2fKB reusable.\n", this,
            (_ptr_max_offset - page_size) / 1024.0, (_num_used_pages * page_size) / 1024.0,
            _num_reusable_bytes / 1024.0);
    unlock();
}

} // namespace data
} // namespace AG
