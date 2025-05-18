public struct PointerOffset<Base, Member> {

    public var byteOffset: Int

    public init(byteOffset: Int) {
        self.byteOffset = byteOffset
    }

    public static func of(_ member: inout Member) -> PointerOffset<Base, Member> {
        return withUnsafePointer(to: &member) { memberPointer in
            let offset = UnsafeRawPointer(memberPointer) - UnsafeRawPointer(invalidScenePointer())
            return PointerOffset(byteOffset: offset)
        }
    }

    public static func offset(_ body: (inout Base) -> PointerOffset<Base, Member>) -> PointerOffset<Base, Member> {
        guard MemoryLayout<Member>.size != 0 else {
            return PointerOffset(byteOffset: 0)
        }
        return body(&invalidScenePointer().pointee)
    }

    public static func invalidScenePointer() -> UnsafeMutablePointer<Base> {
        return UnsafeMutablePointer(bitPattern: MemoryLayout<Base>.stride)!
    }

}

extension PointerOffset where Base == Member {

    public init() {
        self.byteOffset = 0
    }

}

extension PointerOffset {

    public static func + <T>(_ lhs: PointerOffset<Base, T>, _ rhs: PointerOffset<T, Member>) -> PointerOffset<
        Base, Member
    > {
        return PointerOffset(byteOffset: lhs.byteOffset + rhs.byteOffset)
    }

}

extension UnsafePointer {

    public static func + <Member>(_ lhs: UnsafePointer<Pointee>, _ rhs: PointerOffset<Pointee, Member>)
        -> UnsafePointer<
            Member
        >
    {
        return UnsafeRawPointer(lhs)
            .advanced(by: rhs.byteOffset)
            .assumingMemoryBound(to: Member.self)
    }

    public subscript<Member>(offset offset: PointerOffset<Pointee, Member>) -> Member {
        unsafeAddress {
            return UnsafeRawPointer(self)
                .advanced(by: offset.byteOffset)
                .assumingMemoryBound(to: Member.self)
        }
    }

}

extension UnsafeMutablePointer {

    public static func + <Member>(
        _ lhs: UnsafeMutablePointer,
        _ rhs: PointerOffset<Pointee, Member>
    ) -> UnsafeMutablePointer<Member> {
        return UnsafeMutableRawPointer(lhs)
            .advanced(by: rhs.byteOffset)
            .assumingMemoryBound(to: Member.self)
    }

    public subscript<Member>(offset offset: PointerOffset<Pointee, Member>) -> Member {
        unsafeAddress {
            return UnsafeRawPointer(self)
                .advanced(by: offset.byteOffset)
                .assumingMemoryBound(to: Member.self)
        }
        nonmutating unsafeMutableAddress {
            return UnsafeMutableRawPointer(self)
                .advanced(by: offset.byteOffset)
                .assumingMemoryBound(to: Member.self)
        }
    }

}
