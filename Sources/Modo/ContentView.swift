import AppKit
import ModoKit
import SwiftData
import SwiftUI

struct ContentView: View {
    @Environment(AppModel.self) private var model
    @Environment(\.openSettings) private var openSettings
    @Query(sort: \Item.createdAt, order: .reverse) private var items: [Item]

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
                .navigationSplitViewColumnWidth(min: 280, ideal: 328, max: 420)
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
                .help("Pick circular background color from screen with Eyedropper")

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
                .help("Choose Google Pixel / Material You circular background preset")

                Button {
                    model.applyRoundIconsToAll(visibleItems)
                } label: {
                    Label(visibleItems.isEmpty ? "Apply All" : "Apply All (\(visibleItems.count))", systemImage: "checkmark.circle.badge.sparkles")
                }
                .help("Apply circular Google icons to all listed apps (⇧⌘A)")
                .disabled(visibleItems.isEmpty)

                Button {
                    exportFolderPicker(for: visibleItems)
                } label: {
                    Label("Export Pack…", systemImage: "square.and.arrow.up")
                }
                .help("Export .icns, .ico, and 1024px .png icon pack (⌘E)")
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
        .task {
            SeedData.insertIfRequested(into: model)
            if !Store.isOverridden && items.isEmpty {
                model.scanInstalledApplications()
            }
            if model.selection == nil {
                model.selection = visibleItems.first?.id
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
        case .all: "Scan /Applications or add an app to preview Google Pixel circular icons."
        case .applied: "Select an app and click Apply Round Icon to convert its macOS icon."
        case .starred: "Star your favorite apps to keep them handy."
        }
    }

    private func exportFolderPicker(for targets: [Item]) {
        let panel = NSOpenPanel()
        panel.title = "Export Circular Icon Pack (.icns, .ico, .png)"
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = true
        panel.prompt = "Export Here"
        if panel.runModal() == .OK, let url = panel.url {
            model.exportIconPack(for: targets, to: url)
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
