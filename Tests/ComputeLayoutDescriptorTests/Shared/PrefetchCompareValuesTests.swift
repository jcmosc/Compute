import Testing

public struct ValueLayout: CustomStringConvertible, Equatable {

    let storage: UnsafePointer<CUnsignedChar>

    static var trivial: ValueLayout {
        return unsafeBitCast(UnsafePointer<CUnsignedChar>(bitPattern: 1), to: ValueLayout.self)
    }

    public var description: String {
        return "\(storage)".replacing(/^0x0+/, with: "0x")
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

func prefetchCompareValues<Value>(of type: Value.Type, options: ComparisonOptions, priority: Int)
    -> ValueLayout?
{

    guard
        let layout = __AGPrefetchCompareValues(
            Metadata(type),
            options,
            UInt32(priority)
        )
    else {
        return nil
    }
    return ValueLayout(storage: layout)
}

let allOptions: [ComparisonOptions] = [
    ComparisonOptions(mode: .bitwise),
    ComparisonOptions(mode: .equatableUnlessPOD),
    ComparisonOptions(mode: .equatableAlways),
]

@Suite(.serialized)
struct PrefetchCompareValuesTests {

    @Suite
    struct FundamentalTypeTests {

        @Test
        func layoutForNever() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            let layout0 = prefetchCompareValues(
                of: Never.self,
                options: ComparisonOptions(mode: .bitwise),
                priority: 0
            )
            #expect(layout0 == nil)

            let layout1 = prefetchCompareValues(
                of: Never.self,
                options: ComparisonOptions(mode: .indirect),
                priority: 0
            )
            #expect(layout1 == nil)

            let layout2 = prefetchCompareValues(
                of: Never.self,
                options: ComparisonOptions(mode: .equatableUnlessPOD),
                priority: 0
            )
            #expect(layout2 == nil)

            await #expect(processExitsWith: .success) {
                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: Never.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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
        }

        @Test(arguments: allOptions)
        func layoutForVoid(with options: ComparisonOptions) {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)

            let layout = prefetchCompareValues(of: Void.self, options: options, priority: 0)
            #expect(layout == .trivial)
        }

        @Test
        func layoutForBool() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            let layout0 = prefetchCompareValues(
                of: Bool.self,
                options: ComparisonOptions(mode: .bitwise),
                priority: 0
            )
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(
                of: Bool.self,
                options: ComparisonOptions(mode: .indirect),
                priority: 0
            )
            #expect(layout1 == .trivial)

            let layout2 = prefetchCompareValues(
                of: Bool.self,
                options: ComparisonOptions(mode: .equatableUnlessPOD),
                priority: 0
            )
            #expect(layout2 == .trivial)

            await #expect(processExitsWith: .success) {
                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: Bool.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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
        }

        @Test(
            arguments: [
                (Int.self, MemoryLayout<Int>.size),
                (Double.self, MemoryLayout<Double>.size),
                (Float.self, MemoryLayout<Float>.size),
            ] as [(Any.Type, Int)]
        )
        func layoutForNumericNonEquatable(of type: Any.Type, size: Int) async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            let layout0 = prefetchCompareValues(of: type, options: ComparisonOptions(mode: .bitwise), priority: 0)
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(of: type, options: ComparisonOptions(mode: .indirect), priority: 0)
            #expect(layout1 == .trivial)

            let layout2 = prefetchCompareValues(
                of: type,
                options: ComparisonOptions(mode: .equatableUnlessPOD),
                priority: 0
            )
            #expect(layout2 == .trivial)
        }

        @Test
        func layoutForIntEquatable() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            await #expect(processExitsWith: .success) {
                var output = ""
                let layout = await reprintingStandardError(to: &output) {
                    prefetchCompareValues(of: Int.self, options: ComparisonOptions(mode: .equatableAlways), priority: 0)
                }
                #expect(layout != nil)
                #expect(
                    output == """
                        == Int, 8 bytes ==
                        (layout #:length 18 #:address \(String(describing: layout))
                           (== #:size 8 #:type Int))

                        """
                )
            }
        }

        @Test
        func layoutForDoubleEquatable() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            await #expect(processExitsWith: .success) {
                var output = ""
                let layout = await reprintingStandardError(to: &output) {
                    prefetchCompareValues(
                        of: Double.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
                }
                #expect(layout != nil)
                #expect(
                    output == """
                        == Double, 8 bytes ==
                        (layout #:length 18 #:address \(String(describing: layout))
                           (== #:size 8 #:type Double))

                        """
                )
            }
        }

        @Test
        func layoutForFloatEquatable() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            await #expect(processExitsWith: .success) {
                var output = ""
                let layout = await reprintingStandardError(to: &output) {
                    prefetchCompareValues(
                        of: Float.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
                }
                #expect(layout != nil)
                #expect(
                    output == """
                        == Float, 4 bytes ==
                        (layout #:length 18 #:address \(String(describing: layout))
                           (== #:size 4 #:type Float))

                        """
                )
            }
        }

        @Test
        func layoutForString() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            let layout0 = prefetchCompareValues(
                of: String.self,
                options: ComparisonOptions(mode: .bitwise),
                priority: 0
            )
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(
                of: String.self,
                options: ComparisonOptions(mode: .indirect),
                priority: 0
            )
            #expect(layout1 == .trivial)

            await #expect(processExitsWith: .success) {
                var output2 = ""
                let layout2 = await reprintingStandardError(to: &output2) {
                    prefetchCompareValues(
                        of: String.self,
                        options: ComparisonOptions(mode: .equatableUnlessPOD),
                        priority: 0
                    )
                }
                #expect(layout2 != nil)
                #expect(
                    output2 == """
                        == String, 16 bytes ==
                        (layout #:length 18 #:address \(String(describing: layout2))
                           (== #:size 16 #:type String))

                        """
                )
            }

            await #expect(processExitsWith: .success) {
                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: String.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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
        }

        @Test(arguments: allOptions)
        func layoutForStaticString(with options: ComparisonOptions) {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)

            let layout = prefetchCompareValues(of: StaticString.self, options: options, priority: 0)
            #expect(layout == .trivial)
        }

    }

    @Suite
    struct StructTests {

        @Test(arguments: allOptions)
        func layoutForEmptyStruct(with options: ComparisonOptions) {
            struct EmptyStruct {}

            let layout = prefetchCompareValues(of: EmptyStruct.self, options: options, priority: 0)
            #expect(layout == nil)
        }

        @Test("Layout for struct enclosing single element equals the layout of the enclosed element")
        func layoutForStructEnclosingSingleElement() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)

            struct StructEnclosingSingleElement {
                var property: Int
            }

            let layout0 = prefetchCompareValues(
                of: StructEnclosingSingleElement.self,
                options: ComparisonOptions(mode: .bitwise),
                priority: 0
            )
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(
                of: StructEnclosingSingleElement.self,
                options: ComparisonOptions(mode: .indirect),
                priority: 0
            )
            #expect(layout1 == .trivial)

            let layout2 = prefetchCompareValues(
                of: StructEnclosingSingleElement.self,
                options: ComparisonOptions(mode: .equatableUnlessPOD),
                priority: 0
            )
            #expect(layout2 == .trivial)

            let innerLayout = prefetchCompareValues(
                of: Int.self,
                options: ComparisonOptions(mode: .equatableAlways),
                priority: 0
            )

            var output3 = ""
            let layout3 = await reprintingStandardError(to: &output3) {
                prefetchCompareValues(
                    of: StructEnclosingSingleElement.self,
                    options: ComparisonOptions(mode: .equatableAlways),
                    priority: 0
                )
            }
            #expect(layout3 == innerLayout)
        }

        @Test
        func layoutForTrivialStruct() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            struct TrivialStruct {
                var first: Int
                var second: Int
            }

            let layout0 = prefetchCompareValues(
                of: TrivialStruct.self,
                options: ComparisonOptions(mode: .bitwise),
                priority: 0
            )
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(
                of: TrivialStruct.self,
                options: ComparisonOptions(mode: .indirect),
                priority: 0
            )
            #expect(layout1 == .trivial)

            let layout2 = prefetchCompareValues(
                of: TrivialStruct.self,
                options: ComparisonOptions(mode: .equatableUnlessPOD),
                priority: 0
            )
            #expect(layout2 == .trivial)

            await #expect(processExitsWith: .success) {
                let _ = prefetchCompareValues(
                    of: Int.self,
                    options: ComparisonOptions(mode: .equatableAlways),
                    priority: 0
                )

                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: TrivialStruct.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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
        }

        @Test
        func layoutForStructWithAlignedElement() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            struct StructWithAlignedElement {
                var int8: Int8
                var int64: Int64
            }

            await #expect(processExitsWith: .success) {
                var output0 = ""
                let layout0 = await reprintingStandardError(to: &output0) {
                    prefetchCompareValues(
                        of: StructWithAlignedElement.self,
                        options: ComparisonOptions(mode: .bitwise),
                        priority: 0
                    )
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
            }

            await #expect(processExitsWith: .success) {
                var output1 = ""
                let layout1 = await reprintingStandardError(to: &output1) {
                    prefetchCompareValues(
                        of: StructWithAlignedElement.self,
                        options: ComparisonOptions(mode: .indirect),
                        priority: 0
                    )
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
            }

            await #expect(processExitsWith: .success) {
                var output2 = ""
                let layout2 = await reprintingStandardError(to: &output2) {
                    prefetchCompareValues(
                        of: StructWithAlignedElement.self,
                        options: ComparisonOptions(mode: .equatableUnlessPOD),
                        priority: 0
                    )
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
            }

            await #expect(processExitsWith: .success) {
                let _ = prefetchCompareValues(
                    of: Int8.self,
                    options: ComparisonOptions(mode: .equatableAlways),
                    priority: 0
                )
                let _ = prefetchCompareValues(
                    of: Int64.self,
                    options: ComparisonOptions(mode: .equatableAlways),
                    priority: 0
                )

                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: StructWithAlignedElement.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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
        }

        @Test
        func layoutForStructWithComplexProperty() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            typealias ComplexType = Int64???
            struct StructWithComplexProperty {
                var int8: Int8  // force layout to be non-nil
                var complexProperty: ComplexType
            }

            await #expect(processExitsWith: .success) {
                let nestedLayout0 = prefetchCompareValues(
                    of: ComplexType.self,
                    options: ComparisonOptions(mode: .bitwise),
                    priority: 0
                )

                var output0 = ""
                let layout0 = await reprintingStandardError(to: &output0) {
                    prefetchCompareValues(
                        of: StructWithComplexProperty.self,
                        options: ComparisonOptions(mode: .bitwise),
                        priority: 0
                    )
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
            }

            await #expect(processExitsWith: .success) {
                let nestedLayout1 = prefetchCompareValues(
                    of: ComplexType.self,
                    options: ComparisonOptions(mode: .indirect),
                    priority: 0
                )

                var output1 = ""
                let layout1 = await reprintingStandardError(to: &output1) {
                    prefetchCompareValues(
                        of: StructWithComplexProperty.self,
                        options: ComparisonOptions(mode: .indirect),
                        priority: 0
                    )
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
            }

            await #expect(processExitsWith: .success) {
                let nestedLayout2 = prefetchCompareValues(
                    of: ComplexType.self,
                    options: ComparisonOptions(mode: .equatableUnlessPOD),
                    priority: 0
                )

                var output2 = ""
                let layout2 = await reprintingStandardError(to: &output2) {
                    prefetchCompareValues(
                        of: StructWithComplexProperty.self,
                        options: ComparisonOptions(mode: .equatableUnlessPOD),
                        priority: 0
                    )
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
            }

            await #expect(processExitsWith: .success) {
                let _ = prefetchCompareValues(
                    of: Int8.self,
                    options: ComparisonOptions(mode: .equatableAlways),
                    priority: 0
                )
                let _ = prefetchCompareValues(
                    of: ComplexType.self,
                    options: ComparisonOptions(mode: .equatableAlways),
                    priority: 0
                )

                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: StructWithComplexProperty.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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

    }

    @Suite
    struct ClassTests {

        @Test(arguments: allOptions)
        func layoutForEmptyClass(with options: ComparisonOptions) {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)

            class EmptyClass {}

            let layout = prefetchCompareValues(of: EmptyClass.self, options: options, priority: 0)
            #expect(layout == nil)
        }

        @Test(arguments: allOptions)
        func layoutForTrivialClass(with options: ComparisonOptions) {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)

            class TrivialClass {
                var property: Int = 0
            }

            let layout = prefetchCompareValues(of: TrivialClass.self, options: options, priority: 0)
            #expect(layout == nil)
        }

        @Test(arguments: allOptions)
        func layoutForStructWithWeakVar(with options: ComparisonOptions) {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)

            class EmptyClass {}
            struct StructWithWeakVar {
                weak var property: EmptyClass?
            }

            let layout = prefetchCompareValues(of: StructWithWeakVar.self, options: options, priority: 0)
            #expect(layout == .trivial)
        }

    }

    @Suite
    struct EnumTests {

        @Test(arguments: allOptions)
        func layoutForEmptyEnum(with options: ComparisonOptions) {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)

            enum EmptyEnum {}

            let layout = prefetchCompareValues(of: EmptyEnum.self, options: options, priority: 0)
            #expect(layout == nil)
        }

        @Test
        func layoutForBasicEnum() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            enum BasicEnum {
                case first
                case second
            }

            let layout0 = prefetchCompareValues(
                of: BasicEnum.self,
                options: ComparisonOptions(mode: .bitwise),
                priority: 0
            )
            #expect(layout0 == nil)

            let layout1 = prefetchCompareValues(
                of: BasicEnum.self,
                options: ComparisonOptions(mode: .indirect),
                priority: 0
            )
            #expect(layout1 == nil)

            let layout2 = prefetchCompareValues(
                of: BasicEnum.self,
                options: ComparisonOptions(mode: .equatableUnlessPOD),
                priority: 0
            )
            #expect(layout2 == nil)

            await #expect(processExitsWith: .success) {
                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: BasicEnum.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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
        }

        @Test
        func layoutForIntEnum() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            enum IntEnum: Int {
                case first = 1
                case second = 2
            }

            let layout0 = prefetchCompareValues(
                of: IntEnum.self,
                options: ComparisonOptions(mode: .bitwise),
                priority: 0
            )
            #expect(layout0 == nil)

            let layout1 = prefetchCompareValues(
                of: IntEnum.self,
                options: ComparisonOptions(mode: .indirect),
                priority: 0
            )
            #expect(layout1 == nil)

            let layout2 = prefetchCompareValues(
                of: IntEnum.self,
                options: ComparisonOptions(mode: .equatableUnlessPOD),
                priority: 0
            )
            #expect(layout2 == nil)

            await #expect(processExitsWith: .success) {
                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: IntEnum.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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
        }

        @Test
        func layoutForTaggedUnionEnum() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            enum TaggedUnionEnum {
                case first
                case second(Int)
                case third(String)
            }

            await #expect(processExitsWith: .success) {
                var output0 = ""
                let layout0 = await reprintingStandardError(to: &output0) {
                    prefetchCompareValues(
                        of: TaggedUnionEnum.self,
                        options: ComparisonOptions(mode: .bitwise),
                        priority: 0
                    )
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
            }

            await #expect(processExitsWith: .success) {
                var output1 = ""
                let layout1 = await reprintingStandardError(to: &output1) {
                    prefetchCompareValues(
                        of: TaggedUnionEnum.self,
                        options: ComparisonOptions(mode: .indirect),
                        priority: 0
                    )
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
            }

            await #expect(processExitsWith: .success) {
                let _ = prefetchCompareValues(
                    of: String.self,
                    options: ComparisonOptions(mode: .equatableUnlessPOD),
                    priority: 0
                )

                var output2 = ""
                let layout2 = await reprintingStandardError(to: &output2) {
                    prefetchCompareValues(
                        of: TaggedUnionEnum.self,
                        options: ComparisonOptions(mode: .equatableUnlessPOD),
                        priority: 0
                    )
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
            }

            await #expect(processExitsWith: .success) {
                let _ = prefetchCompareValues(
                    of: Int.self,
                    options: ComparisonOptions(mode: .equatableAlways),
                    priority: 0
                )
                let _ = prefetchCompareValues(
                    of: String.self,
                    options: ComparisonOptions(mode: .equatableAlways),
                    priority: 0
                )

                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: TaggedUnionEnum.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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
        }

        @Test
        func layoutForIndirectEnum() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            indirect enum IndirectEnum {
                case string(String)
                case indirectEnum(child: IndirectEnum)
            }

            await #expect(processExitsWith: .success) {
                var output0 = ""
                let layout0 = await reprintingStandardError(to: &output0) {
                    prefetchCompareValues(
                        of: IndirectEnum.self,
                        options: ComparisonOptions(mode: .bitwise),
                        priority: 0
                    )
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
            }

            await #expect(processExitsWith: .success) {
                var output1 = ""
                let layout1 = await reprintingStandardError(to: &output1) {
                    prefetchCompareValues(
                        of: IndirectEnum.self,
                        options: ComparisonOptions(mode: .indirect),
                        priority: 0
                    )
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
            }

            await #expect(processExitsWith: .success) {
                var output2 = ""
                let layout2 = await reprintingStandardError(to: &output2) {
                    prefetchCompareValues(
                        of: IndirectEnum.self,
                        options: ComparisonOptions(mode: .equatableUnlessPOD),
                        priority: 0
                    )
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
            }

            await #expect(processExitsWith: .success) {
                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: IndirectEnum.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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

    }

    @Suite
    struct TupleTests {

        @Test
        func layoutForTuple() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            let layout0 = prefetchCompareValues(
                of: (Int, String).self,
                options: ComparisonOptions(mode: .bitwise),
                priority: 0
            )
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(
                of: (Int, String).self,
                options: ComparisonOptions(mode: .indirect),
                priority: 0
            )
            #expect(layout1 == .trivial)

            await #expect(processExitsWith: .success) {
                let _ = prefetchCompareValues(
                    of: String.self,
                    options: ComparisonOptions(mode: .equatableUnlessPOD),
                    priority: 0
                )

                var output2 = ""
                let layout2 = await reprintingStandardError(to: &output2) {
                    prefetchCompareValues(
                        of: (Int, String).self,
                        options: ComparisonOptions(mode: .equatableUnlessPOD),
                        priority: 0
                    )
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
            }

            await #expect(processExitsWith: .success) {
                let _ = prefetchCompareValues(
                    of: Int.self,
                    options: ComparisonOptions(mode: .equatableAlways),
                    priority: 0
                )
                let _ = prefetchCompareValues(
                    of: String.self,
                    options: ComparisonOptions(mode: .equatableAlways),
                    priority: 0
                )

                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: (Int, String).self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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
        }

        @Test
        func layoutForTupleWithAlignedElement() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            await #expect(processExitsWith: .success) {
                var output0 = ""
                let layout0 = await reprintingStandardError(to: &output0) {
                    prefetchCompareValues(
                        of: (Int8, Int64).self,
                        options: ComparisonOptions(mode: .bitwise),
                        priority: 0
                    )
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
            }

            await #expect(processExitsWith: .success) {
                var output1 = ""
                let layout1 = await reprintingStandardError(to: &output1) {
                    prefetchCompareValues(
                        of: (Int8, Int64).self,
                        options: ComparisonOptions(mode: .indirect),
                        priority: 0
                    )
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
            }

            await #expect(processExitsWith: .success) {
                var output2 = ""
                let layout2 = await reprintingStandardError(to: &output2) {
                    prefetchCompareValues(
                        of: (Int8, Int64).self,
                        options: ComparisonOptions(mode: .equatableUnlessPOD),
                        priority: 0
                    )
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
            }

            await #expect(processExitsWith: .success) {
                let _ = prefetchCompareValues(
                    of: Int8.self,
                    options: ComparisonOptions(mode: .equatableAlways),
                    priority: 0
                )
                let _ = prefetchCompareValues(
                    of: Int64.self,
                    options: ComparisonOptions(mode: .equatableAlways),
                    priority: 0
                )

                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: (Int8, Int64).self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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

    }

    @Suite
    struct CollectionTests {

        @Test
        func layoutForArray() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            let layout0 = prefetchCompareValues(
                of: Array<Int>.self,
                options: ComparisonOptions(mode: .bitwise),
                priority: 0
            )
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(
                of: Array<Int>.self,
                options: ComparisonOptions(mode: .indirect),
                priority: 0
            )
            #expect(layout1 == .trivial)

            await #expect(processExitsWith: .success) {
                var output2 = ""
                let layout2 = await reprintingStandardError(to: &output2) {
                    prefetchCompareValues(
                        of: Array<Int>.self,
                        options: ComparisonOptions(mode: .equatableUnlessPOD),
                        priority: 0
                    )
                }
                #expect(layout2 != nil)
                #expect(
                    output2 == """
                        == Array<Int>, 8 bytes ==
                        (layout #:length 18 #:address \(String(describing: layout2))
                           (== #:size 8 #:type Array<Int>))

                        """
                )
            }

            await #expect(processExitsWith: .success) {
                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: Array<Int>.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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
        }

        @Test(arguments: allOptions)
        func layoutForArrayOfNotEquatable(with options: ComparisonOptions) {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)

            class NotEquatable {}

            let layout = prefetchCompareValues(of: Array<NotEquatable>.self, options: options, priority: 0)
            #expect(layout == .trivial)
        }

        @Test
        func layoutForDictionary() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            let layout0 = prefetchCompareValues(
                of: Dictionary<Int, Int>.self,
                options: ComparisonOptions(mode: .bitwise),
                priority: 0
            )
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(
                of: Dictionary<Int, Int>.self,
                options: ComparisonOptions(mode: .indirect),
                priority: 0
            )
            #expect(layout1 == .trivial)

            await #expect(processExitsWith: .success) {
                var output2 = ""
                let layout2 = await reprintingStandardError(to: &output2) {
                    prefetchCompareValues(
                        of: Dictionary<Int, Int>.self,
                        options: ComparisonOptions(mode: .equatableUnlessPOD),
                        priority: 0
                    )
                }
                #expect(layout2 != nil)
                #expect(
                    output2 == """
                        == Dictionary<Int, Int>, 8 bytes ==
                        (layout #:length 18 #:address \(String(describing: layout2))
                           (== #:size 8 #:type Dictionary<Int, Int>))

                        """
                )
            }

            await #expect(processExitsWith: .success) {
                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: Dictionary<Int, Int>.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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
        }

        @Test(arguments: allOptions)
        func layoutForDictionaryOfNotEquatable(with options: ComparisonOptions) {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)

            class NotEquatable {}

            let layout = prefetchCompareValues(
                of: Dictionary<Int, NotEquatable>.self,
                options: options,
                priority: 0
            )
            #expect(layout == .trivial)
        }

        @Test
        func layoutForSet() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            let layout0 = prefetchCompareValues(
                of: Set<Int>.self,
                options: ComparisonOptions(mode: .bitwise),
                priority: 0
            )
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(
                of: Set<Int>.self,
                options: ComparisonOptions(mode: .indirect),
                priority: 0
            )
            #expect(layout1 == .trivial)

            await #expect(processExitsWith: .success) {
                var output2 = ""
                let layout2 = await reprintingStandardError(to: &output2) {
                    prefetchCompareValues(
                        of: Set<Int>.self,
                        options: ComparisonOptions(mode: .equatableUnlessPOD),
                        priority: 0
                    )
                }
                #expect(layout2 != nil)
                #expect(
                    output2 == """
                        == Set<Int>, 8 bytes ==
                        (layout #:length 18 #:address \(String(describing: layout2))
                           (== #:size 8 #:type Set<Int>))

                        """
                )
            }

            await #expect(processExitsWith: .success) {
                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: Set<Int>.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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

    }

    @Suite
    struct EquatableTests {

        @Test
        func layoutForEquatableStruct() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            struct EquatableStruct: Equatable {
                var property: Int = 0
                static func == (lhs: EquatableStruct, rhs: EquatableStruct) -> Bool {
                    return false
                }
            }

            let layout0 = prefetchCompareValues(
                of: EquatableStruct.self,
                options: ComparisonOptions(mode: .bitwise),
                priority: 0
            )
            #expect(layout0 == .trivial)

            let layout1 = prefetchCompareValues(
                of: EquatableStruct.self,
                options: ComparisonOptions(mode: .indirect),
                priority: 0
            )
            #expect(layout1 == .trivial)

            let layout2 = prefetchCompareValues(
                of: EquatableStruct.self,
                options: ComparisonOptions(mode: .equatableUnlessPOD),
                priority: 0
            )
            #expect(layout2 == .trivial)

            await #expect(processExitsWith: .success) {
                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: EquatableStruct.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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
        }

        @Test
        func layoutForEquatableClass() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            class EquatableClass: Equatable {
                var property: Int = 0
                static func == (lhs: EquatableClass, rhs: EquatableClass) -> Bool {
                    return false
                }
            }

            let layout0 = prefetchCompareValues(
                of: EquatableClass.self,
                options: ComparisonOptions(mode: .bitwise),
                priority: 0
            )
            #expect(layout0 == nil)

            let layout1 = prefetchCompareValues(
                of: EquatableClass.self,
                options: ComparisonOptions(mode: .indirect),
                priority: 0
            )
            #expect(layout1 == nil)

            await #expect(processExitsWith: .success) {
                var output2 = ""
                let layout2 = await reprintingStandardError(to: &output2) {
                    prefetchCompareValues(
                        of: EquatableClass.self,
                        options: ComparisonOptions(mode: .equatableUnlessPOD),
                        priority: 0
                    )
                }
                #expect(layout2 != nil)
                #expect(
                    output2 == """
                        == EquatableClass, 8 bytes ==
                        (layout #:length 18 #:address \(String(describing: layout2))
                           (== #:size 8 #:type EquatableClass))

                        """
                )
            }

            await #expect(processExitsWith: .success) {
                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: EquatableClass.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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

    }

    @Suite
    struct ExistentialTests {

        @Test
        func layoutForAny() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            let layout0 = prefetchCompareValues(of: Any.self, options: ComparisonOptions(mode: .bitwise), priority: 0)
            #expect(layout0 == nil)

            await #expect(processExitsWith: .success) {
                var output1 = ""
                let layout1 = await reprintingStandardError(to: &output1) {
                    prefetchCompareValues(of: Any.self, options: ComparisonOptions(mode: .indirect), priority: 0)
                }
                #expect(layout1 != nil)
                #expect(
                    output1 == """
                        == Any, 32 bytes ==
                        (layout #:length 10 #:address \(String(describing: layout1))
                           (existential #:size 32 #:type Any))

                        """
                )
            }

            await #expect(processExitsWith: .success) {
                var output2 = ""
                let layout2 = await reprintingStandardError(to: &output2) {
                    prefetchCompareValues(
                        of: Any.self,
                        options: ComparisonOptions(mode: .equatableUnlessPOD),
                        priority: 0
                    )
                }
                #expect(layout2 != nil)
                #expect(
                    output2 == """
                        == Any, 32 bytes ==
                        (layout #:length 10 #:address \(String(describing: layout2))
                           (existential #:size 32 #:type Any))

                        """
                )
            }

            var output3 = ""
            let layout3 = await reprintingStandardError(to: &output3) {
                prefetchCompareValues(of: Any.self, options: ComparisonOptions(mode: .equatableAlways), priority: 0)
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
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            let layout0 = prefetchCompareValues(
                of: (any Error).self,
                options: ComparisonOptions(mode: .bitwise),
                priority: 0
            )
            #expect(layout0 == nil)

            await #expect(processExitsWith: .success) {
                var output1 = ""
                let layout1 = await reprintingStandardError(to: &output1) {
                    prefetchCompareValues(
                        of: (any Error).self,
                        options: ComparisonOptions(mode: .indirect),
                        priority: 0
                    )
                }
                #expect(layout1 != nil)
                #expect(
                    output1 == """
                        == Error, 8 bytes ==
                        (layout #:length 10 #:address \(String(describing: layout1))
                           (existential #:size 8 #:type Error))

                        """
                )
            }

            await #expect(processExitsWith: .success) {
                var output2 = ""
                let layout2 = await reprintingStandardError(to: &output2) {
                    prefetchCompareValues(
                        of: (any Error).self,
                        options: ComparisonOptions(mode: .equatableUnlessPOD),
                        priority: 0
                    )
                }
                #expect(layout2 != nil)
                #expect(
                    output2 == """
                        == Error, 8 bytes ==
                        (layout #:length 10 #:address \(String(describing: layout2))
                           (existential #:size 8 #:type Error))

                        """
                )
            }

            await #expect(processExitsWith: .success) {
                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: (any Error).self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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

    }

    @Suite
    struct FunctionTests {

        @Test
        func layoutForFunction() async {
            setenv("AG_PREFETCH_LAYOUTS", "1", 1)
            setenv("AG_ASYNC_LAYOUTS", "0", 1)
            setenv("AG_PRINT_LAYOUTS", "1", 1)

            typealias Function = () -> Void

            let layout0 = prefetchCompareValues(
                of: Function.self,
                options: ComparisonOptions(mode: .bitwise),
                priority: 0
            )
            #expect(layout0 == nil)

            await #expect(processExitsWith: .success) {
                var output1 = ""
                let layout1 = await reprintingStandardError(to: &output1) {
                    prefetchCompareValues(of: Function.self, options: ComparisonOptions(mode: .indirect), priority: 0)
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
            }

            await #expect(processExitsWith: .success) {
                var output2 = ""
                let layout2 = await reprintingStandardError(to: &output2) {
                    prefetchCompareValues(
                        of: Function.self,
                        options: ComparisonOptions(mode: .equatableUnlessPOD),
                        priority: 0
                    )
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
            }

            await #expect(processExitsWith: .success) {
                var output3 = ""
                let layout3 = await reprintingStandardError(to: &output3) {
                    prefetchCompareValues(
                        of: Function.self,
                        options: ComparisonOptions(mode: .equatableAlways),
                        priority: 0
                    )
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

}
