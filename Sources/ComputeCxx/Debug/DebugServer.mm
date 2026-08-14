#include "DebugServer.h"

#if TARGET_OS_MAC

#import <Foundation/Foundation.h>

#include <Utilities/FreeDeleter.h>

#include "ComputeCxx/IAGDescription.h"
#include "Graph/Graph.h"

namespace IAG {

CFDataRef DebugServer::receive(Connection *connection, IAGDebugServerMessageHeader *header, CFDataRef body) {
    @autoreleasepool {
        id body_json = [NSJSONSerialization JSONObjectWithData:(__bridge NSData *)body options:0 error:nullptr];
        if (!body_json) {
            return nullptr;
        }
        if (![body_json isKindOfClass:[NSDictionary class]]) {
            return nullptr;
        }

        NSDictionary *body_dict = (NSDictionary *)body_json;
        NSString *command = body_dict[@"command"];

        if ([command isEqual:@"graph/description"]) {
            NSMutableDictionary *options = [NSMutableDictionary dictionaryWithDictionary:body_dict];
            options[IAGDescriptionFormat] = @"graph/dict";

            id desc = Graph::description(nullptr, options);
            if (!desc) {
                return nullptr;
            }

            return (CFDataRef)CFBridgingRetain([NSJSONSerialization dataWithJSONObject:desc options:0 error:nil]);
        } else if ([command isEqual:@"profiler/start"]) {
            IAGGraphProfileFlags flags = IAGGraphProfileFlagsEnabled;
            id flags_json = body_dict[@"flags"];
            if ([flags_json isKindOfClass:[NSNumber class]]) {
                flags |= [flags_json unsignedIntValue];
            }
            Graph::all_start_profiling(flags);
        } else if ([command isEqual:@"profiler/stop"]) {
            Graph::all_stop_profiling();
        } else if ([command isEqual:@"profiler/reset"]) {
            Graph::all_reset_profile();
        } else if ([command isEqual:@"profiler/mark"]) {
            id name_json = body_dict[@"name"];
            if ([name_json isKindOfClass:[NSString class]]) {
                Graph::all_mark_profile([name_json UTF8String]);
            }
        } else if ([command isEqual:@"tracing/start"]) {
            IAGGraphTraceFlags flags = IAGGraphTraceFlagsEnabled;
            id flags_json = body_dict[@"flags"];
            if ([flags_json isKindOfClass:[NSNumber class]]) {
                flags |= [flags_json unsignedIntValue];
            }

            auto subsystems = std::span<const char *>();
            auto subsystems_vector = vector<std::unique_ptr<const char, util::free_deleter>, 0, uint64_t>();
            id subsystems_json = body_dict[@"subsystems"];
            if ([subsystems_json isKindOfClass:[NSArray class]]) {
                for (id subsystem_json in subsystems_json) {
                    if ([subsystem_json isKindOfClass:[NSString class]]) {
                        const char *str = strdup([subsystem_json UTF8String]);
                        subsystems_vector.push_back(std::unique_ptr<const char, util::free_deleter>(str));
                    }
                }
                subsystems = std::span((const char **)subsystems_vector.data(), subsystems_vector.size());
            }

            Graph::all_start_tracing(flags, subsystems);
        } else if ([command isEqual:@"tracing/stop"]) {
            Graph::all_stop_tracing();
        } else if ([command isEqual:@"tracing/sync"]) {
            Graph::all_sync_tracing();
        }

        return nullptr;
    }
}

} // namespace IAG

#endif
