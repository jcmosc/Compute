#pragma once

#include <CoreFoundation/CFBase.h>
#include <memory>
#include <swift/bridging>

CF_ASSUME_NONNULL_BEGIN

namespace util {

class Heap {
  protected:
    typedef struct Node {
        struct Node *next;
        void *buffer;
    } Node;

    size_t _increment;
    Node *_node;
    char *_free_start;
    size_t _capacity;

    void *alloc_(size_t arg1);

  public:
    static constexpr size_t minimum_increment = 0x400;

    static std::shared_ptr<Heap> make_shared(char *_Nullable start, size_t capacity, size_t increment);

    Heap(char *_Nullable start, size_t capacity, size_t increment);
    ~Heap();

    // non-copyable
    Heap(const Heap &) = delete;
    Heap &operator=(const Heap &) = delete;

    // non-movable
    Heap(Heap &&) = delete;
    Heap &operator=(Heap &&) = delete;

    template <typename T> inline T *_Nonnull alloc(size_t count = 1) {
        return static_cast<T *>(alloc_(sizeof(T) * count));
    };
    void reset(char *_Nullable start, size_t capacity);

    // Debugging

    size_t num_nodes() const;
    size_t increment() const { return _increment; }
    size_t capacity() const { return _capacity; }

    void print() const;

#ifdef SWIFT_TESTING
    uint64_t *alloc_uint64_t(size_t count = 1) { return alloc<uint64_t>(count); }
#endif

} SWIFT_UNSAFE_REFERENCE;

template <unsigned int _inline_size> class InlineHeap : public Heap {
  private:
    char _inline_buffer[_inline_size] = {};

  public:
    InlineHeap() : Heap(_inline_buffer, _inline_size, 0) {}
};

} // namespace util

CF_ASSUME_NONNULL_END
