import AppKit
import ModoKit
import SwiftData
import SwiftUI

struct ContentView: View {
    @Environment(AppModel.self) private var model
    @Environment(\.openSettings) private var openSettings
    @Query(sort: \Item.createdAt, order: .reverse) private var items: [Item]
    @State private var confirmApplyAll = false
    @State private var showThemingWizard = false

    private var visibleItems: [Item] {
        items.filter { item in
            let matchesSidebar: Bool = switch model.sidebar {
            case .all: true
            case .applied: item.isApplied
            case .starred: item.isStarred
            }
            let matchesFilter: Bool = switch model.categoryFilter {
            case .all: true
            case .customized: item.isApplied
            case .system: item.bundlePath.hasPrefix("/System/") || item.categoryName.localizedCaseInsensitiveContains("System")
            case .thirdParty: !item.bundlePath.hasPrefix("/System/")
            }
            let matchesSearch = model.search.isEmpty
                || item.title.localizedStandardContains(model.search)
                || item.categoryName.localizedStandardContains(model.search)
            return matchesSidebar && matchesFilter && matchesSearch
        }
    }

    private var activeItem: Item? {
        if let id = model.selection, let match = visibleItems.first(where: { $0.id == id }) {
            return match
        }
        return visibleItems.first
    }

    var body: some View {
        @Bindable var model = model
        NavigationSplitView {
            SidebarView(items: items)
                .navigationSplitViewColumnWidth(min: 180, ideal: 208, max: 260)
        } content: {
            AppListView(items: visibleItems)
                .navigationSplitViewColumnWidth(
                    min: 280,
                    ideal: model.viewMode == .grid ? 460 : 328,
                    max: model.viewMode == .grid ? 850 : 440
                )
        } detail: {
            if let item = activeItem {
                DetailInspectorView(item: item, allVisibleItems: visibleItems)
            } else {
                ContentUnavailableView {
                    Label(emptyTitle, systemImage: model.sidebar.symbol)
                } description: {
                    Text(emptyPrompt)
                } actions: {
                    Button("Scan Applications") {
                        model.scanInstalledApplications()
                    }
                }
            }
        }
        .confirmationDialog(
            "Apply \(model.activePackName) to \(visibleItems.count) apps?",
            isPresented: $confirmApplyAll,
            titleVisibility: .visible
        ) {
            Button("Apply to \(visibleItems.count) Apps") { model.applyRoundIconsToAll(visibleItems) }
            Button("Cancel", role: .cancel) {}
        } message: {
            Text("This replaces the icons of these apps. You can restore the originals at any time.")
        }
        .navigationTitle(model.sidebar.title)
        .navigationSubtitle("\(visibleItems.count) apps · \(model.style.backgroundStyle.title)")
        .searchable(text: $model.search, prompt: "Search applications")
        .onDrop(of: [.fileURL], isTargeted: nil) { providers in
            handleDrop(providers: providers)
        }
        .toolbar {
            ToolbarItemGroup(placement: .primaryAction) {
                Button {
                    EyedropperPicker.sampleScreenColor { hex in
                        model.style.customHex = hex
                        model.style.backgroundStyle = .customColor
                    }
                } label: {
                    Label("Eyedropper", systemImage: "eyedropper.halffull")
                }
                .help("Pick background color from screen with Eyedropper")

                Menu {
                    Picker("Background Style", selection: $model.style.backgroundStyle) {
                        ForEach(RoundBackgroundStyle.allCases) { preset in
                            Label(preset.title, systemImage: preset.symbol)
                                .tag(preset)
                        }
                    }
                } label: {
                    Label(model.style.backgroundStyle.title, systemImage: "paintpalette")
                }
                .help("Choose surface preset")

                Button {
                    showThemingWizard = true
                } label: {
                    Label("Theme System…", systemImage: "wand.and.stars")
                }
                .help("1-Click System Theming Wizard: batch style all installed apps with instant backup (⇧⌘T)")

                Button {
                    confirmApplyAll = true
                } label: {
                    Label(visibleItems.isEmpty ? "Apply All" : "Apply All (\(visibleItems.count))", systemImage: "checkmark.circle.badge.sparkles")
                }
                .help("Apply the current Modo style to all listed apps (⇧⌘A)")
                .disabled(visibleItems.isEmpty)

                Menu {
                    Button("Standard Pack (.icns, .ico, 1024px .png)") {
                        exportFolderPicker(for: visibleItems)
                    }
                    Button("Light, Dark & Tinted Kit (macOS 15 / iOS 18)") {
                        exportAppearanceVariantsPicker(for: visibleItems)
                    }
                    Button("All Resolutions (16, 32, 48, 64, 128, 256, 512, 1024 px)") {
                        exportCustomSizesPicker(for: visibleItems, sizes: [16, 32, 48, 64, 128, 256, 512, 1024], title: "All Resolutions")
                    }
                    Button("Web & Favicon Kit (16, 32, 180, 192, 512 px)") {
                        exportCustomSizesPicker(for: visibleItems, sizes: [16, 32, 180, 192, 512], title: "Web Favicon Kit")
                    }
                } label: {
                    Label("Export…", systemImage: "square.and.arrow.up")
                }
                .help("Export icon packs or custom multi-resolution PNGs (⌘E)")
                .disabled(visibleItems.isEmpty)

                Button {
                    model.scanInstalledApplications()
                } label: {
                    Label("Scan Apps", systemImage: "arrow.clockwise")
                }
                .help("Rescan /Applications and ~/Applications (⌘R)")
            }
        }
        .noInitialFocus()
        .sheet(isPresented: $showThemingWizard) {
            SystemThemingWizardView(allItems: visibleItems.isEmpty ? items : visibleItems)
        }
        .sheet(isPresented: $model.showUpdateSheet) {
            OTAUpdateSheetView()
                .themed()
        }
        .task {
            SeedData.insertIfRequested(into: model)
            if !Store.isOverridden && items.isEmpty {
                model.scanInstalledApplications()
            }
            if model.selection == nil {
                model.selection = visibleItems.first?.id
            }
            if !Store.isOverridden && !SelfTest.requested && OTAUpdateService.shared.autoCheckEnabled {
                Task {
                    await OTAUpdateService.shared.checkForUpdates(manual: false)
                }
            }
            SelfTest.runIfRequested(model: model)
            Snapshot.runIfRequested(model: model, openSettings: openSettings)
        }
    }

    private var emptyTitle: String {
        if !model.search.isEmpty { return "No Matching Apps" }
        return switch model.sidebar {
        case .all: "No Applications Found"
        case .applied: "No Circular Icons Applied Yet"
        case .starred: "No Favorite Apps"
        }
    }

    private var emptyPrompt: String {
        if !model.search.isEmpty { return "Try a different application name or category." }
        return switch model.sidebar {
        case .all: "Scan /Applications or add an app to preview Modo icons."
        case .applied: "Select an app and apply a Modo icon to restyle its macOS icon."
        case .starred: "Star your favorite apps to keep them handy."
        }
    }

    private func exportFolderPicker(for targets: [Item]) {
        let panel = NSOpenPanel()
        panel.title = "Export Icon Pack (.icns, .ico, .png) — also saved as a .zip"
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = true
        panel.prompt = "Export Here"
        if panel.runModal() == .OK, let url = panel.url {
            let safeName = model.activePackName.replacingOccurrences(of: "/", with: "-")
            let folder = url.appendingPathComponent("Modo-\(safeName)", isDirectory: true)
            let count = model.exportIconPack(for: targets, to: folder)
            if count > 0 {
                let zip = url.appendingPathComponent("Modo-\(safeName).zip")
                let ditto = Process()
                ditto.executableURL = URL(fileURLWithPath: "/usr/bin/ditto")
                ditto.arguments = ["-c", "-k", "--keepParent", folder.path, zip.path]
                try? ditto.run()
                ditto.waitUntilExit()
                model.statusMessage = "Exported \(count) icon sets to \(zip.lastPathComponent)"
            }
        }
    }

    private func exportCustomSizesPicker(for targets: [Item], sizes: [Int], title: String) {
        let panel = NSOpenPanel()
        panel.title = "Export \(title)"
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = true
        panel.prompt = "Export Here"
        if panel.runModal() == .OK, let url = panel.url {
            let safeName = title.replacingOccurrences(of: " ", with: "-")
            let folder = url.appendingPathComponent("Modo-\(safeName)", isDirectory: true)
            let count = model.exportCustomSizes(for: targets, sizes: sizes, to: folder)
            if count > 0 {
                let zip = url.appendingPathComponent("Modo-\(safeName).zip")
                let ditto = Process()
                ditto.executableURL = URL(fileURLWithPath: "/usr/bin/ditto")
                ditto.arguments = ["-c", "-k", "--keepParent", folder.path, zip.path]
                try? ditto.run()
                ditto.waitUntilExit()
                model.statusMessage = "Exported \(sizes.count) resolutions for \(count) apps to \(zip.lastPathComponent)"
            }
        }
    }

    private func exportAppearanceVariantsPicker(for items: [Item]) {
        let panel = NSOpenPanel()
        panel.title = "Export Light, Dark & Tinted PNGs"
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = true
        panel.prompt = "Export"
        if panel.runModal() == .OK, let dir = panel.url {
            _ = model.exportAppearanceVariants(for: items, to: dir)
        }
    }

    private func handleDrop(providers: [NSItemProvider]) -> Bool {
        for provider in providers {
            _ = provider.loadObject(ofClass: URL.self) { url, _ in
                guard let url = url, url.pathExtension == "app" else { return }
                Task { @MainActor in
                    self.model.addApplication(from: url)
                }
            }
        }
        return true
    }
}
