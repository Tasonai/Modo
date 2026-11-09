import AppKit
import ModoKit
import SwiftUI
import UniformTypeIdentifiers

/// Sheet that lists popular icon sets and online community catalogs.
struct IconSetBrowserView: View {
    @Environment(AppModel.self) private var model
    @Environment(\.dismiss) private var dismiss
    @AppStorage("iconSetCatalogURL") private var catalogURL = IconSetRules.defaultCatalogURL

    @State private var catalog: IconCatalog?
    @State private var loading = false
    @State private var busyID: String?
    @State private var message: String?
    @State private var selectedTab: BrowserTab = .popular

    enum BrowserTab: String, CaseIterable, Identifiable {
        case popular = "Popular Packs"
        case online = "Online Catalog"
        var id: String { rawValue }
    }

    var body: some View {
        VStack(alignment: .leading, spacing: Space.md) {
            // Header
            HStack {
                Text("Icon Sets")
                    .font(.title3.weight(.semibold))
                Spacer()
                Button("Install from ZIP…") { installFromFile() }
                    .controlSize(.small)
                Button("Done") { dismiss() }
                    .keyboardShortcut(.cancelAction)
            }

            // Tab Picker
            Picker("", selection: $selectedTab) {
                ForEach(BrowserTab.allCases) { tab in
                    Text(tab.rawValue).tag(tab)
                }
            }
            .pickerStyle(.segmented)
            .labelsHidden()

            if let message {
                HStack(spacing: Space.sm) {
                    Image(systemName: "info.circle")
                        .foregroundStyle(Palette.accentText)
                    Text(message)
                        .font(.callout)
                        .foregroundStyle(.secondary)
                }
                .padding(.horizontal, Space.sm)
                .padding(.vertical, 4)
                .background(.background.secondary, in: RoundedRectangle(cornerRadius: Radius.sm))
            }

            // Content
            switch selectedTab {
            case .popular:
                popularPacksList
            case .online:
                onlineCatalogList
            }
        }
        .padding(Space.xl)
        .frame(width: 620, height: 500)
        .task {
            if !catalogURL.trimmingCharacters(in: .whitespaces).isEmpty {
                await load()
            }
        }
    }

    // MARK: - Popular Packs Tab

    private var popularPacksList: some View {
        VStack(alignment: .leading, spacing: Space.sm) {
            Text("Ready-to-use popular community packs with curated artwork for Chrome, Safari, VS Code, Telegram, Terminal, Finder, and more.")
                .font(.caption)
                .foregroundStyle(.secondary)

            List(CuratedIconSets.popularPacks) { pack in
                HStack(alignment: .center, spacing: Space.md) {
                    Image(systemName: iconForPack(pack.id))
                        .font(.title2)
                        .foregroundStyle(Palette.accentText)
                        .frame(width: 36, height: 36)
                        .background(.background.secondary, in: Circle())

                    VStack(alignment: .leading, spacing: 2) {
                        HStack(spacing: Space.sm) {
                            Text(pack.name)
                                .font(.headline)
                            if model.activeIconSetID == pack.id {
                                Text("Active")
                                    .font(.caption2.weight(.bold))
                                    .foregroundStyle(Palette.accentText)
                                    .padding(.horizontal, Space.sm)
                                    .padding(.vertical, 2)
                                    .background(Palette.accent.opacity(0.12), in: Capsule())
                            }
                        }
                        Text("\(pack.description) · \(pack.author) · \(pack.license)")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }

                    Spacer()

                    let isInstalled = model.installedIconSets.contains(where: { $0.id == pack.id })
                    if busyID == pack.id {
                        ProgressView().controlSize(.small)
                    } else if isInstalled {
                        Menu {
                            Button("Apply / Make Active") {
                                model.activeIconSetID = pack.id
                                message = "Activated \(pack.name)"
                            }
                            Button("Reinstall") {
                                installCurated(pack)
                            }
                            Divider()
                            Button("Remove", role: .destructive) {
                                if let installed = model.installedIconSets.first(where: { $0.id == pack.id }) {
                                    model.removeIconSet(installed)
                                    message = "Removed \(pack.name)"
                                }
                            }
                        } label: {
                            Label("Installed", systemImage: "checkmark")
                        }
                        .controlSize(.small)
                    } else {
                        Button("Install") {
                            installCurated(pack)
                        }
                        .controlSize(.small)
                        .buttonStyle(.borderedProminent)
                    }
                }
                .padding(.vertical, 4)
            }
            .listStyle(.inset(alternatesRowBackgrounds: true))
            .frame(minHeight: 280)
        }
    }

    private func iconForPack(_ id: String) -> String {
        switch id {
        case "papirus-minimal": "square.grid.2x2.fill"
        case "whitesur-icons": "apple.logo"
        case "tela-icons": "circle.grid.cross.fill"
        case "lawnicons-pack": "sparkles.square.filled.on.square"
        case "catppuccin-icons": "cup.and.saucer.fill"
        case "nordic-frost": "snowflake"
        case "macosicons-hub": "macwindow.on.rectangle"
        case "mac-icons-pro": "app.fill"
        case "macos-folders-system": "folder.fill"
        case "tabler-lucide": "square.and.pencil"
        default: "photo.stack"
        }
    }

    private func installCurated(_ pack: CuratedIconSets.CuratedPack) {
        busyID = pack.id
        message = nil
        defer { busyID = nil }
        do {
            let set = try CuratedIconSets.install(id: pack.id)
            model.refreshIconSets()
            model.activeIconSetID = set.id
            model.statusMessage = "Installed and activated \(set.manifest.name)"
            message = "Successfully installed \(set.manifest.name)!"
        } catch {
            message = describe(error)
        }
    }

    // MARK: - Online Catalog Tab

    private var onlineCatalogList: some View {
        VStack(alignment: .leading, spacing: Space.sm) {
            HStack(spacing: Space.sm) {
                TextField("https://…/catalog.json", text: $catalogURL)
                    .textFieldStyle(.roundedBorder)
                    .onSubmit { Task { await load() } }
                Button("Load") { Task { await load() } }
                    .disabled(loading || catalogURL.trimmingCharacters(in: .whitespaces).isEmpty)
                Button("Reset URL") {
                    catalogURL = IconSetRules.defaultCatalogURL
                    Task { await load() }
                }
                .controlSize(.small)
            }

            Text("External artwork catalog over HTTPS. Every archive is verified by SHA-256 and checked for security before installation.")
                .font(.caption)
                .foregroundStyle(.secondary)

            if loading {
                ProgressView().controlSize(.small)
            }

            if let catalog {
                List(catalog.sets) { entry in
                    HStack(alignment: .top, spacing: Space.md) {
                        VStack(alignment: .leading, spacing: 2) {
                            Text(entry.name).font(.headline)
                            Text("\(entry.author) · \(entry.license) · \(entry.iconCount) icons · \(ByteCountFormatter.string(fromByteCount: Int64(entry.sizeBytes), countStyle: .file))")
                                .font(.caption)
                                .foregroundStyle(.secondary)
                        }
                        Spacer()
                        if busyID == entry.id {
                            ProgressView().controlSize(.small)
                        } else if model.installedIconSets.contains(where: { $0.id == entry.id }) {
                            Button("Reinstall") { Task { await install(entry) } }.controlSize(.small)
                        } else {
                            Button("Install") { Task { await install(entry) } }
                                .controlSize(.small)
                                .buttonStyle(.borderedProminent)
                        }
                    }
                    .padding(.vertical, 2)
                }
                .listStyle(.inset(alternatesRowBackgrounds: true))
                .frame(minHeight: 252)

                if catalog.sets.isEmpty {
                    Text("This catalog has no usable sets.").foregroundStyle(.secondary)
                }
            } else if !loading {
                Text("Catalog not loaded. Check the URL or click Load.")
                    .font(.callout)
                    .foregroundStyle(.secondary)
                    .padding(.top, Space.md)
            }
        }
    }

    private func load() async {
        guard let url = URL(string: catalogURL.trimmingCharacters(in: .whitespaces)) else {
            message = "That is not a valid address."
            return
        }
        loading = true
        message = nil
        defer { loading = false }
        do {
            catalog = try await IconSetStore.fetchCatalog(from: url)
        } catch {
            catalog = nil
            message = describe(error)
        }
    }

    private func install(_ entry: IconCatalog.Entry) async {
        busyID = entry.id
        message = nil
        defer { busyID = nil }
        do {
            let set = try await IconSetStore.download(entry)
            model.refreshIconSets()
            model.activeIconSetID = set.id
            model.statusMessage = "Installed icon set \(set.manifest.name)"
            message = "Installed \(set.manifest.name) and switched to it."
        } catch {
            message = describe(error)
        }
    }

    private func installFromFile() {
        let panel = NSOpenPanel()
        panel.title = "Install Icon Set"
        panel.allowedContentTypes = [.zip]
        panel.canChooseDirectories = false
        guard panel.runModal() == .OK, let url = panel.url else { return }
        do {
            let data = try Data(contentsOf: url)
            let set = try IconSetStore.install(zipData: data, expectedSHA256: nil)
            model.refreshIconSets()
            model.activeIconSetID = set.id
            message = "Installed \(set.manifest.name) and switched to it."
        } catch {
            message = describe(error)
        }
    }

    private func describe(_ error: Error) -> String {
        switch error {
        case IconSetError.checksumMismatch: "The download does not match its checksum, so it was discarded."
        case IconSetError.insecureURL: "Only https addresses are allowed."
        case IconSetError.tooLarge: "The download is larger than Modo allows."
        case IconSetError.notACatalog: "That address is not a Modo icon set catalog."
        case IconSetError.notAnIconSet: "That file is not a Modo icon set."
        case IconSetError.unsupportedVersion: "This needs a newer version of Modo."
        case IconSetError.unsafePath, IconSetError.invalidID, IconSetError.duplicateBundleID, IconSetError.tooManyIcons:
            "The archive does not follow the Modo icon set rules."
        default:
            error.localizedDescription
        }
    }
}
