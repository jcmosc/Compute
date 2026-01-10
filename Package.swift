// swift-tools-version: 6.2

import PackageDescription

let swiftCheckoutPath = "\(Context.packageDirectory)/Checkouts/swift"

var dependencies: [Package.Dependency] = [
    .package(url: "https://github.com/apple/swift-algorithms", from: "1.2.0"),
    .package(url: "https://github.com/groue/Semaphore", from: "0.1.0"),
]

if let useLocalDepsEnv = Context.environment["COMPUTE_USE_LOCAL_DEPS"], !useLocalDepsEnv.isEmpty {
    let root: String
    if useLocalDepsEnv == "1" {
        root = ".."
    } else {
        root = useLocalDepsEnv
    }
    dependencies +=
        [
            .package(
                name: "DarwinPrivateFrameworks",
                path: "\(root)/DarwinPrivateFrameworks"
            )
        ]
} else {
    dependencies +=
        [
            .package(
                url: "https://github.com/OpenSwiftUIProject/DarwinPrivateFrameworks",
                from: "0.0.4"
            )
        ]
}

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
                    "-isystem", "\(swiftCheckoutPath)/include",
                    "-isystem", "\(swiftCheckoutPath)/stdlib/include",
                    "-isystem", "\(swiftCheckoutPath)/stdlib/public/SwiftShims",
                    "-DLLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING",
                ])
            ] + cxxSettings
        )
    }
}

let package = Package(
    name: "Compute",
    platforms: [.macOS(.v26)],
    products: [
        .library(name: "Compute", targets: ["Compute"])
    ],
    traits: [
        .trait(name: "CompatibilityModeAttributeGraphV6")
    ],
    dependencies: dependencies,
    targets: [
        .target(
            name: "Utilities"
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
            name: "ComputeCompatibilityTests",
            dependencies: [
                "_ComputeTestSupport",
                .product(name: "AttributeGraph", package: "DarwinPrivateFrameworks"),
                .product(name: "Algorithms", package: "swift-algorithms"),
                .product(name: "Semaphore", package: "Semaphore"),
            ],
            swiftSettings: [
                .interoperabilityMode(.Cxx),
                .enableExperimentalFeature("Extern"),
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
        .testTarget(
            name: "ComputeLayoutDescriptorCompatibilityTests",
            dependencies: [
                .product(name: "AttributeGraph", package: "DarwinPrivateFrameworks")
            ],
            swiftSettings: [
                .enableExperimentalFeature("Extern")
            ],
            linkerSettings: [.linkedLibrary("swiftDemangle")]
        ),
        .swiftRuntimeTarget(
            name: "ComputeCxx",
            dependencies: ["Utilities", "ComputeCxxSwiftSupport"],
            cxxSettings: [.headerSearchPath(""), .unsafeFlags(["-Wno-elaborated-enum-base"])]
        ),
        .target(name: "ComputeCxxSwiftSupport"),
        .target(name: "_ComputeTestSupport"),
    ],
    cxxLanguageStandard: .cxx20,
)
