#pragma once

#include <CoreFoundation/CFBase.h>

#include "Graph.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Graph::UpdateStack {
  public:
    struct Frame {
        data::ptr<Node> attribute;
        uint32_t flags; // first four bits are base
    };
    enum Option : uint8_t {
        WasNotDeferringInvalidation = 1 << 4,
    };

  private:
    Graph *_graph;
    pthread_t _thread;
    TaggedPointer<UpdateStack> _previous;
    pthread_t _previous_thread;
    vector<Frame, 8, uint64_t> _frames;
    uint8_t _options;

  public:
    UpdateStack(Graph *graph, uint8_t options);
    ~UpdateStack();

    Graph *graph() { return _graph; };
    TaggedPointer<UpdateStack> previous() { return _previous; };

    vector<Frame, 8, uint64_t> &frames() { return _frames; };

    Frame *global_top();

    Graph::UpdateStatus update();

    bool push(data::ptr<Node> attribute, Node &node, bool flag1, bool flag2);
    bool push_slow(data::ptr<Node> attribute, Node &node, bool flag1, bool flag2);
};

} // namespace AG

CF_ASSUME_NONNULL_END
