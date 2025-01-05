public func withUnsafeTuple(of type: TupleType, count: Int, _ body: (UnsafeMutableTuple) -> Void) {
    fatalError("not implemented")
}

public struct TupleType {
    
    public struct CopyOptions {}

    public init(_ types: [Any.Type]) {
        fatalError("not implemented")
    }

    public init(_ type: Any.Type) {
        fatalError("not implemented")
    }

    public var type: Any.Type {
        fatalError("not implemented")
    }

    public var isEmpty: Bool {
        fatalError("not implemented")
    }

    public var indices: Range<Int> {
        fatalError("not implemented")
    }

    public func type(at index: Int) -> Any.Type {
        fatalError("not implemented")
    }

    public func offset<T>(at index: Int, as type: T.Type) -> Int {
        fatalError("not implemented")
    }

    public func getElement<T>(
        in tupleValue: UnsafeMutableRawPointer, at index: Int, to destinationValue: UnsafeMutablePointer<T>,
        options: TupleType.CopyOptions
    ) {
        fatalError("not implemented")
    }

    public func setElement<T>(
        in tupleValue: UnsafeMutableRawPointer, at index: Int, from sourceValue: UnsafePointer<T>,
        options: TupleType.CopyOptions
    ) {
        fatalError("not implemented")
    }

}

public struct UnsafeTuple {

    public var count: Int {
        fatalError("not implemented")
    }

    public var isEmpty: Bool {
        fatalError("not implemented")
    }

    public var indices: Range<Int> {
        fatalError("not implemented")
    }

    public func address<T>(as type: T.Type) -> UnsafePointer<T> {
        fatalError("not implemented")
    }

    public func address<T>(of index: Int, as elementType: T.Type) -> UnsafePointer<T> {
        fatalError("not implemented")
    }

    public subscript<T>() -> T {
        unsafeAddress {
            fatalError("not implemented")
        }
    }

    public subscript<T>(_ index: Int) -> T {
        unsafeAddress {
            fatalError("not implemented")
        }
    }

}

public struct UnsafeMutableTuple {

    public init(with tupleType: TupleType) {
        fatalError("not implemented")
    }

    public func deallocate(initialized: Bool) {
        fatalError("not implemented")
    }

    public func initialize<T>(at index: Int, to element: T) {
        fatalError("not implemented")
    }

    public func deinitialize() {
        fatalError("not implemented")
    }

    public func deinitialize(at index: Int) {
        fatalError("not implemented")
    }

    public var count: Int {
        fatalError("not implemented")
    }
    
    public var isEmpty: Bool {
        fatalError("not implemented")
    }
    
    public var indices: Range<Int> {
        fatalError("not implemented")
    }

    public func address<T>(as type: T.Type) -> UnsafeMutablePointer<T> {
        fatalError("not implemented")
    }

    public func address<T>(of index: Int, as elementType: T.Type) -> UnsafeMutablePointer<T> {
        fatalError("not implemented")
    }

    public subscript<T>() -> T {
        unsafeAddress {
            fatalError("not implemented")
        }
        unsafeMutableAddress {
            fatalError("not implemented")
        }
    }

    public subscript<T>(_ index: Int) -> T {
        unsafeAddress {
            fatalError("not implemented")
        }
        unsafeMutableAddress {
            fatalError("not implemented")
        }
    }

}
