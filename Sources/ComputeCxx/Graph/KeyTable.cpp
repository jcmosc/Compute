#include "KeyTable.h"

namespace AG {

Graph::KeyTable::KeyTable(util::Heap *_Nullable heap)
    : _table([](const void *v) { return (uint64_t)util::string_hash(reinterpret_cast<const char *>(v)); },
             [](const void *a, const void *b) {
                 return std::strcmp(reinterpret_cast<const char *>(a), reinterpret_cast<const char *>(b)) == 0;
             },
             nullptr, nullptr, heap){};

uint32_t Graph::KeyTable::lookup(const char *key, const char *_Nullable *_Nullable found_key) const {
    auto result = _table.lookup(key, reinterpret_cast<util::UntypedTable::nullable_key_type *>(found_key));
    return (uint32_t)(uintptr_t)result;
}

uint32_t Graph::KeyTable::size() {
    return _keys.size();
}

uint32_t Graph::KeyTable::insert(const char *key) {
    const char *duplicate = strdup(key);
    if (!duplicate) {
        precondition_failure("memory allocation failure");
    }
    uint32_t key_id = _keys.size();
    _keys.push_back(duplicate);
    _table.insert(duplicate, (void *)(uintptr_t)key_id);
    return key_id;
}

const char *Graph::KeyTable::get(uint32_t key_id) {
    return _keys[key_id];
}

}; // namespace AG
