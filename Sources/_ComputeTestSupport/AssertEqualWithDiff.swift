import Foundation
import Testing

/// Asserts that the two values are equal, providing Unix `diff`-style output if they are not.
///
/// - Parameters:
///   - actual: The actual string.
///   - expected: The expected string.
///   - message: An optional description of the failure.
///   - additionalInfo: Additional information about the failed test case that will be printed after the diff
///   - file: The file in which failure occurred. Defaults to the file name of the test case in
///     which this function was called.
///   - line: The line number on which failure occurred. Defaults to the line number on which this
///     function was called.
public func assertValuesEqualWithDiff<T: Equatable & Encodable>(
    _ actual: T,
    _ expected: T,
    _ message: String = "",
    additionalInfo: @autoclosure () -> String? = nil,
    sourceLocation: SourceLocation = #_sourceLocation
) {
    if actual == expected {
        return
    }
    let encoder = JSONEncoder()
    encoder.outputFormatting = [.sortedKeys, .prettyPrinted]
    let actualData = try! encoder.encode(actual)
    let actualString = String(data: actualData, encoding: .utf8)!
    let expectedData = try! encoder.encode(expected)
    let expectedString = String(data: expectedData, encoding: .utf8)!
    failStringsEqualWithDiff(
        actualString,
        expectedString,
        message,
        additionalInfo: additionalInfo(),
        sourceLocation: sourceLocation
    )
}

/// Asserts that the two strings are equal, providing Unix `diff`-style output if they are not.
///
/// - Parameters:
///   - actual: The actual string.
///   - expected: The expected string.
///   - message: An optional description of the failure.
///   - additionalInfo: Additional information about the failed test case that will be printed after the diff
///   - file: The file in which failure occurred. Defaults to the file name of the test case in
///     which this function was called.
///   - line: The line number on which failure occurred. Defaults to the line number on which this
///     function was called.
public func assertStringsEqualWithDiff(
    _ actual: String,
    _ expected: String,
    _ message: String = "",
    additionalInfo: @autoclosure () -> String? = nil,
    sourceLocation: SourceLocation = #_sourceLocation
) {
    if actual == expected {
        return
    }
    failStringsEqualWithDiff(
        actual,
        expected,
        message,
        additionalInfo: additionalInfo(),
        sourceLocation: sourceLocation
    )
}

/// `Issue.record` with `diff`-style output.
public func failStringsEqualWithDiff(
    _ actual: String,
    _ expected: String,
    _ message: String = "",
    additionalInfo: @autoclosure () -> String? = nil,
    sourceLocation: SourceLocation = #_sourceLocation
) {
    let stringComparison: String

    // Use `CollectionDifference` on supported platforms to get `diff`-like line-based output. On
    // older platforms, fall back to simple string comparison.
    let actualLines = actual.split(separator: "\n", omittingEmptySubsequences: false)
    let expectedLines = expected.split(separator: "\n", omittingEmptySubsequences: false)

    let difference = actualLines.difference(from: expectedLines)

    var result = ""

    var insertions = [Int: Substring]()
    var removals = [Int: Substring]()

    for change in difference {
        switch change {
        case .insert(let offset, let element, _):
            insertions[offset] = element
        case .remove(let offset, let element, _):
            removals[offset] = element
        }
    }

    var expectedLine = 0
    var actualLine = 0

    while expectedLine < expectedLines.count || actualLine < actualLines.count {
        if let removal = removals[expectedLine] {
            result += "–\(removal)\n"
            expectedLine += 1
        } else if let insertion = insertions[actualLine] {
            result += "+\(insertion)\n"
            actualLine += 1
        } else {
            result += " \(expectedLines[expectedLine])\n"
            expectedLine += 1
            actualLine += 1
        }
    }

    stringComparison = result

    var fullMessage = """
        \(message.isEmpty ? "Actual output does not match the expected" : message)
        \(stringComparison)
        """
    if let additional = additionalInfo() {
        fullMessage = """
            \(fullMessage)
            \(additional)
            """
    }
    Issue.record(Comment(rawValue: fullMessage), sourceLocation: sourceLocation)
}
