#pragma once

#include <CoreFoundation/CFBase.h>

#include "Graph.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Graph::UpdateStack {
  public:
    enum Option : uint8_t {
        WasNotDeferringInvalidation = 1 << 4,
    };

  private:
    struct Frame {
        data::ptr<Node> attribute;
        unsigned int needs_update : 1;
        unsigned int cyclic : 1;
        unsigned int flag3 : 1;
        unsigned int cancelled : 1;
        unsigned int num_pushed_inputs : 28;
    };

    Graph *_graph;
    pthread_t _thread;
    TaggedPointer<UpdateStack> _previous;
    pthread_t _previous_thread;
    vector<Frame, 8, uint64_t> _frames;
    uint8_t _options;

    bool push_slow(data::ptr<Node> attribute, Node &node, bool ignore_cycles, bool treat_no_value_as_pending);

  public:
    UpdateStack(Graph *graph, uint8_t options);
    ~UpdateStack();

    Graph *graph() { return _graph; };
    TaggedPointer<UpdateStack> previous() { return _previous; };

    vector<Frame, 8, uint64_t> &frames() { return _frames; };

    Frame *global_top();

    static void cancel();
    bool cancelled();

    bool push(data::ptr<Node> attribute, Node &node, bool ignore_cycles, bool treat_no_value_as_pending);

    Graph::UpdateStatus update();
};

} // namespace AG

CF_ASSUME_NONNULL_END
