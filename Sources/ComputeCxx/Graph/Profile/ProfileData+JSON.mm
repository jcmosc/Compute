#include "ProfileData.h"

#include <Foundation/Foundation.h>
#include <mach/mach_time.h>

#include "Graph/Graph.h"
#include "Time/Time.h"

namespace AG {

CFDictionaryRef Graph::ProfileData::json_data(const Data &data) {
    // TODO: figure out real keys
    NSMutableDictionary *dict = nil;
    if (data.count1) {
        if (!dict) {
            dict = [NSMutableDictionary dictionary];
        }
        dict[@"count1"] = [NSNumber numberWithUnsignedLong:data.count1];
    }
    if (data.count2) {
        if (!dict) {
            dict = [NSMutableDictionary dictionary];
        }
        dict[@"count2"] = [NSNumber numberWithUnsignedLong:data.count2];
    }
    if (data.time1) {
        if (!dict) {
            dict = [NSMutableDictionary dictionary];
        }
        dict[@"time1"] = [NSNumber numberWithDouble:absolute_time_to_seconds(data.time1)];
    }
    if (data.time2) {
        if (!dict) {
            dict = [NSMutableDictionary dictionary];
        }
        dict[@"time2"] = [NSNumber numberWithDouble:absolute_time_to_seconds(data.time2)];
    }
    return (__bridge CFDictionaryRef)dict;
}

CFDictionaryRef Graph::ProfileData::json_data(const Item &item, const Graph &graph) {
    // TODO: figure out real keys
    NSMutableDictionary *json = (__bridge NSMutableDictionary *)json_data(item.data());
    if (!item.marks().empty()) {
        NSMutableArray *array = [NSMutableArray array];
        for (auto mark : item.marks()) {
            NSMutableDictionary *mark_json = (__bridge NSMutableDictionary *)json_data(mark.data);
            if (mark_json) {
                mark_json[@"event"] = [NSString stringWithUTF8String:graph.key_name(mark.event_id)];
                mark_json[@"time"] = [NSNumber numberWithDouble:absolute_time_to_seconds(mark.time)];
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
