// swift-tools-version: 6.2
import PackageDescription

let package = Package(
    name: "Modo",
    platforms: [.macOS(.v26)],
    targets: [
        // Rules and data logic with no SwiftUI, so tests run without the app.
        .target(name: "ModoKit"),
        .executableTarget(
            name: "Modo",
            dependencies: ["ModoKit"],
            // App code runs on the main thread unless you say otherwise.
            swiftSettings: [.defaultIsolation(MainActor.self)]
        ),
        .testTarget(name: "ModoKitTests", dependencies: ["ModoKit"]),
        // Renders real icons and checks pixels.
        .testTarget(
            name: "ModoTests",
            dependencies: ["Modo", "ModoKit"],
            swiftSettings: [.defaultIsolation(MainActor.self)]
        ),
    ]
)
