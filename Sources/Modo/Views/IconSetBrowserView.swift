import AppKit
import ModoKit
import SwiftUI
import UniformTypeIdentifiers

/// Sheet that lists the icon sets of the online catalog and installs them.
struct IconSetBrowserView: View {
    @Environment(AppModel.self) private var model
    @Environment(\.dismiss) private var dismiss
    @AppStorage("iconSetCatalogURL") private var catalogURL = ""

    @State private var catalog: IconCatalog?
    @State private var loading = false
    @State private var busyID: String?
    @State private var message: String?

    var body: some View {
        VStack(alignment: .leading, spacing: Space.md) {
            HStack {
                Text("Online Icon Sets")
                    .font(.title3.weight(.semibold))
                Spacer()
                Button("Install from File…") { installFromFile() }
                    .controlSize(.small)
                Button("Done") { dismiss() }
                    .keyboardShortcut(.cancelAction)
            }

            if catalogURL.trimmingCharacters(in: .whitespaces).isEmpty {
                Text("No catalog yet. Paste the address of an icon set catalog (a JSON file) below, or in Settings.")
                    .foregroundStyle(.secondary)
            }

            HStack(spacing: Space.sm) {
                TextField("https://…/catalog.json", text: $catalogURL)
                    .textFieldStyle(.roundedBorder)
                    .onSubmit { Task { await load() } }
                Button("Load") { Task { await load() } }
                    .disabled(loading || catalogURL.trimmingCharacters(in: .whitespaces).isEmpty)
            }

            Text("Icon sets are artwork made by other people. Check the license shown for each set; Modo does not review them.")
                .font(.caption)
                .foregroundStyle(.secondary)

            if loading { ProgressView().controlSize(.small) }
            if let message {
                Text(message).font(.callout).foregroundStyle(.secondary)
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
                            Button("Install") { Task { await install(entry) } }.controlSize(.small)
                        }
                    }
                    .padding(.vertical, 2)
                }
                .frame(minHeight: 160)
                if catalog.sets.isEmpty {
                    Text("This catalog has no usable sets.").foregroundStyle(.secondary)
                }
            }
        }
        .padding(Space.xl)
        .frame(width: 560, height: 460)
        .task {
            if !catalogURL.trimmingCharacters(in: .whitespaces).isEmpty { await load() }
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
            "The icon set is malformed and was rejected."
        default: error.localizedDescription
        }
    }
}
