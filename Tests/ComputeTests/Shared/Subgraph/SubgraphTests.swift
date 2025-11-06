import Foundation
import Testing

// Why do I have to redeclare this?
extension Subgraph {

    @_extern(c, "AGSubgraphAddObserver")
    fileprivate static func addObserver(_ subgraph: UnsafeRawPointer, observer: @escaping () -> Void) -> Int

    func addObserver(_ observer: @escaping () -> Void) -> Int {
        return Subgraph.addObserver(unsafeBitCast(self, to: UnsafeRawPointer.self), observer: observer)
    }

}

@Suite
struct SubgraphTests {

    @Suite
    struct CFTypeTests {

        @Test
        func typeID() {
            let description = CFCopyTypeIDDescription(Subgraph.typeID) as String?
            #expect(description == "AGSubgraph")
        }

        @Test
        func createSubgraph() async throws {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
            #expect(CFGetTypeID(subgraph) == Subgraph.typeID)
        }

    }

    @Suite
    struct LifecycleTests {

        @Test
        func subgraphAddedToGraph() {
            let graph = Graph()

            let subgraph = Subgraph(graph: graph)
            #expect(subgraph.graph == graph)
        }

        @Test
        func subgraphCounters() {
            let graph = Graph()

            #expect(graph.counter(for: .subgraphs) == 0)
            #expect(graph.counter(for: .createdSubgraphs) == 0)

            autoreleasepool {
                let subgraph = Subgraph(graph: graph)
                #expect(subgraph.graph == graph)

                #expect(graph.counter(for: .subgraphs) == 1)
                #expect(graph.counter(for: .createdSubgraphs) == 1)
            }

            #expect(graph.counter(for: .subgraphs) == 0)
            #expect(graph.counter(for: .createdSubgraphs) == 1)
        }

    }

    @Suite
    struct ObserverTests {

        @Test
        func observerNotifiedOnSubgraphDestroyed() {
            var notifiedCount = 0
            autoreleasepool {
                let graph = Graph()

                autoreleasepool {
                    let subgraph = Subgraph(graph: graph)
                    let _ = subgraph.addObserver {
                        notifiedCount += 1
                    }
                }

                #expect(notifiedCount == 1)
            }

            // Observers aren't notified more than once
            #expect(notifiedCount == 1)
        }

        @Test
        func observerNotifiedOnGraphDestroyed() {
            var notifiedCount = 0
            autoreleasepool {
                let graph = Graph()
                let subgraph = Subgraph(graph: graph)
                let _ = subgraph.addObserver {
                    notifiedCount += 1
                }

                #expect(notifiedCount == 0)
            }

            #expect(notifiedCount == 1)
        }

        @Test
        func removedObserverNotNotified() {
            var notifiedCount = 0
            autoreleasepool {
                let graph = Graph()
                let subgraph = Subgraph(graph: graph)
                let observerID = subgraph.addObserver {
                    notifiedCount += 1
                }
                subgraph.removeObserver(observerID: UInt64(observerID))
            }

            #expect(notifiedCount == 0)
        }

    }

    @Suite
    struct InvalidationTests {

        @Test
        func invalidateSubgraph() throws {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)

            var notifiedCount = 0
            let _ = subgraph.addObserver {
                notifiedCount += 1
            }

            #expect(subgraph.isValid == true)
            #expect(graph.counter(for: .subgraphs) == 1)
            #expect(notifiedCount == 0)

            subgraph.invalidate()

            #expect(subgraph.isValid == false)
            #expect(graph.counter(for: .subgraphs) == 0)
            #expect(notifiedCount == 1)
        }

        @Test
        func deferredInvalidateSubgraph() throws {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)

            var notifiedCount = 0
            let _ = subgraph.addObserver {
                notifiedCount += 1
            }

            #expect(subgraph.isValid == true)
            #expect(graph.counter(for: .subgraphs) == 1)
            #expect(notifiedCount == 0)

            let wasDeferring = graph.beginDeferringSubgraphInvalidation()

            subgraph.invalidate()

            #expect(subgraph.isValid == false)
            #expect(graph.counter(for: .subgraphs) == 1)  // Subgraph not removed until batch end
            #expect(notifiedCount == 0)

            graph.endDeferringSubgraphInvalidation(wasDeferring: wasDeferring)

            #expect(subgraph.isValid == false)
            #expect(graph.counter(for: .subgraphs) == 0)
            #expect(notifiedCount == 1)
        }

        @Test
        func subgraphInvalidatedOnGraphDestroyed() throws {
            var subgraphOrNil: Subgraph? = nil
            try autoreleasepool {
                let graph = Graph()
                subgraphOrNil = Subgraph(graph: graph)

                let subgraph = try #require(subgraphOrNil)
                #expect(subgraph.isValid == true)
            }

            let subgraph = try #require(subgraphOrNil)
            #expect(subgraph.isValid == false)
        }

    }

    @Suite
    struct TraceTests {

        class TraceReporter {
            var createdSubgraphCount = 0
            var invalidateSubgraphCount = 0
        }

        @Test
        func invalidateSubgraph() async throws {
            var trace = AGTrace()
            trace.created_subgraph = { context, graph in
                guard let reporter = context?.assumingMemoryBound(to: TraceReporter.self).pointee else {
                    return
                }
                reporter.createdSubgraphCount += 1
            }
            trace.invalidate_subgraph = { context, graph in
                guard let reporter = context?.assumingMemoryBound(to: TraceReporter.self).pointee else {
                    return
                }
                reporter.invalidateSubgraphCount += 1
            }

            var reporter = TraceReporter()

            let graph = Graph()

            #expect(reporter.createdSubgraphCount == 0)
            #expect(reporter.invalidateSubgraphCount == 0)

            let _ = withUnsafeMutablePointer(to: &trace) { tracePointer in
                withUnsafeMutablePointer(to: &reporter) { reporterPointer in
                    graph.addTrace(tracePointer, context: reporterPointer)
                }
            }

            let subgraph = Subgraph(graph: graph)

            #expect(reporter.createdSubgraphCount == 1)
            #expect(reporter.invalidateSubgraphCount == 0)

            subgraph.invalidate()

            #expect(reporter.createdSubgraphCount == 1)
            #expect(reporter.invalidateSubgraphCount == 1)
        }

    }

    @Suite
    struct CurrentSubgraph {

        @Test
        func currentSubgraph() {
            #expect(Subgraph.current == nil)

            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
            Subgraph.current = subgraph
            #expect(Subgraph.current == subgraph)

            Subgraph.current = nil
            #expect(Subgraph.current == nil)
        }

        @Test
        func invalidateCurrentSubgraph() {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
            Subgraph.current = subgraph

            subgraph.invalidate()
            #expect(Subgraph.current == nil)
        }

    }

    @Suite
    struct ChildrenTests {

        @Test
        func child() {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)

            #expect(subgraph.childCount == 0)

            let child = Subgraph(graph: graph)
            subgraph.addChild(child)

            #expect(subgraph.childCount == 1)
            #expect(subgraph.child(at: 0, tag: nil) == child)
            #expect(subgraph.isAncestor(of: child) == true)
            #expect(child.parentCount == 1)
            #expect(child.parent(at: 0) == subgraph)

            subgraph.removeChild(child)

            #expect(subgraph.childCount == 0)
            #expect(subgraph.isAncestor(of: child) == false)
            #expect(child.parentCount == 0)
        }

        @Test
        func multipleChildren() {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)

            #expect(subgraph.childCount == 0)

            let child1 = Subgraph(graph: graph)
            subgraph.addChild(child1)

            let child2 = Subgraph(graph: graph)
            subgraph.addChild(child2)

            #expect(subgraph.childCount == 2)
            #expect(subgraph.child(at: 0, tag: nil) == child1)
            #expect(subgraph.child(at: 1, tag: nil) == child2)
            #expect(subgraph.isAncestor(of: child1) == true)
            #expect(subgraph.isAncestor(of: child2) == true)
            #expect(child1.parentCount == 1)
            #expect(child1.parent(at: 0) == subgraph)
            #expect(child2.parentCount == 1)
            #expect(child2.parent(at: 0) == subgraph)

            subgraph.removeChild(child1)

            #expect(subgraph.childCount == 1)
            #expect(subgraph.child(at: 0, tag: nil) == child2)
            #expect(subgraph.isAncestor(of: child1) == false)
            #expect(subgraph.isAncestor(of: child2) == true)
            #expect(child1.parentCount == 0)
            #expect(child2.parentCount == 1)
            #expect(child2.parent(at: 0) == subgraph)

            subgraph.removeChild(child2)

            #expect(subgraph.childCount == 0)
            #expect(subgraph.isAncestor(of: child1) == false)
            #expect(subgraph.isAncestor(of: child2) == false)
            #expect(child1.parentCount == 0)
            #expect(child2.parentCount == 0)
        }

        @Test
        func multipleParents() {
            let graph = Graph()
            let parent1 = Subgraph(graph: graph)
            let parent2 = Subgraph(graph: graph)

            let child = Subgraph(graph: graph)
            parent1.addChild(child)
            parent2.addChild(child)

            #expect(parent1.childCount == 1)
            #expect(parent1.child(at: 0, tag: nil) == child)
            #expect(parent1.isAncestor(of: child) == true)
            #expect(parent2.childCount == 1)
            #expect(parent2.child(at: 0, tag: nil) == child)
            #expect(parent2.isAncestor(of: child) == true)
            #expect(child.parentCount == 2)
            #expect(child.parent(at: 0) == parent1)
            #expect(child.parent(at: 1) == parent2)

            parent1.removeChild(child)

            #expect(parent1.childCount == 0)
            #expect(parent1.isAncestor(of: child) == false)
            #expect(parent2.childCount == 1)
            #expect(parent2.child(at: 0, tag: nil) == child)
            #expect(parent2.isAncestor(of: child) == true)
            #expect(child.parentCount == 1)
            #expect(child.parent(at: 0) == parent2)

            parent2.removeChild(child)

            #expect(parent1.childCount == 0)
            #expect(parent1.isAncestor(of: child) == false)
            #expect(parent2.childCount == 0)
            #expect(parent2.isAncestor(of: child) == false)
            #expect(child.parentCount == 0)

        }

        @Test(arguments: [0, 1, 2, 3])
        func childWithTag(_ tag: Int) {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)

            let child = Subgraph(graph: graph)
            subgraph.addChild(child, tag: UInt8(tag))

            var storedTag: Int = 0
            subgraph.child(at: 0, tag: &storedTag)
            #expect(storedTag == tag)
        }

        @Test
        func childWithInvalidTag() {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)

            let child = Subgraph(graph: graph)
            subgraph.addChild(child, tag: UInt8.max)

            var storedTag: Int = 0
            subgraph.child(at: 0, tag: &storedTag)
            #expect(storedTag == 3)  // Only lowest 2 bits stored
        }

        @Test
        func invalidatingSubgraphInvalidatesChildren() {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)

            let child = Subgraph(graph: graph)
            subgraph.addChild(child)

            #expect(subgraph.isValid == true)
            #expect(child.isValid == true)

            subgraph.invalidate()

            #expect(subgraph.isValid == false)
            #expect(child.isValid == false)
        }

    }
    
    @Suite
    struct FlagsTests {
        
        @Test
        func intersects() {
            let graph = Graph()
            let subgraph = Subgraph(graph: graph)
            
            let child = Subgraph(graph: graph)
            subgraph.addChild(child)
            
            Subgraph.current = child
            
            let attribute = Attribute(value: 1)
            #expect(subgraph.intersects(flags: Subgraph.Flags(rawValue: 1)) == false)
            #expect(child.intersects(flags: Subgraph.Flags(rawValue: 1)) == false)
            
            attribute.flags = Subgraph.Flags(rawValue: 1)
            #expect(subgraph.intersects(flags: Subgraph.Flags(rawValue: 1)) == true)
            #expect(child.intersects(flags: Subgraph.Flags(rawValue: 1)) == true)
        }
        
    }

}
