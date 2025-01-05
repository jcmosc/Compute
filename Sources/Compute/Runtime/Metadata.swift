public struct Metadata {
    
    public struct ApplyOptions {
        
    }

    public init(_ type: Any.Type) {
        fatalError("not implemented")
    }

    public var type: Any.Type {
        fatalError("not implemented")
    }

    public func forEachField(options: ApplyOptions, do body: @escaping (UnsafePointer<Int8>, Int, Any.Type) -> Bool)
        -> Bool
    {
        fatalError("not implemented")
    }

}

extension Metadata: CustomStringConvertible {

    public var description: String {
        fatalError("not implemented")
    }

}

extension Metadata: Hashable {

    public func hash(into hasher: inout Hasher) {
        fatalError("not implemented")
    }

}
