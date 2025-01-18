import Compute
import Testing

@Suite("Metadata tests")
struct MetadataTests {

    class CustomClass {}
    struct CustomStruct {}
    enum CustomEnum {}

    @Test func createFromSwiftType() {
        #expect(Metadata(CustomClass.self).description == "MetadataTests.CustomClass")
        #expect(Metadata(CustomStruct.self).description == "MetadataTests.CustomStruct")
        #expect(Metadata(CustomEnum.self).description == "MetadataTests.CustomEnum")

    }
}
