# Compute

An incremental computation library for Swift.

Compute is a reimplementation of the AttributeGraph framework on Apple
platforms.

## Building

### Project setup

This project has a dependency on the Swift language codebase, which must be
configured manually.

1. Clone the `swiftlang/swift` repository to `Checkouts/swift`.

   From within the package directory:

   ```sh
   mkdir -p Checkouts
   cd Checkouts
   git clone git@github.com:swiftlang/swift.git swift
   ```

2. Write the `CMakeConfig.h` file:

   From within the Swift repository:

   ```sh
   cat > include/swift/Runtime/CMakeConfig.h << EOF
   #ifndef SWIFT_RUNTIME_CMAKECONFIG_H
   #define SWIFT_RUNTIME_CMAKECONFIG_H
   #define SWIFT_VERSION_MAJOR "6"
   #define SWIFT_VERSION_MINOR "2"
   #endif
   EOF
   ```

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
