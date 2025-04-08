import ComputeCxx

struct ContextVV {
    let body: () -> Void
}

struct ContextAV {
    let body: (AnyAttribute) -> Void
}

struct ContextPV {
    let body: (() -> Void) -> Void
}

extension Graph {

    public func onUpdate(_ handler: @escaping () -> Void) {
        withUnsafePointer(to: ContextVV(body: handler)) { contextPointer in
            __AGGraphSetUpdateCallback(
                self,
                {
                    $0.assumingMemoryBound(to: ContextVV.self).pointee.body()
                },
                contextPointer
            )
        }
    }

    public func onInvalidation(_ handler: @escaping (AnyAttribute) -> Void) {
        withUnsafePointer(to: ContextAV(body: handler)) { contextPointer in
            __AGGraphSetInvalidationCallback(
                self,
                {
                    // TODO: swap params around
                    $1.assumingMemoryBound(to: ContextAV.self).pointee.body($0)
                },
                contextPointer
            )
        }
    }

    public func withDeadline<T>(_ deadline: UInt64, _ body: () -> T) -> T {
        let oldDeadline = __AGGraphGetDeadline(self)
        __AGGraphSetDeadline(self, deadline)
        let result = body()
        __AGGraphSetDeadline(self, oldDeadline)
        return result
    }

    public func withoutUpdate<T>(_ body: () -> T) -> T {
        let previousUpdate = __AGGraphClearUpdate()
        let result = body()
        __AGGraphSetUpdate(previousUpdate)
        return result
    }

    public func withoutSubgraphInvalidation<T>(_ body: () -> T) -> T {
        let wasDeferringSubgraphInvalidation = __AGGraphBeginDeferringSubgraphInvalidation(self)
        let result = body()
        __AGGraphEndDeferringSubgraphInvalidation(self, wasDeferringSubgraphInvalidation)
        return result
    }

    // todo: delete @escaping
    public func withMainThreadHandler(
        _ mainThreadHandler: @escaping (() -> Void) -> Void,
        do body: @escaping () -> Void
    ) {
        //        let bodyContext = ContextVV(body: body)
        //        let mainThreadHandlerContext = ContextPV(body: mainThreadHandler)
        //        withUnsafePointer(to: bodyContext) { bodyContextPointer in
        //            withUnsafePointer(
        //                to: mainThreadHandlerContext
        //            ) { mainThreadHandlerContextPointer in
        //                __AGGraphWithMainThreadHandler(
        //                    self,
        //                    {
        //                        $0.assumingMemoryBound(to: ContextVV.self).pointee.body()
        //                    },
        //                    bodyContextPointer,
        //                    { thunk, context in
        //
        //
        //                        // TODO: swap params
        //                        context.assumingMemoryBound(to: ContextPV.self).pointee.body({
        //                            thunk()
        //                        })
        //                    },
        //                    mainThreadHandlerContextPointer
        //                )
        //            }
        //
        //        }
    }

}

extension Graph {

    public static func startProfiling() {
        __AGGraphStartProfiling(nil)
    }

    public static func stopProfiling() {
        __AGGraphStopProfiling(nil)
    }

    public static func markProfile(name: UnsafePointer<Int8>) {
        __AGGraphMarkProfile(nil, name)

    }

    public static func resetProfile() {
        __AGGraphResetProfile(nil)
    }

}

extension Graph {

    public func addTraceEvent<T>(_ event: UnsafePointer<Int8>, context: UnsafePointer<T>) {
        __AGGraphAddTraceEvent(self, event, context, Metadata(T.self))
    }

    public func addTraceEvent<T>(_ event: UnsafePointer<Int8>, value: T) {
        withUnsafePointer(to: value) { valuePointer in
            __AGGraphAddTraceEvent(self, event, valuePointer, Metadata(T.self))
        }
    }

}

extension Graph {

    public func print(includeValues: Bool) {
        Swift.print(graphvizDescription(includeValues: includeValues))
    }

    public func archiveJSON(name: String?) {
        __AGGraphArchiveJSON(name?.cString(using: .utf8))
    }

    public func graphvizDescription(includeValues: Bool) -> String {
        let result = __AGGraphDescription(
            self,
            [AGDescriptionFormat: "graph/dot", AGDescriptionIncludeValues: includeValues] as CFDictionary
        ).takeUnretainedValue()
        guard let description = result as? String else {
            preconditionFailure()
        }
        return description
    }

    public static func printStack(maxFrames: Int) {
        Swift.print(stackDescription(maxFrames: maxFrames))
    }

    public static func stackDescription(maxFrames: Int) -> String {
        let result = __AGGraphDescription(
            nil,
            [AGDescriptionFormat: "stack/text", AGDescriptionMaxFrames: maxFrames] as CFDictionary
        ).takeUnretainedValue()
        guard let description = result as? String else {
            preconditionFailure()
        }
        return description
    }

}

extension Graph: @retroactive Equatable {

    public static func == (_ lhs: Graph, _ rhs: Graph) -> Bool {
        return __AGGraphGetCounter(lhs, .graphID) == __AGGraphGetCounter(rhs, .graphID)
    }

}
