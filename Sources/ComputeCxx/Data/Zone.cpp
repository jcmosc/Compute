#include "Zone.h"

#include <malloc/malloc.h>

#include "Errors/Errors.h"
#include "Page.h"
#include "Table.h"
#include <stdio.h>

namespace AG {
namespace data {

zone::zone() : _info(info().with_zone_id(table::shared().make_zone_id())) {}

zone::~zone() { clear(); }

void zone::clear() {
    table::shared().lock();
    while (_last_page) {
        auto page = _last_page;
        _last_page = page->previous;
        table::shared().dealloc_page_locked(page);
    }
    table::shared().unlock();
}

void zone::realloc_bytes(ptr<void> *buffer, uint32_t size, uint32_t new_size, uint32_t alignment_mask) {
    if (new_size > size && *buffer) {
        auto page = buffer->page_ptr();
        if ((page->in_use == buffer->offset_from_page() + size &&
             page->total >= buffer->offset_from_page() + new_size)) {
            page->in_use += new_size - size;
        } else {
            ptr<void> new_buffer = alloc_bytes_recycle(new_size, alignment_mask);
            if (*buffer) {
                memcpy(new_buffer.get(), (*buffer).get(), size);

                ptr<bytes_info> old_bytes = (*buffer).aligned<bytes_info>();
                uint32_t remaining_size = size + (*buffer - old_bytes);
                if (remaining_size >= sizeof(bytes_info)) {
                    old_bytes->next = _free_bytes;
                    old_bytes->size = remaining_size;
                    _free_bytes = old_bytes;
                }
            }
            *buffer = new_buffer;
        }
    }
}

ptr<void> zone::alloc_bytes_recycle(uint32_t size, uint32_t alignment_mask) {
    for (ptr<bytes_info> bytes = _free_bytes; bytes; bytes = bytes->next) {
        if (size > bytes->size) {
            continue;
        }

        ptr<void> aligned_bytes = bytes.aligned<void>(alignment_mask);
        uint32_t usable_size = bytes->size + (bytes - aligned_bytes);
        if (size > usable_size) {
            continue;
        }

        // Check the difference between needed and available bytes
        // XXX: Dissassembly seems to rely on unsigned integer underflow here?
        if (size - bytes->size > 0xff) {
            continue;
        }

        // check if there will be some bytes remaining within the same page
        ptr<void> end = aligned_bytes + size;
        if ((uint32_t(aligned_bytes) ^ uint32_t(end)) <= page_alignment_mask) {
            ptr<bytes_info> aligned_end = end.aligned<bytes_info>();
            uint32_t remaining_size = usable_size - size + (end - aligned_end);
            if (remaining_size >= sizeof(bytes_info)) {
                bytes_info *remaining_bytes = aligned_end.get();
                remaining_bytes->next = _free_bytes;
                remaining_bytes->size = remaining_size;
                _free_bytes = aligned_end;
            }
        }

        return aligned_bytes;
    }

    return alloc_bytes(size, alignment_mask);
}

ptr<void> zone::alloc_bytes(uint32_t size, uint32_t alignment_mask) {
    if (_last_page) {
        uint32_t aligned_in_use = (_last_page->in_use + alignment_mask) & ~alignment_mask;
        uint32_t new_used_size = aligned_in_use + size;
        if (new_used_size <= _last_page->total) {
            _last_page->in_use = new_used_size;
            return _last_page + aligned_in_use;
        }
    }

    return alloc_slow(size, alignment_mask);
}

ptr<void> zone::alloc_slow(uint32_t size, uint32_t alignment_mask) {
    if (_last_page) {

        // check if we can use remaining bytes in this page
        ptr<void> next_bytes = _last_page + _last_page->in_use;
        if (next_bytes.page_ptr() == _last_page) {

            ptr<bytes_info> aligned_next_bytes = next_bytes.aligned<bytes_info>();
            int32_t remaining_size = _last_page->total - _last_page->in_use + (next_bytes - aligned_next_bytes);
            if (remaining_size >= sizeof(bytes_info)) {
                bytes_info *remaining_bytes = aligned_next_bytes.get();
                remaining_bytes->next = _free_bytes;
                remaining_bytes->size = remaining_size;
                _free_bytes = aligned_next_bytes;
            }

            // consume this entire page
            _last_page->in_use = _last_page->total;
        }
    }

    ptr<page> new_page;
    if (size <= page_size / 2) {
        new_page = table::shared().alloc_page(this, page_size);
        new_page->previous = _last_page;
        _last_page = new_page;
    } else {
        uint32_t aligned_size = ((sizeof(page) + size) + alignment_mask) & ~alignment_mask;
        new_page = table::shared().alloc_page(this, aligned_size);
        if (_last_page) {
            // It's less likely we will be able to alloc unused bytes from this page,
            // so insert it before the last page.
            new_page->previous = _last_page->previous;
            _last_page->previous = new_page;
        } else {
            _last_page = new_page;
        }
    }

    int32_t aligned_used_bytes = (new_page->in_use + alignment_mask) & ~alignment_mask;

    // Sanity check
    if (aligned_used_bytes + size > new_page->total) {
        precondition_failure("internal error");
    }

    new_page->in_use = aligned_used_bytes + size;
    return new_page + aligned_used_bytes;
};

#pragma mark - Persistent memory

void *zone::alloc_persistent(size_t size) {
    if (size == 0) {
        return nullptr;
    }

    auto buffer = table::shared().alloc_persistent(size);
    _malloc_buffers.push_back(std::move(buffer));

    return _malloc_buffers.back().get();
}

#pragma mark - Printing

void zone::print_header() {
    fprintf(stdout, "Zones\n%-16s %6s %8s %8s    %6s %6s     %6s %8s\n", "zone ptr", "pages", "total", "in-use", "free",
            "bytes", "malloc", "total");
}

void zone::print() {

    unsigned long num_pages = 0;
    double pages_total_kb = 0.0;
    double pages_in_use_kb = 0.0;
    if (_last_page) {
        int64_t pages_total = 0;
        int64_t pages_in_use = 0;
        for (auto page = _last_page; page; page = page->previous) {
            num_pages++;
            pages_total += page->total;
            pages_in_use += page->in_use;
        }
        pages_total_kb = pages_total / 1024.0;
        pages_in_use_kb = pages_in_use / 1024.0;
    }

    unsigned long num_free_elements = 0;
    unsigned long free_bytes = 0;
    if (_free_bytes) {
        for (auto bytes = _free_bytes; bytes; bytes = bytes->next) {
            num_free_elements++;
            free_bytes += bytes->size;
        }
    }

    unsigned long num_persistent_buffers = _malloc_buffers.size();
    size_t malloc_total_size = 0;
    for (auto &element : _malloc_buffers) {
        malloc_total_size += malloc_size(element.get());
    }
    double malloc_total_size_kb = malloc_total_size / 1024.0;

    fprintf(stdout, "%-16p %6lu %8.2f %8.2f    %6lu %6lu     %6lu %8.2f\n",
            this,                   // zone ptr
            num_pages,              // pages
            pages_total_kb,         // total
            pages_in_use_kb,        // in-use
            num_free_elements,      // free
            free_bytes,             // bytes
            num_persistent_buffers, // malloc
            malloc_total_size_kb    // total
    );
}

} // namespace data
} // namespace AG
