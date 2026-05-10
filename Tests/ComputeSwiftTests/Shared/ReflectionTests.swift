import Testing

@Suite
struct ReflectionTests {

    @Test
    func reflectEmptyStruct() {

        struct EmptyStruct {}

        var fields: [(String, Int, any Any.Type)] = []
        let finished = Metadata(EmptyStruct.self).forEachField(options: []) {
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
    func reflectStaticString() {

        var fields: [(String, Int, any Any.Type)] = []
        let finished = Metadata(StaticString.self).forEachField(options: []) {
            fieldName,
            fieldOffset,
            fieldType in
            fields.append((String(cString: fieldName), fieldOffset, fieldType))
            return true
        }

        #expect(finished == true)
        #expect(fields.count == 3)

        #expect(fields[0].1 == 0)
        #expect(fields[1].1 == 8)
        #expect(fields[2].1 == 16)
    }

    @Test
    func reflectOptionalInt() {

        var fields: [(String, Int, any Any.Type)] = []
        let finished = Metadata(Optional<Int>.self).forEachField(options: [.enumerateEnumCases]) {
            fieldName,
            fieldOffset,
            fieldType in
            fields.append((String(cString: fieldName), fieldOffset, fieldType))
            return true
        }

        #expect(finished == true)
        #expect(fields.count == 1)
    }

}
