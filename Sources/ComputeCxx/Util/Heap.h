#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

CF_ASSUME_NONNULL_BEGIN

namespace util {

class Heap {
  private:
    typedef struct Node {
        struct Node *next;
        void *buffer;
    } Node;

    uint64_t _increment;
    Node *_node;
    char *_free_start;
    uint64_t _capacity;

  public:
    static constexpr uint64_t minimum_increment = 0x400;

    Heap(char *_Nullable start, uint64_t capacity, uint64_t increment);
    ~Heap();

    char *alloc_(uint64_t arg1);
    void reset(char *_Nullable start, uint64_t capacity);
};

} // namespace util

CF_ASSUME_NONNULL_END
