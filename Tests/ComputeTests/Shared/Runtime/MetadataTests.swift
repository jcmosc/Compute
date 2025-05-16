import Algorithms
import Testing

@Suite("Metadata tests")
struct MetadataTests {

    @Test(
        arguments: [
            (TestClass.self, "TestClass"),
            (TestStruct.self, "TestStruct"),
            (TestEnum.self, "TestEnum"),
            (TestTaggedEnum.self, "TestTaggedEnum"),
            (TestIndirectEnum.self, "TestIndirectEnum"),
            (TestOptionalClass.self, "Optional<TestClass>"),
            (TestOptionalStruct.self, "Optional<TestStruct>"),
            (TestForeignClass.self, "CFDateRef"),
            (TestTuple.self, "(String, Int)"),
            (TestFunction.self, "(String) -> Int"),
            (TestExistential.self, "Hashable"),
            (TestConstrainedExistential.self, "<<< invalid type >>>"),
            (TestComposedExistential.self, "CustomStringConvertible & Hashable"),
            (TestMetatype.self, "TestClass.Type"),
            (TestObjCClass.self, "NSDate"),
            (TestExistentialMetatype.self, "Hashable.Protocol"),
            (TestNamespace.TestNestedStruct.self, "TestNamespace.TestNestedStruct"),
            (
                TestGenericStruct<String>.TestNestedGenericStruct<Int>.self,
                "TestGenericStruct<String>.TestNestedGenericStruct<Int>"
            ),
        ] as [(Any.Type, String)]
    )
    func description(of type: Any.Type, equals expectedDescription: String) {
        #expect(Metadata(type).description == expectedDescription)
    }

    @Test(
        arguments: [
            (TestClass.self, .class),
            (TestStruct.self, .struct),
            (TestEnum.self, .enum),
            (TestTaggedEnum.self, .enum),
            (TestIndirectEnum.self, .enum),
            (TestOptionalClass.self, .optional),
            (TestOptionalStruct.self, .optional),
            (TestForeignClass.self, .none),
            (TestTuple.self, .tuple),
            (TestFunction.self, .function),
            (TestExistential.self, .existential),
            (TestConstrainedExistential.self, .none),
            (TestComposedExistential.self, .existential),
            (TestMetatype.self, .metatype),
            (TestObjCClass.self, .none),
            (TestExistentialMetatype.self, .metatype),
            (TestNamespace.TestNestedStruct.self, .struct),
            (
                TestGenericStruct<String>.TestNestedGenericStruct<Int>.self,
                .struct
            ),
        ] as [(Any.Type, Metadata.Kind)]
    )
    func kind(of type: Any.Type, equals expectedKind: Metadata.Kind) {
        #expect(Metadata(type).kind == expectedKind)
    }

    @Suite
    struct SignatureTests {

        @Test(
            "Metadata for nominal type has a valid signature",
            arguments: [
                (TestClass.self, true),
                (TestStruct.self, true),
                (TestEnum.self, true),
                (TestTaggedEnum.self, true),
                (TestIndirectEnum.self, true),
                (TestOptionalClass.self, true),
                (TestOptionalStruct.self, true),
                (TestForeignClass.self, false),
                (TestTuple.self, false),
                (TestFunction.self, false),
                (TestExistential.self, false),
                (TestConstrainedExistential.self, false),
                (TestComposedExistential.self, false),
                (TestMetatype.self, false),
                (TestObjCClass.self, false),
                (TestExistentialMetatype.self, false),
                (TestNamespace.TestNestedStruct.self, true),
                (
                    TestGenericStruct<String>.TestNestedGenericStruct<Int>.self,
                    true
                ),
            ] as [(Any.Type, Bool)]
        )
        func signature(of type: Any.Type, isValid: Bool) {
            if isValid {
                #expect(Metadata(type).signature != Signature())
            } else {
                #expect(Metadata(type).signature == Signature())
            }
        }

        func signaturesAreUnique() {
            var signatures: [Signature] = []

            signatures.append(Metadata(TestClass.self).signature)
            signatures.append(Metadata(TestStruct.self).signature)
            signatures.append(Metadata(TestEnum.self).signature)
            signatures.append(Metadata(TestTaggedEnum.self).signature)
            signatures.append(Metadata(TestIndirectEnum.self).signature)
            signatures.append(Metadata(TestOptionalClass.self).signature)
            signatures.append(Metadata(TestOptionalStruct.self).signature)
            signatures.append(Metadata(TestNamespace.TestNestedStruct.self).signature)
            signatures.append(Metadata(TestGenericStruct<String>.TestNestedGenericStruct<Int>.self).signature)

            #expect(signatures.count == 9)
            signatures.combinations(ofCount: 2).forEach { elements in
                #expect(elements[0] != elements[1])
            }
        }

    }

    @Suite
    struct DescriptorTests {

        @Test(
            "Metadata for nominal type has a descriptor",
            arguments: [
                (TestClass.self, true),
                (TestStruct.self, true),
                (TestEnum.self, true),
                (TestTaggedEnum.self, true),
                (TestIndirectEnum.self, true),
                (TestOptionalClass.self, true),
                (TestOptionalStruct.self, true),
                (TestForeignClass.self, false),
                (TestTuple.self, false),
                (TestFunction.self, false),
                (TestExistential.self, false),
                (TestConstrainedExistential.self, false),
                (TestComposedExistential.self, false),
                (TestMetatype.self, false),
                (TestObjCClass.self, false),
                (TestExistentialMetatype.self, false),
                (TestNamespace.TestNestedStruct.self, true),
                (
                    TestGenericStruct<String>.TestNestedGenericStruct<Int>.self,
                    true
                ),
            ] as [(Any.Type, Bool)]
        )
        func descriptor(of type: Any.Type, hasDescriptor: Bool) {
            if hasDescriptor {
                #expect(Metadata(type).descriptor != nil)
            } else {
                #expect(Metadata(type).descriptor == nil)
            }
        }

        @Test(
            "Metadata for non-class nominal type has a nominal descriptor",
            arguments: [
                (TestClass.self, false),
                (TestStruct.self, true),
                (TestEnum.self, true),
                (TestTaggedEnum.self, true),
                (TestIndirectEnum.self, true),
                (TestOptionalClass.self, true),
                (TestOptionalStruct.self, true),
                (TestForeignClass.self, false),
                (TestTuple.self, false),
                (TestFunction.self, false),
                (TestExistential.self, false),
                (TestConstrainedExistential.self, false),
                (TestComposedExistential.self, false),
                (TestMetatype.self, false),
                (TestObjCClass.self, false),
                (TestExistentialMetatype.self, false),
                (TestNamespace.TestNestedStruct.self, true),
                (
                    TestGenericStruct<String>.TestNestedGenericStruct<Int>.self,
                    true
                ),
            ] as [(Any.Type, Bool)]
        )
        func nominalDescriptor(of type: Any.Type, hasNominalDescriptor: Bool) {
            if hasNominalDescriptor {
                #expect(Metadata(type).nominalDescriptor != nil)
            } else {
                #expect(Metadata(type).nominalDescriptor == nil)
            }
        }

        @Test(
            "Metadata for nominal type has a nominal descriptor name",
            arguments: [
                (TestClass.self, nil),
                (TestStruct.self, "TestStruct"),
                (TestEnum.self, "TestEnum"),
                (TestTaggedEnum.self, "TestTaggedEnum"),
                (TestIndirectEnum.self, "TestIndirectEnum"),
                (TestOptionalClass.self, "Optional"),
                (TestOptionalStruct.self, "Optional"),
                (TestForeignClass.self, nil),
                (TestTuple.self, nil),
                (TestFunction.self, nil),
                (TestExistential.self, nil),
                (TestConstrainedExistential.self, nil),
                (TestComposedExistential.self, nil),
                (TestMetatype.self, nil),
                (TestObjCClass.self, nil),
                (TestExistentialMetatype.self, nil),
                (TestNamespace.TestNestedStruct.self, "TestNestedStruct"),
                (
                    TestGenericStruct<String>.TestNestedGenericStruct<Int>.self,
                    "TestNestedGenericStruct"
                ),
            ] as [(Any.Type, String?)]
        )
        func nominalDescriptorName(of type: Any.Type, equals expectedNominalDescriptorName: String?) {
            if let expectedNominalDescriptorName {
                let nominalDescriptorName = Metadata(type).nominalDescriptorName.map { String(cString: $0) }
                #expect(nominalDescriptorName == expectedNominalDescriptorName)
            } else {
                #expect(Metadata(type).nominalDescriptorName == nil)
            }
        }
    }

    @Suite
    struct GlobalForEachFieldTests {

        struct StructFields {
            var first: Int = 0
            var second: Float = 0.0
            var third: Double = 0.0
        }

        @Test
        func enumeratesStructFields() {
            var fields: [(String, Int, any Any.Type)] = []
            forEachField(of: StructFields.self) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
            }

            #expect(fields.count == 3)

            #expect(fields[0].0 == "first")
            #expect(fields[0].1 == 0)
            #expect(fields[0].2 as Any.Type == Int.self)
            #expect(fields[1].0 == "second")
            #expect(fields[1].1 == 8)
            #expect(fields[1].2 as Any.Type == Float.self)
            #expect(fields[2].0 == "third")
            #expect(fields[2].1 == 16)
            #expect(fields[2].2 as Any.Type == Double.self)
        }

        struct NestedStructFields {
            var nested: StructFields
        }

        @Test
        func doesNotEnumerateNestedStructFields() {
            var fields: [(String, Int, any Any.Type)] = []
            forEachField(of: NestedStructFields.self) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
            }

            #expect(fields.count == 1)

            #expect(fields[0].0 == "nested")
            #expect(fields[0].1 == 0)
            #expect(fields[0].2 as Any.Type == StructFields.self)
        }

        class ClassFields {
            var first: Int = 0
            var second: Float = 0.0
            var third: Double = 0.0
        }

        @Test
        func doesNotEnumerateClassFields() {
            var fields: [(String, Int, any Any.Type)] = []
            forEachField(of: ClassFields.self) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
            }

            #expect(fields.count == 0)
        }

        class NestedClassFields {
            var nested: ClassFields = ClassFields()
        }

        @Test
        func doesNotEnumerateNestedClassFields() {
            var fields: [(String, Int, any Any.Type)] = []
            forEachField(of: NestedClassFields.self) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
            }

            #expect(fields.count == 0)
        }

        enum EnumFields {
            case first
            case second(Int)
            case third(Double)
        }

        @Test
        func doesNotEnumerateEnumFields() {
            var fields: [(String, Int, any Any.Type)] = []
            forEachField(of: EnumFields.self) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
            }

            #expect(fields.count == 0)
        }

        enum NestedEnumFields {
            case nested(EnumFields)
        }

        @Test
        func doesNotEnumerateNestedEnumFields() {
            var fields: [(String, Int, any Any.Type)] = []
            forEachField(of: EnumFields.self) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
            }

            #expect(fields.count == 0)
        }

        typealias TupleFields = (Int, Float, Double)

        @Test
        func doesNotEnumeratesTupleFields() {
            var fields: [(String, Int, any Any.Type)] = []
            forEachField(of: TupleFields.self) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
            }

            #expect(fields.count == 0)
        }

    }

    @Suite
    struct ForEachFieldTests {

        struct StructFields {
            var first: Int = 0
            var second: Float = 0.0
            var third: Double = 0.0
        }

        @Test
        func enumeratesStructFields() {
            let options: AGTypeApplyOptions = []

            var fields: [(String, Int, any Any.Type)] = []
            let finished = Metadata(StructFields.self).forEachField(options: options) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
                return true
            }

            #expect(finished == true)
            #expect(fields.count == 3)

            #expect(fields[0].0 == "first")
            #expect(fields[0].1 == 0)
            #expect(fields[0].2 as Any.Type == Int.self)
            #expect(fields[1].0 == "second")
            #expect(fields[1].1 == 8)
            #expect(fields[1].2 as Any.Type == Float.self)
            #expect(fields[2].0 == "third")
            #expect(fields[2].1 == 16)
            #expect(fields[2].2 as Any.Type == Double.self)
        }

        @Test
        func doesNotEnumerateStructFieldsWhenEnumerateClassFieldsIsSpecified() {
            let options: AGTypeApplyOptions = [.enumerateClassFields]

            var fields: [(String, Int, any Any.Type)] = []
            let finished = Metadata(StructFields.self).forEachField(options: options) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
                return true
            }

            #expect(finished == false)
            #expect(fields.count == 0)
        }

        @Test
        func doesNotEnumerateStructFieldsWhenEnumerateEnumCasesIsSpecified() {
            let options: AGTypeApplyOptions = [.enumerateEnumCases]

            var fields: [(String, Int, any Any.Type)] = []
            let finished = Metadata(StructFields.self).forEachField(options: options) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
                return true
            }

            #expect(finished == false)
            #expect(fields.count == 0)
        }

        struct NestedStructFields {
            var nested: StructFields
        }

        @Test
        func doesNotEnumerateNestedStructFields() {
            let options: AGTypeApplyOptions = []

            var fields: [(String, Int, any Any.Type)] = []
            let finished = Metadata(NestedStructFields.self).forEachField(options: options) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
                return true
            }

            #expect(finished == true)
            #expect(fields.count == 1)

            #expect(fields[0].0 == "nested")
            #expect(fields[0].1 == 0)
            #expect(fields[0].2 as Any.Type == StructFields.self)
        }

        class ClassFields {
            var first: Int = 0
            var second: Float = 0.0
            var third: Double = 0.0
        }

        @Test
        func doesNotEnumeratesClassFields() {
            let options: AGTypeApplyOptions = []

            var fields: [(String, Int, any Any.Type)] = []
            let finished = Metadata(ClassFields.self).forEachField(options: options) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
                return true
            }

            #expect(finished == false)
            #expect(fields.count == 0)
        }

        @Test
        func enumeratesClassFieldsWhenEnumerateClassFieldsIsSpecified() {
            let options: AGTypeApplyOptions = [.enumerateClassFields]

            var fields: [(String, Int, any Any.Type)] = []
            let finished = Metadata(ClassFields.self).forEachField(options: options) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
                return true
            }

            #expect(finished == true)
            #expect(fields.count == 3)

            #expect(fields[0].0 == "first")
            #expect(fields[0].1 == 16)
            #expect(fields[0].2 as Any.Type == Int.self)
            #expect(fields[1].0 == "second")
            #expect(fields[1].1 == 24)
            #expect(fields[1].2 as Any.Type == Float.self)
            #expect(fields[2].0 == "third")
            #expect(fields[2].1 == 32)
            #expect(fields[2].2 as Any.Type == Double.self)
        }

        class NestedClassFields {
            var nested: ClassFields = ClassFields()
        }

        @Test
        func doesNotEnumerateNestedClassFields() {
            let options: AGTypeApplyOptions = [.enumerateClassFields]

            var fields: [(String, Int, any Any.Type)] = []
            let finished = Metadata(NestedClassFields.self).forEachField(options: options) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
                return true
            }

            #expect(finished == true)
            #expect(fields.count == 1)

            #expect(fields[0].0 == "nested")
            #expect(fields[0].1 == 16)
            #expect(fields[0].2 as Any.Type == ClassFields.self)
        }

        enum EnumFields {
            case first
            case second(Int)
            case third(Double)
        }

        @Test
        func doesNotEnumerateEnumFields() {
            let options: AGTypeApplyOptions = []

            var fields: [(String, Int, any Any.Type)] = []
            let finished = Metadata(EnumFields.self).forEachField(options: options) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
                return true
            }

            #expect(finished == false)
            #expect(fields.count == 0)
        }

        @Test
        func enumeratesEnumFieldsWhenEnumerateEnumCasesIsSpecified() {
            let options: AGTypeApplyOptions = [.enumerateEnumCases]

            var fields: [(String, Int, any Any.Type)] = []
            let finished = Metadata(EnumFields.self).forEachField(options: options) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
                return true
            }

            #expect(finished == true)
            #expect(fields.count == 2)

            #expect(fields[0].0 == "second")
            #expect(fields[0].1 == 0)
            #expect(fields[0].2 as Any.Type == Int.self)
            #expect(fields[1].0 == "third")
            #expect(fields[1].1 == 1)
            #expect(fields[1].2 as Any.Type == Double.self)
        }

        enum NestedEnumFields {
            case nested(EnumFields)
        }

        @Test
        func doesNotEnumerateNestedEnumFields() {
            let options: AGTypeApplyOptions = [.enumerateEnumCases]

            var fields: [(String, Int, any Any.Type)] = []
            let finished = Metadata(NestedEnumFields.self).forEachField(options: options) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
                return true
            }

            #expect(finished == true)
            #expect(fields.count == 1)

            #expect(fields[0].0 == "nested")
            #expect(fields[0].1 == 0)
            #expect(fields[0].2 as Any.Type == EnumFields.self)
        }

        typealias TupleFields = (Int, Float, Double)

        @Test
        func doesNotEnumeratesTupleFields() {
            let options: AGTypeApplyOptions = []

            var fields: [(String, Int, any Any.Type)] = []
            let finished = Metadata(TupleFields.self).forEachField(options: options) {
                fieldName,
                fieldOffset,
                fieldType in
                fields.append((String(cString: fieldName), fieldOffset, fieldType))
                return true
            }

            #expect(finished == false)
            #expect(fields.count == 0)
        }

    }

}
