import SExp

extension TreeElement {
    var debugDescription: String {
        var printer = SExpPrinter(tag: "tree")
        print(into: &printer)
        return printer.end()
    }

    func print(into printer: inout SExpPrinter) {
        printer.push("element")
        if unsafeBitCast(type, to: UInt64.self) != 0 {
            printer.print("#:type \(type)", newline: false)
        }
        if let value {
            printer.print("#:value \(value)", newline: false)
        }
        if flags != 0 {
            printer.print("#:flags \(flags)", newline: false)
        }
        var children = self.children
        while let child = children.next() {
            child.print(into: &printer)
        }
        printer.pop()
    }
}
