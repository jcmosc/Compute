#pragma once

#include <CoreFoundation/CFBase.h>
#include <stdint.h>

CF_ASSUME_NONNULL_BEGIN

namespace util {

class Heap;

uint64_t string_hash(char const *str);

class UntypedTable {
  public:
    using key_type = const void *_Nonnull;
    using nullable_key_type = const void *_Nullable;
    using value_type = const void *_Nonnull;
    using nullable_value_type = const void *_Nullable;
    using size_type = uint64_t;
    using hasher = uint64_t (*)(void const *);
    using key_equal = bool (*)(void const *, void const *);
    using key_callback = void (*)(const key_type);
    using value_callback = void (*)(const value_type);
    using entry_callback = void (*)(const key_type, const value_type, const void *context);

  private:
    struct HashNode {
        HashNode *next;
        key_type key;
        value_type value;
        uint64_t hash_value;
    };
    using Bucket = HashNode *_Nonnull;

    hasher _hash;
    key_equal _compare;
    key_callback _did_remove_key;
    value_callback _did_remove_value;
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
    UntypedTable(hasher custom_hasher, key_equal custom_compare, key_callback _Nullable did_remove_key,
                 value_callback _Nullable did_remove_value, Heap *_Nullable heap);
    ~UntypedTable();

    // Lookup
    bool empty() const noexcept;
    size_type count() const noexcept;
    nullable_value_type lookup(key_type key, nullable_key_type *_Nullable found_key) const noexcept;
    void for_each(entry_callback body, const void *context) const;

    // Modifiers
    bool insert(const key_type key, const value_type value);
    bool remove(const key_type key);
    bool remove_ptr(const key_type key);
};

template <typename Key, typename Value> class Table : UntypedTable {

  public:
    using key_type = Key;
    using value_type = Value;
    using hasher = uint64_t (*)(const key_type);
    using key_equal = bool (*)(const key_type, const key_type);
    using key_callback = void (*)(const key_type);
    using value_callback = void (*)(const value_type);
    using entry_callback = void (*)(const key_type, const value_type, const void *context);

    Table() : UntypedTable(){};
    Table(hasher _Nullable custom_hasher, key_equal _Nullable custom_compare, key_callback _Nullable did_remove_key,
          value_callback _Nullable did_remove_value, Heap *_Nullable heap)
        : UntypedTable(reinterpret_cast<UntypedTable::hasher>(custom_hasher),
                       reinterpret_cast<UntypedTable::key_equal>(custom_compare),
                       reinterpret_cast<UntypedTable::key_callback>(did_remove_key),
                       reinterpret_cast<UntypedTable::value_callback>(did_remove_value), heap){};

    // Lookup

    value_type _Nullable lookup(const key_type key, key_type *_Nullable found_key) const noexcept {
        auto result =
            UntypedTable::lookup(key, reinterpret_cast<UntypedTable::nullable_key_type *_Nullable>(found_key));
        return reinterpret_cast<value_type _Nullable>(result);
    };

    void for_each(entry_callback _Nonnull body, void const *context) const { UntypedTable::for_each(body, context); };

    // Modifying entries

    bool insert(const key_type key, const value_type value) { return UntypedTable::insert(key, value); };
    bool remove(const key_type key) { return UntypedTable::remove(key); };
    bool remove_ptr(const key_type key) { return UntypedTable::remove_ptr(key); };
};

} // namespace util

CF_ASSUME_NONNULL_END
