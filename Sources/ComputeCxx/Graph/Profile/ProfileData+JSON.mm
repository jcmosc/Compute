#include "ProfileData.h"

#include <Foundation/Foundation.h>
#include <mach/mach_time.h>

#include "Graph/Graph.h"
#include "Time/Time.h"

namespace AG {

CFDictionaryRef Graph::ProfileData::json_data(const Data &data) {
    NSMutableDictionary *dict = nil;
    if (data.update_count) {
        if (!dict) {
            dict = [NSMutableDictionary dictionary];
        }
        dict[@"update_count"] = [NSNumber numberWithUnsignedLong:data.update_count];
    }
    if (data.change_count) {
        if (!dict) {
            dict = [NSMutableDictionary dictionary];
        }
        dict[@"change_count"] = [NSNumber numberWithUnsignedLong:data.change_count];
    }
    if (data.update_total) {
        if (!dict) {
            dict = [NSMutableDictionary dictionary];
        }
        dict[@"update_total"] = [NSNumber numberWithDouble:absolute_time_to_seconds(data.update_total)];
    }
    if (data.changed_total) {
        if (!dict) {
            dict = [NSMutableDictionary dictionary];
        }
        dict[@"changed_total"] = [NSNumber numberWithDouble:absolute_time_to_seconds(data.changed_total)];
    }
    return (__bridge CFDictionaryRef)dict;
}

CFDictionaryRef Graph::ProfileData::json_data(const Item &item, const Graph &graph) {
    NSMutableDictionary *json = (__bridge NSMutableDictionary *)json_data(item.data());
    if (!item.marks().empty()) {
        NSMutableArray *array = [NSMutableArray array];
        for (auto mark : item.marks()) {
            NSMutableDictionary *mark_json = (__bridge NSMutableDictionary *)json_data(mark.data);
            if (mark_json) {
                mark_json[@"name"] = [NSString stringWithUTF8String:graph.key_name(mark.event_id)];
                mark_json[@"timestamp"] = [NSNumber numberWithDouble:absolute_time_to_seconds(mark.timestamp)];
                [array addObject:mark_json];
            }
        }
        if (!json) {
            json = [NSMutableDictionary dictionary];
        }
        json[@"marks"] = array;
    }
    return (__bridge CFDictionaryRef)json;
}

} // namespace AG
