import Testing
import Utilities

@Suite("List tests")
struct ListTests {

    @Test("Initialize empty list")
    func initEmpty() {
        let listPointer = util.UInt64ForwardList.make_shared()
        let list = listPointer.pointee

        #expect(list.empty())
    }

    @Test("Push element")
    func pushElement() {
        let listPointer = util.UInt64ForwardList.make_shared()
        let list = listPointer.pointee

        list.push_front(1)

        #expect(list.empty() == false)

        let front = list.front()
        #expect(front == 1)
    }

    @Test("Push multiple elements")
    func pushMultipleElements() {
        let listPointer = util.UInt64ForwardList.make_shared()
        let list = listPointer.pointee

        list.push_front(1)
        list.push_front(2)
        list.push_front(3)

        let front = list.front()
        #expect(front == 3)
    }

    @Test("Remove element")
    func removeElement() {
        let listPointer = util.UInt64ForwardList.make_shared()
        let list = listPointer.pointee

        list.push_front(1)
        list.push_front(2)
        list.push_front(3)
        list.pop_front()

        let front = list.front()
        #expect(front == 2)
    }

}
