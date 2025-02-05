#include "Subgraph.h"

#include <stack>

#include "Attribute/AttributeType.h"
#include "Attribute/Node/IndirectNode.h"
#include "Attribute/Node/Node.h"
#include "Attribute/OffsetAttributeID.h"
#include "Encoder/Encoder.h"
#include "Errors/Errors.h"
#include "Graph/Context.h"
#include "Graph/Graph.h"
#include "Graph/Trace.h"
#include "Graph/Tree/TreeElement.h"
#include "NodeCache.h"
#include "UniqueID/AGUniqueID.h"
#include "Util/CFPointer.h"

namespace AG {

pthread_key_t Subgraph::_current_subgraph_key;

void Subgraph::make_current_subgraph_key() { pthread_key_create(&Subgraph::_current_subgraph_key, 0); }

Subgraph *Subgraph::current_subgraph() { return (Subgraph *)pthread_getspecific(Subgraph::_current_subgraph_key); }

void Subgraph::set_current_subgraph(Subgraph *subgraph) {
    pthread_setspecific(Subgraph::_current_subgraph_key, subgraph);
}

Subgraph::Subgraph(SubgraphObject *object, Graph::Context &context, AttributeID owner) {
    _object = object;

    Graph &graph = context.graph();
    _graph = &graph;
    _graph_context_id = context.unique_id();

    _invalidated = false;

    begin_tree(owner, nullptr, 0);

    graph.foreach_trace([*this](Trace &trace) { trace.created(*this); });
}

Subgraph::~Subgraph() {
    if (_observers) {
        notify_observers();
        // delete *_observers
    }
    //    if (_node_cache) {
    //        _node_cache::~NodeCache();
    //    }
}

#pragma mark - CoreFoundation

Subgraph *Subgraph::from_cf(SubgraphObject *object) { return object->subgraph(); }

SubgraphObject *Subgraph::to_cf() { return _object; }

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

#pragma mark - Graph

void Subgraph::graph_destroyed() {
    bool old_invalidated = _invalidated;
    _invalidated = true;

    if (!old_invalidated) {
        graph()->foreach_trace([*this](Trace &trace) { trace.invalidate(*this); });
    }
    notify_observers();

    for (data::ptr<data::page> page = last_page(); page != nullptr; page = page->previous) {
        uint16_t relative_offset = page->relative_offset_1;
        while (relative_offset) {
            AttributeID attribute = AttributeID(page + relative_offset);
            if (attribute.is_nil()) {
                break; // TODO: check if this should break out of entire loop
            }

            if (attribute.is_direct()) {
                relative_offset = attribute.to_node().flags().relative_offset();
            } else if (attribute.is_indirect()) {
                relative_offset = attribute.to_indirect_node().relative_offset();
            }

            if (attribute.is_direct()) {
                attribute.to_node().destroy(*_graph);
            }
        }
    }

    _parents.clear();
    _children.clear();
    clear();
}

#pragma mark - Managing children

void Subgraph::add_child(Subgraph &child, SubgraphChild::Flags flags) {
    if (child.graph() != graph()) {
        precondition_failure("child subgraph must have same graph");
    }
    for (auto parent : child._parents) {
        if (parent == this) {
            precondition_failure("child already attached to new parent");
        }
    }
    graph()->foreach_trace([this, &child](Trace &trace) { trace.add_child(*this, child); });
    _children.push_back(SubgraphChild(&child, flags));

    uint8_t new_flags = child._flags.value1 | child._flags.value2;
    if (new_flags & ~_flags.value2) {
        _flags.value2 |= new_flags; // value2 so can't call add_flags
        propagate_flags();
    }

    uint8_t dirty_flags = child._flags.value3 | child._flags.value4;
    if (dirty_flags & ~_flags.value4) {
        _flags.value4 |= dirty_flags; // value4 so can't call add_dirty_flags
        propagate_dirty_flags();
    }

    child._parents.push_back(*this);
}

void Subgraph::remove_child(Subgraph &child, bool without_trace) {
    auto parents_end = std::remove(child._parents.begin(), child._parents.end(), this);
    child._parents.erase(parents_end, child._parents.end());

    if (!without_trace) {
        graph()->foreach_trace([this, &child](Trace &trace) { trace.remove_child(*this, child); });
    }

    auto children_end = std::remove(_children.begin(), _children.end(), &child);
    _children.erase(children_end, _children.end());
}

bool Subgraph::ancestor_of(const Subgraph &other) {
    auto other_parents = std::stack<const Subgraph *, vector<const Subgraph *, 32, uint64_t>>();
    const Subgraph *candidate = &other;
    while (true) {
        if (candidate == nullptr) {
            // previous candidate was top level
            if (other_parents.empty()) {
                return false;
            }
            candidate = other_parents.top();
            other_parents.pop();
        }

        if (candidate == this) {
            return true;
        }

        candidate = candidate->_parents.empty() ? nullptr : &candidate->_parents.front();
        for (auto iter = other._parents.begin() + 1, end = other._parents.end(); iter != end; ++iter) {
            auto parent = *iter;
            other_parents.push(parent);
        }
    }
}

template <typename Callable>
    requires std::invocable<Callable, Subgraph &> && std::same_as<std::invoke_result_t<Callable, Subgraph &>, bool>
void Subgraph::foreach_ancestor(Callable body) {
    // Status: Verified
    for (auto iter = _parents.rbegin(), end = _parents.rend(); iter != end; ++iter) {
        auto parent = *iter;
        if (body(*parent)) {
            parent->foreach_ancestor(body);
        }
    }
}

#pragma mark - Attributes

void Subgraph::add_node(data::ptr<Node> node) {
    node->flags().set_value3(0);
    insert_attribute(AttributeID(node), true);

    if (_tree_root) {
        auto tree_data_element = graph()->tree_data_element_for_subgraph(this);
        tree_data_element.push_back({
            _tree_root,
            node,
        });
    }

    graph()->foreach_trace([&node](Trace &trace) { trace.added(node); });
}

void Subgraph::add_indirect(data::ptr<IndirectNode> node, bool flag) {
    insert_attribute(AttributeID(node), flag); // make sure adds Indirect kind to node

    graph()->foreach_trace([&node](Trace &trace) { trace.added(node); });
}

void Subgraph::insert_attribute(AttributeID attribute, bool flag) {
    AttributeID source = AttributeID::make_nil();

    if (flag) {
        if (!attribute.is_direct() || attribute.to_node().flags().value3() == 0) {
            uint16_t relative_offset = attribute.page_ptr()->relative_offset_1;
            while (relative_offset) {
                AttributeID next_attribute = AttributeID(attribute.page_ptr().offset() + relative_offset);
                if (next_attribute.is_direct()) {
                    auto node = next_attribute.to_node();
                    if (node.flags().value3() == 0) {
                        break;
                    }
                    source = next_attribute;
                    relative_offset = node.flags().relative_offset();
                    if (relative_offset == 0) {
                        break;
                    }
                }
            }
        }
    }

    // TODO: add RelativeAttributeID and simplify this

    uint16_t next_value = (attribute.without_kind() - attribute.page_ptr()) | attribute.kind();

    uint16_t previous_value = 0;
    if (source.is_direct()) {
        previous_value = source.to_node().flags().relative_offset();
        source.to_node().flags().set_relative_offset(next_value);
    } else if (source.is_indirect()) {
        previous_value = source.to_indirect_node().relative_offset();
        source.to_indirect_node().set_relative_offset(next_value);
    } else {
        if (flag) {
            previous_value = source.page_ptr()->relative_offset_1;
            source.page_ptr()->relative_offset_1 = next_value;
        } else {
            previous_value = source.page_ptr()->relative_offset_2;
            source.page_ptr()->relative_offset_2 = next_value;
        }
    }

    if (attribute.is_direct()) {
        attribute.to_node().flags().set_relative_offset(previous_value);
    } else if (attribute.is_indirect()) {
        attribute.to_indirect_node().set_relative_offset(previous_value);
    }
}

void Subgraph::unlink_attribute(AttributeID attribute) {

    uint16_t relative_offset = attribute.page_ptr()->relative_offset_1;

    // Find the attribute before the given attribute
    // TODO: what happens if attribute is not found
    AttributeID before_attribute = AttributeID::make_nil();
    while (relative_offset) {
        AttributeID next_attribute = AttributeID(attribute.page_ptr().offset() + relative_offset);
        if (next_attribute.is_nil()) {
            break;
        }

        if (next_attribute == attribute) {
            break;
        }

        if (next_attribute.is_direct()) {
            relative_offset = next_attribute.to_node().flags().relative_offset();
            before_attribute = next_attribute;
        } else if (next_attribute.is_indirect()) {
            relative_offset = next_attribute.to_indirect_node().relative_offset();
            before_attribute = next_attribute;
        }
    }

    uint16_t old_value = 0;
    if (attribute.is_direct()) {
        old_value = attribute.to_node().flags().relative_offset();
        attribute.to_node().flags().set_relative_offset(0);
    } else {
        old_value = attribute.to_indirect_node().relative_offset();
        attribute.to_indirect_node().set_relative_offset(0);
    }

    if (before_attribute.is_direct()) {
        before_attribute.to_node().flags().set_relative_offset(old_value);
    } else if (before_attribute.is_indirect()) {
        before_attribute.to_indirect_node().set_relative_offset(old_value);
    } else {
        attribute.page_ptr()->relative_offset_1 = old_value;
    }
}

void Subgraph::invalidate_now(Graph &graph) {
    // TODO: double check graph param vs _graph instance var

    graph.set_deferring_invalidation(true);

    auto removed_subgraphs = vector<Subgraph *, 16, uint64_t>();
    auto stack = std::stack<Subgraph *, vector<Subgraph *, 16, uint64_t>>();

    bool was_invalidated = _invalidated;
    if (!_invalidated) {
        _invalidated = true;
        if (!was_invalidated) {
            _graph->foreach_trace([*this](Trace &trace) { trace.invalidate(*this); });
        }
        clear_object();
        stack.push(this);

        while (!stack.empty()) {
            Subgraph *subgraph = stack.top();
            stack.pop();

            _graph->foreach_trace([subgraph](Trace &trace) { trace.destroy(*subgraph); });

            notify_observers();
            _graph->remove_subgraph(*subgraph);

            subgraph->set_invalidated_flag();
            removed_subgraphs.push_back(subgraph);

            for (auto child : subgraph->_children) {
                Subgraph *child_subgraph = child.subgraph();
                if (child_subgraph->_graph_context_id == _graph_context_id) {

                    // for each other parent of the child, remove the child from that parent
                    for (auto parent : child_subgraph->_parents) {
                        if (parent != subgraph) {
                            auto r = std::remove_if(parent->_children.begin(), parent->_children.end(),
                                                    [child_subgraph](auto other_child) {
                                                        return other_child.subgraph() == child_subgraph;
                                                    });
                            parent->_children.erase(r);

                            //                            for (auto other_child : parent->_children) {
                            //                                if (other_child.subgraph() == child) {
                            //                                    parent->_children[i] =
                            //                                    parent->_children[parent->_children.size() - 1];
                            //                                    parent->_children.pop_back();
                            //                                }
                            //                            }
                        }
                    }

                    child_subgraph->_parents.clear();

                    bool child_was_invalidated = child_subgraph->_invalidated;
                    if (!child_was_invalidated) {
                        child_subgraph->_invalidated = true;
                        if (!child_was_invalidated) {
                            _graph->foreach_trace(
                                [child_subgraph](Trace &trace) { trace.invalidate(*child_subgraph); });
                        }
                        child_subgraph->clear_object();
                        stack.push(child_subgraph);
                    }
                } else {
                    // remove the subgraph from the parents vector of each child
                    auto r = std::remove(child_subgraph->_parents.begin(), child_subgraph->_parents.end(), subgraph);
                    child_subgraph->_parents.erase(r);

                    //                    for (auto parent : child_subgraph->_parents) {
                    //                        if (parent == subgraph) {
                    //                            child._parents[i] = child._parents[child._parents.size() - 1];
                    //                            child._parents.resize(child._parents.size() - 1);
                    //                            break;
                    //                        }
                    //                    }
                }
            }
        }
    }

    for (auto removed_subgraph : removed_subgraphs) {
        for (data::ptr<data::page> page = removed_subgraph->last_page(); page != nullptr; page = page->previous) {
            bool found_nil_attribute = false;

            uint16_t relative_offset = page->relative_offset_1;
            while (relative_offset) {
                AttributeID attribute = AttributeID(page + relative_offset);
                if (attribute.is_direct()) {
                    relative_offset = attribute.to_node().flags().relative_offset();
                    graph.remove_node(attribute.to_node_ptr());
                } else if (attribute.is_indirect()) {
                    relative_offset = attribute.to_indirect_node().relative_offset();
                    graph.remove_indirect_node(attribute.to_indirect_node_ptr());
                } else if (attribute.is_nil()) {
                    found_nil_attribute = true;
                }
            }
            if (found_nil_attribute) {
                break;
            }
        }
    }

    for (auto removed_subgraph : removed_subgraphs) {
        for (data::ptr<data::page> page = removed_subgraph->last_page(); page != nullptr; page = page->previous) {
            bool found_nil_attribute = false;

            uint16_t relative_offset = page->relative_offset_1;
            while (relative_offset) {
                AttributeID attribute = AttributeID(page + relative_offset);

                if (attribute.is_direct()) {
                    relative_offset = attribute.to_node().flags().relative_offset();
                } else if (attribute.is_indirect()) {
                    relative_offset = attribute.to_indirect_node().relative_offset();
                } else if (attribute.is_nil()) {
                    found_nil_attribute = true;
                }

                if (attribute.is_direct()) {
                    attribute.to_node().destroy(*_graph);

                    _graph->did_destroy_node(); // decrement counter
                }
            }
            if (found_nil_attribute) {
                break;
            }
        }
    }

    // TODO: does this execute anyway...
    for (auto removed_subgraph : removed_subgraphs) {
        removed_subgraph->~Subgraph();
        free(removed_subgraph); // or delete?
    }

    graph.set_deferring_invalidation(false);
}

void Subgraph::invalidate_and_delete_(bool flag) {
    if (flag) {
        set_invalidated_flag();
    }

    // comparison is actually (2 < (dword)(this->invalidated - 1), is this a flag?
    if (!_invalidated) {
        for (auto parent : _parents) {
            parent->remove_child(*this, true);
        }
        _parents.clear();

        // Check Graph::invalidate_subgraphs
        if (_graph->deferring_invalidation() == false && _graph->main_handler() == nullptr) {
            invalidate_now(*_graph);
            _graph->invalidate_subgraphs();
            return;
        }

        bool was_invalidated = _invalidated;
        if (!was_invalidated) {
            _graph->will_invalidate_subgraph(*this);
            _invalidated = true;
            if (!was_invalidated) {
                _graph->foreach_trace([*this](Trace &trace) { trace.invalidate(*this); });
            }
        }
    }
}

void Subgraph::update(uint8_t flags) {
    if (_graph->needs_update()) {
        if (!_graph->thread_is_updating()) {
            _graph->call_update();
        }
    }

    if (!_invalidated) {
        if ((flags & (_flags.value1 | _flags.value2))) {

            _graph->foreach_trace([*this, &flags](Trace &trace) { trace.begin_update(*this, flags); });

            _last_traversal_seed += 1; // TODO: check atomics

            auto stack =
                std::stack<util::cf_ptr<SubgraphObject *>, AG::vector<util::cf_ptr<SubgraphObject *>, 32, uint64_t>>();
            auto nodes_to_update = vector<data::ptr<Node>, 256, uint64_t>();

            stack.push(std::move(to_cf()));
            _traversal_seed = _last_traversal_seed;

            bool thread_is_updating = false;
            while (!stack.empty()) {

                util::cf_ptr<SubgraphObject *> object = stack.top();
                stack.pop();

                Subgraph *subgraph = Subgraph::from_cf(object);
                if (subgraph) {

                    while (!subgraph->_invalidated) {
                        if ((flags & subgraph->_flags.value3) == 0) {
                            // LABEL: LAB_1afe6ac70

                            if (flags & subgraph->_flags.value4) {
                                subgraph->_flags.value4 &= ~flags;
                                for (auto child : subgraph->_children) {
                                    Subgraph *child_subgraph = child.subgraph();
                                    // TODO: check child has 0x3 pointer tag that needs to be masked...
                                    if (flags & (child_subgraph->_flags.value3 | child_subgraph->_flags.value4) &&
                                        child_subgraph->_traversal_seed != _traversal_seed) {
                                        stack.push(std::move(child_subgraph->to_cf()));
                                        child_subgraph->_traversal_seed = _traversal_seed;
                                    }
                                }
                            }
                            break;
                        }

                        uint8_t old_flags_1 = subgraph->_flags.value1;
                        subgraph->_flags.value3 &= ~flags;
                        if (flags == 0 || (old_flags_1 & flags)) {
                            for (data::ptr<data::page> page = subgraph->last_page(); page != nullptr;
                                 page = page->previous) {
                                uint16_t relative_offset = page->relative_offset_1;
                                while (relative_offset) {
                                    AttributeID attribute = AttributeID(page + relative_offset);
                                    if (attribute.is_nil()) {
                                        break;
                                    }

                                    if (attribute.is_direct()) {
                                        relative_offset = attribute.to_node().flags().relative_offset();
                                        auto node = attribute.to_node();
                                        if (flags) {
                                            if (node.flags().value3() == 0) {
                                                break;
                                            }
                                            if ((node.flags().value3() & flags) == 0) {
                                                continue;
                                            }
                                        }
                                        if (node.state().is_dirty()) {
                                            nodes_to_update.push_back(attribute.to_node_ptr());
                                        }
                                    } else if (attribute.is_indirect()) {
                                        relative_offset = attribute.to_indirect_node().relative_offset();
                                        if (flags) {
                                            break;
                                        }
                                    }
                                }
                            }
                        }

                        if (nodes_to_update.size() == 0) {
                            if (subgraph->_invalidated == false) {
                                // goto LAB_1afe6ac70
                            }
                            break;
                        }

                        for (auto node : nodes_to_update) {
                            if (!thread_is_updating) {
                                thread_is_updating = _graph->thread_is_updating();
                                if (!thread_is_updating) {
                                    _graph->increment_counter_0x1b8();
                                }
                                _graph->update_attribute(node, true);

                                if (subgraph->_invalidated) {
                                    break;
                                }
                            }
                        }
                        nodes_to_update.clear();
                    }

                    _graph->invalidate_subgraphs();
                }

                // CFRelease happens automatically

            } // while !stack.empty

            _graph->invalidate_subgraphs();
            _graph->foreach_trace([*this](Trace &trace) { trace.end_update(*this); });
        }
    }
}

#pragma mark - Traversal

std::atomic<uint32_t> Subgraph::_last_traversal_seed = {};

void Subgraph::apply(Flags flags, ClosureFunctionAV<void, unsigned int> body) {
    // Status: Verified, needs checks for atomics
    if (_invalidated) {
        return;
    }
    if ((flags.value1 & (_flags.value1 | _flags.value2)) == 0) {
        return;
    }

    // Defer invalidation until the end of this method's scope
    auto without_invalidating = Graph::without_invalidating(graph());

    _last_traversal_seed += 1; // TODO: check atomics

    auto stack = std::stack<Subgraph *, vector<Subgraph *, 32, uint64_t>>();

    stack.push(this);
    _traversal_seed = _last_traversal_seed;

    while (!stack.empty()) {
        auto subgraph = stack.top();
        stack.pop();

        if (subgraph->_invalidated) {
            continue;
        }

        if (flags.value4 & 1 || subgraph->_graph_context_id == _graph_context_id) {
            if (flags.is_null() || (flags.value1 & subgraph->_flags.value1)) {
                for (data::ptr<data::page> page = subgraph->last_page(); page != nullptr; page = page->previous) {
                    uint16_t relative_offset = page->relative_offset_1;
                    while (relative_offset) {
                        AttributeID attribute = AttributeID(page + relative_offset);
                        if (attribute.is_nil()) {
                            break; // TODO: check if this should break out of entire loop
                        }

                        if (attribute.is_direct()) {
                            relative_offset = attribute.to_node().flags().relative_offset();

                            if (!flags.is_null()) {
                                if (attribute.to_node().flags().value3() == 0) {
                                    break;
                                }
                                if (flags.value3 & (attribute.to_node().flags().value3() == 0)) {
                                    continue;
                                }
                            }

                            body(attribute);
                        } else if (attribute.is_indirect()) {
                            relative_offset = attribute.to_indirect_node().relative_offset();
                            if (!flags.is_null()) {
                                break;
                            }
                        }
                    }
                }
            }

            for (auto child : subgraph->_children) {
                Subgraph *child_subgraph = child.subgraph();
                if (flags.value1 & (child_subgraph->_flags.value1 | child_subgraph->_flags.value2) &&
                    child_subgraph->_traversal_seed != _last_traversal_seed) {
                    stack.push(child_subgraph);
                    child_subgraph->_traversal_seed = _last_traversal_seed;
                }
            }
        }
    }

    // ~without_invalidating();
}

// MARK: - Tree

void Subgraph::begin_tree(AttributeID owner, const swift::metadata *type, uint32_t flags) {

    data::ptr<Graph::TreeElement> tree = (data::ptr<Graph::TreeElement>)alloc_bytes(sizeof(Graph::TreeElement), 7);
    tree->type = type;
    tree->owner = owner;
    tree->flags = flags;
    tree->parent = _tree_root;
    tree->old_parent = nullptr;

    auto old_root = _tree_root;
    _tree_root = tree;

    if (old_root) {
        _tree_root->old_parent = old_root->next;
        old_root->next = _tree_root;
    }
}

void Subgraph::end_tree(AttributeID attribute) {
    if (_tree_root && _tree_root->parent) {
        _tree_root = _tree_root->parent;
    }
}

void Subgraph::set_tree_owner(AttributeID owner) {
    if (_tree_root) {
        return;
    }
    if (_tree_root->parent) {
        precondition_failure("setting owner of non-root tree");
    }
    _tree_root->owner = owner;
}

void Subgraph::add_tree_value(AttributeID attribute, const swift::metadata *type, const char *key, uint32_t flags) {
    if (!_tree_root) {
        return;
    }

    auto key_id = graph()->intern_key(key);

    data::ptr<Graph::TreeValue> tree_value = (data::ptr<Graph::TreeValue>)alloc_bytes(sizeof(Graph::TreeValue), 7);
    tree_value->type = type;
    tree_value->value = attribute;
    tree_value->key_id = key_id;
    tree_value->flags = flags;
    tree_value->previous_sibling = _tree_root->last_value;

    _tree_root->last_value = tree_value;
}

/// Returns the node after the given tree element.
AttributeID Subgraph::tree_node_at_index(data::ptr<Graph::TreeElement> tree_element, uint64_t index) {
    if (auto map = graph()->tree_data_elements()) {
        auto tree_data_element = map->find(this);
        if (tree_data_element != map->end()) {
            tree_data_element->second.sort_nodes();

            auto nodes = tree_data_element->second.nodes();
            std::pair<data::ptr<Graph::TreeElement>, data::ptr<Node>> *found = std::find_if(
                nodes.begin(), nodes.end(), [&tree_element](auto node) { return node.first == tree_element; });

            uint64_t i = index;
            for (auto node = found; node != nodes.end(); ++node) {
                if (found->first != tree_element) {
                    break;
                }
                if (i == 0) {
                    return AttributeID(node->second);
                }
                --index;
            }
        }
    }
    return AttributeID::make_nil();
}

uint32_t Subgraph::tree_subgraph_child(data::ptr<Graph::TreeElement> tree_element) {
    auto map = _graph->tree_data_elements();
    if (!map) {
        return 0;
    }
    auto tree_data_element = map->find(this);
    tree_data_element->second.sort_nodes();

    auto nodes = tree_data_element->second.nodes();
    if (!nodes.empty()) {
        return;
    }

    // TODO: verify this is lower_bound
    auto iter = std::lower_bound(nodes.begin(), nodes.end(), tree_element,
                                 [](auto iter, auto value) -> bool { return iter.first.offset() < value; });
    if (iter == nodes.end()) {
        return;
    }

    auto subgraph_vector = vector<Subgraph *, 32, uint64_t>();

    for (auto subgraph : _graph->subgraphs()) {
        if (subgraph->_invalidated) {
            continue;
        }
        if (subgraph->_tree_root == nullptr) {
            continue;
        }
        AttributeID owner = subgraph->_tree_root->owner;
        if (owner.without_kind() == 0) {
            continue;
        }
        OffsetAttributeID resolved = owner.resolve(AttributeID::TraversalOptions::None);
        owner = resolved.attribute();
        if (owner.is_direct()) {
            for (auto node_iter = iter; node_iter != nodes.end(); ++node_iter) {
                if (node_iter->first->owner == owner) {
                    subgraph_vector.push_back(subgraph);
                }
            }
        }
    }

    std::sort(subgraph_vector.begin(), subgraph_vector.end());

    // TODO: not sure what is happening here
    uint32_t result = 0;
    uint32_t last_old_parent = 0;
    for (auto subgraph : subgraph_vector) {
        result = subgraph->_tree_root;
        subgraph->_tree_root->old_parent = last_old_parent;
        last_old_parent = result;
    }
    return result;
}

// MARK: - Flags

void Subgraph::set_flags(data::ptr<Node> node, NodeFlags::Flags3 flags) {
    if (node->flags().value3() == flags) {
        return;
    }
    if (node->flags().value3() == 0 || flags == 0) {
        unlink_attribute(node);
        node->flags().set_value3(flags);
        insert_attribute(node, true);
    } else {
        node->flags().set_value3(flags);
    }

    add_flags(flags);
    if (node->state().is_dirty()) {
        add_dirty_flags(flags);
    }
}

void Subgraph::add_flags(uint8_t flags) {
    // Status: doesn't exist in decompile
    if (flags & ~_flags.value1) {
        _flags.value1 |= flags;
        propagate_flags();
    }
}

void Subgraph::add_dirty_flags(uint8_t dirty_flags) {
    // Status: Verified
    if (dirty_flags & ~_flags.value3) {
        _flags.value3 |= dirty_flags;
        propagate_dirty_flags();
    }
}

void Subgraph::propagate_flags() {
    // Status: doesn't exist so not sure if new_flags is _flags.value1 | _flags.value2;
    uint8_t new_flags = _flags.value1 | _flags.value2;
    foreach_ancestor([&new_flags](Subgraph &ancestor) -> bool {
        if (new_flags & ~ancestor._flags.value2) {
            ancestor._flags.value2 |= new_flags;
            return true;
        }
        return false;
    });
}

void Subgraph::propagate_dirty_flags() {
    // Status: Verified
    uint8_t arg = _flags.value3 | _flags.value4;
    foreach_ancestor([&arg](Subgraph &ancestor) -> bool {
        if (arg & ancestor._flags.value4) {
            ancestor._flags.value4 |= arg;
            return true;
        }
        return false;
    });
}

// MARK: - Observers

uint64_t Subgraph::add_observer(ClosureFunctionVV<void> observer) {
    if (!_observers) {
        _observers = (data::ptr<observers_vector>)alloc_bytes(sizeof(observers_vector), 7);
        *_observers = observers_vector();
    }

    auto observer_id = AGMakeUniqueID();
    _observers->push_back({
        observer,
        observer_id,
    });
    return observer_id;
}

void Subgraph::remove_observer(uint64_t observer_id) {
    if (_observers) {
        auto iter = std::remove_if(_observers->begin(), _observers->end(), [&observer_id](auto pair) -> bool {
            if (pair.second == observer_id) {
                pair.first.release_context(); // TODO: where is retain?
                return true;
            }
            return false;
        });
        _observers->erase(iter, _observers->end());
    }
}

void Subgraph::notify_observers() {
    while (!_observers->empty()) {
        auto observer = _observers->back();
        observer.first();
        observer.first.release_context(); // TODO: where is retain?
        _observers->pop_back();
    }
}

#pragma mark - Cache

data::ptr<Node> Subgraph::cache_fetch(uint64_t identifier, const swift::metadata &metadata, void *body,
                                      ClosureFunctionCI<unsigned long, Graph *> closure) {
    if (_cache == nullptr) {
        _cache = (data::ptr<NodeCache>)alloc_bytes(sizeof(NodeCache), 7);
    }

    auto type = _cache->types().lookup(&metadata, nullptr);
    if (type == nullptr) {
        auto equatable = metadata.equatable();
        if (equatable == nullptr) {
            precondition_failure("cache key must be equatable: %s", metadata.name(false));
        }

        type = (data::ptr<NodeCache::Type>)alloc_bytes(sizeof(NodeCache::Type), 7);
        type->type = &metadata;
        type->equatable = equatable;
        type->last_item = nullptr;
        type->first_item = nullptr;
        type->type_id = (uint32_t)closure(_graph);
        _cache->types().insert(&metadata, type);
    }

    NodeCache::ItemKey item_lookup_key = {identifier << 8, type, nullptr, body};
    NodeCache::Item *item = _cache->table2().lookup(&item_lookup_key, nullptr);
    if (item == nullptr) {
        //        if (closure == nullptr) {
        //            return 0;
        //        }

        item = type->first_item;
        if (item == 0 || item->field_0x00 < 2) {
            data::ptr<Node> node = graph()->add_attribute(*this, type->type_id, body, nullptr);

            item = new NodeCache::Item(item_lookup_key.field_0x00, item_lookup_key.equals_item, node, nullptr, nullptr);

            node->flags().set_value4_unknown0x10(true);

            _cache->items().insert(node, item);
        } else {
            // remove item from linked list
            if (item->prev != nullptr) {
                item->prev->next = item->next;
            } else {
                type->first_item = item->next;
            }
            if (item->next != nullptr) {
                item->next->prev = item->prev;
            } else {
                type->last_item = item->prev;
            }

            if (item->field_0x00 != 0xff) {
                _cache->table2().remove_ptr((const NodeCache::ItemKey *)item);
            }

            data::ptr<Node> node = item->node;
            _graph->remove_all_inputs(node);
            node->set_state(node->state().with_dirty(true).with_pending(true));
            node->update_self(*_graph, body);
            item->field_0x00 |= identifier << 8;
        }

        _cache->table2().insert((const NodeCache::ItemKey *)item, item);
    } else {
        if (item->field_0x00 & 0xff) {
            // remove item from linked list
            if (item->prev != nullptr) {
                item->prev->next = item->next;
            } else {
                type->first_item = item->next;
            }
            if (item->next != nullptr) {
                item->next->prev = item->prev;
            } else {
                type->last_item = item->prev;
            }
        }
    }
    item->field_0x00 &= 0xffffffffffffff00;
    return item->node;
}

void Subgraph::cache_insert(data::ptr<Node> node) {
    if (_invalidated) {
        return;
    }

    if (node->flags().value4_unknown0x10() && !node->state().is_evaluating() && node->num_output_edges() == 0) {
        // TODO: one of these flags must indicate it is cached

        const AttributeType &attribute_type = _graph->attribute_type(node->type_id());
        data::ptr<NodeCache::Type> type = _cache->types().lookup(&attribute_type.self_metadata(), nullptr);

        NodeCache::Item *item = _cache->items().lookup(node, nullptr);
        item->field_0x00 += 1;

        // insert into linked list
        item->prev = type->last_item;
        item->next = nullptr;
        type->last_item = item;
        if (item->prev != nullptr) {
            item->prev->next = item;
        } else {
            type->first_item = item;
        }

        if ((_other_state & CacheState::Option1) == 0) {
            if ((_other_state & CacheState::Option2) == 0) {
                _graph->add_subgraphs_with_cached_node(this);
            }
            _other_state |= CacheState::Option1;
        }
    }
}

void Subgraph::cache_collect() {
    _other_state &= ~CacheState::Option1; // turn off 0x1 bit

    std::pair<Subgraph *, NodeCache *> context = {this, _cache.get()};
    if (_cache != nullptr && !_invalidated) {
        _cache->types().for_each(
            [](const swift::metadata *metadata, const data::ptr<NodeCache::Type> type, const void *context) {
                Subgraph *subgraph = reinterpret_cast<const std::pair<Subgraph *, NodeCache *> *>(context)->first;
                NodeCache *cache = reinterpret_cast<const std::pair<Subgraph *, NodeCache *> *>(context)->second;

                NodeCache::Item *item = type->last_item;
                while (item) {
                    if ((~item->field_0x00 & 0xff) == 0) {
                        return; // identifier == 0xff
                    }
                    item->field_0x00 += 1;
                    if ((~item->field_0x00 & 0xff) == 0) {
                        // identifier == 0xff
                        cache->table2().remove_ptr((const NodeCache::ItemKey *)item);
                        data::ptr<Node> node = item->node;
                        node->destroy_self(*subgraph->_graph);
                        node->destroy_value(*subgraph->_graph);
                        subgraph->_graph->remove_all_inputs(node);
                    } else {
                        subgraph->_other_state |= CacheState::Option1;
                    }
                    item = item->prev;
                }
            },
            &context);
    }
}

#pragma mark - Encoding

void Subgraph::encode(Encoder &encoder) {

    auto zone_id = info().zone_id();
    if (zone_id != 0) {
        encoder.encode_varint(8);
        encoder.encode_varint(zone_id);
    }

    if (_graph_context_id != 0) {
        encoder.encode_varint(0x10);
        encoder.encode_varint(_graph_context_id);
    }

    for (auto parent : _parents) {
        auto zone_id = parent->info().zone_id();
        if (zone_id != 0) {
            encoder.encode_varint(0x18);
            encoder.encode_varint(zone_id);
        }
    }

    for (auto child : _children) {
        auto zone_id = child.subgraph()->info().zone_id();
        if (zone_id != 0) {
            encoder.encode_varint(0x20);
            encoder.encode_varint(zone_id);
        }
    }

    if (_invalidated) {
        encoder.encode_varint(0x28);
        encoder.encode_varint(true);
    }

    if (last_page() == nullptr) {
        if (_tree_root) {
            encoder.encode_varint(0x3a);
            encoder.begin_length_delimited();
            _graph->encode_tree(encoder, _tree_root);
            encoder.end_length_delimited();
        }
        return;
    }

    for (data::ptr<data::page> page = last_page(); page != nullptr; page = page->previous) {
        bool bVar4 = true;
        bool bVar3 = true;
        do {
            bVar3 = bVar4;

            uint16_t relative_offset = bVar3 ? page->relative_offset_1 : page->relative_offset_2;
            while (relative_offset) {
                AttributeID attribute = AttributeID(page + relative_offset);

                if (attribute.is_direct()) {
                    attribute.to_node().flags().relative_offset();
                } else if (attribute.is_indirect()) {
                    attribute.to_indirect_node().relative_offset();
                } else {
                    if (attribute.is_nil() || relative_offset == 0) {
                        break;
                    }
                    continue;
                }

                encoder.encode_varint(0x32);
                encoder.begin_length_delimited();

                if (attribute.to_node_ptr() == nullptr) {
                    encoder.encode_varint(0x12);
                    encoder.begin_length_delimited();
                    _graph->encode_node(encoder, attribute.to_node(), false);
                    encoder.end_length_delimited();
                } else {
                    encoder.encode_varint(8);
                    encoder.encode_varint(attribute);

                    if (attribute.is_direct()) {
                        encoder.encode_varint(0x12);
                        encoder.begin_length_delimited();
                        _graph->encode_node(encoder, attribute.to_node(), false);
                        encoder.end_length_delimited();
                    } else if (attribute.is_indirect()) {
                        encoder.encode_varint(0x1a);
                        encoder.begin_length_delimited();
                        _graph->encode_indirect_node(encoder, attribute.to_indirect_node());
                        encoder.end_length_delimited();
                    }
                }

                encoder.end_length_delimited();
            }
            bVar4 = false;
        } while (bVar3);
    }
}

#pragma mark - Printing

void Subgraph::print(uint32_t indent_level) {
    uint64_t indent_length = 2 * indent_level;
    char *indent_string = (char *)alloca(indent_length + 1);
    memset(indent_string, ' ', indent_length);
    indent_string[indent_length] = '\0';

    fprintf(stdout, "%s+ %p: %u in %lu [", indent_string, this, info().zone_id(), (unsigned long)_graph_context_id);

    bool first = true;
    for (data::ptr<data::page> page = last_page(); page != nullptr; page = page->previous) {
        uint16_t relative_offset = page->relative_offset_1;
        while (relative_offset) {
            AttributeID attribute = AttributeID(page + relative_offset);
            if (attribute.is_nil()) {
                break;
            }

            if (attribute.is_direct()) {
                relative_offset = attribute.to_node().flags().relative_offset();
            } else if (attribute.is_indirect()) {
                relative_offset = attribute.to_indirect_node().relative_offset();
            }

            if (attribute.is_direct()) {
                fprintf(stdout, "%s%u", first ? "" : " ", attribute.value());
                if (attribute.to_node().flags().value3()) {
                    fprintf(stdout, "(%u)", attribute.to_node().flags().value3());
                }
                first = false;
            }
        }
    }

    fwrite("]\n", 2, 1, stdout);

    for (auto child : _children) {
        child.subgraph()->print(indent_level + 1);
    }
}

} // namespace AG
