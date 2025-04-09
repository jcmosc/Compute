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

    void *alloc_(uint64_t arg1);

  public:
    static constexpr uint64_t minimum_increment = 0x400;

    Heap(char *_Nullable start, uint64_t capacity, uint64_t increment);
    ~Heap();

    template <typename T> T *_Nonnull alloc(size_t count = 1) { return static_cast<T *>(alloc_(sizeof(T) * count)); };
    void reset(char *_Nullable start, uint64_t capacity);
};

template <unsigned int _inline_size> class InlineHeap : public Heap {
  private:
    char _inline_buffer[_inline_size] = {};

  public:
    InlineHeap() : Heap(_inline_buffer, _inline_size, 0) {}
};

} // namespace util

CF_ASSUME_NONNULL_END
