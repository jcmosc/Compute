#include "Subgraph.h"

#include <ranges>

#include "AGSubgraph-Private.h"
#include "Attribute/AttributeData/Node/IndirectNode.h"
#include "Attribute/AttributeData/Node/Node.h"
#include "Attribute/AttributeID/OffsetAttributeID.h"
#include "Attribute/AttributeView/AttributeView.h"
#include "Graph/Context.h"
#include "Trace/Trace.h"

namespace AG {

Subgraph::Subgraph(SubgraphObject *object, Graph::Context &context, AttributeID attribute) {
    _object = object;

    Graph *graph = &context.graph();
    _graph = graph;
    _context_id = context.id();

    graph->add_subgraph(*this);

    if (AGSubgraphShouldRecordTree()) {
        if (!attribute || attribute.is_nil()) {
            // TODO: get currently updating attribute
        }
        begin_tree(attribute, nullptr, 0);
    }

    context.graph().foreach_trace([this](Trace &trace) { trace.created(*this); });
}

Subgraph::~Subgraph() {
    if (auto observers_ptr = _observers.get()) {
        auto observers = *observers_ptr;

        notify_observers();
        delete observers;
    }
}

#pragma mark - CFType

Subgraph *Subgraph::from_cf(AGSubgraphStorage *storage) { return storage->subgraph; }

AGSubgraphStorage *Subgraph::to_cf() const { return reinterpret_cast<AGSubgraphStorage *>(_object); }

void Subgraph::clear_object() {
    auto object = _object;
    if (object) {
        object->clear_subgraph();
        _object = nullptr;

        if (current_subgraph() == this) {
            set_current_subgraph(nullptr);
            CFRelease(object);
        }
    }
}

#pragma mark - Current subgraph

pthread_key_t Subgraph::_current_subgraph_key;

void Subgraph::make_current_subgraph_key() { pthread_key_create(&_current_subgraph_key, 0); }

Subgraph *Subgraph::current_subgraph() { return (Subgraph *)pthread_getspecific(_current_subgraph_key); }

void Subgraph::set_current_subgraph(Subgraph *subgraph) { pthread_setspecific(_current_subgraph_key, subgraph); }

#pragma mark - Observers

uint64_t Subgraph::add_observer(ClosureFunctionVV<void> callback) {
    if (!_observers) {
        _observers =
            alloc_bytes(sizeof(vector<Observer, 0, uint64_t> *), 7).unsafe_cast<vector<Observer, 0, uint64_t> *>();
        *_observers = new vector<Observer, 0, uint64_t>();
        ;
    }

    auto observer_id = AGMakeUniqueID();
    (*_observers)->push_back(Observer(callback, observer_id));
    return observer_id;
}

void Subgraph::remove_observer(uint64_t observer_id) {
    if (auto observers_ptr = _observers.get()) {
        auto observers = *observers_ptr;
        auto iter = std::remove_if(observers->begin(), observers->end(), [&observer_id](auto observer) -> bool {
            if (observer.observer_id == observer_id) {
                return true;
            }
            return false;
        });
        observers->erase(iter);
    }
}

void Subgraph::notify_observers() {
    if (auto observers_ptr = _observers.get()) {
        auto observers = *observers_ptr;
        while (!observers->empty()) {
            observers->back().callback();
            observers->pop_back();
        }
    }
}

#pragma mark - Graph

void Subgraph::invalidate_and_delete_(bool delete_zone_data) {
    if (delete_zone_data) {
        mark_deleted();
    }

    if (is_invalidating()) {
        return;
    }

    for (auto parent : _parents) {
        parent->remove_child(*this, true);
    }
    _parents.clear();

    // Store the graph locally since the `this` pointer may be deleted
    Graph &graph = *_graph;
    if (!graph.is_deferring_subgraph_invalidation() && !graph.has_main_handler()) {
        invalidate_now(graph);
        graph.invalidate_subgraphs();
    } else {
        invalidate_deferred(graph);
    }
}

void Subgraph::invalidate_deferred(Graph &graph) {
    auto old_invalidation_state = _invalidation_state;
    if (old_invalidation_state != InvalidationState::Deferred) {
        graph.defer_subgraph_invalidation(*this);
        _invalidation_state = InvalidationState::Deferred;
        if (old_invalidation_state == InvalidationState::None) {
            graph.foreach_trace([this](Trace &trace) { trace.invalidate(*this); });
        }
    }
}

void Subgraph::invalidate_now(Graph &graph) {
    graph.will_invalidate_subgraph();

    auto removed_subgraphs = vector<Subgraph *, 16, uint64_t>();
    auto invalidating_subgraphs = std::stack<Subgraph *, vector<Subgraph *, 16, uint64_t>>();

    auto old_invalidation_state = _invalidation_state;
    if (_invalidation_state != InvalidationState::Completed) {
        _invalidation_state = InvalidationState::Completed;
        if (old_invalidation_state == InvalidationState::None) {
            graph.foreach_trace([this](Trace &trace) { trace.invalidate(*this); });
        }
        clear_object();
        invalidating_subgraphs.push(this);

        while (!invalidating_subgraphs.empty()) {
            Subgraph *subgraph = invalidating_subgraphs.top();
            invalidating_subgraphs.pop();

            graph.foreach_trace([subgraph](Trace &trace) { trace.destroy(*subgraph); });

            notify_observers();
            graph.remove_subgraph(*subgraph);

            subgraph->mark_deleted();
            removed_subgraphs.push_back(subgraph);

            // Invalidate all children that belong to the same context
            // Update parent and child vectors of subgraphs that aren't being invalidated
            for (auto child : subgraph->_children) {
                if (child.subgraph()->context_id() == _context_id) {
                    // for each other parent of the child, remove the child from that parent
                    for (auto other_parent : child.subgraph()->_parents) {
                        if (other_parent == subgraph) {
                            continue;
                        }

                        auto iter = std::remove_if(other_parent->_children.begin(), other_parent->_children.end(),
                                                   [&child](auto other_parent_child) -> bool {
                                                       return other_parent_child.subgraph() == child.subgraph();
                                                   });
                        other_parent->_children.erase(iter);
                    }

                    child.subgraph()->_parents.clear();

                    // Invalidate this child
                    auto old_invalidation_state = child.subgraph()->_invalidation_state;
                    if (child.subgraph()->_invalidation_state != InvalidationState::Completed) { // TODO: check
                        child.subgraph()->_invalidation_state = InvalidationState::Completed;
                        if (old_invalidation_state == InvalidationState::None) {
                            graph.foreach_trace([&child](Trace &trace) { trace.invalidate(*child.subgraph()); });
                        }
                        child.subgraph()->clear_object();
                        invalidating_subgraphs.push(child.subgraph());
                    }
                } else {
                    // don't invalidate this child but remove this subgraph from its parents vector
                    auto iter =
                        std::remove(child.subgraph()->_parents.begin(), child.subgraph()->_parents.end(), subgraph);
                    child.subgraph()->_parents.erase(iter);
                }
            }
        }
    }

    // TODO: remove nodes
    // TODO: destroy nodes

    for (Subgraph *removed_subgraph : removed_subgraphs) {
        delete removed_subgraph;
    }

    graph.did_invalidate_subgraph();
}

void Subgraph::graph_destroyed() {
    auto old_invalidation_state = _invalidation_state;
    _invalidation_state = InvalidationState::GraphDestroyed;

    if (old_invalidation_state == InvalidationState::None) {
        graph()->foreach_trace([this](Trace &trace) { trace.invalidate(*this); });
    }

    notify_observers();

    // TODO: destroy nodes

    _parents.clear();
    _children.clear();

    clear();
}

#pragma mark - Children

void Subgraph::add_child(Subgraph &child, uint8_t tag) {
    if (child.graph() != graph()) {
        precondition_failure("child subgraph must have same graph");
    }
    for (auto parent : child._parents) {
        if (parent == this) {
            precondition_failure("child already attached to new parent");
        }
    }
    graph()->foreach_trace([this, &child](Trace &trace) { trace.add_child(*this, child); });
    _children.push_back(SubgraphChild(&child, tag));

    AGAttributeFlags descendent_flags = child._flags | child._descendent_flags;
    if (descendent_flags & ~_descendent_flags) {
        _descendent_flags |= descendent_flags;
        propagate_flags();
    }

    AGAttributeFlags descendent_dirty_flags = child._dirty_flags | child._descendent_dirty_flags;
    if (descendent_dirty_flags & ~_descendent_dirty_flags) {
        _descendent_dirty_flags |= descendent_dirty_flags;
        propagate_dirty_flags();
    }

    child._parents.push_back(this);
}

void Subgraph::remove_child(Subgraph &child, bool suppress_trace) {
    auto parent_iter = std::remove(child._parents.begin(), child._parents.end(), this);
    child._parents.erase(parent_iter);

    if (!suppress_trace) {
        graph()->foreach_trace([this, &child](Trace &trace) { trace.remove_child(*this, child); });
    }

    auto child_iter = std::remove_if(_children.begin(), _children.end(), [&child](auto subgraph_child) -> bool {
        return subgraph_child.subgraph() == &child;
    });
    _children.erase(child_iter);
}

bool Subgraph::ancestor_of(const Subgraph &other) {
    auto untraversed_parents = std::stack<const Subgraph *, vector<const Subgraph *, 32, uint64_t>>();
    const Subgraph *candidate = &other;
    while (true) {
        if (candidate == nullptr) {
            // previous candidate was a top-level subgraph
            if (untraversed_parents.empty()) {
                return false;
            }
            candidate = untraversed_parents.top();
            untraversed_parents.pop();
        }

        if (candidate == this) {
            return true;
        }

        // partition parents into first and remaining
        candidate = candidate->_parents.empty() ? nullptr : candidate->_parents.front();
        for (Subgraph *parent : std::ranges::drop_view(other._parents, 1)) {
            untraversed_parents.push(parent);
        }
    }
}

#pragma mark - Flags

void Subgraph::set_flags(data::ptr<Node> node, AGAttributeFlags flags) {
    if (node->subgraph_flags() == flags) {
        return;
    }
    if (node->subgraph_flags() == AGAttributeFlagsDefault || flags == AGAttributeFlagsDefault) {
        // unlink and reinsert to trigger a reorder
        unlink_attribute(AttributeID(node));
        node->set_subgraph_flags(flags);
        insert_attribute(AttributeID(node), true);
    } else {
        node->set_subgraph_flags(flags);
    }

    add_flags(flags);
    if (node->is_dirty()) {
        add_dirty_flags(flags);
    }
}

void Subgraph::add_flags(AGAttributeFlags flags) {
    if (!(flags & ~_flags)) {
        return;
    }

    _flags |= flags;
    propagate_flags();
}

void Subgraph::propagate_flags() {
    uint8_t flags = _flags | _descendent_flags;
    foreach_ancestor([&flags](Subgraph &ancestor) -> bool {
        if (!(flags & ~ancestor._descendent_flags)) {
            return false;
        }

        ancestor._descendent_flags |= flags;
        return true;
    });
}

void Subgraph::add_dirty_flags(AGAttributeFlags dirty_flags) {
    if (!(dirty_flags & ~_dirty_flags)) {
        return;
    }

    _dirty_flags |= dirty_flags;
    propagate_dirty_flags();
}

void Subgraph::propagate_dirty_flags() {
    uint8_t dirty_flags = _dirty_flags | _descendent_dirty_flags;
    foreach_ancestor([&dirty_flags](Subgraph &ancestor) -> bool {
        if (!(dirty_flags & ~ancestor._descendent_dirty_flags)) {
            return false;
        }

        ancestor._descendent_dirty_flags |= dirty_flags;
        return true;
    });
}

// MARK: Attributes

void Subgraph::insert_attribute(AttributeID attribute, bool updatable) {
    AttributeID previous_attribute = AttributeID(AGAttributeNil);

    // sort attributes with flags before indirect attributes or attributes without flags
    // the attribute will only be non-updatable if it is a non-mutable indirect node that does not traverse subgraphs
    if (updatable && !attribute.has_subgraph_flags()) {
        for (auto other : attribute_view(attribute.page_ptr())) {
            if (!other.has_subgraph_flags()) {
                break;
            }
            previous_attribute = other;
        }
    }

    RelativeAttributeID next_attribute;
    if (auto previous_node = previous_attribute.get_node()) {
        next_attribute = previous_node->next_attribute();
        previous_node->set_next_attribute(RelativeAttributeID(attribute));
    } else if (auto previous_indirect_node = previous_attribute.get_indirect_node()) {
        next_attribute = previous_indirect_node->next_attribute();
        previous_indirect_node->set_next_attribute(RelativeAttributeID(attribute));
    } else {
        if (updatable) {
            next_attribute = RelativeAttributeID(attribute.page_ptr()->bytes_list);
            attribute.page_ptr()->bytes_list = RelativeAttributeID(attribute);
        } else {
            next_attribute = RelativeAttributeID(attribute.page_ptr()->const_bytes_list);
            attribute.page_ptr()->const_bytes_list = RelativeAttributeID(attribute);
        }
    }

    if (auto node = attribute.get_node()) {
        node->set_next_attribute(next_attribute);
    } else if (auto indirect_node = attribute.get_indirect_node()) {
        indirect_node->set_next_attribute(next_attribute);
    }
}

void Subgraph::unlink_attribute(AttributeID attribute) {
    AttributeID previous_attribute = AttributeID(nullptr);
    for (auto other : attribute_view(attribute.page_ptr())) {
        if (!other || other.is_nil()) {
            break;
        }
        if (other == attribute) {
            break;
        }
        previous_attribute = other;
    }

    RelativeAttributeID next_attribute = RelativeAttributeID(nullptr);
    if (auto node = attribute.get_node()) {
        next_attribute = node->next_attribute();
        node->set_next_attribute(RelativeAttributeID(nullptr));
    } else if (auto indirect_node = attribute.get_indirect_node()) {
        next_attribute = indirect_node->next_attribute();
        indirect_node->set_next_attribute(RelativeAttributeID(nullptr));
    }

    if (auto previous_node = previous_attribute.get_node()) {
        previous_node->set_next_attribute(next_attribute);
    } else if (auto previous_indirect_node = previous_attribute.get_indirect_node()) {
        previous_indirect_node->set_next_attribute(next_attribute);
    } else {
        attribute.page_ptr()->bytes_list = next_attribute;
    }
}

void Subgraph::add_node(data::ptr<Node> node) {
    node->set_subgraph_flags(AGAttributeFlagsDefault);
    insert_attribute(AttributeID(node), true);

    //    if (_tree_root) {
    //        graph()->add_tree_data_for_subgraph(this, _tree_root, node);
    //    }

    graph()->foreach_trace([&node](Trace &trace) { trace.added(node); });
}

void Subgraph::add_indirect(data::ptr<IndirectNode> node, bool flag) {
    insert_attribute(AttributeID(node), flag); // make sure adds Indirect kind to node

    graph()->foreach_trace([&node](Trace &trace) { trace.added(node); });
}

std::atomic<uint32_t> Subgraph::_last_traversal_seed = {};

void Subgraph::apply(uint32_t options, ClosureFunctionAV<void, AGAttribute> body) {
    if (!is_valid()) {
        return;
    }

    AGAttributeFlags flags = options & AGAttributeFlagsMask;
    if (!intersects(flags)) {
        return;
    }

    // Defer subgraph invalidation until the end of this method's scope
    auto without_invalidating = Graph::without_invalidating(graph());
    _last_traversal_seed += 1;

    auto subgraphs = std::stack<Subgraph *, vector<Subgraph *, 32, uint64_t>>();
    subgraphs.push(this);
    _traversal_seed = _last_traversal_seed;

    while (!subgraphs.empty()) {
        auto subgraph = subgraphs.top();
        subgraphs.pop();

        if (!subgraph->is_valid()) {
            continue;
        }

        if ((options >> 0x18) & 0x1 || subgraph->context_id() == _context_id) {
            if (!options || (subgraph->_flags & flags)) {
                for (auto page : subgraph->pages()) {
                    for (auto attribute : attribute_view(page)) {
                        if (!attribute) { // TODO: nil or null
                            break;
                        }
                        if (auto node = attribute.get_node()) {
                            if (options) {
                                if (node->subgraph_flags() == AGAttributeFlagsDefault) {
                                    // we know this attribute is sorted after all nodes with flags
                                    // so we aren't going to match any more attributes after this
                                    break;
                                }
                                if (!(node->subgraph_flags() & flags)) {
                                    continue;
                                }
                            }

                            body(attribute);
                        } else if (attribute.is_indirect_node()) {
                            if (options) {
                                // we know this attribute is sorted after all nodes with flags
                                // so we aren't going to match any more attributes after this
                                break;
                            }
                        }
                    }
                }
            }

            for (auto child : subgraph->children()) {
                if (child.subgraph()->intersects(flags) && child.subgraph()->_traversal_seed != _last_traversal_seed) {

                    subgraphs.push(child.subgraph());
                    child.subgraph()->_traversal_seed = _last_traversal_seed;
                }
            }
        }
    }
}

#pragma mark - Tree

void Subgraph::begin_tree(AttributeID value, const swift::metadata *type, uint32_t flags) {
    data::ptr<Graph::TreeElement> tree = alloc_bytes(sizeof(Graph::TreeElement), 7).unsafe_cast<Graph::TreeElement>();
    tree->type = type;
    tree->value = value;
    tree->flags = flags;
    tree->parent = _tree_root;
    tree->next_sibling = Graph::TreeElementID();

    auto old_root = _tree_root;
    _tree_root = Graph::TreeElementID(tree);

    if (old_root) {
        _tree_root->next_sibling = old_root->first_child;
        old_root->first_child = _tree_root;
    }
}

void Subgraph::end_tree() {
    if (_tree_root && _tree_root->parent) {
        _tree_root = _tree_root->parent;
    }
}

void Subgraph::set_tree_owner(AttributeID owner) {
    if (!_tree_root) {
        return;
    }
    if (_tree_root->parent) {
        precondition_failure("setting owner of non-root tree");
    }
    _tree_root->value = owner;
}

void Subgraph::add_tree_value(AttributeID value, const swift::metadata *type, const char *key, uint32_t flags) {
    if (!_tree_root) {
        return;
    }

    auto key_id = graph()->intern_key(key);

    data::ptr<Graph::TreeValue> tree_value = alloc_bytes(sizeof(Graph::TreeValue), 7).unsafe_cast<Graph::TreeValue>();
    tree_value->type = type;
    tree_value->value = value;
    tree_value->key_id = key_id;
    tree_value->flags = flags;
    tree_value->next = _tree_root->first_value;

    _tree_root->first_value = Graph::TreeValueID(tree_value);
}

AttributeID Subgraph::tree_node_at_index(Graph::TreeElementID tree_element, uint64_t index) {
    if (auto tree_data_element = graph()->tree_data_element_for_subgraph(this)) {
        auto &nodes = tree_data_element->nodes();

        auto found = std::lower_bound(nodes.begin(), nodes.end(), tree_element,
                                      [](auto iter, auto value) { return iter.first < value; });

        // Find the element that is `index` places after the first given tree element,
        // stopping if we reach the next tree element before `index` places.
        uint64_t i = index;
        for (auto iter = found; iter != nodes.end(); ++iter) {
            if (iter->first != tree_element) {
                break;
            }
            if (i == 0) {
                return AttributeID(iter->second);
            }
            --i;
        }
    }
    return AttributeID(AGAttributeNil);
}

Graph::TreeElementID Subgraph::tree_subgraph_child(Graph::TreeElementID tree_element) {
    auto tree_data_element = graph()->tree_data_element_for_subgraph(this);
    if (!tree_data_element) {
        return Graph::TreeElementID(nullptr);
    }

    auto &nodes = tree_data_element->nodes();
    if (nodes.empty()) {
        return Graph::TreeElementID(nullptr);
    }

    auto found = std::lower_bound(nodes.begin(), nodes.end(), tree_element,
                                  [](auto iter, auto value) -> bool { return iter.first < value; });
    if (found == nodes.end()) {
        return;
    }

    auto subgraph_children = vector<Subgraph *, 32, uint64_t>();

    for (auto subgraph : _graph->subgraphs()) {
        if (!subgraph->is_valid()) {
            continue;
        }
        if (subgraph->_tree_root == nullptr) {
            continue;
        }
        AttributeID attribute = subgraph->_tree_root->value;
        if (!attribute || attribute.is_nil()) {
            continue;
        }

        attribute = attribute.resolve(TraversalOptions::None).attribute();
        if (auto node = attribute.get_node()) {
            for (auto iter = found; iter != nodes.end(); ++iter) {
                if (iter->first != tree_element) {
                    break;
                }
                if (iter->second == node) {
                    subgraph_children.push_back(subgraph);
                    break;
                }
            }
        }
    }

    std::sort(subgraph_children.begin(), subgraph_children.end());

    auto first_tree_child = Graph::TreeElementID();

    // Link all tree roots of child subgraphs together as siblings
    auto old_first_tree_child = Graph::TreeElementID();
    for (auto subgraph : subgraph_children) {
        first_tree_child = subgraph->_tree_root;
        first_tree_child->next_sibling = old_first_tree_child;
        old_first_tree_child = first_tree_child;
    }

    return first_tree_child;
}

} // namespace AG
