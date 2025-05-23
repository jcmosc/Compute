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
  public:
    class info {
      private:
        enum {
            id_mask = 0x7fffffff,
            deleted = 0x80000000,
        };
        uint32_t _value;
        info(uint32_t value) : _value(value) {};

      public:
        uint32_t zone_id() { return _value & id_mask; };
        info with_zone_id(uint32_t zone_id) const { return info((_value & ~id_mask) | (zone_id & id_mask)); };

        info with_deleted() const { return info(_value | deleted); };

        uint32_t to_raw_value() { return _value; };
        static info from_raw_value(uint32_t value) { return info(value); };
    };

  private:
    typedef struct _bytes_info {
        ptr<struct _bytes_info> next;
        uint32_t size;
    } bytes_info;

    vector<std::unique_ptr<void, table::malloc_zone_deleter>, 0, uint32_t> _malloc_buffers;
    ptr<page> _first_page;
    ptr<bytes_info> _free_bytes;
    info _info;

    ptr<void> alloc_slow(uint32_t size, uint32_t alignment_mask);

  public:
    zone();
    ~zone();

    info info() const { return _info; };
    void mark_deleted() { _info = _info.with_deleted(); };

    page_ptr_list pages() const { return page_ptr_list(_first_page); };

    void clear();
    void realloc_bytes(ptr<void> *buffer, uint32_t size, uint32_t new_size, uint32_t alignment_mask);

    // Paged memory
    ptr<void> alloc_bytes(uint32_t size, uint32_t alignment_mask);
    ptr<void> alloc_bytes_recycle(uint32_t size, uint32_t alignment_mask);
    

    // Persistent memory
    void *alloc_persistent(size_t size);

    // Printing
    static void print_header();
    void print();
};

} // namespace data
} // namespace AG

CF_ASSUME_NONNULL_END
