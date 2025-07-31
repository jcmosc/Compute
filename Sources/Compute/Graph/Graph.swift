import ComputeCxx

extension Graph {

    @_transparent
    public static func anyInputsChanged(excluding excludedAttributes: [AnyAttribute]) -> Bool {
        return __AGGraphAnyInputsChanged(excludedAttributes, excludedAttributes.count)
    }

}

extension Graph {

    @_extern(c, "AGGraphSetUpdateCallback")
    private static func setUpdateCallback(_ graph: UnsafeRawPointer, callback: (() -> Void)?)

    public func onUpdate(_ handler: @escaping () -> Void) {
        Graph.setUpdateCallback(unsafeBitCast(self, to: UnsafeRawPointer.self), callback: handler)
    }

    @_extern(c, "AGGraphSetInvalidationCallback")
    private static func setInvalidationCallback(_ graph: UnsafeRawPointer, callback: ((AnyAttribute) -> Void)?)

    public func onInvalidation(_ handler: @escaping (AnyAttribute) -> Void) {
        Graph.setInvalidationCallback(unsafeBitCast(self, to: UnsafeRawPointer.self), callback: handler)
    }

    public func withDeadline<T>(_ deadline: UInt64, _ body: () -> T) -> T {
        let oldDeadline = self.deadline
        self.deadline = deadline
        let result = body()
        self.deadline = oldDeadline
        return result
    }

    public func withoutUpdate<T>(_ body: () -> T) -> T {
        let previousUpdate = Graph.clearUpdate()
        let result = body()
        Graph.setUpdate(previousUpdate)
        return result
    }

    public func withoutSubgraphInvalidation<T>(_ body: () -> T) -> T {
        let wasDeferring = self.beginDeferringSubgraphInvalidation()
        let result = body()
        self.endDeferringSubgraphInvalidation(wasDeferring: wasDeferring)
        return result
    }

    @_extern(c, "AGGraphWithMainThreadHandler")
    private static func withMainThreadHandler(
        _ graph: UnsafeRawPointer,
        body: () -> Void,
        mainThreadHandler: (() -> Void) -> Void
    )

    public func withMainThreadHandler(_ mainThreadHandler: (() -> Void) -> Void, do body: () -> Void) {
        Graph.withMainThreadHandler(
            unsafeBitCast(self, to: UnsafeRawPointer.self),
            body: body,
            mainThreadHandler: mainThreadHandler
        )
    }

}

extension Graph {

    public static func startProfiling() {
        
        fatalError("not implemented")
    }

    public static func stopProfiling() {
        fatalError("not implemented")
    }

    public static func markProfile(name: UnsafePointer<Int8>) {
        fatalError("not implemented")

    }

    public static func resetProfile() {
        fatalError("not implemented")
    }

}

extension Graph {

    public func addTraceEvent<T>(_ event: UnsafePointer<Int8>, value: T) {
        fatalError("not implemented")
    }

    public func addTraceEvent<T>(_ event: UnsafePointer<Int8>, context: UnsafePointer<T>) {
        fatalError("not implemented")
    }

}

extension Graph {

    public func print(includeValues: Bool) {
        fatalError("not implemented")
    }

    public func archiveJSON(name: String?) {
        fatalError("not implemented")
    }

    public func graphvizDescription(includeValues: Bool) -> String {
        fatalError("not implemented")
    }

    public static func printStack(maxFrames: Int) {
        fatalError("not implemented")
    }

    public static func stackDescription(maxFrames: Int) -> String {
        fatalError("not implemented")
    }

}

extension Graph: @retroactive Equatable {

    public static func == (_ lhs: Graph, _ rhs: Graph) -> Bool {
        return lhs.counter(for: .contextID) == rhs.counter(for: .contextID)
    }

}
