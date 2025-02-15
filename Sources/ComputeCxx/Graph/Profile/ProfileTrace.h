#pragma once

#include <CoreFoundation/CFBase.h>
#include <execinfo.h>

#include "Encoder/Encoder.h"
#include "Trace/Trace.h"
#include "Utilities/HashTable.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Graph::ProfileTrace : public Trace {
  private:
    struct UpdateData {
        int x;
    };

    std::unordered_map<Graph::UpdateStack *, UpdateData> _map;

  public:
    ~ProfileTrace();

    void begin_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node, uint32_t options);
    void end_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node, Graph::UpdateStatus update_status);
    void begin_update(data::ptr<Node> node);
    void end_update(data::ptr<Node> node, bool changed);
};

} // namespace AG

CF_ASSUME_NONNULL_END
