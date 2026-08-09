#pragma once

#include "Trace/Trace.h"

IAG_ASSUME_NONNULL_BEGIN

namespace IAG {

class Graph::ProfileTrace : public Trace {
  private:
    struct UpdateData {
        uint64_t stack_start_time;
        uint64_t child_stack_duration;
        uint64_t update_start_time;
    };

    std::unordered_map<const Graph::UpdateStack *, UpdateData> _update_data;

  public:
    void begin_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node, uint32_t options) override;
    void end_update(const Graph::UpdateStack &update_stack, data::ptr<Node> node,
                    IAGGraphUpdateStatus update_status) override;
    void begin_update(data::ptr<Node> node) override;
    void end_update(data::ptr<Node> node, bool changed) override;
};

} // namespace IAG

IAG_ASSUME_NONNULL_END
