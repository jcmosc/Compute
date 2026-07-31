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

    @Suite(.serialized(for: \Subgraph.Type.current))
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

        @Test
        func weakAttributeExpiresBeforeBodyIsDestroyed() throws {
            struct TestBody: _AttributeBody {
                var weakAttribute: WeakAttribute<Int>
                var weakAttributeExpired: UnsafeMutablePointer<Bool>
                static func _destroySelf(_ self: UnsafeMutableRawPointer) {
                    let testBody = self.assumingMemoryBound(to: Self.self)
                    testBody.pointee.weakAttributeExpired.pointee = testBody.pointee.weakAttribute.attribute == nil
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
                
                let attribute = Attribute(value: 0)
                var weakAttributeExpired = false
                withUnsafeMutablePointer(to: &weakAttributeExpired) { weakAttributeExpiredPointer in
                    let body = TestBody(
                        weakAttribute: WeakAttribute(attribute),
                        weakAttributeExpired: weakAttributeExpiredPointer
                    )
                    let _ = withUnsafePointer(to: body) { bodyPointer in
                        Attribute<Int>(body: bodyPointer, value: nil, flags: []) {
                            return { _, _ in }
                        }
                    }
                    
                    #expect(weakAttributeExpiredPointer.pointee == false)
                    subgraph.invalidate()
                    #expect(weakAttributeExpiredPointer.pointee == true)
                }
            }
        }
    }
}
