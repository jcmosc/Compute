// swift-tools-version: 6.3

import PackageDescription

let package = Package(
    name: "CompatibilityTesting",
    platforms: [.macOS(.v26)],
    dependencies: [
        .package(name: "Compute", path: ".."),
        .package(url: "https://github.com/apple/swift-algorithms", from: "1.2.0"),
        .package(url: "https://github.com/groue/Semaphore", from: "0.1.0"),
        .package(url: "https://github.com/jcmosc/swift-sexp", branch: "main"),
    ],
    targets: [
        .target(name: "CompatibilityTesting"),
        .testTarget(
            name: "ComputeCompatibilityTests",
            dependencies: [
                "AttributeGraph",
                .product(name: "_ComputeTestSupport", package: "Compute"),
                .product(name: "Algorithms", package: "swift-algorithms"),
                .product(name: "Semaphore", package: "Semaphore"),
                .product(name: "SExp", package: "swift-sexp"),
            ],
            swiftSettings: [
                .define("COMPATIBILITY_TESTS"),
                .enableExperimentalFeature("Extern"),
            ],
            linkerSettings: [.linkedLibrary("swiftDemangle")]
        ),
        .testTarget(
            name: "ComputeLayoutDescriptorCompatibilityTests",
            dependencies: [
                "AttributeGraph"
            ],
            swiftSettings: [
                .define("COMPATIBILITY_TESTS"),
                .enableExperimentalFeature("Extern"),
            ],
            linkerSettings: [.linkedLibrary("swiftDemangle")]
        ),
        .binaryTarget(name: "AttributeGraph", path: "Frameworks/AttributeGraph.xcframework"),
    ]
)
