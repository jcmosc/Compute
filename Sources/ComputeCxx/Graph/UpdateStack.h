#pragma once

#include <CoreFoundation/CFBase.h>

#include "Graph.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Graph::UpdateStack {
  public:
    struct Frame {
        data::ptr<Node> attribute;
        uint32_t flags;
    };
    enum Option : uint8_t {
        WasDeferringInvalidation = 1 << 4,
    };

  private:
    Graph *_graph;
    pthread_t _thread;
    TaggedPointer<UpdateStack> _previous;
    pthread_t _previous_thread;
    vector<Frame, 8, uint64_t> _frames;
    uint8_t _options;

  public:
    UpdateStack(Graph *graph, pthread_t thread, TaggedPointer<UpdateStack> previous, pthread_t previous_thread);

    pthread_t thread() { return _thread; };

    uint8_t options() { return _options; };
    void set_options(uint8_t options) { _options = options; };

    void set_was_deferring_invalidation(bool value) {
        _options = (_options & ~Option::WasDeferringInvalidation) | (value ? Option::WasDeferringInvalidation : 0);
    };
    bool was_deferring_invalidation() { return _options & Option::WasDeferringInvalidation; };

    TaggedPointer<UpdateStack> previous() { return _previous; };
    pthread_t previous_thread() { return _previous_thread; };

    vector<Frame, 8, uint64_t> &frames() { return _frames; };

    Frame *global_top();

    Graph::UpdateStatus update();

    bool push(data::ptr<Node> attribute, Node &node, bool flag1, bool flag2);
    bool push_slow(data::ptr<Node> attribute, Node &node, bool flag1, bool flag2);
};

} // namespace AG

CF_ASSUME_NONNULL_END
