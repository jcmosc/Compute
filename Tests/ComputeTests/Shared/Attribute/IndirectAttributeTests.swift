import Foundation
import Testing

@Suite
struct IndirectAttributeTests {

    @Suite
    final class InitTests: GraphHost {

        @Test
        func initWithSource() {
            let source = Attribute(value: 1)
            let indirect = IndirectAttribute(source: source)
            #expect(indirect.identifier != source.identifier)
            #expect(indirect.source.identifier == source.identifier)
            #expect(indirect.attribute.identifier == indirect.identifier)
            #expect(indirect.dependency == .init(rawValue: 0))

            let foundSource = indirect.attribute.breadthFirstSearch(options: [.searchInputs]) { candidate in
                return candidate == source.identifier
            }
            #expect(foundSource == true)
        }

    }

    @Suite
    final class SourceTests: GraphHost {

        @Test
        func source() {
            let source1 = Attribute(value: 1)
            let source2 = Attribute(value: 2)
            let indirect = IndirectAttribute(source: source1)
            #expect(indirect.source.identifier == source1.identifier)

            var foundSource1 = indirect.attribute.breadthFirstSearch(options: [.searchInputs]) { candidate in
                return candidate == source1.identifier
            }
            #expect(foundSource1 == true)

            var foundSource2 = indirect.attribute.breadthFirstSearch(options: [.searchInputs]) { candidate in
                return candidate == source2.identifier
            }
            #expect(foundSource2 == false)

            indirect.source = source2

            foundSource1 = indirect.attribute.breadthFirstSearch(options: [.searchInputs]) { candidate in
                return candidate == source1.identifier
            }
            #expect(foundSource1 == false)

            foundSource2 = indirect.attribute.breadthFirstSearch(options: [.searchInputs]) { candidate in
                return candidate == source2.identifier
            }
            #expect(foundSource2 == true)

            indirect.resetSource()

            foundSource1 = indirect.attribute.breadthFirstSearch(options: [.searchInputs]) { candidate in
                return candidate == source1.identifier
            }
            #expect(foundSource1 == true)

            foundSource2 = indirect.attribute.breadthFirstSearch(options: [.searchInputs]) { candidate in
                return candidate == source2.identifier
            }
            #expect(foundSource2 == false)
        }

    }

    @Suite
    final class DependencyTests: GraphHost {

        @Test
        func dependency() {
            let source = Attribute(value: 1)
            let dependency = Attribute(value: 2)
            let indirect = IndirectAttribute(source: source)
            #expect(indirect.dependency == .init(0))

            indirect.dependency = dependency.identifier
            #expect(indirect.dependency == dependency.identifier)

            let foundDependency = indirect.attribute.breadthFirstSearch(options: [.searchInputs]) { candidate in
                return candidate == dependency.identifier
            }
            #expect(foundDependency == false)

            let foundSource = dependency.breadthFirstSearch(options: [.searchOutputs]) { candidate in
                return candidate == source.identifier
            }
            #expect(foundSource == false)
        }

    }

}
