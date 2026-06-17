import ComputeCxx

@_silgen_name("IAGGraphSetOutputValue")
@inline(__always)
@inlinable
func IAGGraphSetOutputValue(_ value: UnsafeRawPointer, of type: Metadata)

extension Graph {

    @inline(__always)
    @inlinable
    public static func setOutputValue<Value>(_ value: UnsafePointer<Value>) {
        IAGGraphSetOutputValue(UnsafeRawPointer(value), of: Metadata(Value.self))
    }

    @_transparent
    @inline(__always)
    public var mainUpdates: Int { numericCast(counter(for: .mainThreadUpdates)) }

}

extension Graph {

    @_transparent
    public static func anyInputsChanged(excluding excludedAttributes: [AnyAttribute]) -> Bool {
        return __IAGGraphAnyInputsChanged(excludedAttributes, excludedAttributes.count)
    }

}

@_silgen_name("IAGGraphSetUpdateCallback")
func IAGGraphSetUpdateCallback(
    _ graph: UnsafeRawPointer,
    callback: (() -> Void)?
)

@_silgen_name("IAGGraphSetInvalidationCallback")
func IAGGraphSetInvalidationCallback(
    _ graph: UnsafeRawPointer,
    callback: ((AnyAttribute) -> Void)?
)

@_silgen_name("IAGGraphWithMainThreadHandler")
func IAGGraphWithMainThreadHandler(
    _ graph: UnsafeRawPointer,
    body: () -> Void,
    mainThreadHandler: (() -> Void) -> Void
)

extension Graph {

    public func onUpdate(_ handler: @escaping () -> Void) {
        IAGGraphSetUpdateCallback(unsafeBitCast(self, to: UnsafeRawPointer.self), callback: handler)
    }

    public func onInvalidation(_ handler: @escaping (AnyAttribute) -> Void) {
        IAGGraphSetInvalidationCallback(unsafeBitCast(self, to: UnsafeRawPointer.self), callback: handler)
    }

    public func withDeadline<T>(_ deadline: UInt64, _ body: () -> T) -> T {
        let oldDeadline = self.deadline
        self.deadline = deadline
        let result = body()
        self.deadline = oldDeadline
        return result
    }

    // check is static
    public static func withoutUpdate<T>(_ body: () -> T) -> T {
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

    public func withMainThreadHandler(_ mainThreadHandler: (() -> Void) -> Void, do body: () -> Void) {
        IAGGraphWithMainThreadHandler(
            unsafeBitCast(self, to: UnsafeRawPointer.self),
            body: body,
            mainThreadHandler: mainThreadHandler
        )
    }

}

extension Graph {

    public static func startProfiling(_ graph: Graph?) {
        fatalError("not implemented")
    }

    public static func stopProfiling(_ graph: Graph?) {
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
