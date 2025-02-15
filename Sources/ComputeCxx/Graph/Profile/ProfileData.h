#pragma once

#include <CoreFoundation/CFBase.h>
#include <mach/mach_time.h>

#include "Graph/Graph.h"
#include "Utilities/HashTable.h"

CF_ASSUME_NONNULL_BEGIN

namespace AG {

class Graph::ProfileData {
  public:
    struct Data {
        uint64_t count1;
        uint64_t count2;
        uint64_t time1;
        uint64_t time2;
    };

    struct Mark {
        uint32_t event_id;
        uint64_t time;
        Data data;
    };

    class Item {
      private:
        Data _data;
        vector<Mark, 0, uint64_t> _marks;

      public:
        Data &data() { return _data; };
        const Data &data() const { return _data; };
        const vector<Mark, 0, uint64_t> &marks() const { return _marks; };

        void operator+=(const Item &other);

        void mark(uint32_t event_id, uint64_t time);
    };

    class Category : public Item {
      private:
        std::unordered_map<data::ptr<Node>, Item> _items_by_attribute;
        std::unordered_map<uint32_t, Item> _removed_items_by_type_id;

      public:
        std::unordered_map<data::ptr<Node>, Item> &items_by_attribute() { return _items_by_attribute; };

        void add_update(data::ptr<Node> node, uint64_t time, bool flag);
        void remove_node(data::ptr<Node> node, uint32_t type_id) {
            auto found = _items_by_attribute.find(node);
            if (found != _items_by_attribute.end()) {
                auto &item = _removed_items_by_type_id.try_emplace(type_id).first->second;
                item += found->second;
                _items_by_attribute.erase(found);
            }
        };

        void mark(uint32_t event_id, uint64_t time);
    };

  private:
    uint64_t _precision;
    Category _current_category;

    std::unordered_map<uint32_t, Category> _categories;
    bool _has_unmarked_categories;

  public:
    ProfileData(Graph *graph);

    uint64_t precision() const { return _precision; };

    Category &current_category() { return _current_category; };
    std::unordered_map<uint32_t, Category> &categories() { return _categories; };

    bool has_unmarked_categories() const { return _has_unmarked_categories; };
    void set_has_unmarked_categories(bool value) { _has_unmarked_categories = value; };

    void remove_node(data::ptr<Node> node, uint32_t type_id);

    void mark(uint32_t event_id, uint64_t time) {
        if (_has_unmarked_categories) {
            if (time == 0) {
                time = mach_absolute_time();
            }
            _current_category.mark(event_id, time);
            for (auto &entry : _categories) {
                entry.second.mark(event_id, time); // TODO: does this modify in place or a copy?
            }
            _has_unmarked_categories = false;
        }
    }

    CFDictionaryRef json_data(const Data &data);
    CFDictionaryRef json_data(const Item &item, const Graph &graph);
};

} // namespace AG

CF_ASSUME_NONNULL_END
