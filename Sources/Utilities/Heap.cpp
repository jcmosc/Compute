#include "Utilities/Heap.h"

#include <algorithm>

namespace util {

constexpr uint64_t default_increment = 0x2000;

Heap::Heap(char *start, uint64_t capacity, uint64_t increment) {
    // enforce minimum but treat 0 as the default
    uint64_t effective_increment = increment > 0 ? std::max(increment, minimum_increment) : default_increment;

    _increment = effective_increment;
    _node = nullptr;
    reset(start, capacity);
};

util::Heap::~Heap() { reset(nullptr, 0); }

void *util::Heap::alloc_(uint64_t size) {
    if (_capacity >= size) {
        char *result = _free_start;
        _free_start += size;
        _capacity -= size;
        return result;
    }

    if (size <= minimum_increment) {
        int64_t increment = _increment;
        char *buffer = static_cast<char *>(malloc(increment));

        _free_start = buffer;
        _capacity = increment;

        Node *node = alloc<Node>();
        node->next = _node;
        node->buffer = buffer;
        _node = node;

        char *result = _free_start;
        _free_start += size;
        _capacity -= size;
        return result;
    }

    Node *node = alloc<Node>();
    char *result = (char *)malloc(size);
    if (result) {
        node->next = _node;
        node->buffer = result;
        _node = node;
    }
    return result;
}

void util::Heap::reset(char *_Nullable start, uint64_t capacity) {
    while (_node) {
        void *buffer = _node->buffer;
        _node = _node->next;
        free(buffer);
    }

    constexpr uintptr_t alignment_mask = sizeof(char *) - 1;
    char *aligned_start = (char *)(((uintptr_t)start + alignment_mask) & ~alignment_mask);

    bool prealigned = ((uintptr_t)start & alignment_mask) == 0;
    _free_start = prealigned ? start : aligned_start;
    _capacity = capacity + (start - aligned_start);
}

}; // namespace util
