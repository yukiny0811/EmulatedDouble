// swift-tools-version: 6.3
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "EmulatedDouble",
    products: [
        .library(
            name: "EmulatedDouble",
            targets: ["EmulatedDouble"]
        ),
    ],
    targets: [
        .target(
            name: "EmulatedDoubleCore",
            dependencies: [],
            publicHeadersPath: "include",
            cxxSettings: [
                .headerSearchPath("include"),
            ],
        ),
        .target(
            name: "EmulatedDouble",
            dependencies: [
                "EmulatedDoubleCore"
            ]
        ),
        .testTarget(
            name: "EmulatedDoubleTests",
            dependencies: [
                "EmulatedDouble",
            ]
        ),
    ],
    swiftLanguageModes: [.v6]
)
