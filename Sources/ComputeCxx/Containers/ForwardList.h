#pragma once

#include <CoreFoundation/CFBase.h>

#include "Util/Heap.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

/// ForwardList is a linked list container that uses util::Heap to allocate nodes,
/// reusing previously removed nodes where possible.
template <typename T> class ForwardList {
  public:
    using reference = T &;
    using const_reference = const T &;

  private:
    struct Node {
        Node *_Nullable next;
        T value;
    };

    util::Heap *_heap;
    Node *_Nullable _front;
    Node *_Nullable _spare;
    bool _is_heap_owner;

  public:
    ForwardList() {
        _heap = new util::Heap(nullptr, 0, util::Heap::minimum_increment);
        _is_heap_owner = true;
    };
    ForwardList(util::Heap *heap) {
        _heap = heap;
        _is_heap_owner = false;
    };
    ~ForwardList() {
        if (_is_heap_owner && _heap) {
            delete _heap;
        }
    };

    // MARK: Element access

    reference front() {
        assert(!empty());
        return _front->value;
    }

    const_reference front() const {
        assert(!empty());
        return _front->value;
    }

    // MARK: Capacity

    bool empty() const noexcept { return _front == nullptr; }

    // MARK: Modifiers

    void push_front(const T &value) {
        Node *new_node;
        if (_spare != nullptr) {
            new_node = _spare;
            _spare = _spare->previous;
        } else {
            new_node = _heap->alloc<Node>();
        }
        new_node->next = _front;
        new_node->value = value;
        _front = new_node;
    }

    void push_front(T &&value) {
        Node *new_node;
        if (_spare != nullptr) {
            new_node = _spare;
            _spare = _spare->previous;
        } else {
            new_node = _heap->alloc<Node>();
        }
        new_node->next = _front;
        new_node->value = std::move(value);
        _front = new_node;
    }

    template <class... Args> void emplace_front(Args &&...args) {
        Node *new_node;
        if (_spare != nullptr) {
            new_node = _spare;
            _spare = _spare->next;
        } else {
            new_node = _heap->alloc<Node>();
        }
        new_node->next = _front;
        new (&new_node->value) T(args...);
        _front = new_node;
    }

    void pop_front() {
        if (_front == nullptr) {
            return;
        }

        Node *next = _front->next;
        T value = _front->value;

        _front->next = _spare;
        _spare = _front;

        _front = next;
    }
};

} // namespace AG

CF_ASSUME_NONNULL_END
