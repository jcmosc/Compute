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

            #expect(graph.counter(for: .subgraphCount) == 0)
            #expect(graph.counter(for: .subgraphTotalCount) == 0)

            autoreleasepool {
                let subgraph = Subgraph(graph: graph)
                #expect(subgraph.graph == graph)

                #expect(graph.counter(for: .subgraphCount) == 1)
                #expect(graph.counter(for: .subgraphTotalCount) == 1)
            }

            #expect(graph.counter(for: .subgraphCount) == 0)
            #expect(graph.counter(for: .subgraphTotalCount) == 1)
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
            #expect(graph.counter(for: .subgraphCount) == 1)
            #expect(notifiedCount == 0)

            subgraph.invalidate()

            #expect(subgraph.isValid == false)
            #expect(graph.counter(for: .subgraphCount) == 0)
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
            #expect(graph.counter(for: .subgraphCount) == 1)
            #expect(notifiedCount == 0)

            let wasDeferring = graph.beginDeferringSubgraphInvalidation()

            subgraph.invalidate()

            #expect(subgraph.isValid == false)
            #expect(graph.counter(for: .subgraphCount) == 1)  // Subgraph not removed until batch end
            #expect(notifiedCount == 0)

            graph.endDeferringSubgraphInvalidation(wasDeferring: wasDeferring)

            #expect(subgraph.isValid == false)
            #expect(graph.counter(for: .subgraphCount) == 0)
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
            trace.createdSubgraph = { context, graph in
                guard let reporter = context?.assumingMemoryBound(to: TraceReporter.self).pointee else {
                    return
                }
                reporter.createdSubgraphCount += 1
            }
            trace.invalidateSubgraph = { context, graph in
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

}
