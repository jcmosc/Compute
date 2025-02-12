#pragma once

#include <CoreFoundation/CFBase.h>
#include <bitset>
#include <mach/vm_types.h>
#include <malloc/malloc.h>
#include <os/lock.h>
#include <stdint.h>
#include <utility>

#include "Containers/Vector.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {
namespace data {

class zone;
class page;
template <typename T> class ptr;

class table {
  public:
    class malloc_zone_deleter {
      public:
        void operator()(void *p) { malloc_zone_free(_malloc_zone, p); }
    };

    static malloc_zone_t *_malloc_zone;
    static std::unique_ptr<void, malloc_zone_deleter> alloc_persistent(size_t size);

  private:
    vm_address_t _ptr_base;
    vm_address_t _vm_region_base_address;
    os_unfair_lock _lock = OS_UNFAIR_LOCK_INIT;
    uint32_t _vm_region_size;
    uint32_t _ptr_max_offset;

    uint32_t _num_used_pages = 0;
    uint32_t _num_reusable_bytes = 0;
    uint32_t _map_search_start = 0;

    uint32_t _num_zones = 0;

    using remapped_region = std::pair<vm_address_t, int64_t>;
    vector<remapped_region, 0, uint32_t> _remapped_regions = {};

    constexpr static unsigned int pages_per_map = 64;
    using page_map_type = std::bitset<pages_per_map>;
    vector<page_map_type, 0, uint32_t> _page_maps = {};
    vector<page_map_type, 0, uint32_t> _page_metadata_maps = {};

  public:
    static table &ensure_shared();
    static table &shared();

    table();

    void lock();
    void unlock();

    // Pointers
    vm_address_t ptr_base() { return _ptr_base; };
    uint32_t ptr_max_offset() { return _ptr_max_offset; };

    // Region
    void grow_region();

    // Zones
    uint32_t make_zone_id();

    // Pages
    ptr<page> alloc_page(zone *zone, uint32_t size);
    void dealloc_page_locked(ptr<page> page);
    void make_pages_reusable(uint32_t page_index, bool flag);
    uint64_t raw_page_seed(ptr<page> page);

    // Printing
    void print();
};

} // namespace data
} // namespace AG

CF_ASSUME_NONNULL_END
