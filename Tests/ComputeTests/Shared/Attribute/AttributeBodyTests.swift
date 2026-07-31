import Foundation
import Testing

@Suite
struct AttributeBodyTests {
    @Test
    func defaultImplementation() {
        struct DefaultAttributeBody: _AttributeBody {}

        #expect(DefaultAttributeBody._hasDestroySelf == false)
        #expect(DefaultAttributeBody.comparisonMode == .equatableUnlessPOD)
        #expect(DefaultAttributeBody.flags == [.mainThread])
    }

    @Suite(.serialized(for: \GraphHost.Type.sharedGraph))
    struct DestroySelfTests {
        @Test
        func callsDestroySelf() throws {
            struct TestBody: _AttributeBody {
                var destroyed: UnsafeMutablePointer<Bool>
                static func _destroySelf(_ self: UnsafeMutableRawPointer) {
                    let testBody = self.assumingMemoryBound(to: Self.self)
                    testBody.pointee.destroyed.pointee = true
                }
                static var _hasDestroySelf: Bool {
                    return true
                }
            }

            try withGraph {
                let globalSubgraph = try #require(Subgraph.current)
                let subgraph = Subgraph(graph: globalSubgraph.graph)
                globalSubgraph.addChild(subgraph)
                Subgraph.current = subgraph

                var destroyed = false
                withUnsafeMutablePointer(to: &destroyed) { destroyedPointer in
                    let _ = withUnsafePointer(to: TestBody(destroyed: destroyedPointer)) { bodyPointer in
                        Attribute<Int>(body: bodyPointer, value: nil, flags: []) {
                            return { _, _ in }
                        }
                    }

                    #expect(destroyedPointer.pointee == false)
                    subgraph.invalidate()
                    #expect(destroyedPointer.pointee == true)
                }
            }
        }

        @Test
        func doesNotCallDestroySelfWhenHasDestroySelfIsFalse() throws {
            struct TestBody: _AttributeBody {
                var destroyed: UnsafeMutablePointer<Bool>
                static func _destroySelf(_ self: UnsafeMutableRawPointer) {
                    let testBody = self.assumingMemoryBound(to: Self.self)
                    testBody.pointee.destroyed.pointee = true
                }
                static var _hasDestroySelf: Bool {
                    return false
                }
            }

            try withGraph {
                let globalSubgraph = try #require(Subgraph.current)
                let subgraph = Subgraph(graph: globalSubgraph.graph)
                globalSubgraph.addChild(subgraph)
                Subgraph.current = subgraph

                var destroyed = false
                withUnsafeMutablePointer(to: &destroyed) { destroyedPointer in
                    let _ = withUnsafePointer(to: TestBody(destroyed: destroyedPointer)) { bodyPointer in
                        Attribute<Int>(body: bodyPointer, value: nil, flags: []) {
                            return { _, _ in }
                        }
                    }

                    #expect(destroyedPointer.pointee == false)
                    subgraph.invalidate()
                    #expect(destroyedPointer.pointee == false)
                }
            }
        }

        @Test
        func callsDestroySelfWhenHasDestroySelfIsFalseAndFlagIsTrue() throws {
            struct TestBody: _AttributeBody {
                var destroyed: UnsafeMutablePointer<Bool>
                static func _destroySelf(_ self: UnsafeMutableRawPointer) {
                    let testBody = self.assumingMemoryBound(to: Self.self)
                    testBody.pointee.destroyed.pointee = true
                }
                static var _hasDestroySelf: Bool {
                    return false
                }
            }

            try withGraph {
                let globalSubgraph = try #require(Subgraph.current)
                let subgraph = Subgraph(graph: globalSubgraph.graph)
                globalSubgraph.addChild(subgraph)
                Subgraph.current = subgraph

                var destroyed = false
                withUnsafeMutablePointer(to: &destroyed) { destroyedPointer in
                    let _ = withUnsafePointer(to: TestBody(destroyed: destroyedPointer)) { bodyPointer in
                        Attribute<Int>(body: bodyPointer, value: nil, flags: [.hasDestroySelf]) {
                            return { _, _ in }
                        }
                    }

                    #expect(destroyedPointer.pointee == false)
                    subgraph.invalidate()
                    #expect(destroyedPointer.pointee == true)
                }
            }
        }
    }
}
