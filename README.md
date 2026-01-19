# Compute

An incremental computation library for Swift.

Compute is a reimplementation of the AttributeGraph framework on Apple
platforms.

## Building

### Project setup

This project has a dependency on the Swift language codebase, which must be
cloned separately:

```sh
./Scripts/clone-swift.sh
```

This script will clone the Swift language repository to
`.build/checkouts/swift`.

### Build as a Swift package

```sh
swift build
```

### Build as a framework

This package can also be built as a XCFramework bundle using Xcode by running
`Scripts/create-xcframework.sh`.

## Acknowledgments

Thank you to [OpenSwiftUIProject](https://github.com/OpenSwiftUIProject/OpenGraph/tree/main)
for providing much insight into AttributeGraph and the Swift runtime.
