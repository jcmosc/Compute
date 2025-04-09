// swift-tools-version: 6.0

import PackageDescription

let swiftProjectPath = "\(Context.packageDirectory)/../swift-project"

extension Target {
    fileprivate static func swiftRuntimeTarget(
        name: String,
        dependencies: [Dependency] = [],
        path: String? = nil,
        sources: [String]? = nil,
        cxxSettings: [CXXSetting] = []
    ) -> Target {
        .target(
            name: name,
            dependencies: dependencies,
            path: path ?? "Sources/\(name)",
            sources: sources,
            cxxSettings: [
                .unsafeFlags([
                    "-static",
                    "-DCOMPILED_WITH_SWIFT",
                    "-DPURE_BRIDGING_MODE",
                    "-UIBOutlet", "-UIBAction", "-UIBInspectable",
                    "-isystem", "\(swiftProjectPath)/swift/include",
                    "-isystem", "\(swiftProjectPath)/swift/stdlib/public/SwiftShims",
                    "-isystem", "\(swiftProjectPath)/swift/stdlib/public/runtime",
                    "-isystem", "\(swiftProjectPath)/llvm-project/llvm/include",
                    "-isystem", "\(swiftProjectPath)/llvm-project/clang/include",
                    "-isystem", "\(swiftProjectPath)/build/Default/swift/include",
                    "-isystem", "\(swiftProjectPath)/build/Default/llvm/include",
                    "-isystem", "\(swiftProjectPath)/build/Default/llvm/tools/clang/include",
                    "-DLLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING",
                ])
            ] + cxxSettings
        )
    }
}

let package = Package(
    name: "Compute",
    platforms: [.macOS(.v15)],
    products: [
        .library(name: "Compute", targets: ["Compute"])
    ],
    dependencies: [
        .package(url: "https://github.com/apple/swift-algorithms", from: "1.2.0")
    ],
    targets: [
        .target(name: "Utilities"),
        .target(
            name: "Compute",
            dependencies: ["ComputeCxx"],
            cxxSettings: [.headerSearchPath("../ComputeCxx")],
            swiftSettings: [.interoperabilityMode(.Cxx)]
        ),
        .testTarget(
            name: "ComputeTests",
            dependencies: ["Compute", .product(name: "Algorithms", package: "swift-algorithms")],
            swiftSettings: [.interoperabilityMode(.Cxx)],
            linkerSettings: [.linkedLibrary("swiftDemangle")]
        ),
        .swiftRuntimeTarget(
            name: "ComputeCxx",
            dependencies: ["Utilities", "EquatableSupport"],
            cxxSettings: [.headerSearchPath("")]
        ),
        .target(name: "EquatableSupport"),
    ],
    cxxLanguageStandard: .cxx20
)
