@_extern(c, "AGTupleWithBuffer")
public func withUnsafeTuple(of type: TupleType, count: Int, _ body: (UnsafeMutableTuple) -> Void)

extension TupleType {

    public init(_ types: [Any.Type]) {
        self.init(count: UInt32(types.count), elements: types.map(Metadata.init))
    }

    public init(_ type: Any.Type) {
        self.init(rawValue: unsafeBitCast(type, to: OpaquePointer.self))
    }

    public var type: Any.Type {
        return unsafeBitCast(rawValue, to: Any.Type.self)
    }

    public var isEmpty: Bool {
        return count == 0
    }

    public var indices: Range<Int> {
        return 0..<count
    }

    public func type(at index: Int) -> Any.Type {
        return elementType(at: UInt32(index)).type
    }

    public func offset<T>(at index: Int, as type: T.Type) -> Int {
        return elementOffset(at: UInt32(index), type: Metadata(type))
    }

    public func getElement<T>(
        in tupleValue: UnsafeMutableRawPointer,
        at index: Int,
        to destinationValue: UnsafeMutablePointer<T>,
        options: CopyOptions
    ) {
        __AGTupleGetElement(self, tupleValue, UInt32(index), destinationValue, Metadata(T.self), options)
    }

    public func setElement<T>(
        in tupleValue: UnsafeMutableRawPointer,
        at index: Int,
        from sourceValue: UnsafePointer<T>,
        options: CopyOptions
    ) {
        __AGTupleSetElement(self, tupleValue, UInt32(index), sourceValue, Metadata(T.self), options)
    }

}

extension UnsafeTuple {

    public var count: Int {
        return type.count
    }

    public var isEmpty: Bool {
        return type.isEmpty
    }

    public var indices: Range<Int> {
        return type.indices
    }

    public func address<T>(as expectedType: T.Type) -> UnsafePointer<T> {
        guard type.type == expectedType else {
            preconditionFailure()
        }
        return value.assumingMemoryBound(to: expectedType)
    }

    public func address<T>(of index: Int, as elementType: T.Type) -> UnsafePointer<T> {
        return value.advanced(by: type.elementOffset(at: UInt32(index), type: Metadata(elementType)))
            .assumingMemoryBound(to: elementType)
    }

    public subscript<T>() -> T {
        unsafeAddress {
            return address(as: T.self)
        }
    }

    public subscript<T>(_ index: Int) -> T {
        unsafeAddress {
            return address(of: index, as: T.self)
        }
    }

}

extension UnsafeMutableTuple {

    public init(with tupleType: TupleType) {
        self.init(
            type: tupleType,
            value: UnsafeMutableRawPointer.allocate(
                byteCount: tupleType.size,
                alignment: -1
            )
        )
    }

    public func deallocate(initialized: Bool) {
        if initialized {
            deinitialize()
        }
        value.deallocate()
    }

    public func initialize<T>(at index: Int, to element: T) {
        withUnsafePointer(to: element) { elementPointer in
            type.setElement(in: value, at: index, from: elementPointer, options: .initCopy)
        }
    }

    public func deinitialize() {
        type.destroy(value)
    }

    public func deinitialize(at index: Int) {
        type.destroy(value, at: UInt32(index))
    }

    public var count: Int {
        return type.count
    }

    public var isEmpty: Bool {
        return type.isEmpty
    }

    public var indices: Range<Int> {
        return type.indices
    }

    public func address<T>(as expectedType: T.Type) -> UnsafeMutablePointer<T> {
        guard type.type == expectedType else {
            preconditionFailure()
        }
        return value.assumingMemoryBound(to: expectedType)
    }

    public func address<T>(of index: Int, as elementType: T.Type) -> UnsafeMutablePointer<T> {
        return value.advanced(by: type.elementOffset(at: UInt32(index), type: Metadata(elementType)))
            .assumingMemoryBound(to: elementType)
    }

    public subscript<T>() -> T {
        unsafeAddress {
            return UnsafePointer(address(as: T.self))
        }
        nonmutating unsafeMutableAddress {
            return address(as: T.self)
        }
    }

    public subscript<T>(_ index: Int) -> T {
        unsafeAddress {
            return UnsafePointer(address(of: index, as: T.self))
        }
        nonmutating unsafeMutableAddress {
            return address(of: index, as: T.self)
        }
    }

}
