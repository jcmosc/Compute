#include "ProfileData.h"

#include <mach/mach_time.h>

#include "Graph/Graph.h"

namespace AG {

#pragma mark - ProfileData::Item

void Graph::ProfileData::Item::operator+=(const Item &other) {
    _data.count1 += other._data.count1;
    _data.count2 += other._data.count2;
    _data.time1 += other._data.time1;
    _data.time2 += other._data.time2;
    
    Mark *iter = _marks.begin();
    for (auto other_mark : other._marks) {
        bool merged = false;
        while (iter != _marks.end() && iter->time <= other_mark.time) {
            if (iter == &other_mark) {
                iter->data.count1 += other_mark.data.count1;
                iter->data.count2 += other_mark.data.count2;
                iter->data.time1 += other_mark.data.time1;
                iter->data.time2 += other_mark.data.time2;
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
    if (_data.count1) {
        _marks.push_back({
            event_id,
            time,
            _data,
        });
        _data = {0, 0, 0, 0};
    }
}

#pragma mark - ProfileData::Category

void Graph::ProfileData::Category::add_update(data::ptr<Node> node, uint64_t time, bool flag) {
    data().count1 += 1;
    data().time1 += time;
    if (flag) {
        data().count2 += 1;
        data().time2 += time;
    }
    
    Item &item = _items_by_attribute.try_emplace(node).first->second;
    
    item.data().count1 += 1;
    item.data().time1 += time;
    if (flag) {
        item.data().count2 += 1;
        item.data().time2 += time;
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
    _current_category.remove_node(node, type_id);
    for (auto &entry : _categories) {
        entry.second.remove_node(node, type_id);
    }
}

} // namespace AG
