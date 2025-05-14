import Testing

extension ValueLayout: @retroactive CustomStringConvertible {

    static var trivial: ValueLayout {
        return unsafeBitCast(UnsafePointer<CUnsignedChar>(bitPattern: 1), to: ValueLayout.self)
    }

    public var description: String {
        return "\(rawValue)".replacing(/^0x0+/, with: "0x")
    }

}

extension Optional: @retroactive CustomStringConvertible where Wrapped == ValueLayout {

    public var description: String {
        switch self {
        case .none:
            return "0x00000000"
        case .some(let wrapped):
            return wrapped.description
        }
    }

}

let allOptions: [AGComparisonOptions] = [[], [._1], [._2], [._1, ._2]]

// The code being tested uses a shared global cache, hence this test suite may
// exhibit different behavior depending on whether a single test or the entire
// suite is run. To prevent this, avoid referencing metadata for a non-trivial
// type more than once within the entire suite.
@Suite(.serialized)
struct PrefetchCompareValuesTests {

    @Suite
    struct FundamentalTypeTests {

        @Test
        func layoutForNever() async {
            let layout0 = prefetchCompareValues(type: Metadata(Never.self), options: [], priority: 0)
            #expect(layout0 == nil)

            let layout1 = prefetchCompareValues(type: Metadata(Never.self), options: [._1], priority: 0)
            #expect(layout1 == nil)

            let layout2 = prefetchCompareValues(type: Metadata(Never.self), options: [._2], priority: 0)
            #expect(layout2 == nil)

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(Never.self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == Never, 0 bytes ==
                    (layout #:length 18 #:address \(String(describing: layout3))
                          (== #:size 0 #:type Never))

                    """
            )
        }

        @Test(arguments: allOptions)
        func layoutForVoid(with options: AGComparisonOptions) {
            let layout = prefetchCompareValues(type: Metadata(Void.self), options: options, priority: 0)
            #expect(layout == .trivial)
        }

        @Test
        func layoutForBool() async {
            let layout0 = prefetchCompareValues(type: Metadata(Bool.self), options: [], priority: 0)
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(type: Metadata(Bool.self), options: [._1], priority: 0)
            #expect(layout1 == .trivial)

            let layout2 = prefetchCompareValues(type: Metadata(Bool.self), options: [._2], priority: 0)
            #expect(layout2 == .trivial)

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(Bool.self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == Bool, 1 bytes ==
                    (layout #:length 18 #:address \(String(describing: layout3))
                          (== #:size 1 #:type Bool))

                    """
            )
        }

        @Test(
            arguments: [
                (Int.self, MemoryLayout<Int>.size),
                (Double.self, MemoryLayout<Double>.size),
                (Float.self, MemoryLayout<Float>.size),
            ] as [(Any.Type, Int)]
        )
        func layoutForNumeric(of type: Any.Type, size: Int) async {
            let layout0 = prefetchCompareValues(type: Metadata(type), options: [], priority: 0)
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(type: Metadata(type), options: [._1], priority: 0)
            #expect(layout1 == .trivial)

            let layout2 = prefetchCompareValues(type: Metadata(type), options: [._2], priority: 0)
            #expect(layout2 == .trivial)

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(type), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == \(type), \(size) bytes ==
                    (layout #:length 18 #:address \(String(describing: layout3))
                          (== #:size \(size) #:type \(type)))

                    """
            )
        }

        @Test
        func layoutForString() async {
            let layout0 = prefetchCompareValues(type: Metadata(String.self), options: [], priority: 0)
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(type: Metadata(String.self), options: [._1], priority: 0)
            #expect(layout1 == .trivial)

            var output2 = ""
            let layout2 = await printingStandardError(to: &output2) {
                prefetchCompareValues(type: Metadata(String.self), options: [._2], priority: 0)
            }
            #expect(layout2 != nil)
            #expect(
                output2 == """
                    == String, 16 bytes ==
                    (layout #:length 18 #:address \(String(describing: layout2))
                          (== #:size 16 #:type String))

                    """
            )

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(String.self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == String, 16 bytes ==
                    (layout #:length 18 #:address \(String(describing: layout3))
                          (== #:size 16 #:type String))

                    """
            )
        }

        @Test(arguments: allOptions)
        func layoutForStaticString(with options: AGComparisonOptions) {
            let layout = prefetchCompareValues(type: Metadata(StaticString.self), options: options, priority: 0)
            #expect(layout == .trivial)
        }

    }

    @Suite
    struct StructTests {

        @Test(arguments: allOptions)
        func layoutForEmptyStruct(with options: AGComparisonOptions) {
            struct EmptyStruct {}

            let layout = prefetchCompareValues(type: Metadata(EmptyStruct.self), options: options, priority: 0)
            #expect(layout == nil)
        }

        @Test("Layout for struct enclosing single element equals the layout of the enclosed element")
        func layoutForStructEnclosingSingleElement() async {
            struct StructEnclosingSingleElement {
                var property: Int
            }

            let layout0 = prefetchCompareValues(
                type: Metadata(StructEnclosingSingleElement.self),
                options: [],
                priority: 0
            )
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(
                type: Metadata(StructEnclosingSingleElement.self),
                options: [._1],
                priority: 0
            )
            #expect(layout1 == .trivial)

            let layout2 = prefetchCompareValues(
                type: Metadata(StructEnclosingSingleElement.self),
                options: [._2],
                priority: 0
            )
            #expect(layout2 == .trivial)

            let innerLayout = prefetchCompareValues(type: Metadata(Int.self), options: [._1, ._2], priority: 0)

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(
                    type: Metadata(StructEnclosingSingleElement.self),
                    options: [._1, ._2],
                    priority: 0
                )
            }
            #expect(layout3 == innerLayout)
        }

        @Test
        func layoutForTrivialStruct() async {
            struct TrivialStruct {
                var first: Int
                var second: Int
            }

            let layout0 = prefetchCompareValues(type: Metadata(TrivialStruct.self), options: [], priority: 0)
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(type: Metadata(TrivialStruct.self), options: [._1], priority: 0)
            #expect(layout1 == .trivial)

            let layout2 = prefetchCompareValues(type: Metadata(TrivialStruct.self), options: [._2], priority: 0)
            #expect(layout2 == .trivial)

            let _ = prefetchCompareValues(type: Metadata(Int.self), options: [._1, ._2], priority: 0)

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(TrivialStruct.self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == TrivialStruct, 16 bytes ==
                    (layout #:length 35 #:address \(String(describing: layout3))
                          (== #:size 8 #:type Int)
                          (== #:size 8 #:type Int))

                    """
            )
        }

        @Test
        func layoutForStructWithAlignedElement() async {
            struct StructWithAlignedElement {
                var int8: Int8
                var int64: Int64
            }

            var output0 = ""
            let layout0 = await printingStandardError(to: &output0) {
                prefetchCompareValues(type: Metadata(StructWithAlignedElement.self), options: [], priority: 0)
            }
            #expect(layout0 != nil)
            #expect(
                output0 == """
                    == StructWithAlignedElement, 16 bytes ==
                    (layout #:length 4 #:address \(String(describing: layout0))
                          (read 1)
                          (skip 7)
                          (read 8))

                    """
            )

            var output1 = ""
            let layout1 = await printingStandardError(to: &output1) {
                prefetchCompareValues(type: Metadata(StructWithAlignedElement.self), options: [._1], priority: 0)
            }
            #expect(layout1 != nil)
            #expect(
                output1 == """
                    == StructWithAlignedElement, 16 bytes ==
                    (layout #:length 4 #:address \(String(describing: layout1))
                          (read 1)
                          (skip 7)
                          (read 8))

                    """
            )

            var output2 = ""
            let layout2 = await printingStandardError(to: &output2) {
                prefetchCompareValues(type: Metadata(StructWithAlignedElement.self), options: [._2], priority: 0)
            }
            #expect(layout2 != nil)
            #expect(
                output2 == """
                    == StructWithAlignedElement, 16 bytes ==
                    (layout #:length 4 #:address \(String(describing: layout2))
                          (read 1)
                          (skip 7)
                          (read 8))

                    """
            )

            let _ = prefetchCompareValues(type: Metadata(Int8.self), options: [._1, ._2], priority: 0)
            let _ = prefetchCompareValues(type: Metadata(Int64.self), options: [._1, ._2], priority: 0)

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(StructWithAlignedElement.self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == StructWithAlignedElement, 16 bytes ==
                    (layout #:length 36 #:address \(String(describing: layout3))
                          (== #:size 1 #:type Int8)
                          (skip 7)
                          (== #:size 8 #:type Int64))

                    """
            )
        }

        @Test
        func layoutForStructWithComplexProperty() async {
            typealias ComplexType = Int64???
            struct StructWithComplexProperty {
                var int8: Int8  // force layout to be non-nil
                var complexProperty: ComplexType
            }

            let nestedLayout0 = prefetchCompareValues(type: Metadata(ComplexType.self), options: [], priority: 0)

            var output0 = ""
            let layout0 = await printingStandardError(to: &output0) {
                prefetchCompareValues(type: Metadata(StructWithComplexProperty.self), options: [], priority: 0)
            }
            #expect(layout0 != nil)
            #expect(
                output0 == """
                    == StructWithComplexProperty, 19 bytes ==
                    (layout #:length 13 #:address \(String(describing: layout0))
                          (read 1)
                          (skip 7)
                          (nested #:size 11 #:layout \(String(describing: nestedLayout0))))

                    """
            )

            let nestedLayout1 = prefetchCompareValues(type: Metadata(ComplexType.self), options: [._1], priority: 0)

            var output1 = ""
            let layout1 = await printingStandardError(to: &output1) {
                prefetchCompareValues(type: Metadata(StructWithComplexProperty.self), options: [._1], priority: 0)
            }
            #expect(layout1 != nil)
            #expect(
                output1 == """
                    == StructWithComplexProperty, 19 bytes ==
                    (layout #:length 13 #:address \(String(describing: layout1))
                          (read 1)
                          (skip 7)
                          (nested #:size 11 #:layout \(String(describing: nestedLayout1))))

                    """
            )

            let nestedLayout2 = prefetchCompareValues(type: Metadata(ComplexType.self), options: [._2], priority: 0)

            var output2 = ""
            let layout2 = await printingStandardError(to: &output2) {
                prefetchCompareValues(type: Metadata(StructWithComplexProperty.self), options: [._2], priority: 0)
            }
            #expect(layout2 != nil)
            #expect(
                output2 == """
                    == StructWithComplexProperty, 19 bytes ==
                    (layout #:length 13 #:address \(String(describing: layout2))
                          (read 1)
                          (skip 7)
                          (nested #:size 11 #:layout \(String(describing: nestedLayout2))))

                    """
            )

            let _ = prefetchCompareValues(type: Metadata(Int8.self), options: [._1, ._2], priority: 0)
            let _ = prefetchCompareValues(type: Metadata(ComplexType.self), options: [._1, ._2], priority: 0)

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(StructWithComplexProperty.self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == StructWithComplexProperty, 19 bytes ==
                    (layout #:length 36 #:address \(String(describing: layout3))
                          (== #:size 1 #:type Int8)
                          (skip 7)
                          (== #:size 11 #:type Optional<Optional<Optional<Int64>>>))

                    """
            )
        }

    }

    @Suite
    struct ClassTests {

        @Test(arguments: allOptions)
        func layoutForEmptyClass(with options: AGComparisonOptions) {
            class EmptyClass {}

            let layout = prefetchCompareValues(type: Metadata(EmptyClass.self), options: options, priority: 0)
            #expect(layout == nil)
        }

        @Test(arguments: allOptions)
        func layoutForTrivialClass(with options: AGComparisonOptions) {
            class TrivialClass {
                var property: Int = 0
            }

            let layout = prefetchCompareValues(type: Metadata(TrivialClass.self), options: options, priority: 0)
            #expect(layout == nil)
        }

        @Test(arguments: allOptions)
        func layoutForStructWithWeakVar(with options: AGComparisonOptions) {
            class EmptyClass {}
            struct StructWithWeakVar {
                weak var property: EmptyClass?
            }

            let layout = prefetchCompareValues(type: Metadata(StructWithWeakVar.self), options: options, priority: 0)
            #expect(layout == .trivial)
        }

    }

    @Suite
    struct EnumTests {

        @Test(arguments: allOptions)
        func layoutForEmptyEnum(with options: AGComparisonOptions) {
            enum EmptyEnum {}

            let layout = prefetchCompareValues(type: Metadata(EmptyEnum.self), options: options, priority: 0)
            #expect(layout == nil)
        }

        @Test
        func layoutForBasicEnum() async {
            enum BasicEnum {
                case first
                case second
            }

            let layout0 = prefetchCompareValues(type: Metadata(BasicEnum.self), options: [], priority: 0)
            #expect(layout0 == nil)

            let layout1 = prefetchCompareValues(type: Metadata(BasicEnum.self), options: [._1], priority: 0)
            #expect(layout1 == nil)

            let layout2 = prefetchCompareValues(type: Metadata(BasicEnum.self), options: [._2], priority: 0)
            #expect(layout2 == nil)

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(BasicEnum.self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == BasicEnum, 1 bytes ==
                    (layout #:length 18 #:address \(String(describing: layout3))
                          (== #:size 1 #:type BasicEnum))

                    """
            )
        }

        @Test
        func layoutForIntEnum() async {
            enum IntEnum: Int {
                case first = 1
                case second = 2
            }

            let layout0 = prefetchCompareValues(type: Metadata(IntEnum.self), options: [], priority: 0)
            #expect(layout0 == nil)

            let layout1 = prefetchCompareValues(type: Metadata(IntEnum.self), options: [._1], priority: 0)
            #expect(layout1 == nil)

            let layout2 = prefetchCompareValues(type: Metadata(IntEnum.self), options: [._2], priority: 0)
            #expect(layout2 == nil)

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(IntEnum.self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == IntEnum, 1 bytes ==
                    (layout #:length 18 #:address \(String(describing: layout3))
                          (== #:size 1 #:type IntEnum))

                    """
            )
        }

        @Test
        func layoutForTaggedUnionEnum() async {
            enum TaggedUnionEnum {
                case first
                case second(Int)
                case third(String)
            }

            var output0 = ""
            let layout0 = await printingStandardError(to: &output0) {
                prefetchCompareValues(type: Metadata(TaggedUnionEnum.self), options: [], priority: 0)
            }
            #expect(layout0 != nil)
            #expect(
                output0 == """
                    == TaggedUnionEnum, 17 bytes ==
                    (layout #:length 14 #:address \(String(describing: layout0))
                          (enum #:size 17 #:type TaggedUnionEnum
                              (case 0
                                  (read 8))
                              (case 1
                                  (read 16))))

                    """
            )

            var output1 = ""
            let layout1 = await printingStandardError(to: &output1) {
                prefetchCompareValues(type: Metadata(TaggedUnionEnum.self), options: [._1], priority: 0)
            }
            #expect(layout1 != nil)
            #expect(
                output1 == """
                    == TaggedUnionEnum, 17 bytes ==
                    (layout #:length 14 #:address \(String(describing: layout1))
                          (enum #:size 17 #:type TaggedUnionEnum
                              (case 0
                                  (read 8))
                              (case 1
                                  (read 16))))

                    """
            )

            let _ = prefetchCompareValues(type: Metadata(String.self), options: [._2], priority: 0)

            var output2 = ""
            let layout2 = await printingStandardError(to: &output2) {
                prefetchCompareValues(type: Metadata(TaggedUnionEnum.self), options: [._2], priority: 0)
            }
            #expect(layout2 != nil)
            #expect(
                output2 == """
                    == TaggedUnionEnum, 17 bytes ==
                    (layout #:length 30 #:address \(String(describing: layout2))
                          (enum #:size 17 #:type TaggedUnionEnum
                              (case 0
                                  (read 8))
                              (case 1
                                  (== #:size 16 #:type String))))

                    """
            )

            let _ = prefetchCompareValues(type: Metadata(Int.self), options: [._1, ._2], priority: 0)
            let _ = prefetchCompareValues(type: Metadata(String.self), options: [._1, ._2], priority: 0)

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(TaggedUnionEnum.self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == TaggedUnionEnum, 17 bytes ==
                    (layout #:length 46 #:address \(String(describing: layout3))
                          (enum #:size 17 #:type TaggedUnionEnum
                              (case 0
                                  (== #:size 8 #:type Int))
                              (case 1
                                  (== #:size 16 #:type String))))

                    """
            )
        }

        @Test
        func layoutForIndirectEnum() async {
            indirect enum IndirectEnum {
                case string(String)
                case indirectEnum(child: IndirectEnum)
            }

            var output0 = ""
            let layout0 = await printingStandardError(to: &output0) {
                prefetchCompareValues(type: Metadata(IndirectEnum.self), options: [], priority: 0)
            }
            #expect(layout0 != nil)
            #expect(
                output0 == """
                    == IndirectEnum, 8 bytes ==
                    (layout #:length 14 #:address \(String(describing: layout0))
                          (enum #:size 8 #:type IndirectEnum
                              (case 0
                                  (read 8))
                              (case 1
                                  (read 8))))

                    """
            )

            var output1 = ""
            let layout1 = await printingStandardError(to: &output1) {
                prefetchCompareValues(type: Metadata(IndirectEnum.self), options: [._1], priority: 0)
            }
            #expect(layout1 != nil)
            #expect(
                output1 == """
                    == IndirectEnum, 8 bytes ==
                    (layout #:length 46 #:address \(String(describing: layout1))
                          (enum #:size 8 #:type IndirectEnum
                              (case 0
                                  (indirect #:size 16 #:type String))
                              (case 1
                                  (indirect #:size 8 #:type (child: IndirectEnum)))))

                    """
            )

            var output2 = ""
            let layout2 = await printingStandardError(to: &output2) {
                prefetchCompareValues(type: Metadata(IndirectEnum.self), options: [._2], priority: 0)
            }
            #expect(layout2 != nil)
            #expect(
                output2 == """
                    == IndirectEnum, 8 bytes ==
                    (layout #:length 46 #:address \(String(describing: layout2))
                          (enum #:size 8 #:type IndirectEnum
                              (case 0
                                  (indirect #:size 16 #:type String))
                              (case 1
                                  (indirect #:size 8 #:type (child: IndirectEnum)))))

                    """
            )

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(IndirectEnum.self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == IndirectEnum, 8 bytes ==
                    (layout #:length 46 #:address \(String(describing: layout3))
                          (enum #:size 8 #:type IndirectEnum
                              (case 0
                                  (indirect #:size 16 #:type String))
                              (case 1
                                  (indirect #:size 8 #:type (child: IndirectEnum)))))

                    """
            )
        }

    }

    @Suite
    struct TupleTests {

        @Test
        func layoutForTuple() async {
            let layout0 = prefetchCompareValues(type: Metadata((Int, String).self), options: [], priority: 0)
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(type: Metadata((Int, String).self), options: [._1], priority: 0)
            #expect(layout1 == .trivial)

            let _ = prefetchCompareValues(type: Metadata(String.self), options: [._2], priority: 0)

            var output2 = ""
            let layout2 = await printingStandardError(to: &output2) {
                prefetchCompareValues(type: Metadata((Int, String).self), options: [._2], priority: 0)
            }
            #expect(layout2 != nil)
            #expect(
                output2 == """
                    == (Int, String), 24 bytes ==
                    (layout #:length 19 #:address \(String(describing: layout2))
                          (read 8)
                          (== #:size 16 #:type String))

                    """
            )

            let _ = prefetchCompareValues(type: Metadata(Int.self), options: [._1, ._2], priority: 0)
            let _ = prefetchCompareValues(type: Metadata(String.self), options: [._1, ._2], priority: 0)

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata((Int, String).self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == (Int, String), 24 bytes ==
                    (layout #:length 35 #:address \(String(describing: layout3))
                          (== #:size 8 #:type Int)
                          (== #:size 16 #:type String))

                    """
            )
        }

        @Test
        func layoutForTupleWithAlignedElement() async {
            var output0 = ""
            let layout0 = await printingStandardError(to: &output0) {
                prefetchCompareValues(type: Metadata((Int8, Int64).self), options: [], priority: 0)
            }
            #expect(layout0 != nil)
            #expect(
                output0 == """
                    == (Int8, Int64), 16 bytes ==
                    (layout #:length 4 #:address \(String(describing: layout0))
                          (read 1)
                          (skip 7)
                          (read 8))

                    """
            )

            var output1 = ""
            let layout1 = await printingStandardError(to: &output1) {
                prefetchCompareValues(type: Metadata((Int8, Int64).self), options: [._1], priority: 0)
            }
            #expect(layout1 != nil)
            #expect(
                output1 == """
                    == (Int8, Int64), 16 bytes ==
                    (layout #:length 4 #:address \(String(describing: layout1))
                          (read 1)
                          (skip 7)
                          (read 8))

                    """
            )

            var output2 = ""
            let layout2 = await printingStandardError(to: &output2) {
                prefetchCompareValues(type: Metadata((Int8, Int64).self), options: [._2], priority: 0)
            }
            #expect(layout2 != nil)
            #expect(
                output2 == """
                    == (Int8, Int64), 16 bytes ==
                    (layout #:length 4 #:address \(String(describing: layout2))
                          (read 1)
                          (skip 7)
                          (read 8))

                    """
            )

            let _ = prefetchCompareValues(type: Metadata(Int8.self), options: [._1, ._2], priority: 0)
            let _ = prefetchCompareValues(type: Metadata(Int64.self), options: [._1, ._2], priority: 0)

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata((Int8, Int64).self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == (Int8, Int64), 16 bytes ==
                    (layout #:length 36 #:address \(String(describing: layout3))
                          (== #:size 1 #:type Int8)
                          (skip 7)
                          (== #:size 8 #:type Int64))

                    """
            )
        }

    }

    @Suite
    struct CollectionTests {

        @Test
        func layoutForArray() async {
            let layout0 = prefetchCompareValues(type: Metadata(Array<Int>.self), options: [], priority: 0)
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(type: Metadata(Array<Int>.self), options: [._1], priority: 0)
            #expect(layout1 == .trivial)

            var output2 = ""
            let layout2 = await printingStandardError(to: &output2) {
                prefetchCompareValues(type: Metadata(Array<Int>.self), options: [._2], priority: 0)
            }
            #expect(layout2 != nil)
            #expect(
                output2 == """
                    == Array<Int>, 8 bytes ==
                    (layout #:length 18 #:address \(String(describing: layout2))
                          (== #:size 8 #:type Array<Int>))

                    """
            )

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(Array<Int>.self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == Array<Int>, 8 bytes ==
                    (layout #:length 18 #:address \(String(describing: layout3))
                          (== #:size 8 #:type Array<Int>))

                    """
            )
        }

        @Test(arguments: allOptions)
        func layoutForArrayOfNotEquatable(with options: AGComparisonOptions) {
            class NotEquatable {}

            let layout = prefetchCompareValues(type: Metadata(Array<NotEquatable>.self), options: options, priority: 0)
            #expect(layout == .trivial)
        }

        @Test
        func layoutForDictionary() async {
            let layout0 = prefetchCompareValues(type: Metadata(Dictionary<Int, Int>.self), options: [], priority: 0)
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(type: Metadata(Dictionary<Int, Int>.self), options: [._1], priority: 0)
            #expect(layout1 == .trivial)

            var output2 = ""
            let layout2 = await printingStandardError(to: &output2) {
                prefetchCompareValues(type: Metadata(Dictionary<Int, Int>.self), options: [._2], priority: 0)
            }
            #expect(layout2 != nil)
            #expect(
                output2 == """
                    == Dictionary<Int, Int>, 8 bytes ==
                    (layout #:length 18 #:address \(String(describing: layout2))
                          (== #:size 8 #:type Dictionary<Int, Int>))

                    """
            )

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(Dictionary<Int, Int>.self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == Dictionary<Int, Int>, 8 bytes ==
                    (layout #:length 18 #:address \(String(describing: layout3))
                          (== #:size 8 #:type Dictionary<Int, Int>))

                    """
            )
        }

        @Test(arguments: allOptions)
        func layoutForDictionaryOfNotEquatable(with options: AGComparisonOptions) {
            class NotEquatable {}

            let layout = prefetchCompareValues(
                type: Metadata(Dictionary<Int, NotEquatable>.self),
                options: options,
                priority: 0
            )
            #expect(layout == .trivial)
        }

        @Test
        func layoutForSet() async {
            let layout0 = prefetchCompareValues(type: Metadata(Set<Int>.self), options: [], priority: 0)
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(type: Metadata(Set<Int>.self), options: [._1], priority: 0)
            #expect(layout1 == .trivial)

            var output2 = ""
            let layout2 = await printingStandardError(to: &output2) {
                prefetchCompareValues(type: Metadata(Set<Int>.self), options: [._2], priority: 0)
            }
            #expect(layout2 != nil)
            #expect(
                output2 == """
                    == Set<Int>, 8 bytes ==
                    (layout #:length 18 #:address \(String(describing: layout2))
                          (== #:size 8 #:type Set<Int>))

                    """
            )

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(Set<Int>.self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == Set<Int>, 8 bytes ==
                    (layout #:length 18 #:address \(String(describing: layout3))
                          (== #:size 8 #:type Set<Int>))

                    """
            )
        }

    }

    @Suite
    struct EquatableTests {

        @Test
        func layoutForEquatableStruct() async {
            struct EquatableStruct: Equatable {
                var property: Int = 0
                static func == (lhs: EquatableStruct, rhs: EquatableStruct) -> Bool {
                    return false
                }
            }

            let layout0 = prefetchCompareValues(type: Metadata(EquatableStruct.self), options: [], priority: 0)
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(type: Metadata(EquatableStruct.self), options: [._1], priority: 0)
            #expect(layout1 == .trivial)

            let layout2 = prefetchCompareValues(type: Metadata(EquatableStruct.self), options: [._2], priority: 0)
            #expect(layout2 == .trivial)

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(EquatableStruct.self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == EquatableStruct, 8 bytes ==
                    (layout #:length 18 #:address \(String(describing: layout3))
                          (== #:size 8 #:type EquatableStruct))

                    """
            )
        }

        @Test
        func layoutForEquatableClass() async {
            class EquatableClass: Equatable {
                var property: Int = 0
                static func == (lhs: EquatableClass, rhs: EquatableClass) -> Bool {
                    return false
                }
            }

            let layout0 = prefetchCompareValues(type: Metadata(EquatableClass.self), options: [], priority: 0)
            #expect(layout0 == nil)

            let layout1 = prefetchCompareValues(type: Metadata(EquatableClass.self), options: [._1], priority: 0)
            #expect(layout1 == nil)

            var output2 = ""
            let layout2 = await printingStandardError(to: &output2) {
                prefetchCompareValues(type: Metadata(EquatableClass.self), options: [._2], priority: 0)
            }
            #expect(layout2 != nil)
            #expect(
                output2 == """
                    == EquatableClass, 8 bytes ==
                    (layout #:length 18 #:address \(String(describing: layout2))
                          (== #:size 8 #:type EquatableClass))

                    """
            )

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(EquatableClass.self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == EquatableClass, 8 bytes ==
                    (layout #:length 18 #:address \(String(describing: layout3))
                          (== #:size 8 #:type EquatableClass))

                    """
            )
        }

    }

    @Suite
    struct ExistentialTests {

        @Test
        func layoutForAny() async {
            let layout0 = prefetchCompareValues(type: Metadata(Any.self), options: [], priority: 0)
            #expect(layout0 == nil)

            var output1 = ""
            let layout1 = await printingStandardError(to: &output1) {
                prefetchCompareValues(type: Metadata(Any.self), options: [._1], priority: 0)
            }
            #expect(layout1 != nil)
            #expect(
                output1 == """
                    == Any, 32 bytes ==
                    (layout #:length 10 #:address \(String(describing: layout1))
                          (existential #:size 32 #:type Any))

                    """
            )

            var output2 = ""
            let layout2 = await printingStandardError(to: &output2) {
                prefetchCompareValues(type: Metadata(Any.self), options: [._2], priority: 0)
            }
            #expect(layout2 != nil)
            #expect(
                output2 == """
                    == Any, 32 bytes ==
                    (layout #:length 10 #:address \(String(describing: layout2))
                          (existential #:size 32 #:type Any))

                    """
            )

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(Any.self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == Any, 32 bytes ==
                    (layout #:length 10 #:address \(String(describing: layout3))
                          (existential #:size 32 #:type Any))

                    """
            )
        }

        @Test
        func layoutForAnyError() async {
            let layout0 = prefetchCompareValues(type: Metadata((any Error).self), options: [], priority: 0)
            #expect(layout0 == nil)

            var output1 = ""
            let layout1 = await printingStandardError(to: &output1) {
                prefetchCompareValues(type: Metadata((any Error).self), options: [._1], priority: 0)
            }
            #expect(layout1 != nil)
            #expect(
                output1 == """
                    == Error, 8 bytes ==
                    (layout #:length 10 #:address \(String(describing: layout1))
                          (existential #:size 8 #:type Error))

                    """
            )

            var output2 = ""
            let layout2 = await printingStandardError(to: &output2) {
                prefetchCompareValues(type: Metadata((any Error).self), options: [._2], priority: 0)
            }
            #expect(layout2 != nil)
            #expect(
                output2 == """
                    == Error, 8 bytes ==
                    (layout #:length 10 #:address \(String(describing: layout2))
                          (existential #:size 8 #:type Error))

                    """
            )

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata((any Error).self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == Error, 8 bytes ==
                    (layout #:length 10 #:address \(String(describing: layout3))
                          (existential #:size 8 #:type Error))

                    """
            )
        }

    }

    @Suite
    struct FunctionTests {

        @Test
        func layoutForFunction() async {
            typealias Function = () -> Void

            let layout0 = prefetchCompareValues(type: Metadata(Function.self), options: [], priority: 0)
            #expect(layout0 == nil)

            var output1 = ""
            let layout1 = await printingStandardError(to: &output1) {
                prefetchCompareValues(type: Metadata(Function.self), options: [._1], priority: 0)
            }
            #expect(layout1 != nil)
            #expect(
                output1 == """
                    == () -> (), 16 bytes ==
                    (layout #:length 3 #:address \(String(describing: layout1))
                          (read 8)
                          (capture-ref))

                    """
            )

            var output2 = ""
            let layout2 = await printingStandardError(to: &output2) {
                prefetchCompareValues(type: Metadata(Function.self), options: [._2], priority: 0)
            }
            #expect(layout2 != nil)
            #expect(
                output2 == """
                    == () -> (), 16 bytes ==
                    (layout #:length 3 #:address \(String(describing: layout2))
                          (read 8)
                          (capture-ref))

                    """
            )

            var output3 = ""
            let layout3 = await printingStandardError(to: &output3) {
                prefetchCompareValues(type: Metadata(Function.self), options: [._1, ._2], priority: 0)
            }
            #expect(layout3 != nil)
            #expect(
                output3 == """
                    == () -> (), 16 bytes ==
                    (layout #:length 3 #:address \(String(describing: layout3))
                          (read 8)
                          (capture-ref))

                    """
            )
        }

    }

}
