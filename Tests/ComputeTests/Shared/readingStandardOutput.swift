import Foundation

/// Additionally writes any data written to standard output into the given output stream.
///
/// - Parameters:
///   - output: An output stream to receive the standard output text
///   - encoding: The encoding to use when converting standard output into text.
///   - body: A closure that is executed immediately.
/// - Returns: The return value, if any, of the `body` closure.
func printingStandardOutput<Target, Result>(
    to output: inout Target,
    encoding: String.Encoding = .utf8,
    body: () -> Result
)
    async -> Result where Target: TextOutputStream
{
    var result: Result? = nil

    let consumer = Pipe()  // reads from stdout
    let producer = Pipe()  // writes to stdout

    let stream = AsyncStream<Data> { continuation in
        let clonedStandardOutput = dup(STDOUT_FILENO)
        defer {
            dup2(clonedStandardOutput, STDOUT_FILENO)
            close(clonedStandardOutput)
        }

        dup2(STDOUT_FILENO, producer.fileHandleForWriting.fileDescriptor)
        dup2(consumer.fileHandleForWriting.fileDescriptor, STDOUT_FILENO)

        consumer.fileHandleForReading.readabilityHandler = { fileHandle in
            let chunk = fileHandle.availableData
            if chunk.isEmpty {
                continuation.finish()
            } else {
                continuation.yield(chunk)
                producer.fileHandleForWriting.write(chunk)
            }
        }

        result = body()
        try! consumer.fileHandleForWriting.close()
    }

    for await chunk in stream {
        output.write(String(data: chunk, encoding: encoding)!)
    }

    return result!
}

func printingStandardError<Target, Result>(
    to stream: inout Target,
    encoding: String.Encoding = .utf8,
    body: () -> Result
) async
    -> Result where Target: TextOutputStream
{
    var result: Result? = nil

    let consumer = Pipe()  // reads from stderr
    let producer = Pipe()  // writes to stderr

    let chunks = AsyncStream<Data> { continuation in
        let clonedStandardError = dup(STDERR_FILENO)
        defer {
            dup2(clonedStandardError, STDERR_FILENO)
            close(clonedStandardError)
        }

        dup2(STDERR_FILENO, producer.fileHandleForWriting.fileDescriptor)
        dup2(consumer.fileHandleForWriting.fileDescriptor, STDERR_FILENO)

        consumer.fileHandleForReading.readabilityHandler = { fileHandle in
            let chunk = fileHandle.availableData
            if chunk.isEmpty {
                continuation.finish()
            } else {
                continuation.yield(chunk)
                producer.fileHandleForWriting.write(chunk)
            }
        }

        result = body()
        try! consumer.fileHandleForWriting.close()
    }

    for await chunk in chunks {
        guard let chunkString = String(data: chunk, encoding: encoding) else {
            continue
        }
        stream.write(chunkString)
    }

    return result!
}
