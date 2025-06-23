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

    @Suite
    struct DestroySelfTests {

        @Test
        func callsDestroySelf() {
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

            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
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

        @Test
        func doesNotCallDestroySelfWhenHasDestroySelfIsFalse() {
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

            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
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

        @Test
        func callsDestroySelfWhenHasDestroySelfIsFalseAndFlagIsTrue() {
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

            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
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
