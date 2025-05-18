#include "ProfileData.h"

#include <mach/mach_time.h>

#include "Graph/Graph.h"

namespace AG {

#pragma mark - ProfileData::Item

void Graph::ProfileData::Item::operator+=(const Item &other) {
    _data.update_count += other._data.update_count;
    _data.change_count += other._data.change_count;
    _data.update_total += other._data.update_total;
    _data.changed_total += other._data.changed_total;
    
    Mark *iter = _marks.begin();
    for (auto other_mark : other._marks) {
        bool merged = false;
        while (iter != _marks.end() && iter->timestamp <= other_mark.timestamp) {
            if (iter == &other_mark) {
                iter->data.update_count += other_mark.data.update_count;
                iter->data.change_count += other_mark.data.change_count;
                iter->data.update_total += other_mark.data.update_total;
                iter->data.changed_total += other_mark.data.changed_total;
                merged = true;
                break;
            }
            iter += 1;
        }
        if (merged) {
            continue;
        }
        
        _marks.insert(iter, other_mark);
        iter += 1;
    }
}

void Graph::ProfileData::Item::mark(uint32_t event_id, uint64_t time) {
    if (_data.update_count) {
        _marks.push_back({
            event_id,
            time,
            _data,
        });
        _data = {0, 0, 0, 0};
    }
}

#pragma mark - ProfileData::Category

void Graph::ProfileData::Category::add_update(data::ptr<Node> node, uint64_t duration, bool changed) {
    data().update_count += 1;
    data().update_total += duration;
    if (changed) {
        data().change_count += 1;
        data().changed_total += duration;
    }
    
    Item &item = _items_by_attribute.try_emplace(node).first->second;
    
    item.data().update_count += 1;
    item.data().update_total += duration;
    if (changed) {
        item.data().change_count += 1;
        item.data().changed_total += duration;
    }
}

void Graph::ProfileData::Category::mark(uint32_t event_id, uint64_t time) {
    Item::mark(event_id, time);
    for (auto &entry : _items_by_attribute) {
        entry.second.mark(event_id, time);
    }
    for (auto &entry : _removed_items_by_type_id) {
        entry.second.mark(event_id, time);
    }
}

#pragma mark - ProfileData

Graph::ProfileData::ProfileData(Graph *graph) {

    uint64_t delta = 0;
    uint64_t last = mach_absolute_time();
    for (uint32_t i = 16; i; --i) {
        uint64_t current = mach_absolute_time();
        delta += current - last;
        last = current;
    }
    _precision = delta / 16;
}

void Graph::ProfileData::remove_node(data::ptr<Node> node, uint32_t type_id) {
    _all_events.remove_node(node, type_id);
    for (auto &entry : _categories) {
        entry.second.remove_node(node, type_id);
    }
}

} // namespace AG
