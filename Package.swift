// swift-tools-version: 6.0

import PackageDescription

let package = Package(
    name: "Compute",
    platforms: [.macOS(.v15)],
    products: [
        .library(name: "Compute", targets: ["Compute"])
    ],
    targets: [
        .target(
            name: "Compute",
            dependencies: ["ComputeCxx"],
            cxxSettings: [.headerSearchPath("../ComputeCxx")],
            swiftSettings: [.interoperabilityMode(.Cxx)]
        ),
        .target(
            name: "ComputeCxx"
        ),
    ],
    cxxLanguageStandard: .cxx20
)
