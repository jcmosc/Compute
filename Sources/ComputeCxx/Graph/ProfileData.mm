#include "Graph.h"

#if TARGET_OS_MAC

#import <Foundation/Foundation.h>

#include "Time/Time.h"

namespace IAG {

NSMutableDictionary *Graph::ProfileData::json_data(const Data &data) {
    NSMutableDictionary *dict = nil;
    if (data.update_count()) {
        if (!dict) {
            dict = [NSMutableDictionary dictionary];
        }
        dict[@"update_count"] = @(data.update_count());
    }
    if (data.change_count()) {
        if (!dict) {
            dict = [NSMutableDictionary dictionary];
        }
        dict[@"change_count"] = @(data.change_count());
    }
    if (data.update_total()) {
        if (!dict) {
            dict = [NSMutableDictionary dictionary];
        }
        dict[@"update_total"] = @(absolute_time_to_seconds(data.update_total()));
    }
    if (data.change_total()) {
        if (!dict) {
            dict = [NSMutableDictionary dictionary];
        }
        dict[@"changed_total"] = @(absolute_time_to_seconds(data.change_total()));
    }
    return dict;
}

NSDictionary *Graph::ProfileData::json_data(const Item &item, const Graph &graph) {
    NSMutableDictionary *dict = json_data(item);
    if (!item.marks().empty()) {
        NSMutableArray *array = [NSMutableArray array];
        for (auto mark : item.marks()) {
            NSMutableDictionary *mark_dict = json_data(mark.data);
            if (mark_dict) {
                mark_dict[@"name"] = [NSString stringWithUTF8String:graph.key_name(mark.event_id)];
                mark_dict[@"timestamp"] = [NSNumber numberWithDouble:absolute_time_to_seconds(mark.time)];
                [array addObject:mark_dict];
            }
        }
        if (!dict) {
            dict = [NSMutableDictionary dictionary];
        }
        dict[@"marks"] = array;
    }
    return dict;
}

} // namespace IAG

#endif
