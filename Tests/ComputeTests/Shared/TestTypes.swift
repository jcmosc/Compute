import Foundation

// Class

class TestClass {
    var a: Bool = false
}

// Struct

struct TestStruct {
    var a: Bool = false
}

// Enum

enum TestEnum {
    case a
    case b
}

enum TestTaggedEnum {
    case a(TestClass)
    case b(TestStruct)
}

indirect enum TestIndirectEnum {
    case a
    case b(TestIndirectEnum)
}

// Optional

typealias TestOptionalClass = TestClass?
typealias TestOptionalStruct = TestStruct?

// Foreign Class

typealias TestForeignClass = CFDate

// ForeignReferenceType (non-swift non-objc-c class type)
//typealias TestForeignReferenceType = util.UntypedTable

// Opaque, type not exposed in metadata system

// Tuple

typealias TestTuple = (String, Int)

// Function

typealias TestFunction = (String) -> Int

// Existentialtype

typealias TestExistential = any Hashable
typealias TestConstrainedExistential = any Sequence<String>
typealias TestComposedExistential = any Hashable & CustomStringConvertible

// Metatype

typealias TestMetatype = TestClass.Type

// ObjC

typealias TestObjCClass = NSDate

// Existential metatype

typealias TestExistentialMetatype = (any Hashable).Type

// Nesting

enum TestNamespace {
    struct TestNestedStruct {}
}

// Generic

struct TestGenericStruct<T> {
    struct TestNestedGenericStruct<U> {}
}

// extended existential type

// heap-allocated local variable using statically generated metadata
// heap-allocated local variable using runtime instantiated meatdta

// native error object

// heap allocated task
// non-task async ob

// POD types

struct PODStruct {
    var a: Int
    var b: Double
    var c: Float
}

// Enum types

enum CEnum {
    case a
    case b
    case c
}

enum TaggedEnum {
    case a(Int)
    case b(Double)
    case c(Float)
}

indirect enum IndirectEnum {
    case a
    case b
    case c(IndirectEnum)
}

// Heap types

class HeapClass {
    var a: Int = 0
    var b: Double = 0.0
    var c: Float = 0.0
}

// Product types

//struct TestStruct {
//    var a: Void
//
//    var b: Bool
//
//    var c: Int
//    var d: Double
//    var e: Float
//
//    var f: String
//    var g: Character
//
//    var h: CEnum
//    var i: TaggedEnum
//    var j: IndirectEnum
//
//    var k: [String]
//    var l: [String: String]
//    var m: Set<String>
//
//    var n: (String, String)
//
//    var o: (Int) -> String
//    var p: any Equatable
//}
//
//class TestClass {
//    var a: Void = ()
//
//    var b: Bool = false
//
//    var c: Int = 0
//    var d: Double = 0.0
//    var e: Float = 0.0
//
//    var f: String = ""
//    var g: Character = "\0"
//
//    var h: CEnum = .a
//    var i: TaggedEnum = .a(0)
//    var j: IndirectEnum = .a
//
//    var k: [String] = []
//    var l: [String: String] = [:]
//    var m: Set<String> = []
//
//    var n: (String, String) = ("", "")
//
//    var o: (Int) -> String = { _ in "" }
//    var p: any Equatable = 0
//}
