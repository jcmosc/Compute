import Testing

@Suite
struct PrefetchCompareValuesTests {

    enum LayoutExpectation: ExpressibleByStringLiteral {
        case empty
        case layout(String)

        init(stringLiteral value: StringLiteralType) {
            self = .layout(value)
        }
    }

    @Test(
        arguments: [
            // POD
//            (Void.self, .empty),
//            (Bool.self, .empty),
//            (Int.self, .empty),
//            (Double.self, .empty),
//            (Float.self, .empty),
//            (PODStruct.self, .empty),
            
            // Non-POD
            (String.self, .empty),
//
//            // Tuple
//            ((Int, String).self, .empty),
//
//            // Enum
//            (CEnum.self, nil),
//            (TaggedEnum.self, "\t\u{fffd}\u{fffd}W\u{4}\u{1}"),
//            (IndirectEnum.self, "\t0\u{fffd}W\u{4}\u{1}"),
//
//            // Heap
//            (HeapClass.self, nil),
//
//            // Product types
//            (TestStruct.self, "\u{fffd}F\u{fffd}C\u{fffd}F\t\u{fffd}\u{fffd}W\u{4}\u{1}"),
//            (TestClass.self, nil),
        ] as [(Any.Type, LayoutExpectation?)]
    )
    func layout(of type: Any.Type, matches layoutExpectation: LayoutExpectation?) {
        setenv("AG_ASYNC_LAYOUTS", "0", 1)

        let layout = prefetchCompareValues(type: Metadata(type), options: [], priority: 0)

        if let layoutExpectation {
            #expect(layout != nil)
            if let layout {
                switch layoutExpectation {
                case .empty:
                    #expect(layout == UnsafePointer(bitPattern: 1))
                case .layout(let expectedLayout):
                    #expect(String(cString: layout) == expectedLayout)
                }
            }
        } else {
            #expect(layout == nil)
        }
    }

    @Test(arguments: [
        Triple(first: 0, second: 1, third: 2),
        Triple(first: 3, second: 4, third: 5),
    ])
    func attributeWithSubscript(_ value: Triple<Int, Int, Int>) {
        let attribute = Attribute(value: value)
        let offsetValue = attribute[offset: { _ in
            PointerOffset<Triple<Int, Int, Int>, Int>(byteOffset: 8)
        }]
        #expect(offsetValue.wrappedValue == value.second)
        #expect(attribute.first.wrappedValue == value.first)
        #expect(attribute[keyPath: \.third].wrappedValue == value.third)
    }

}

@Suite
struct CompareValuesTests {

    @Test
    func intCompare() throws {
        #expect(compareValues(1, 1) == true)
        #expect(compareValues(1, 2) == false)
    }

    @Test
    func enumCompare() throws {
        enum A { case a, b }
        #expect(compareValues(A.a, A.a) == true)
        #expect(compareValues(A.a, A.b) == false)

        enum B { case a, b, c }
        let b = B.b
        withUnsafePointer(to: b) { p in
            p.withMemoryRebound(to: A.self, capacity: MemoryLayout<A>.size) { pointer in
                #expect(compareValues(pointer.pointee, A.b) == true)
            }
        }
        withUnsafePointer(to: b) { p in
            p.withMemoryRebound(to: A.self, capacity: MemoryLayout<A>.size) { pointer in
                #expect(compareValues(pointer.pointee, A.a) == false)
            }
        }
    }

    @Test
    func testStructCompare() throws {
        struct A1 {
            var a: Int
            var b: Bool
        }
        struct A2 {
            var a: Int
            var b: Bool
        }
        let a = A1(a: 1, b: true)
        let b = A1(a: 1, b: true)
        let c = A1(a: 1, b: false)
        #expect(compareValues(b, a) == true)
        #expect(compareValues(c, a) == false)
        let d = A2(a: 1, b: true)
        withUnsafePointer(to: d) { p in
            p.withMemoryRebound(to: A1.self, capacity: MemoryLayout<A1>.size) { pointer in
                #expect(compareValues(pointer.pointee, a) == true)
            }
        }
        withUnsafePointer(to: d) { p in
            p.withMemoryRebound(to: A1.self, capacity: MemoryLayout<A1>.size) { pointer in
                #expect(compareValues(pointer.pointee, c) == false)
            }
        }
    }

}
