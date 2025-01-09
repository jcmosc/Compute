#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

CF_ASSUME_NONNULL_BEGIN

namespace util {

class Heap;

uint64_t string_hash(char const *str);

class UntypedTable {
    using key_type = void *_Nonnull;
    using nullable_key_type = void *_Nullable;
    using value_type = void *_Nonnull;
    using nullable_value_type = void *_Nullable;
    using size_type = uint64_t;
    using hasher = uint64_t (*)(void const *);
    using key_equal = bool (*)(void const *, void const *);
    using key_callback_t = void (*)(const key_type);
    using value_callback_t = void (*)(const value_type);
    using entry_callback_t = void (*)(const key_type, const value_type, const void *context);

    struct HashNode {
        HashNode *next;
        key_type key;
        value_type value;
        uint64_t hash_value;
    };
    using Bucket = HashNode *_Nonnull;

    hasher _hash;
    key_equal _compare;
    key_callback_t _did_remove_key;
    value_callback_t _did_remove_value;
    Heap *_heap;
    HashNode *_spare_node;
    Bucket *_Nonnull _buckets;
    uint64_t _count;
    uint64_t _bucket_mask;
    uint32_t _bucket_mask_width;
    bool _is_heap_owner;
    bool _compare_by_pointer;

    // Managing buckets
    void create_buckets();
    void grow_buckets();

  public:
    UntypedTable();
    UntypedTable(hasher custom_hasher, key_equal custom_compare, key_callback_t did_remove_key,
                 value_callback_t did_remove_value, Heap *_Nullable heap);
    ~UntypedTable();

    // Lookup
    bool empty() const noexcept;
    size_type count() const noexcept;
    nullable_value_type lookup(key_type key, nullable_key_type *_Nullable found_key_out);
    void for_each(entry_callback_t body, const void *context);

    // Modifiers
    bool insert(key_type key, value_type value);
    bool remove(key_type key);
    bool remove_ptr(key_type key);
};

} // namespace util

CF_ASSUME_NONNULL_END
