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

#if os(macOS) || os(iOS)
typealias TestForeignClass = CFDate
#endif

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

struct TestPackedGenericStruct<each T> {
    var value: (repeat each T)
}
