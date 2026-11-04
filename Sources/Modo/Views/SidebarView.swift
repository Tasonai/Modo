import AppKit
import ModoKit
import SwiftData
import SwiftUI
import UniformTypeIdentifiers

struct SidebarView: View {
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

            Section("Icon Packs") {
                ForEach(IconPack.allPacks) { pack in
                    Button {
                        model.applyPack(pack)
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
            }

            Section("My Packs") {
                ForEach(model.customPacks) { pack in
                    Button {
                        model.applyCustomPack(pack)
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
