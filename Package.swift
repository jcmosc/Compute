// swift-tools-version: 6.2

import PackageDescription

let swiftCheckoutPath = "\(Context.packageDirectory)/.build/checkouts/swift"

var dependencies: [Package.Dependency] = [
    .package(url: "https://github.com/apple/swift-algorithms", from: "1.2.0"),
    .package(url: "https://github.com/groue/Semaphore", from: "0.1.0"),
]

let package = Package(
    name: "Compute",
    platforms: [.macOS(.v26)],
    products: [
        .library(name: "Compute", targets: ["Compute"]),
        .library(name: "_ComputeTestSupport", targets: ["_ComputeTestSupport"]),
    ],
    traits: [
        .trait(name: "CompatibilityModeAttributeGraphV6")
    ],
    dependencies: dependencies,
    targets: [
        .target(
            name: "Platform",
            cSettings: [
                .define("_GNU_SOURCE", .when(platforms: [.linux]))
            ]
        ),
        .target(
            name: "SwiftCorelibsCoreFoundation"
        ),
        .target(
            name: "Utilities",
            dependencies: [
                "Platform",
                .target(name: "SwiftCorelibsCoreFoundation", condition: .when(platforms: [.linux])),
            ]
        ),
        .testTarget(
            name: "UtilitiesTests",
            dependencies: ["Utilities"],
            cxxSettings: [.define("SWIFT_TESTING")],
            swiftSettings: [.interoperabilityMode(.Cxx)]
        ),
        .target(
            name: "Compute",
            dependencies: ["ComputeCxx"],
            swiftSettings: [
                .enableExperimentalFeature("Extern"),
                .unsafeFlags([
                    "-enable-library-evolution",
                    // When -enable-library-evolution is specified verify-emitted-module-interface command fails
                    "-no-verify-emitted-module-interface",
                ]),
            ]
        ),
        .testTarget(
            name: "ComputeTests",
            dependencies: [
                "Compute",
                "_ComputeTestSupport",
                .product(name: "Algorithms", package: "swift-algorithms"),
                .product(name: "Semaphore", package: "Semaphore"),
            ],
            swiftSettings: [
                .enableExperimentalFeature("Extern")
            ],
            linkerSettings: [.linkedLibrary("swiftDemangle")]
        ),
        .testTarget(
            name: "ComputeLayoutDescriptorTests",
            dependencies: [
                "Compute"
            ],
            swiftSettings: [
                .enableExperimentalFeature("Extern")
            ],
            linkerSettings: [.linkedLibrary("swiftDemangle")]
        ),
        .target(
            name: "ComputeCxx",
            dependencies: [
                "Platform",
                "Utilities",
                "ComputeCxxSwiftSupport",
                .target(name: "SwiftCorelibsCoreFoundation", condition: .when(platforms: [.linux])),
            ],
            cSettings: [
                .unsafeFlags(["-Wno-elaborated-enum-base"])
            ],
            cxxSettings: [
                .headerSearchPath(""),
                .headerSearchPath("internalInclude"),
                .define("_GNU_SOURCE", .when(platforms: [.linux])),
                .unsafeFlags([
                    "-Wno-elaborated-enum-base",
                    "-static",
                    "-DCOMPILED_WITH_SWIFT",
                    "-DPURE_BRIDGING_MODE",
                    "-isystem", "\(swiftCheckoutPath)/include",
                    "-isystem", "\(swiftCheckoutPath)/stdlib/include",
                    "-isystem", "\(swiftCheckoutPath)/stdlib/public/SwiftShims",
                ]),
            ]
        ),
        .target(name: "ComputeCxxSwiftSupport"),
        .target(name: "_ComputeTestSupport"),
    ],
    cxxLanguageStandard: .cxx20,
)
