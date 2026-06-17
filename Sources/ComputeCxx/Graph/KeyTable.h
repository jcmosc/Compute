#pragma once

#include <Utilities/HashTable.h>

#include "ComputeCxx/IAGBase.h"
#include "Graph.h"

IAG_ASSUME_NONNULL_BEGIN

namespace IAG {

class Graph::KeyTable {
private:
    vector<const char *, 0ul, uint32_t> _keys;
    util::Table<const char *, uint32_t> _table;
    
public:
    KeyTable(util::Heap *_Nullable heap);
    
    uint32_t size();
    
    uint32_t lookup(const char *key, const char *_Nullable *_Nullable found_key) const;
    const char *get(uint32_t key_id);
    
    uint32_t insert(const char *key);
};

} // namespace IAG

IAG_ASSUME_NONNULL_END
