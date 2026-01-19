import Testing
import Utilities

@Suite("HashTable tests")
struct HashTableTests {

    @Test("Initialize empty table")
    func initEmpty() {
        let table = util.UntypedTable.create()
        defer {
            util.UntypedTable.destroy(table)
        }

        #expect(table.empty())
        #expect(table.count() == 0)
    }

    @Test("Insert entry")
    func insertEntry() {
        class Value {
            let prop: String
            init(prop: String) {
                self.prop = prop
            }
        }

        let table = util.UntypedTable.create()
        defer {
            util.UntypedTable.destroy(table)
        }

        let key = "key1"
        let value = Value(prop: "valueProp")
        withUnsafePointer(to: key) { keyPointer in
            withUnsafePointer(to: value) { valuePointer in
                let inserted = table.insert(keyPointer, valuePointer)

                #expect(inserted == true)
                #expect(table.count() == 1)

                let foundValue = table.__lookupUnsafe(keyPointer, nil)
                #expect(foundValue?.assumingMemoryBound(to: Value.self).pointee.prop == "valueProp")
            }
        }
    }

    @Test("Remove entry")
    func removeEntry() {
        class Value {
            let prop: String
            init(prop: String) {
                self.prop = prop
            }
        }

        let table = util.UntypedTable.create()
        defer {
            util.UntypedTable.destroy(table)
        }

        let key = "key1"
        let value = Value(prop: "valueProp")
        withUnsafePointer(to: key) { keyPointer in
            withUnsafePointer(to: value) { valuePointer in
                let inserted = table.insert(keyPointer, valuePointer)

                try! #require(inserted == true)
                try! #require(table.count() == 1)

                let removed = table.remove(keyPointer)

                #expect(removed == true)
                #expect(table.count() == 0)

                let foundValue = table.__lookupUnsafe(keyPointer, nil)
                #expect(foundValue == nil)
            }
        }
    }

    @Test("Remove head of bucket then insert - should not create cycle")
    func removeHeadThenInsert() {
        let table = util.UntypedTable.create()
        defer {
            util.UntypedTable.destroy(table)
        }

        // We'll insert many items to increase chance of bucket collisions
        // With 16 initial buckets, inserting 32 items guarantees collisions
        var keys: [Int] = Array(0..<32)

        // Insert all keys
        for i in 0..<keys.count {
            withUnsafePointer(to: &keys[i]) { keyPointer in
                let inserted = table.insert(keyPointer, keyPointer)
                #expect(inserted == true)
            }
        }

        #expect(table.count() == 32)

        // Remove first key (likely head of some bucket)
        withUnsafePointer(to: &keys[0]) { keyPointer in
            let removed = table.remove(keyPointer)
            #expect(removed == true)
        }

        #expect(table.count() == 31)

        // Insert a new key - this will reuse the spare node from removal
        var newKey = 100
        withUnsafePointer(to: &newKey) { keyPointer in
            let inserted = table.insert(keyPointer, keyPointer)
            #expect(inserted == true)
        }

        #expect(table.count() == 32)

        // Count iterations via for_each - if there's a cycle, this will exceed count
        // or hang forever. We use a manual iteration limit to detect cycles.
        var iterationCount = 0
        let maxIterations = 1000 // Way more than count, to detect infinite loop

        table.for_each({ _, _, context in
            let countPtr = context.assumingMemoryBound(to: Int.self)
            countPtr.pointee += 1
            // If we've iterated too many times, we have a cycle
            if countPtr.pointee > 100 {
                fatalError("Cycle detected in hash table - iteration count exceeded expected")
            }
        }, &iterationCount)

        #expect(iterationCount == 32, "for_each should visit exactly count() items")
    }

    @Test("Remove and reinsert same key - lookup should work")
    func removeAndReinsertSameKey() {
        let table = util.UntypedTable.create()
        defer {
            util.UntypedTable.destroy(table)
        }

        var key1 = 1
        var key2 = 2
        var value1 = 100
        var value2 = 200

        // Insert two keys
        withUnsafePointer(to: &key1) { k1 in
            withUnsafePointer(to: &value1) { v1 in
                _ = table.insert(k1, v1)
            }
        }
        withUnsafePointer(to: &key2) { k2 in
            withUnsafePointer(to: &value2) { v2 in
                _ = table.insert(k2, v2)
            }
        }

        #expect(table.count() == 2)

        // Remove first key
        withUnsafePointer(to: &key1) { k1 in
            let removed = table.remove(k1)
            #expect(removed == true)
        }

        #expect(table.count() == 1)

        // Verify key1 is not found
        withUnsafePointer(to: &key1) { k1 in
            let found = table.__lookupUnsafe(k1, nil)
            #expect(found == nil, "Removed key should not be found")
        }

        // Verify key2 is still found
        withUnsafePointer(to: &key2) { k2 in
            let found = table.__lookupUnsafe(k2, nil)
            #expect(found != nil, "Remaining key should still be found")
        }

        // Reinsert key1 with a new value
        var value1New = 999
        withUnsafePointer(to: &key1) { k1 in
            withUnsafePointer(to: &value1New) { v1 in
                let inserted = table.insert(k1, v1)
                #expect(inserted == true, "Reinserting removed key should succeed")
            }
        }

        #expect(table.count() == 2)

        // Verify key1 is found with new value
        withUnsafePointer(to: &key1) { k1 in
            let found = table.__lookupUnsafe(k1, nil)
            #expect(found != nil, "Reinserted key should be found")
            if let found = found {
                #expect(found.assumingMemoryBound(to: Int.self).pointee == 999, "Reinserted key should have new value")
            }
        }

        // Count via for_each should be 2
        var iterationCount = 0
        table.for_each({ _, _, context in
            let countPtr = context.assumingMemoryBound(to: Int.self)
            countPtr.pointee += 1
        }, &iterationCount)

        #expect(iterationCount == 2, "for_each should visit exactly 2 items after reinsertion")
    }

    @Test("Grow buckets should not lose nodes")
    func growBucketsPreservesAllNodes() {
        let table = util.UntypedTable.create()
        defer {
            util.UntypedTable.destroy(table)
        }

        // Initial bucket_mask_width is 4 (16 buckets)
        // Growth happens when count + 1 > 4 << bucket_mask_width
        // So growth happens at count > 64 for first growth
        // Insert enough items to trigger multiple growths
        let itemCount = 200
        var keys: [Int] = Array(0..<itemCount)

        for i in 0..<keys.count {
            withUnsafePointer(to: &keys[i]) { keyPointer in
                let inserted = table.insert(keyPointer, keyPointer)
                #expect(inserted == true, "Insert \(i) should succeed")
            }
        }

        #expect(table.count() == UInt64(itemCount))

        // Verify all items can be found
        for i in 0..<keys.count {
            withUnsafePointer(to: &keys[i]) { keyPointer in
                let found = table.__lookupUnsafe(keyPointer, nil)
                #expect(found != nil, "Key \(i) should be found after growth")
            }
        }

        // Count via for_each should match
        var iterationCount = 0
        table.for_each({ _, _, context in
            let countPtr = context.assumingMemoryBound(to: Int.self)
            countPtr.pointee += 1
        }, &iterationCount)

        #expect(iterationCount == itemCount, "for_each should visit all \(itemCount) items")
    }

}
