#pragma once

#include <CoreFoundation/CFBase.h>

#include "Graph.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Graph::UpdateStack {
  private:
    struct Frame {
        data::ptr<Node> attribute;
        unsigned int pending : 1;
        unsigned int cancelled : 1;
        unsigned int next_input_index : 30;
    };

    Graph *_graph;
    pthread_t _thread;
    util::tagged_ptr<UpdateStack> _next;
    pthread_t _next_thread;
    vector<Frame, 8, uint64_t> _frames;
    AGGraphUpdateOptions _options;

  public:
    UpdateStack(Graph *graph, AGGraphUpdateOptions options);
    ~UpdateStack();
    
    // Non-copyable
    UpdateStack(const UpdateStack &) = delete;
    UpdateStack &operator=(const UpdateStack &) = delete;
    
    // Non-movable
    UpdateStack(UpdateStack &&) = delete;
    UpdateStack &operator=(UpdateStack &&) = delete;

    Graph *graph() const { return _graph; };
    const util::tagged_ptr<UpdateStack> next() const { return _next; };
    vector<Frame, 8, uint64_t> &frames() { return _frames; };

    static void cancel();
    bool cancelled();
};

} // namespace AG

CF_ASSUME_NONNULL_END
