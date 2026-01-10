// swift-tools-version: 6.2

import PackageDescription

let swiftCheckoutPath = "\(Context.packageDirectory)/Checkouts/swift"

var dependencies: [Package.Dependency] = [
    .package(name: "Compute", path: ".."),
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

let package = Package(
    name: "CompatibilityTesting",
    platforms: [.macOS(.v26)],
    dependencies: dependencies,
    targets: [
        .target(name: "CompatibilityTesting"),
        .testTarget(
            name: "ComputeCompatibilityTests",
            dependencies: [
                .product(name: "_ComputeTestSupport", package: "Compute"),
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
            name: "ComputeLayoutDescriptorCompatibilityTests",
            dependencies: [
                .product(name: "AttributeGraph", package: "DarwinPrivateFrameworks")
            ],
            swiftSettings: [
                .enableExperimentalFeature("Extern")
            ],
            linkerSettings: [.linkedLibrary("swiftDemangle")]
        )
    ]
)
