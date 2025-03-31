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
        uint64_t start_time;
        uint64_t child_update_stack_duration;
        uint64_t update_node_start_time;
    };

    std::unordered_map<const Graph::UpdateStack *, UpdateData> _map;

  public:
    void begin_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node, uint32_t options);
    void end_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node, Graph::UpdateStatus update_status);
    void begin_update(data::ptr<Node> node);
    void end_update(data::ptr<Node> node, bool changed);
};

} // namespace AG

CF_ASSUME_NONNULL_END
