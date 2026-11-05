import AppKit
import ModoKit
import SwiftData
import SwiftUI

// Modo — Native macOS Adaptive Icon Studio by Atoma
//   Modo                      the app
//   APP_STORE=<file> Modo     use another database (tests, experiments)
//   APP_SNAPSHOT=<dir> Modo   save a PNG of every sidebar page and Settings tab in light and dark mode, then quit
//   APP_SEED=1 Modo           fill an empty /tmp store with sample items (tools/snap.sh)
//   Modo --selftest           run the self-test on a /tmp store and quit with 0 or 1 (tools/selftest.sh)
//   APP_ACCENT=<id> Modo      run with another accent from Theme.all (UI/Theme.swift)
//   APP_GRID=1 Modo           draw the 8 pt grid and the layout probes over the window

@main
struct ModoApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) private var delegate
    @State private var model = AppModel(container: Store.makeContainer())

    var body: some Scene {
        Window("Modo", id: "main") {
            ContentView()
                .environment(model)
                .themed()
                .overlay { if GridOverlay.enabled { GridOverlay() } }
                .frame(minWidth: 840, minHeight: 520)
        }
        .modelContainer(model.container)
        .defaultSize(width: 1120, height: 720)
        .windowToolbarStyle(.unified)
        .commands {
            SidebarCommands()
            ToolbarCommands()
            CommandGroup(replacing: .newItem) {
                Button("Add Custom App…") { model.addItem() }
                    .keyboardShortcut("n")
                Button("Scan Installed Applications") { model.scanInstalledApplications() }
                    .keyboardShortcut("r")
            }
            CommandMenu("Icons") {
                ForEach(RoundBackgroundStyle.allCases) { preset in
                    Button(preset.title) {
                        model.style.backgroundStyle = preset
                    }
                }
                Divider()
                Button("Choose Custom Icon / SVG…") {
                    model.promptCustomArtworkForSelection()
                }
                .keyboardShortcut("i", modifiers: [.command, .shift])
                Button("Paste Custom Icon") {
                    model.pasteCustomArtworkForSelection()
                }
                .keyboardShortcut("v", modifiers: [.command, .shift])
            }
        }

        Settings {
            SettingsView()
                .environment(model)
                .themed()
        }
        .windowResizability(.contentSize)
    }
}

final class AppDelegate: NSObject, NSApplicationDelegate {
    func applicationDidFinishLaunching(_ notification: Notification) {
        NSApp.setActivationPolicy(.regular)
        NSApp.activate()
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }

    func applicationWillTerminate(_ notification: Notification) {
        if SelfTest.requested { exit(SelfTest.exitCode) }
    }
}

/// Where the data lives: ~/Library/Application Support/Modo/Modo.store
enum Store {
    nonisolated static var isOverridden: Bool {
        !(ProcessInfo.processInfo.environment["APP_STORE"] ?? "").isEmpty
    }

    static var url: URL {
        if let path = ProcessInfo.processInfo.environment["APP_STORE"], !path.isEmpty {
            return URL(fileURLWithPath: path)
        }
        let dir = URL.applicationSupportDirectory.appending(path: "Modo", directoryHint: .isDirectory)
        try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
        return dir.appending(path: "Modo.store")
    }

    static func makeContainer() -> ModelContainer {
        do {
            return try ModelContainer(for: Item.self, configurations: ModelConfiguration(url: url))
        } catch {
            fatalError("Could not open the database at \(url.path): \(error)")
        }
    }
}
