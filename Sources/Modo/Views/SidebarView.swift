import AppKit
import ModoKit
import SwiftData
import SwiftUI
import UniformTypeIdentifiers

struct SidebarView: View {
    @Environment(AppModel.self) private var model
    let items: [Item]
    @State private var showIconSets = false
    @State private var showGlyphLibrary = false

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

            Section("Icon Packs") {
                ForEach(IconPack.allPacks) { pack in
                    Button {
                        model.applyPack(pack, items: items)
                    } label: {
                        HStack(spacing: Space.sm) {
                            Image(systemName: pack.iconSymbol)
                                .foregroundStyle(model.activePackId == pack.id ? Palette.accentText : .secondary)
                            Text(pack.name)
                                .foregroundStyle(.primary)
                            Spacer()
                            if model.activePackId == pack.id {
                                Image(systemName: "checkmark")
                                    .font(.caption.weight(.bold))
                                    .foregroundStyle(Palette.accentText)
                            }
                        }
                    }
                    .buttonStyle(.plain)
                    .help("\(pack.name) — \(pack.subtitle)")
                }

                Menu {
                    Toggle("Auto-match Brand Vectors on Pack Selection", isOn: $model.autoVectorizePacks)
                    Divider()
                    Button {
                        model.autoVectorizeAll(items: items)
                    } label: {
                        Label("Auto-match Brand Vectors for All Apps", systemImage: "sparkles")
                    }
                    Button(role: .destructive) {
                        model.resetAllToOriginalArtwork(items: items)
                    } label: {
                        Label("Reset All to Original App Icons", systemImage: "arrow.counterclockwise")
                    }
                } label: {
                    Label("Vector Options…", systemImage: "sparkles")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
                .menuStyle(.borderlessButton)
            }

            Section("Icon Sets") {
                Button {
                    model.activeIconSetID = nil
                } label: {
                    HStack(spacing: Space.sm) {
                        Image(systemName: "app.dashed")
                            .foregroundStyle(model.activeIconSetID == nil ? Palette.accentText : .secondary)
                        Text("Original icons")
                            .foregroundStyle(.primary)
                        Spacer()
                        if model.activeIconSetID == nil {
                            Image(systemName: "checkmark")
                                .font(.caption.weight(.bold))
                                .foregroundStyle(Palette.accentText)
                        }
                    }
                }
                .buttonStyle(.plain)
                ForEach(model.installedIconSets) { set in
                    Button {
                        model.activeIconSetID = set.id
                    } label: {
                        HStack(spacing: Space.sm) {
                            Image(systemName: "photo.stack")
                                .foregroundStyle(model.activeIconSetID == set.id ? Palette.accentText : .secondary)
                            Text(set.manifest.name)
                                .foregroundStyle(.primary)
                            Spacer()
                            if model.activeIconSetID == set.id {
                                Image(systemName: "checkmark")
                                    .font(.caption.weight(.bold))
                                    .foregroundStyle(Palette.accentText)
                            }
                        }
                    }
                    .buttonStyle(.plain)
                    .help("\(set.manifest.name) by \(set.manifest.author) · \(set.manifest.license) · \(set.manifest.icons.count) icons")
                    .contextMenu {
                        Button("Remove Icon Set", role: .destructive) { model.removeIconSet(set) }
                    }
                }
                Button {
                    showIconSets = true
                } label: {
                    Label("Get Icon Sets…", systemImage: "arrow.down.circle")
                        .foregroundStyle(.secondary)
                }
                .buttonStyle(.plain)
            }

            Section("Vector & Brand Glyphs") {
                Button {
                    showGlyphLibrary = true
                } label: {
                    HStack(spacing: Space.sm) {
                        Image(systemName: "sparkles.square.filled.on.square")
                            .foregroundStyle(Palette.accentText)
                        Text("Simple Icons & Glyphs…")
                            .foregroundStyle(.primary)
                        Spacer()
                    }
                }
                .buttonStyle(.plain)
                .help("Browse official Simple Icons brand logos & Apple SF Symbols")
            }

            Section("My Packs") {
                ForEach(model.customPacks) { pack in
                    Button {
                        model.applyCustomPack(pack, items: items)
                    } label: {
                        HStack(spacing: Space.sm) {
                            Image(systemName: "star.square")
                                .foregroundStyle(model.activePackId == pack.id ? Palette.accentText : .secondary)
                            Text(pack.name)
                                .foregroundStyle(.primary)
                            Spacer()
                            if model.activePackId == pack.id {
                                Image(systemName: "checkmark")
                                    .font(.caption.weight(.bold))
                                    .foregroundStyle(Palette.accentText)
                            }
                        }
                    }
                    .buttonStyle(.plain)
                    .contextMenu {
                        Button("Export Pack…") { promptExport(pack) }
                        Button("Delete Pack", role: .destructive) { model.deleteCustomPack(pack) }
                    }
                }
                Button {
                    promptSavePack()
                } label: {
                    Label("Save Current Style…", systemImage: "plus.square.dashed")
                        .foregroundStyle(.secondary)
                }
                .buttonStyle(.plain)
                Button {
                    promptImport()
                } label: {
                    Label("Import Pack…", systemImage: "square.and.arrow.down")
                        .foregroundStyle(.secondary)
                }
                .buttonStyle(.plain)
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
        .sheet(isPresented: $showIconSets) {
            IconSetBrowserView()
                .environment(model)
        }
        .sheet(isPresented: $showGlyphLibrary) {
            GlyphLibraryView(allItems: items)
                .environment(model)
        }
    }

    private func count(for page: SidebarItem) -> Int {
        switch page {
        case .all: items.count
        case .applied: items.filter(\.isApplied).count
        case .starred: items.filter(\.isStarred).count
        }
    }

    private func promptSavePack() {
        let alert = NSAlert()
        alert.messageText = "Save Current Style as a Pack"
        alert.informativeText = "Name your pack. It appears under My Packs."
        let field = NSTextField(frame: NSRect(x: 0, y: 0, width: 240, height: 24)) // grid: ok AppKit alert accessory
        field.stringValue = "My Pack \(model.customPacks.count + 1)"
        alert.accessoryView = field
        alert.addButton(withTitle: "Save")
        alert.addButton(withTitle: "Cancel")
        if alert.runModal() == .alertFirstButtonReturn {
            model.saveCurrentStyleAsPack(named: field.stringValue)
        }
    }

    private var packType: [UTType] { UTType(filenameExtension: PackFile.fileExtension).map { [$0] } ?? [] }

    private func promptExport(_ pack: CustomIconPack) {
        let panel = NSSavePanel()
        panel.title = "Export Pack"
        panel.nameFieldStringValue = "\(pack.name).\(PackFile.fileExtension)"
        panel.allowedContentTypes = packType
        panel.allowsOtherFileTypes = true
        if panel.runModal() == .OK, let url = panel.url {
            model.exportPacks([pack], to: url)
        }
    }

    private func promptImport() {
        let panel = NSOpenPanel()
        panel.title = "Import Pack"
        panel.canChooseDirectories = false
        panel.allowsMultipleSelection = true
        panel.allowedContentTypes = packType + [.json]
        if panel.runModal() == .OK {
            for url in panel.urls { model.importPacks(from: url) }
        }
    }
}
