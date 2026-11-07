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
    @AppStorage("showMenuBarExtra") private var showMenuBarExtra = true

    init() {
        let args = Array(CommandLine.arguments.dropFirst())
        if !args.isEmpty && !SelfTest.requested && !args[0].hasPrefix("-psn") {
            let first = args[0].lowercased()
            let knownCommands: Set<String> = [
                "style", "restore", "folder", "batch", "showcase", "export", "theme",
                "--help", "-h", "help", "--version", "-v", "version"
            ]
            if knownCommands.contains(first) {
                let code = ModoCLI.run(arguments: args)
                exit(code)
            }
        }
    }

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
            CommandGroup(after: .appInfo) {
                Button("Check for Updates…") {
                    model.showUpdateSheet = true
                    Task {
                        await OTAUpdateService.shared.checkForUpdates(manual: true)
                    }
                }
                Divider()
            }

            CommandGroup(replacing: .newItem) {
                Button("Add Custom App…") { model.addItem() }
                    .keyboardShortcut("n")
                Button("Add Folder to Style…") { model.promptFolderPicker() }
                    .keyboardShortcut("o", modifiers: [.command, .shift])
                Divider()
                Button("Export Standard Icon Pack…") {
                    if let item = model.selectedItem() {
                        model.promptExportStandard(for: [item])
                    }
                }
                .keyboardShortcut("e")
                .disabled(model.selection == nil)

                Button("Export Light, Dark & Tinted Kit…") {
                    if let item = model.selectedItem() {
                        model.promptExportAppearance(for: [item])
                    }
                }
                .disabled(model.selection == nil)

                Button("Export All Resolutions (16–1024px)…") {
                    if let item = model.selectedItem() {
                        model.promptExportCustomSizes(for: [item], sizes: [16, 32, 48, 64, 128, 256, 512, 1024], title: "All Resolutions")
                    }
                }
                .disabled(model.selection == nil)

                Divider()
                Button("Batch Convert Folder to Icon Pack…") { model.showBatchCreator = true }
                    .keyboardShortcut("b", modifiers: [.command, .shift])
                Divider()
                Button("Scan Installed Applications") { model.scanInstalledApplications() }
                    .keyboardShortcut("r")
            }

            CommandGroup(after: .pasteboard) {
                Divider()
                Button("Choose Custom Icon / SVG…") {
                    model.promptCustomArtworkForSelection()
                }
                .keyboardShortcut("i", modifiers: [.command, .shift])

                Button("Paste Custom Icon") {
                    model.pasteCustomArtworkForSelection()
                }
                .keyboardShortcut("v", modifiers: [.command, .shift])

                Button("Clear Custom Icon") {
                    if let item = model.selectedItem() {
                        model.clearCustomArtwork(for: item)
                    }
                }
                .disabled(model.selection == nil)
            }

            CommandGroup(before: .sidebar) {
                Button("As List") { model.viewMode = .list }
                    .keyboardShortcut("1", modifiers: [.command])
                Button("As Grid") { model.viewMode = .grid }
                    .keyboardShortcut("2", modifiers: [.command])
                Divider()
                Button("Zoom In Grid Icons") {
                    model.gridIconSize = min(160, model.gridIconSize + 16)
                }
                .keyboardShortcut("+", modifiers: [.command])

                Button("Zoom Out Grid Icons") {
                    model.gridIconSize = max(36, model.gridIconSize - 16)
                }
                .keyboardShortcut("-", modifiers: [.command])

                Button("Actual Size Grid Icons") {
                    model.gridIconSize = 64
                }
                .keyboardShortcut("0", modifiers: [.command])
                Divider()
            }

            SidebarCommands()
            ToolbarCommands()

            CommandGroup(after: .toolbar) {
                Divider()
                Menu("Filter Library") {
                    Button("All Apps") { model.sidebar = .all }
                    Button("Styled Only") { model.sidebar = .applied }
                    Button("Favorites") { model.sidebar = .starred }
                    Button("Folders") { model.sidebar = .folders }
                }
            }

            CommandMenu("Style") {
                Button("Apply Style to Selected") {
                    if let item = model.selectedItem() {
                        model.applyRoundIcon(to: item)
                    }
                }
                .keyboardShortcut(.return, modifiers: [.command])
                .disabled(model.selection == nil)

                Button("Restore Original Icon") {
                    if let item = model.selectedItem() {
                        model.restoreOriginalIcon(for: item)
                    }
                }
                .keyboardShortcut(.delete, modifiers: [.command])
                .disabled(model.selection == nil)

                Divider()

                Menu("Surface Presets") {
                    ForEach(RoundBackgroundStyle.allCases) { preset in
                        Button {
                            model.style.backgroundStyle = preset
                        } label: {
                            if model.style.backgroundStyle == preset {
                                Text("✓ \(preset.title)")
                            } else {
                                Text(preset.title)
                            }
                        }
                    }
                }

                Button("Eyedropper Color Picker…") {
                    EyedropperPicker.sampleScreenColor { hex in
                        model.style.customHex = hex
                        model.style.backgroundStyle = .customColor
                    }
                }
                .keyboardShortcut("c", modifiers: [.command, .shift])

                Button("Reset Style to Default") {
                    model.style = RoundIconStyle()
                }
                .keyboardShortcut("0", modifiers: [.command, .shift])
            }

            CommandMenu("Tools") {
                Button("1-Click System Theming Wizard…") {
                    model.showSystemTheming = true
                }
                .keyboardShortcut("t", modifiers: [.command, .shift])

                Button("Batch Pack Generator…") {
                    model.showBatchCreator = true
                }

                Button("App Showcase & README Hero Studio…") {
                    model.showShowcaseStudio = true
                }

                Divider()

                Button("Restart macOS Dock") {
                    model.restartDock()
                }
            }
        }

        Settings {
            SettingsView()
                .environment(model)
                .themed()
        }
        .windowResizability(.contentSize)

        MenuBarExtra("Modo Quick Drop", systemImage: "circle.grid.2x2.fill", isInserted: $showMenuBarExtra) {
            QuickDropMenuBarView()
                .environment(model)
                .themed()
        }
        .menuBarExtraStyle(.window)
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
