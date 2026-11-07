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
            case .folders: item.isFolder
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
            ToolbarItem(placement: .navigation) {
                Picker("View Mode", selection: $model.viewMode) {
                    Image(systemName: "list.bullet")
                        .tag(AppViewMode.list)
                        .help("View as List (⌘1)")
                    Image(systemName: "square.grid.2x2")
                        .tag(AppViewMode.grid)
                        .help("View as Grid (⌘2)")
                }
                .pickerStyle(.segmented)
            }

            ToolbarItemGroup(placement: .primaryAction) {
                Button {
                    model.showSystemTheming = true
                } label: {
                    Label("Theme System…", systemImage: "wand.and.stars")
                }
                .help("1-Click System Theming Wizard: batch style all installed apps with instant backup (⇧⌘T)")

                Button {
                    model.showBatchCreator = true
                } label: {
                    Label("Batch Pack…", systemImage: "square.stack.3d.up.fill")
                }
                .help("Batch convert a folder of logos/images into an icon pack (⇧⌘B)")

                Menu {
                    Button("Standard Pack (.icns, .ico, 1024px .png)") {
                        model.promptExportStandard(for: visibleItems)
                    }
                    Button("Light, Dark & Tinted Kit (macOS 15 / iOS 18)") {
                        model.promptExportAppearance(for: visibleItems)
                    }
                    Button("All Resolutions (16, 32, 48, 64, 128, 256, 512, 1024 px)") {
                        model.promptExportCustomSizes(for: visibleItems, sizes: [16, 32, 48, 64, 128, 256, 512, 1024], title: "All Resolutions")
                    }
                    Button("Web & Favicon Kit (16, 32, 180, 192, 512 px)") {
                        model.promptExportCustomSizes(for: visibleItems, sizes: [16, 32, 180, 192, 512], title: "Web Favicon Kit")
                    }
                } label: {
                    Label("Export…", systemImage: "square.and.arrow.up")
                }
                .help("Export icon packs or custom multi-resolution PNGs (⌘E)")
                .disabled(visibleItems.isEmpty)

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
                    model.scanInstalledApplications()
                } label: {
                    Label("Scan Apps", systemImage: "arrow.clockwise")
                }
                .help("Rescan /Applications and ~/Applications (⌘R)")

                Button {
                    confirmApplyAll = true
                } label: {
                    Label(visibleItems.isEmpty ? "Apply All" : "Apply All (\(visibleItems.count))", systemImage: "checkmark.circle.badge.sparkles")
                }
                .help("Apply the current Modo style to all listed apps (⇧⌘A)")
                .disabled(visibleItems.isEmpty)
            }
        }
        .noInitialFocus()
        .sheet(isPresented: $model.showSystemTheming) {
            SystemThemingWizardView(allItems: visibleItems.isEmpty ? items : visibleItems)
        }
        .sheet(isPresented: $model.showUpdateSheet) {
            OTAUpdateSheetView()
                .themed()
        }
        .sheet(isPresented: $model.showBatchCreator) {
            BatchCreatorSheetView()
                .themed()
        }
        .sheet(isPresented: $model.showShowcaseStudio) {
            if let item = activeItem {
                ShowcaseSheetView(item: item)
                    .themed()
            }
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
        case .folders: "No Folders Added"
        }
    }

    private var emptyPrompt: String {
        if !model.search.isEmpty { return "Try a different application name or category." }
        return switch model.sidebar {
        case .all: "Scan /Applications or add an app to preview Modo icons."
        case .applied: "Select an app and apply a Modo icon to restyle its macOS icon."
        case .starred: "Star your favorite apps to keep them handy."
        case .folders: "Drop any folder into Modo or use File > Add Folder to Style."
        }
    }

    private func handleDrop(providers: [NSItemProvider]) -> Bool {
        for provider in providers {
            _ = provider.loadObject(ofClass: URL.self) { url, _ in
                guard let url = url else { return }
                Task { @MainActor in
                    var isDir: ObjCBool = false
                    if FileManager.default.fileExists(atPath: url.path, isDirectory: &isDir), isDir.boolValue {
                        if url.pathExtension == "app" {
                            self.model.addApplication(from: url)
                        } else {
                            let candidates = BatchIconEngine.scanDirectory(at: url)
                            if candidates.count >= 2 {
                                self.model.batchInitialFolderURL = url
                                self.model.showBatchCreator = true
                            } else {
                                self.model.addFolder(from: url)
                            }
                        }
                    }
                }
            }
        }
        return true
    }
}
