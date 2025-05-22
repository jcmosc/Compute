#include "KeyTable.h"

namespace AG {

Graph::KeyTable::KeyTable(util::Heap *_Nullable heap)
    : _table(
          util::string_hash, [](const char *a, const char *b) { return std::strcmp(a, b) == 0; },
          [](const char *key) { free((void *)key); }, nullptr, heap) {};

uint32_t Graph::KeyTable::lookup(const char *key, const char *_Nullable *_Nullable found_key) const {
    return _table.lookup(key, found_key);
}

uint32_t Graph::KeyTable::size() { return _keys.size(); }

uint32_t Graph::KeyTable::insert(const char *key) {
    const char *duplicate = strdup(key);
    if (!duplicate) {
        precondition_failure("memory allocation failure");
    }
    uint32_t key_id = _keys.size();
    _keys.push_back(duplicate);
    _table.insert(duplicate, key_id);
    return key_id;
}

const char *Graph::KeyTable::get(uint32_t key_id) { return _keys[key_id]; }

}; // namespace AG
