import Algorithms
import Compute
import Testing

@Suite("Metadata tests")
struct MetadataTests {

    class CustomClass {}

    struct CustomStruct {}

    enum CustomEnum {}

    typealias CustomTuple = (String, Int)

    typealias CustomFunction = (String) -> Int

    struct CustomGeneric<T> {
        struct Nested<U> {}
    }

    @Test("Metadata description matches declaration name")
    func hasDescription() {
        #expect(Metadata(CustomClass.self).description == "MetadataTests.CustomClass")
        #expect(Metadata(CustomStruct.self).description == "MetadataTests.CustomStruct")
        #expect(Metadata(CustomEnum.self).description == "MetadataTests.CustomEnum")
        #expect(Metadata(Optional<Void>.self).description == "Optional<Void>")
        #expect(Metadata(CustomTuple.self).description == "(String, Int)")
        #expect(Metadata(CustomFunction.self).description == "(String) -> Int")
        #expect(Metadata(Any.self).description == "Any")
        #expect(Metadata(Void.Type.self).description == "().Type")
        #expect(
            Metadata(CustomGeneric<String>.Nested<Int>.self).description
                == "MetadataTests.CustomGeneric<String>.Nested<Int>")
    }

    @Test("Metadata kind matches type kind")
    func hasKind() {
        #expect(Metadata(CustomClass.self).kind == .class)
        #expect(Metadata(CustomStruct.self).kind == .struct)
        #expect(Metadata(CustomEnum.self).kind == .enum)
        #expect(Metadata(Optional<Void>.self).kind == .optional)
        #expect(Metadata(CustomTuple.self).kind == .tuple)
        #expect(Metadata(CustomFunction.self).kind == .function)
        #expect(Metadata(Any.self).kind == .existential)
        #expect(Metadata(Void.Type.self).kind == .metatype)
        #expect(Metadata(CustomGeneric<String>.Nested<Int>.self).kind == .struct)
    }

    @Suite
    struct SignatureTests {

        @Test(
            "Metadata for nominal type has a valid signature",
            arguments: [
                Metadata(CustomClass.self),
                Metadata(CustomStruct.self),
                Metadata(CustomEnum.self),
                Metadata(Optional<Void>.self),
            ]
        )
        func nominalTypeSignatureIsValid(metadata: Metadata) {
            #expect(metadata.signature != Signature())
        }

        @Test(
            "Metadata for non-nominal type does not have a signature",
            arguments: [
                Metadata(CustomTuple.self),
                Metadata(CustomFunction.self),
                Metadata(Any.self),
                Metadata(Void.Type.self),
            ]
        )
        func nonNominalTypeSignatureIsNull(metadata: Metadata) {
            #expect(metadata.signature == Signature(), "\(metadata)")
        }

        func signaturesAreUnique() {
            var signatures: [Signature] = []

            signatures.append(Metadata(CustomClass.self).signature)
            signatures.append(Metadata(CustomStruct.self).signature)
            signatures.append(Metadata(CustomEnum.self).signature)
            signatures.append(Metadata(Optional<Void>.self).signature)

            #expect(signatures.count == 4)
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
                Metadata(CustomClass.self),
                Metadata(CustomStruct.self),
                Metadata(CustomEnum.self),
                Metadata(Optional<Void>.self),
            ]
        )
        func hasDescriptor(metadata: Metadata) {
            #expect(metadata.descriptor != nil)
        }

        @Test(
            "Metadata for non-class nominal type has a nominal descriptor",
            arguments: [
                Metadata(CustomStruct.self),
                Metadata(CustomEnum.self),
                Metadata(Optional<Void>.self),
            ]
        )
        func nominalDescriptorIsValid(metadata: Metadata) {
            #expect(metadata.nominalDescriptor != nil)
        }

        @Test(
            "Metadata for class does not have a nominal descriptor"
        )
        func classNominalDescriptorIsNil() {
            #expect(Metadata(CustomClass.self).nominalDescriptor == nil)
        }

        @Test("Metadata for nominal type has a nominal descriptor name")
        func hasNominalDescriptorName() throws {
            if let customStructName = Metadata(CustomStruct.self).nominalDescriptorName {
                #expect(String(cString: customStructName) == "CustomStruct")
            } else {
                #expect(Bool(false))
            }
            if let customEnumName = Metadata(CustomEnum.self).nominalDescriptorName {
                #expect(String(cString: customEnumName) == "CustomEnum")
            } else {
                #expect(Bool(false))
            }
            if let optionalName = Metadata(Optional<Void>.self).nominalDescriptorName {
                #expect(String(cString: optionalName) == "Optional")
            } else {
                #expect(Bool(false))
            }

        }

    }

    // These tests are taken from https://github.com/OpenSwiftUIProject/OpenGraph/blob/main/Tests/OpenGraphCompatibilityTests/Runtime/MetadataTests.swift
    @Suite
    struct ApplyFieldsTests {
        class T1 {
            var a = 0
            var b: Double = 0
        }

        struct T2 {
            var a: Int
            var b: Double
        }

        enum T3 {
            case a, b
        }

        @Test
        func forEachField() throws {
            for options in [Metadata.ApplyOptions.heapClasses] {
                let result = Metadata(T1.self).forEachField(options: options) { name, offset, type in
                    if offset == 16 {
                        #expect(type is Int.Type)
                        #expect(String(cString: name) == "a")
                        return true
                    } else if offset == 24 {
                        #expect(type is Double.Type)
                        #expect(String(cString: name) == "b")
                        return true
                    } else {
                        return false
                    }
                }
                #expect(result == true)
            }
            for options in [Metadata.ApplyOptions._2, .enumCases, []] {
                let result = Metadata(T1.self).forEachField(options: options) { name, offset, type in
                    if offset == 16 {
                        #expect(type is Int.Type)
                        #expect(String(cString: name) == "a")
                        return true
                    } else if offset == 24 {
                        #expect(type is Double.Type)
                        #expect(String(cString: name) == "b")
                        return true
                    } else {
                        return false
                    }
                }
                #expect(result == false)
            }
            for options in [Metadata.ApplyOptions._2, []] {
                let result = Metadata(T2.self).forEachField(options: options) { name, offset, type in
                    if offset == 0 {
                        #expect(type is Int.Type)
                        return true
                    } else if offset == 8 {
                        #expect(type is Double.Type)
                        return true
                    } else {
                        return false
                    }
                }
                #expect(result == true)
            }
            for options in [Metadata.ApplyOptions.heapClasses, .enumCases] {
                let result = Metadata(T2.self).forEachField(options: options) { name, offset, type in
                    if offset == 0 {
                        #expect(type is Int.Type)
                        #expect(String(cString: name) == "a")
                        return true
                    } else if offset == 8 {
                        #expect(type is Double.Type)
                        #expect(String(cString: name) == "b")
                        return true
                    } else {
                        return false
                    }
                }
                #expect(result == false)
            }
            for options in [Metadata.ApplyOptions.heapClasses, ._2, .enumCases, []] {
                let result = Metadata(T3.self).forEachField(options: options) { _, _, _ in
                    true
                }
                #expect(result == false)
            }
        }
    }

}
