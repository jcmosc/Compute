#pragma once

#include <CoreFoundation/CFBase.h>

#include "Graph.h"
#include "Utilities/HashTable.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Graph::KeyTable {
  private:
    vector<const char *, 0ul, uint32_t> _keys;
    util::UntypedTable _table;

  public:
    KeyTable(util::Heap *_Nullable heap);

    uint32_t size();

    uint32_t lookup(const char *key, const char *_Nullable *_Nullable found_key) const;
    const char *get(uint32_t key_id);

    uint32_t insert(const char *key);
};

} // namespace AG

CF_ASSUME_NONNULL_END
