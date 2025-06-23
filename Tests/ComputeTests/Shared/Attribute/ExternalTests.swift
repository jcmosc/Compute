import Testing

@Suite
struct ExternalTests {
    
    @Test
    func external() throws {
        let type = External<Int>.self
        
        #expect(type.comparisonMode == .equatableAlways)
        #expect(type.flags == [])
        
        let externalInt = type.init()
        
        #expect(externalInt.description == "Int")
    }
    
}
