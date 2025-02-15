#include "AGAppObserver.h"

#include <Foundation/Foundation.h>

#include "Graph/Graph.h"

@interface AGAppObserver : NSObject

+ (void)foreground:(AG::Graph *)graph;
+ (void)background:(AG::Graph *)graph;

@end

@implementation AGAppObserver

+ (void)foreground:(AG::Graph *)graph {
    graph->all_mark_profile("app/foreground");
}

+ (void)background:(AG::Graph *)graph {
    graph->all_mark_profile("app/background");
}

@end

void AGAppObserverStartObserving() {
    if (NSClassFromString(@"UIApplication")) {
        [[NSNotificationCenter defaultCenter] addObserver:[AGAppObserver class]
                                                 selector:@selector(foreground:)
                                                     name:@"UIApplicationWillEnterForeground" // TODO get real name
                                                   object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:[AGAppObserver class]
                                                 selector:@selector(background:)
                                                     name:@"UIApplicationDidEnterBackground" // TODO get real name
                                                   object:nil];
    }
}
