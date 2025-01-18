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

    @Test func description() {
        #expect(Metadata(CustomClass.self).description == "MetadataTests.CustomClass")
        #expect(Metadata(CustomStruct.self).description == "MetadataTests.CustomStruct")
        #expect(Metadata(CustomEnum.self).description == "MetadataTests.CustomEnum")
        #expect(Metadata(CustomTuple.self).description == "(String, Int)")
        #expect(Metadata(CustomFunction.self).description == "(String) -> Int")
        #expect(
            Metadata(CustomGeneric<String>.Nested<Int>.self).description
                == "MetadataTests.CustomGeneric<String>.Nested<Int>")
    }
}
