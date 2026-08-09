#include "Graph/Graph.h"

#include <mach/mach_time.h>

namespace IAG {

void Graph::ProfileData::Data::add_update(uint64_t duration, bool changed) noexcept {
    _update_count += 1;
    _update_total += duration;
    if (changed) {
        _change_count += 1;
        _change_total += duration;
    }
}

void Graph::ProfileData::Data::reset() noexcept {
    _update_count = 0;
    _change_count = 0;
    _update_total = 0;
    _change_total = 0;
}

Graph::ProfileData::Data &Graph::ProfileData::Data::operator+=(const Data &other) noexcept {
    _update_count += other._update_count;
    _change_count += other._change_count;
    _update_total += other._update_total;
    _change_total += other._change_total;
    return *this;
}

void Graph::ProfileData::Item::mark(uint32_t event_id, uint64_t time) {
    if (!update_count()) {
        return;
    }
    _marks.push_back({event_id, time, *this});
    reset();
}

Graph::ProfileData::Item &Graph::ProfileData::Item::operator+=(const Item &other) {
    Data::operator+=(other);

    for (auto &other_mark : other._marks) {
        auto iter = _marks.begin();
        while (iter != _marks.end() && iter->time < other_mark.time) {
            ++iter;
        }
        if (iter != _marks.end() && iter->time == other_mark.time) {
            iter->data += other_mark.data;
        } else {
            _marks.insert(iter, other_mark);
        }
    }
    return *this;
}

void Graph::ProfileData::Category::mark(uint32_t event_id, uint64_t time) {
    Item::mark(event_id, time);

    for (auto &item : _items) {
        item.second.mark(event_id, time);
    }
    for (auto &item : _removed_items) {
        item.second.mark(event_id, time);
    }
}

void Graph::ProfileData::Category::add_update(data::ptr<Node> node, uint64_t duration, bool changed) {
    Item::add_update(duration, changed);

    auto &item = _items.try_emplace(node).first->second;
    item.add_update(duration, changed);
}

void Graph::ProfileData::Category::remove_node(data::ptr<Node> node, uint32_t type_id) {
    auto found = _items.find(node);
    if (found == _items.end()) {
        return;
    }
    auto &item = _removed_items.try_emplace(type_id).first->second;
    item += found->second;
    _items.erase(found);
};

Graph::ProfileData::ProfileData(const Graph &graph) {
    const int32_t loop_count = 0x10;

    uint64_t delta = 0;
    uint64_t previous_time = mach_absolute_time();
    for (uint32_t i = loop_count; i; --i) {
        const uint64_t current_time = mach_absolute_time();
        delta += current_time - previous_time;
        previous_time = current_time;
    }
    _time_overhead = delta / loop_count;
}

void Graph::ProfileData::mark(uint32_t event_id, uint64_t time) {
    if (!_needs_mark) {
        return;
    }
    if (time == 0) {
        time = mach_absolute_time();
    }
    _profile_updates.mark(event_id, time);
    for (auto &category : _profile_events) {
        category.second.mark(event_id, time);
    }
    _needs_mark = false;
}

void Graph::ProfileData::add_profile_update(data::ptr<Node> node, uint64_t duration, bool changed) {
    uint64_t effective_duration = 0;
    if (duration > _time_overhead) {
        effective_duration = duration - _time_overhead;
    }
    _profile_updates.add_update(node, effective_duration, changed);
    _needs_mark = true;
}

void Graph::ProfileData::add_profile_event(data::ptr<Node> node, uint64_t duration, bool changed, uint32_t event_id) {
    uint64_t effective_duration = 0;
    if (duration > _time_overhead) {
        effective_duration = duration - _time_overhead;
    }
    auto &category = _profile_events.try_emplace(event_id).first->second;
    category.add_update(node, effective_duration, changed);
    _needs_mark = true;
}

void Graph::ProfileData::remove_node(data::ptr<Node> node, uint32_t type_id) {
    _profile_updates.remove_node(node, type_id);
    for (auto &category : _profile_events) {
        category.second.remove_node(node, type_id);
    }
}

} // namespace IAG
