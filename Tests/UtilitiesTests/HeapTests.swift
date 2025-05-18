import Testing
import Utilities

@Suite("Heap tests")
struct HeapTests {

    let nodeSize = 16

    @Test("Initializing with default arguments")
    func initDefault() {
        let heap = util.Heap.create(nil, 0, 0)
        defer {
            util.Heap.destroy(heap)
        }

        #expect(heap.capacity() == 0)
        #expect(heap.increment() == 0x2000)
        #expect(heap.num_nodes() == 0)
    }

    @Test("Allocating small object uses node")
    func allocateSmallObjects() {
        let heap = util.Heap.create(nil, 0, 0)
        defer {
            util.Heap.destroy(heap)
        }

        let _ = heap.__alloc_uint64_tUnsafe()

        // creates 1 node
        #expect(heap.num_nodes() == 1)
        #expect(heap.capacity() == 0x2000 - nodeSize - 8)

        let _ = heap.__alloc_uint64_tUnsafe()

        // second object is allocated from same node
        #expect(heap.num_nodes() == 1)
        #expect(heap.capacity() == 0x2000 - nodeSize - 2 * 8)
    }

    @Test("Allocating large object creates new node")
    func allocateLargeObject() {
        let heap = util.Heap.create(nil, 0, 0)
        defer {
            util.Heap.destroy(heap)
        }

        // larger than minimum increment
        let _ = heap.__alloc_uint64_tUnsafe(500)

        // data is allocated from second node
        #expect(heap.num_nodes() == 2)
        #expect(heap.capacity() == 0x2000 - 2 * nodeSize)
    }

}
