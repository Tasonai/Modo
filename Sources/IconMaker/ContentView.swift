import AppKit
import IconMakerKit
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

private struct SidebarView: View {
    @Environment(AppModel.self) private var model
    let items: [Item]

    var body: some View {
        @Bindable var model = model
        List(selection: $model.sidebar) {
            Section("Library") {
                ForEach(SidebarItem.allCases) { page in
                    Label(page.title, systemImage: page.symbol)
                        .badge(count(for: page))
                        .tag(page)
                }
            }

            Section("Surface Presets") {
                ForEach(RoundBackgroundStyle.allCases) { preset in
                    Button {
                        model.style.backgroundStyle = preset
                    } label: {
                        HStack(spacing: Space.sm) {
                            Image(systemName: preset.symbol)
                                .foregroundStyle(model.style.backgroundStyle == preset ? Palette.accentText : .secondary)
                            Text(preset.title)
                                .foregroundStyle(.primary)
                            Spacer()
                            if model.style.backgroundStyle == preset {
                                Image(systemName: "checkmark")
                                    .font(.caption.weight(.bold))
                                    .foregroundStyle(Palette.accentText)
                            }
                        }
                    }
                    .buttonStyle(.plain)
                }
            }
        }
    }

    private func count(for page: SidebarItem) -> Int {
        switch page {
        case .all: items.count
        case .applied: items.filter(\.isApplied).count
        case .starred: items.filter(\.isStarred).count
        }
    }
}

private struct AppListView: View {
    @Environment(AppModel.self) private var model
    @Environment(\.modelContext) private var context
    let items: [Item]

    var body: some View {
        @Bindable var model = model
        VStack(spacing: 0) {
            // Quick Filter Pills (All, Customized, System, Third-Party)
            HStack(spacing: Space.xs) {
                ForEach(AppCategoryFilter.allCases) { filter in
                    Button {
                        withAnimation(.snappy(duration: 0.2)) {
                            model.categoryFilter = filter
                        }
                    } label: {
                        Text(filter.rawValue)
                            .font(.caption2.weight(model.categoryFilter == filter ? .semibold : .regular))
                            .padding(.horizontal, Space.sm)
                            .padding(.vertical, 3)
                            .background(
                                Capsule()
                                    .fill(model.categoryFilter == filter ? Palette.accent.opacity(0.18) : Color.clear)
                            )
                            .overlay(
                                Capsule()
                                    .strokeBorder(model.categoryFilter == filter ? Palette.accent : Color.secondary.opacity(0.2), lineWidth: 1)
                            )
                            .foregroundStyle(model.categoryFilter == filter ? Palette.accentText : Color.secondary)
                    }
                    .buttonStyle(.plain)
                }
                Spacer(minLength: 0)
            }
            .padding(.horizontal, Space.md)
            .padding(.vertical, Space.xs)
            .background(.background.secondary.opacity(0.4))

            Divider()

            List(items, selection: $model.selection) { item in
                AppRowView(item: item)
                    .tag(item.id)
                    .contextMenu {
                        Button(item.isApplied ? "Update Round Icon" : "Apply Round Icon") {
                            model.applyRoundIcon(to: item)
                        }
                        if item.isApplied {
                            Button("Restore Original Icon") {
                                model.restoreOriginalIcon(for: item)
                            }
                        }
                        Divider()
                        Button(item.isStarred ? "Remove from Favorites" : "Add to Favorites") {
                            item.isStarred.toggle()
                            try? context.save()
                        }
                        Divider()
                        Button("Delete from Library", role: .destructive) {
                            if model.selection == item.id { model.selection = nil }
                            context.delete(item)
                            try? context.save()
                        }
                    }
            }
        }
        .overlay {
            if items.isEmpty {
                VStack(spacing: Space.sm) {
                    Image(systemName: "magnifyingglass")
                        .font(.title2)
                        .foregroundStyle(.tertiary)
                    Text("No Matching Apps")
                        .font(.callout.weight(.medium))
                        .foregroundStyle(.secondary)
                    if !model.search.isEmpty {
                        Button("Clear Search") {
                            model.search = ""
                        }
                        .controlSize(.small)
                    }
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
                .padding()
            }
        }
    }
}

private struct AppRowView: View {
    @Environment(AppModel.self) private var model
    let item: Item

    var body: some View {
        HStack(spacing: Space.md) {
            Image(nsImage: model.roundIcon(for: item))
                .resizable()
                .interpolation(.high)
                .frame(width: 40, height: 40)

            VStack(alignment: .leading, spacing: 2) {
                HStack(spacing: Space.xs) {
                    Text(item.title)
                        .font(.body.weight(.medium))
                        .lineLimit(1)
                    if item.isStarred {
                        Image(systemName: "star.fill")
                            .font(.caption2)
                            .foregroundStyle(.yellow)
                    }
                }
                Text(item.categoryName)
                    .font(.caption)
                    .foregroundStyle(.secondary)
                    .lineLimit(1)
            }

            Spacer(minLength: Space.sm)

            if item.isApplied {
                Text("Round")
                    .font(.caption2.weight(.semibold))
                    .foregroundStyle(Palette.accentText)
                    .padding(.horizontal, Space.sm)
                    .padding(.vertical, 2)
                    .background(Palette.accent.opacity(0.14), in: Capsule())
            }
        }
        .padding(.vertical, Space.xs)
    }
}

private struct DetailInspectorView: View {
    @Environment(AppModel.self) private var model
    @Environment(\.modelContext) private var context
    @Bindable var item: Item
    let allVisibleItems: [Item]
    @State private var dragStartOffset: CGPoint?

    private var customColorBinding: Binding<Color> {
        Binding(
            get: { EyedropperPicker.color(from: model.style.customHex) },
            set: { newColor in
                model.style.customHex = EyedropperPicker.hex(from: newColor)
                model.style.backgroundStyle = .customColor
            }
        )
    }

    var body: some View {
        @Bindable var model = model
        ScrollView {
            VStack(alignment: .leading, spacing: Space.lg) {
                headerSection
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .gridProbe("header", area: "detail")

                comparisonCard
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .gridProbe("comparison", area: "detail")

                styleControlsCard
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .gridProbe("controls", area: "detail")

                actionFooter
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .gridProbe("actions", area: "detail")
            }
            .padding(Space.margin)
            .frame(maxWidth: .infinity, alignment: .topLeading)
            .gridArea("detail", inset: Space.margin)
        }
    }

    private var headerSection: some View {
        HStack(alignment: .center, spacing: Space.md) {
            VStack(alignment: .leading, spacing: Space.xs) {
                HStack(spacing: Space.sm) {
                    Text(item.title)
                        .font(.title2.weight(.semibold))
                    if item.isApplied {
                        Label("Circular Icon Active", systemImage: "checkmark.seal.fill")
                            .font(.caption.weight(.semibold))
                            .foregroundStyle(Palette.accentText)
                            .padding(.horizontal, Space.sm)
                            .padding(.vertical, Space.xs)
                            .background(Palette.accent.opacity(0.14), in: Capsule())
                    }
                }
                Text("\(item.categoryName) · \(item.bundleIdentifier)")
                    .font(.subheadline)
                    .foregroundStyle(.secondary)
            }
            Spacer()
            Button {
                item.isStarred.toggle()
                try? context.save()
            } label: {
                Label(item.isStarred ? "Starred" : "Favorite",
                      systemImage: item.isStarred ? "star.fill" : "star")
            }
            .help("Toggle Favorite status")
        }
    }

    private var comparisonCard: some View {
        let shape = RoundedRectangle(cornerRadius: Radius.md, style: .continuous)
        return HStack(spacing: Space.xxl) {
            Spacer(minLength: 0)
            VStack(spacing: Space.sm) {
                Image(nsImage: model.originalIcon(for: item))
                    .resizable()
                    .interpolation(.high)
                    .frame(width: 96, height: 96)
                Text("Original Squircle")
                    .font(.callout.weight(.medium))
                Text("macOS Default")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            VStack(spacing: Space.xs) {
                Image(systemName: "arrow.right.circle.fill")
                    .font(.title2)
                    .foregroundStyle(Palette.accentText)
                Text("\(Int((model.style.iconScale * 100).rounded()))% Scale")
                    .font(.caption2.monospacedDigit())
                    .foregroundStyle(.secondary)
            }

            VStack(spacing: Space.sm) {
                Image(nsImage: model.roundIcon(for: item))
                    .resizable()
                    .interpolation(.high)
                    .frame(width: 96, height: 96)
                    .animation(.interactiveSpring(response: 0.28, dampingFraction: 0.86), value: model.style.offsetX)
                    .animation(.interactiveSpring(response: 0.28, dampingFraction: 0.86), value: model.style.offsetY)
                    .gesture(
                        DragGesture(minimumDistance: 1)
                            .onChanged { value in
                                let base = dragStartOffset ?? CGPoint(x: model.style.offsetX, y: model.style.offsetY)
                                if dragStartOffset == nil { dragStartOffset = base }
                                let nx = min(max(base.x + Double(value.translation.width) / 48.0, -1.0), 1.0)
                                let ny = min(max(base.y - Double(value.translation.height) / 48.0, -1.0), 1.0)
                                model.style.offsetX = nx
                                model.style.offsetY = ny
                            }
                            .onEnded { _ in
                                dragStartOffset = nil
                            }
                    )
                    .onTapGesture(count: 2) {
                        NSHapticFeedbackManager.defaultPerformer.perform(.alignment, performanceTime: .default)
                        model.style.resetPosition()
                    }
                    .help("Drag to pan icon · Double click to center")
                Text("Google Pixel Round")
                    .font(.callout.weight(.semibold))
                Text("Drag to Pan · Double click to Center")
                    .font(.caption)
                    .foregroundStyle(Palette.accentText)
            }
            Spacer(minLength: 0)
        }
        .padding(Space.lg)
        .background(.background.secondary, in: shape)
        .overlay(shape.strokeBorder(.separator, lineWidth: 1))
    }

    private var styleControlsCard: some View {
        @Bindable var model = model
        let shape = RoundedRectangle(cornerRadius: Radius.md, style: .continuous)
        return VStack(alignment: .leading, spacing: Space.md) {
            Text("Google Pixel & Material You Studio")
                .font(.headline)

            LabeledContent("Surface Preset") {
                Picker("Surface Preset", selection: $model.style.backgroundStyle) {
                    ForEach(RoundBackgroundStyle.allCases) { preset in
                        Text(preset.title).tag(preset)
                    }
                }
                .labelsHidden()
                .frame(width: 240)
            }

            LabeledContent("Color & Eyedropper") {
                HStack(spacing: Space.sm) {
                    Button {
                        EyedropperPicker.sampleScreenColor { hex in
                            model.style.customHex = hex
                            model.style.backgroundStyle = .customColor
                        }
                    } label: {
                        Label("Sample Color", systemImage: "eyedropper.halffull")
                    }
                    .controlSize(.small)
                    .help("Pick any color from the icon or screen with macOS Eyedropper")

                    ColorPicker("Custom Surface Color", selection: customColorBinding, supportsOpacity: false)
                        .labelsHidden()
                        .help("Choose custom surface color")

                    Text(MaterialRGB(hex: model.style.customHex).hexString)
                        .font(.caption.monospaced())
                        .foregroundStyle(.secondary)

                    HStack(spacing: Space.xs) {
                        ForEach(ColorSwatchToken.presets.prefix(5)) { swatch in
                            Button {
                                model.style.customHex = swatch.hex
                                model.style.backgroundStyle = .customColor
                            } label: {
                                Circle()
                                    .fill(swatch.color)
                                    .frame(width: 16, height: 16)
                                    .overlay(
                                        Circle()
                                            .strokeBorder(.primary.opacity(model.style.customHex == swatch.hex ? 0.65 : 0.18),
                                                          lineWidth: 1)
                                    )
                            }
                            .buttonStyle(.plain)
                            .help(swatch.name)
                        }
                    }
                }
            }

            Divider()

            LabeledContent("Glyph Scale (\(Int((model.style.iconScale * 100).rounded()))%)") {
                HStack(spacing: Space.sm) {
                    Slider(value: $model.style.iconScale, in: 0.30...2.50)
                        .frame(width: 176)
                    Button("100%") { model.style.iconScale = 1.0 }
                        .controlSize(.small)
                        .help("Set scale to 100%")
                    Button("135%") { model.style.iconScale = 1.35 }
                        .controlSize(.small)
                        .help("Zoom icon to 135% inside circular mask")
                }
            }

            LabeledContent("Horizontal X (\(Int((model.style.offsetX * 100).rounded()))%)") {
                HStack(spacing: Space.xs) {
                    Button {
                        model.style.offsetX = max(model.style.offsetX - 0.05, -1.0)
                    } label: {
                        Image(systemName: "arrow.left")
                    }
                    .controlSize(.small)
                    .help("Move Left")

                    Slider(value: $model.style.offsetX, in: -1.0...1.0)
                        .frame(width: 160)

                    Button {
                        model.style.offsetX = min(model.style.offsetX + 0.05, 1.0)
                    } label: {
                        Image(systemName: "arrow.right")
                    }
                    .controlSize(.small)
                    .help("Move Right")
                }
            }

            LabeledContent("Vertical Y (\(Int((model.style.offsetY * 100).rounded()))%)") {
                HStack(spacing: Space.xs) {
                    Button {
                        model.style.offsetY = max(model.style.offsetY - 0.05, -1.0)
                    } label: {
                        Image(systemName: "arrow.down")
                    }
                    .controlSize(.small)
                    .help("Move Down")

                    Slider(value: $model.style.offsetY, in: -1.0...1.0)
                        .frame(width: 160)

                    Button {
                        model.style.offsetY = min(model.style.offsetY + 0.05, 1.0)
                    } label: {
                        Image(systemName: "arrow.up")
                    }
                    .controlSize(.small)
                    .help("Move Up")

                    Button("Center") {
                        NSHapticFeedbackManager.defaultPerformer.perform(.alignment, performanceTime: .default)
                        model.style.resetPosition()
                    }
                    .controlSize(.small)
                    .disabled(abs(model.style.offsetX) < 0.005 && abs(model.style.offsetY) < 0.005)
                    .help("Center icon inside circle (0%, 0%)")
                }
            }

            LabeledContent("Elevation Shadow (\(Int((model.style.shadowIntensity * 100).rounded()))%)") {
                Slider(value: $model.style.shadowIntensity, in: 0.0...1.0)
                    .frame(width: 240)
            }

            Divider()

            HStack(spacing: Space.xl) {
                Toggle("Auto-Trim Margins", isOn: $model.style.autoTrim)
                Toggle("Inner Glyph Shadow", isOn: $model.style.innerShadow)
            }
            HStack(spacing: Space.xl) {
                Toggle("Full-Bleed Square Mask", isOn: $model.style.fullBleedSquares)
                Toggle("Material Monochrome Tint", isOn: $model.style.monochromeTint)
            }
        }
        .padding(Space.lg)
        .background(.background.secondary, in: shape)
        .overlay(shape.strokeBorder(.separator, lineWidth: 1))
    }

    private var actionFooter: some View {
        HStack(spacing: Space.md) {
            Button {
                model.applyRoundIcon(to: item)
            } label: {
                Label(item.isApplied ? "Update Round Icon" : "Apply Round Icon",
                      systemImage: "checkmark.circle.fill")
            }
            .buttonStyle(.borderedProminent)
            .keyboardShortcut(.return, modifiers: .command)

            Button {
                model.restoreOriginalIcon(for: item)
            } label: {
                Label("Restore Original", systemImage: "arrow.uturn.backward")
            }
            .disabled(!item.isApplied)

            Spacer()

            Text(model.statusMessage)
                .font(.caption)
                .foregroundStyle(.secondary)
                .lineLimit(1)
        }
    }
}
