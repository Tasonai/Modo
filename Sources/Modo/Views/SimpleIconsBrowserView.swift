import AppKit
import ModoKit
import SwiftData
import SwiftUI

/// Browser and search sheet for official Simple Icons vector brand logos.
struct SimpleIconsBrowserView: View {
    @Environment(AppModel.self) private var model
    @Environment(\.modelContext) private var context
    @Environment(\.dismiss) private var dismiss

    let item: Item
    @State private var search: String = ""
    @State private var applyBrandColor: Bool = true
    @State private var isDownloading: Bool = false
    @State private var downloadError: String?

    private var filteredIcons: [SimpleIcon] {
        SimpleIconCatalog.search(query: search)
    }

    var body: some View {
        VStack(spacing: 0) {
            header
            Divider()

            searchBar
            Divider()

            ScrollView {
                VStack(alignment: .leading, spacing: Space.md) {
                    if isDownloading {
                        HStack(spacing: Space.sm) {
                            ProgressView()
                                .controlSize(.small)
                            Text("Downloading vector from Simple Icons CDN…")
                                .font(.caption)
                                .foregroundStyle(.secondary)
                        }
                        .frame(maxWidth: .infinity, alignment: .center)
                        .padding(.vertical, Space.sm)
                    }

                    if let error = downloadError {
                        HStack(spacing: Space.xs) {
                            Image(systemName: "exclamationmark.triangle.fill")
                                .foregroundStyle(.orange)
                            Text(error)
                                .font(.caption)
                        }
                        .padding(Space.sm)
                        .background(Color.orange.opacity(0.12), in: RoundedRectangle(cornerRadius: Radius.sm))
                    }

                    LazyVGrid(columns: [GridItem(.adaptive(minimum: 140, maximum: 200), spacing: Space.md)], spacing: Space.md) {
                        ForEach(filteredIcons) { icon in
                            iconCard(icon)
                        }
                    }

                    if filteredIcons.isEmpty && !search.isEmpty {
                        remoteFetchCard
                    }
                }
                .padding(Space.md)
            }

            Divider()
            footer
        }
        .frame(minWidth: 540, idealWidth: 620, minHeight: 460, idealHeight: 520)
        .onAppear {
            let suggested = SimpleIconCatalog.normalizeSlug(item.title)
            if SimpleIconCatalog.find(for: item.title) != nil {
                search = item.title
            } else {
                search = suggested
            }
        }
    }

    private var header: some View {
        HStack(alignment: .center) {
            VStack(alignment: .leading, spacing: 2) {
                HStack(spacing: Space.xs) {
                    Image(systemName: "sparkles")
                        .foregroundStyle(Palette.accentText)
                    Text("Simple Icons Library")
                        .font(.headline)
                }
                Text("Official vector brand logos for \(item.title)")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
            Spacer()
            Button("Done") {
                dismiss()
            }
            .keyboardShortcut(.defaultAction)
        }
        .padding(Space.md)
    }

    private var searchBar: some View {
        HStack(spacing: Space.sm) {
            Image(systemName: "magnifyingglass")
                .foregroundStyle(.secondary)
            TextField("Search 3,000+ brands (e.g. Slack, Chrome, Spotify, Figma)…", text: $search)
                .textFieldStyle(.plain)
            if !search.isEmpty {
                Button {
                    search = ""
                } label: {
                    Image(systemName: "xmark.circle.fill")
                        .foregroundStyle(.secondary)
                }
                .buttonStyle(.plain)
            }
        }
        .padding(.horizontal, Space.md)
        .padding(.vertical, Space.sm)
        .background(.background.secondary)
    }

    private func iconCard(_ icon: SimpleIcon) -> some View {
        let cardShape = RoundedRectangle(cornerRadius: Radius.md, style: .continuous)
        let brandColor = Color(
            red: Double((icon.hex >> 16) & 0xFF) / 255.0,
            green: Double((icon.hex >> 8) & 0xFF) / 255.0,
            blue: Double(icon.hex & 0xFF) / 255.0
        )

        return Button {
            applyIcon(icon)
        } label: {
            VStack(spacing: Space.sm) {
                if let img = NSImage(data: icon.svgData()) {
                    Image(nsImage: img)
                        .resizable()
                        .renderingMode(.template)
                        .foregroundStyle(brandColor)
                        .frame(width: 44, height: 44)
                        .padding(Space.sm)
                        .background(brandColor.opacity(0.12), in: Circle())
                }

                Text(icon.title)
                    .font(.callout.weight(.medium))
                    .lineLimit(1)

                Text(icon.slug)
                    .font(.caption2.monospaced())
                    .foregroundStyle(.secondary)
                    .lineLimit(1)

                Text("Apply Vector")
                    .font(.caption2.weight(.semibold))
                    .foregroundStyle(Palette.accentText)
                    .padding(.horizontal, Space.sm)
                    .padding(.vertical, 2)
                    .background(Palette.accent.opacity(0.14), in: Capsule())
            }
            .frame(maxWidth: .infinity)
            .padding(Space.md)
            .background(.background.secondary, in: cardShape)
            .overlay(cardShape.strokeBorder(Color(nsColor: .separatorColor), lineWidth: 1))
        }
        .buttonStyle(.plain)
    }

    private var remoteFetchCard: some View {
        let cardShape = RoundedRectangle(cornerRadius: Radius.md, style: .continuous)
        let cleanSlug = SimpleIconCatalog.normalizeSlug(search)

        return VStack(spacing: Space.md) {
            Image(systemName: "arrow.down.circle")
                .font(.system(size: 36))
                .foregroundStyle(Palette.accentText)

            Text("No local match for '\(search)'")
                .font(.headline)

            Text("Download official SVG from Simple Icons CDN via slug '**\(cleanSlug)**'?")
                .font(.caption)
                .foregroundStyle(.secondary)
                .multilineTextAlignment(.center)

            Button {
                fetchRemoteSVG(slug: cleanSlug)
            } label: {
                Label("Fetch '\(cleanSlug).svg'", systemImage: "icloud.and.arrow.down")
            }
            .buttonStyle(.borderedProminent)
            .disabled(cleanSlug.isEmpty || isDownloading)
        }
        .frame(maxWidth: .infinity)
        .padding(Space.xl)
        .background(.background.secondary, in: cardShape)
        .overlay(cardShape.strokeBorder(Color(nsColor: .separatorColor), lineWidth: 1))
    }

    private var footer: some View {
        HStack {
            Toggle("Also set app brand color to match logo", isOn: $applyBrandColor)
                .font(.caption)

            Spacer()

            Text("\(filteredIcons.count) icons")
                .font(.caption2)
                .foregroundStyle(.secondary)
        }
        .padding(Space.md)
        .background(.background.secondary)
    }

    private func applyIcon(_ icon: SimpleIcon) {
        let svgData = icon.svgData()
        model.setCustomArtwork(data: svgData, for: item)
        if applyBrandColor {
            item.brandHex = Int(icon.hex)
        }
        try? context.save()
        dismiss()
    }

    private func fetchRemoteSVG(slug: String) {
        guard let url = SimpleIconCatalog.remoteURL(for: slug) else { return }
        isDownloading = true
        downloadError = nil

        Task {
            do {
                var request = URLRequest(url: url)
                request.timeoutInterval = 8.0
                let (data, response) = try await URLSession.shared.data(for: request)
                guard let http = response as? HTTPURLResponse, http.statusCode == 200, !data.isEmpty else {
                    await MainActor.run {
                        isDownloading = false
                        downloadError = "Could not find '\(slug).svg' on Simple Icons CDN (404)."
                    }
                    return
                }

                guard NSImage(data: data) != nil || CustomArtworkHelper.isSVGData(data) else {
                    await MainActor.run {
                        isDownloading = false
                        downloadError = "Downloaded data was not a valid SVG."
                    }
                    return
                }

                await MainActor.run {
                    isDownloading = false
                    model.setCustomArtwork(data: data, for: item)
                    try? context.save()
                    dismiss()
                }
            } catch {
                await MainActor.run {
                    isDownloading = false
                    downloadError = "Network error: \(error.localizedDescription)"
                }
            }
        }
    }
}
