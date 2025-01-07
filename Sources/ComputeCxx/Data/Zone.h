#pragma once

#include <CoreFoundation/CFBase.h>
#include <malloc/malloc.h>

#include "Page.h"
#include "Pointer.h"
#include "Table.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {
namespace data {

class zone {
  private:
    typedef struct _bytes_info {
        ptr<struct _bytes_info> next;
        uint32_t size;
    } bytes_info;

    constexpr static uint32_t seed_id_mask = 0x7fffffff;

    vector<std::unique_ptr<void, table::malloc_zone_deleter>, 0, uint32_t> _malloc_buffers;
    ptr<page> _last_page;
    ptr<bytes_info> _free_bytes;
    uint32_t _seed; // contains id (31 bits) and one bit flag for invalidation

  public:
    zone();
    ~zone();

    uint32_t seed() { return _seed; };
    uint32_t id() { return _seed & seed_id_mask; };

    void clear();
    void realloc_bytes(ptr<void> *buffer, uint32_t size, uint32_t new_size, uint32_t alignment_mask);

    // Paged memory
    ptr<void> alloc_bytes_recycle(uint32_t size, uint32_t alignment_mask);
    ptr<void> alloc_bytes(uint32_t size, uint32_t alignment_mask);
    ptr<void> alloc_slow(uint32_t size, uint32_t alignment_mask);

    // Persistent memory
    void *alloc_persistent(size_t size);

    // Printing
    void print_header();
    void print();
};

} // namespace data
} // namespace AG

CF_ASSUME_NONNULL_END
