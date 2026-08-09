#include "IAGAppObserver.h"

#import <Foundation/Foundation.h>

#include "Graph/Graph.h"

#if TARGET_OS_MAC

@interface IAGAppObserver : NSObject

+ (void)foreground:(IAG::Graph *)graph;
+ (void)background:(IAG::Graph *)graph;

@end

@implementation IAGAppObserver

+ (void)foreground:(IAG::Graph *)graph {
    graph->all_mark_profile("app/foreground");
}

+ (void)background:(IAG::Graph *)graph {
    graph->all_mark_profile("app/background");
}

@end

void IAGAppObserverStartObserving() {
    if (NSClassFromString(@"UIApplication")) {
        [[NSNotificationCenter defaultCenter] addObserver:[IAGAppObserver class]
                                                 selector:@selector(foreground:)
                                                     name:@"UIApplicationWillEnterForegroundNotification"
                                                   object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:[IAGAppObserver class]
                                                 selector:@selector(background:)
                                                     name:@"UIApplicationDidEnterBackgroundNotification"
                                                   object:nil];
    }
}

#endif
