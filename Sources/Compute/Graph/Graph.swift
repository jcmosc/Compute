import ComputeCxx

extension Graph {

    public func onUpdate(_ handler: () -> Void) {
        fatalError("not implemented")
    }

    public func onInvalidation(_ handler: (AnyAttribute) -> Void) {
        fatalError("not implemented")
    }

    public func withDeadline<T>(_ deadline: UInt64, _ body: () -> T) -> T {
        fatalError("not implemented")
    }

    public func withoutUpdate<T>(_ body: () -> T) -> T {
        fatalError("not implemented")
    }

    public func withoutSubgraphInvalidation<T>(_ body: () -> T) -> T {
        fatalError("not implemented")
    }

    public func withMainThreadHandler(_ body: (() -> Void) -> Void, do: () -> Void) {
        fatalError("not implemented")
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
        fatalError("not implemented")
    }

}
