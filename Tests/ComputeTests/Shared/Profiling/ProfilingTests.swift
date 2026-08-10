import Testing
import _ComputeTestSupport

#if canImport(Darwin)
@Suite(.serialized(for: \Subgraph.Type.current))
struct ProfilingTests {
    struct TestRule: Rule {
        @Attribute var input: Int
        var value: Int { 1 }
    }

    @Test
    func nodeWithProfiler() throws {
        let graph = Graph()
        Graph.startProfiling()
        defer {
            Graph.stopProfiling()
        }

        let subgraph = Subgraph(graph: graph)
        let attribute = subgraph.apply {
            let input = Attribute(value: 1)
            return Attribute(TestRule(input: input))
        }

        let _ = attribute.value

        let description = try #require(graph.dictionaryDescription())

        #expect(description.graphs[0].updateCount ?? 0 == 1)
        #expect(description.graphs[0].updateTotal ?? 0.0 > 0.0)
        #expect(description.graphs[0].changeCount ?? 0 == 1)
        #expect(description.graphs[0].changedTotal ?? 0.0 > 0.0)

        #expect(description.graphs[0].types[0].profile == nil)
        #expect(description.graphs[0].types[1].profile == nil)

        let nodeProfile = try #require(description.graphs[0].nodes[0].profile)
        #expect(nodeProfile.updateCount ?? 0 == 1)
        #expect(nodeProfile.updateTotal ?? 0.0 > 0.0)
        #expect(nodeProfile.changeCount ?? 0 == 1)
        #expect(nodeProfile.changedTotal ?? 0.0 > 0.0)
    }

    @Suite
    struct ProfileEventTests {
        @Test
        func nodeWithProfileEvent() throws {
            let graph = Graph()
            Graph.startProfiling()
            defer {
                Graph.stopProfiling()
            }

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                let input = Attribute(value: 1)
                return Attribute(TestRule(input: input))
            }

            let startTime = attribute.identifier.beginProfileEvent(name: "test/event")

            let _ = attribute.value

            attribute.identifier.endProfileEvent(name: "test/event", startTime: startTime, changed: true)

            let description = try #require(graph.dictionaryDescription())

            typealias Profile = Graph.DictionaryDescription.Graph.Profile

            func assertProfile(_ profile: Profile) {
                #expect(profile.updateCount ?? 0 == 1)
                #expect(profile.updateTotal ?? 0.0 > 0.0)
                #expect(profile.changeCount ?? 0 == 1)
                #expect(profile.changedTotal ?? 0.0 > 0.0)
                #expect(profile.marks == nil)
            }

            func assertEvents(_ events: [String: Profile]) {
                #expect(events.keys.sorted() == ["test/event"])
                events.values.forEach { profile in
                    assertProfile(profile)
                }
            }

            let graphProfile = Profile(
                updateCount: description.graphs[0].updateCount,
                updateTotal: description.graphs[0].updateTotal,
                changeCount: description.graphs[0].changeCount,
                changedTotal: description.graphs[0].changedTotal,
                marks: description.graphs[0].marks
            )
            assertProfile(graphProfile)

            let graphEvents = try #require(description.graphs[0].events)
            assertEvents(graphEvents)

            let nodeProfile = try #require(description.graphs[0].nodes[0].profile)
            assertProfile(nodeProfile)

            let nodeEvents = try #require(description.graphs[0].nodes[0].events)
            assertEvents(nodeEvents)
        }

        @Test
        func nodeWithProfileEventMarked() throws {
            let graph = Graph()
            Graph.startProfiling()
            defer {
                Graph.stopProfiling()
            }

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                let input = Attribute(value: 1)
                return Attribute(TestRule(input: input))
            }

            let startTime = attribute.identifier.beginProfileEvent(name: "test/event")

            let _ = attribute.value

            attribute.identifier.endProfileEvent(name: "test/event", startTime: startTime, changed: true)

            "mark_name".withCString { pointer in
                Graph.markProfile(name: pointer)
            }

            let description = try #require(graph.dictionaryDescription())

            typealias Profile = Graph.DictionaryDescription.Graph.Profile

            func assertMark(_ mark: Profile.Mark) {
                #expect(mark.name == "mark_name")
                #expect(mark.timestamp > 0)
                #expect(mark.updateCount ?? 0 == 1)
                #expect(mark.updateTotal ?? 0.0 > 0.0)
                #expect(mark.changeCount ?? 0 == 1)
                #expect(mark.changedTotal ?? 0.0 > 0.0)
            }

            func assertProfile(_ profile: Profile) {
                // Will be nil as the marks array is non-empty
                #expect(profile.updateCount == nil)
                #expect(profile.updateTotal == nil)
                #expect(profile.changeCount == nil)
                #expect(profile.changedTotal == nil)

                #expect(profile.marks?.count ?? 0 > 0)
                profile.marks?.forEach { mark in
                    assertMark(mark)
                }
            }

            func assertEvents(_ events: [String: Profile]) {
                #expect(events.keys.sorted() == ["test/event"])
                events.values.forEach { profile in
                    assertProfile(profile)
                }
            }

            let graphProfile = Profile(
                updateCount: description.graphs[0].updateCount,
                updateTotal: description.graphs[0].updateTotal,
                changeCount: description.graphs[0].changeCount,
                changedTotal: description.graphs[0].changedTotal,
                marks: description.graphs[0].marks
            )
            assertProfile(graphProfile)

            let graphEvents = try #require(description.graphs[0].events)
            assertEvents(graphEvents)

            let nodeProfile = try #require(description.graphs[0].nodes[0].profile)
            assertProfile(nodeProfile)

            let nodeEvents = try #require(description.graphs[0].nodes[0].events)
            assertEvents(nodeEvents)
        }
    }

    @Suite
    struct RemovedNodeTests {
        @Test
        func removedNodeWithProfiler() throws {
            let graph = Graph()
            Graph.startProfiling()
            defer {
                Graph.stopProfiling()
            }

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                let input = Attribute(value: 1)
                return Attribute(TestRule(input: input))
            }

            let _ = attribute.value

            subgraph.invalidate()

            let description = try #require(graph.dictionaryDescription())
            #expect(description.graphs[0].nodes.isEmpty)

            #expect(description.graphs[0].updateCount ?? 0 == 1)
            #expect(description.graphs[0].updateTotal ?? 0.0 > 0.0)
            #expect(description.graphs[0].changeCount ?? 0 == 1)
            #expect(description.graphs[0].changedTotal ?? 0.0 > 0.0)

            let typeProfile = try #require(description.graphs[0].types[0].profile)
            #expect(typeProfile.updateCount ?? 0 == 1)
            #expect(typeProfile.updateTotal ?? 0.0 > 0.0)
            #expect(typeProfile.changeCount ?? 0 == 1)
            #expect(typeProfile.changedTotal ?? 0.0 > 0.0)
        }

        @Test
        func removedNodeWithoutProfiler() throws {
            let graph = Graph()

            let subgraph = Subgraph(graph: graph)
            let attribute = subgraph.apply {
                let input = Attribute(value: 1)
                return Attribute(TestRule(input: input))
            }

            let _ = attribute.value

            subgraph.invalidate()

            let description = try #require(graph.dictionaryDescription())
            #expect(description.graphs[0].types.isEmpty)
            #expect(description.graphs[0].nodes.isEmpty)

            #expect(description.graphs[0].updateCount ?? 0 == 1)
            #expect(description.graphs[0].updateTotal == nil)
            #expect(description.graphs[0].changeCount ?? 0 == 2)
            #expect(description.graphs[0].changedTotal == nil)
        }
    }
}
#endif
