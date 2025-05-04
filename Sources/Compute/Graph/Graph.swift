import ComputeCxx

extension Graph {

    @_extern(c, "AGGraphWithUpdate")
    static func withUpdate(attribute: AnyAttribute, body: () -> Void)

}

extension Graph {

    @_extern(c, "AGGraphSearch")
    static func search(attribute: AnyAttribute, options: AGSearchOptions, predicate: (AnyAttribute) -> Bool) -> Bool

}

extension Graph {

    @_extern(c, "AGGraphMutateAttribute")
    static func mutateAttribute(
        _ attribute: AnyAttribute,
        type: Metadata,
        invalidating: Bool,
        body: (UnsafeMutableRawPointer) -> Void
    )

}

extension Graph {

    @_extern(c, "AGGraphSetUpdateCallback")
    private static func setUpdateCallback(_ graph: UnsafeRawPointer, callback: (() -> Void)?)

    public static func setUpdateCallback(_ graph: Graph, callback: (() -> Void)?) {
        Graph.setUpdateCallback(unsafeBitCast(graph, to: UnsafeRawPointer.self), callback: callback)
    }

    public func onUpdate(_ handler: @escaping () -> Void) {
        Graph.setUpdateCallback(self, callback: handler)
    }

}

extension Graph {

    @_extern(c, "AGGraphSetInvalidationCallback")
    private static func setInvalidationCallback(_ graph: UnsafeRawPointer, callback: ((AnyAttribute) -> Void)?)

    public static func setInvalidationCallback(_ graph: Graph, callback: ((AnyAttribute) -> Void)?) {
        Graph.setInvalidationCallback(unsafeBitCast(graph, to: UnsafeRawPointer.self), callback: callback)
    }

    public func onInvalidation(_ handler: @escaping (AnyAttribute) -> Void) {
        Graph.setInvalidationCallback(self, callback: handler)
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

    @_extern(c, "AGGraphWithMainThreadHandler")
    private static func withMainThreadHandler(
        _ graph: UnsafeRawPointer,
        body: () -> Void,
        mainThreadHandler: (() -> Void) -> Void
    )

    public func withMainThreadHandler(
        _ mainThreadHandler: (() -> Void) -> Void,
        do body: () -> Void
    ) {
        withUnsafePointer(to: self) { selfPointer in
            Graph.withMainThreadHandler(selfPointer, body: body, mainThreadHandler: mainThreadHandler)
        }
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

    public var mainUpdates: Int { numericCast(counter(for: .mainThreadUpdateCount)) }

}

extension Graph {

    public func print(includeValues: Bool) {
        Swift.print(graphvizDescription(includeValues: includeValues))
    }

    public static func archiveJSON(name: String?) {
        __AGGraphArchiveJSON(name?.cString(using: .utf8))
    }

    public func graphvizDescription(includeValues: Bool) -> String {
        let result = Graph.description(
            self,
            options: [AGDescriptionFormat: "graph/dot", AGDescriptionIncludeValues: includeValues] as CFDictionary
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
        let result = Graph.description(
            nil,
            options: [AGDescriptionFormat: "stack/text", AGDescriptionMaxFrames: maxFrames] as CFDictionary
        ).takeUnretainedValue()
        guard let description = result as? String else {
            preconditionFailure()
        }
        return description
    }

}

extension Graph: @retroactive Equatable {

    public static func == (_ lhs: Graph, _ rhs: Graph) -> Bool {
        return lhs.counter(for: .graphID) == rhs.counter(for: .graphID)
    }

}
