import AppKit
import ModoKit
import SwiftData
import SwiftUI

/// Comprehensive Vector Glyphs & Brand Icons Studio.
/// Allows browsing, searching, and applying official Simple Icons (3,000+ brands) and Apple SF Symbols to any application.
struct GlyphLibraryView: View {
    @Environment(AppModel.self) private var model
    @Environment(\.modelContext) private var context
    @Environment(\.dismiss) private var dismiss

    var targetItem: Item? = nil
    var allItems: [Item] = []

    @State private var selectedTab: LibraryTab = .simpleIcons
    @State private var search: String = ""
    @State private var selectedCategory: String = "All"
    @State private var selectedAppId: UUID?
    @State private var isDownloading: Bool = false
    @State private var downloadStatus: String?
    @State private var downloadedIcons: [SimpleIcon] = []

    enum LibraryTab: String, CaseIterable, Identifiable {
        case simpleIcons = "Brand Logos (Simple Icons)"
        case sfSymbols = "System Glyphs (SF Symbols)"
        var id: String { rawValue }
    }

    private var currentTargetItem: Item? {
        if let target = targetItem { return target }
        if let id = selectedAppId {
            return allItems.first(where: { $0.id == id })
        }
        if let sel = model.selection {
            return allItems.first(where: { $0.id == sel })
        }
        return allItems.first
    }

    private var filteredSimpleIcons: [SimpleIcon] {
        let base = SimpleIconCatalog.curated + downloadedIcons
        let q = search.trimmingCharacters(in: .whitespacesAndNewlines).lowercased()
        if q.isEmpty {
            if selectedCategory == "All" { return base }
            return filterByCategory(base, category: selectedCategory)
        }
        return base.filter {
            $0.title.lowercased().contains(q) || $0.slug.contains(q)
        }
    }

    private func filterByCategory(_ icons: [SimpleIcon], category: String) -> [SimpleIcon] {
        switch category {
        case "Developer":
            return icons.filter { ["visualstudiocode", "github", "gitlab", "docker", "xcode", "cursor", "neovim", "intellijidea", "pycharm", "webstorm", "androidstudio", "rust", "go", "iterm2", "warp", "alacritty", "tableplus", "insomnia", "postman", "sublimetext"].contains($0.slug) }
        case "Creative":
            return icons.filter { ["figma", "sketch", "canva", "adobephotoshop", "adobeillustrator", "adobepremierepro", "blender", "linear", "raycast"].contains($0.slug) }
        case "Social & Chat":
            return icons.filter { ["telegram", "slack", "discord", "whatsapp", "signal", "x", "reddit", "bluesky", "twitch", "youtube"].contains($0.slug) }
        case "Productivity":
            return icons.filter { ["notion", "obsidian", "1password", "bitwarden", "dropbox", "trello", "zoom", "alfred", "cleanmymac", "tailscale"].contains($0.slug) }
        default:
            return icons
        }
    }

    private let sfSymbolCategories: [String: [String]] = [
        "Developer": ["terminal.fill", "cpu", "memorychip", "hammer.fill", "wrench.and.screwdriver.fill", "macwindow", "network", "server.rack", "curlybraces", "chevron.left.forwardslash.chevron.right"],
        "Media": ["music.note", "play.circle.fill", "film.fill", "camera.fill", "mic.fill", "speaker.wave.3.fill", "headphones", "radio.fill", "tv.fill"],
        "Communication": ["message.fill", "bubble.left.and.bubble.right.fill", "envelope.fill", "paperplane.fill", "phone.fill", "video.fill", "bell.fill", "at"],
        "Productivity": ["doc.text.fill", "folder.fill", "briefcase.fill", "calendar", "chart.bar.fill", "checklist", "book.fill", "pencil.and.outline"],
        "System": ["gearshape.fill", "shield.fill", "lock.fill", "globe", "cloud.fill", "magnifyingglass", "trash.fill", "bolt.fill", "battery.100.bolt"],
        "Creative": ["paintbrush.fill", "paintpalette.fill", "wand.and.stars", "cube.transparent.fill", "circle.hexagongrid.fill", "eyedropper.full", "sparkles"]
    ]

    private var filteredSFSymbols: [String] {
        let q = search.trimmingCharacters(in: .whitespacesAndNewlines).lowercased()
        let allSyms = sfSymbolCategories.values.flatMap { $0 }
        let uniqueSyms = Array(NSOrderedSet(array: allSyms)).compactMap { $0 as? String }
        if q.isEmpty {
            if selectedCategory == "All" { return uniqueSyms }
            return sfSymbolCategories[selectedCategory] ?? uniqueSyms
        }
        return uniqueSyms.filter { $0.lowercased().contains(q) }
    }

    var body: some View {
        VStack(spacing: 0) {
            headerBar
            Divider()

            controlsBar
            Divider()

            ScrollView {
                VStack(alignment: .leading, spacing: Space.md) {
                    if let status = downloadStatus {
                        HStack(spacing: Space.sm) {
                            if isDownloading {
                                ProgressView().controlSize(.small)
                            } else {
                                Image(systemName: "checkmark.circle.fill")
                                    .foregroundStyle(.green)
                            }
                            Text(status)
                                .font(.caption)
                                .foregroundStyle(.secondary)
                        }
                        .padding(.horizontal, Space.md)
                        .padding(.vertical, Space.xs)
                    }

                    if selectedTab == .simpleIcons {
                        simpleIconsGrid
                    } else {
                        sfSymbolsGrid
                    }
                }
                .padding(Space.md)
            }

            Divider()
            footerBar
        }
        .frame(minWidth: 680, idealWidth: 760, minHeight: 520, idealHeight: 600)
    }

    // MARK: - Header
    private var headerBar: some View {
        HStack(alignment: .center) {
            Image(systemName: "sparkles.square.filled.on.square")
                .font(.title2)
                .foregroundStyle(Palette.accentText)

            VStack(alignment: .leading, spacing: 2) {
                Text("Vector Glyphs & Brand Icons")
                    .font(.headline)
                Text("Explore official brand vector logos and Apple SF Symbols")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            Spacer()

            if !allItems.isEmpty {
                Picker("Target App:", selection: $selectedAppId) {
                    ForEach(allItems) { item in
                        Text(item.title).tag(Optional(item.id))
                    }
                }
                .frame(width: 180)
                .controlSize(.small)
            }

            Button("Done") { dismiss() }
                .keyboardShortcut(.defaultAction)
        }
        .padding(Space.md)
    }

    // MARK: - Controls
    private var controlsBar: some View {
        VStack(spacing: Space.sm) {
            HStack(spacing: Space.md) {
                Picker("", selection: $selectedTab) {
                    ForEach(LibraryTab.allCases) { tab in
                        Text(tab.rawValue).tag(tab)
                    }
                }
                .pickerStyle(.segmented)
                .frame(maxWidth: 380)

                Spacer()

                HStack(spacing: Space.xs) {
                    Image(systemName: "magnifyingglass")
                        .foregroundStyle(.secondary)
                    TextField(selectedTab == .simpleIcons ? "Search brand or slug (e.g. Cursor, Slack, Figma)…" : "Search SF Symbol…", text: $search)
                        .textFieldStyle(.plain)
                    if !search.isEmpty {
                        Button { search = "" } label: {
                            Image(systemName: "xmark.circle.fill")
                                .foregroundStyle(.secondary)
                        }
                        .buttonStyle(.plain)
                    }
                }
                .padding(.horizontal, Space.sm)
                .padding(.vertical, 5)
                .background(.background.secondary, in: RoundedRectangle(cornerRadius: Radius.sm))
            }

            // Category filter pills
            HStack(spacing: Space.xs) {
                categoryPill("All")
                if selectedTab == .simpleIcons {
                    categoryPill("Developer")
                    categoryPill("Creative")
                    categoryPill("Social & Chat")
                    categoryPill("Productivity")
                } else {
                    categoryPill("Developer")
                    categoryPill("Media")
                    categoryPill("Communication")
                    categoryPill("Productivity")
                    categoryPill("System")
                    categoryPill("Creative")
                }
                Spacer()
            }
        }
        .padding(.horizontal, Space.md)
        .padding(.vertical, Space.sm)
        .background(.background.secondary.opacity(0.5))
    }

    private func categoryPill(_ name: String) -> some View {
        let isSel = selectedCategory == name
        return Button {
            selectedCategory = name
        } label: {
            Text(name)
                .font(.caption2.weight(isSel ? .bold : .regular))
                .padding(.horizontal, Space.sm)
                .padding(.vertical, 3)
                .background(isSel ? Palette.accent : Color.primary.opacity(0.06), in: Capsule())
                .foregroundStyle(isSel ? .white : .primary)
        }
        .buttonStyle(.plain)
    }

    // MARK: - Simple Icons Grid
    private var simpleIconsGrid: some View {
        VStack(alignment: .leading, spacing: Space.md) {
            LazyVGrid(columns: [GridItem(.adaptive(minimum: 140, maximum: 190), spacing: Space.md)], spacing: Space.md) {
                ForEach(filteredSimpleIcons) { icon in
                    simpleIconCard(icon)
                }
            }

            if filteredSimpleIcons.isEmpty && !search.isEmpty {
                remoteFetchBanner
            }
        }
    }

    private func simpleIconCard(_ icon: SimpleIcon) -> some View {
        let cardShape = RoundedRectangle(cornerRadius: Radius.md, style: .continuous)
        let brandColor = Color(
            red: Double((icon.hex >> 16) & 0xFF) / 255.0,
            green: Double((icon.hex >> 8) & 0xFF) / 255.0,
            blue: Double(icon.hex & 0xFF) / 255.0
        )

        return VStack(spacing: Space.xs) {
            if let img = NSImage(data: icon.svgData()) {
                Image(nsImage: img)
                    .resizable()
                    .renderingMode(.template)
                    .foregroundStyle(brandColor)
                    .frame(width: 40, height: 40)
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

            Button {
                applySimpleIcon(icon)
            } label: {
                Text(currentTargetItem != nil ? "Apply to \(currentTargetItem!.title)" : "Apply")
                    .font(.caption2.weight(.semibold))
                    .foregroundStyle(Palette.accentText)
                    .padding(.horizontal, Space.sm)
                    .padding(.vertical, 3)
                    .background(Palette.accent.opacity(0.14), in: Capsule())
            }
            .buttonStyle(.plain)
            .padding(.top, 2)
        }
        .frame(maxWidth: .infinity)
        .padding(Space.md)
        .background(.background.secondary, in: cardShape)
        .overlay(cardShape.strokeBorder(Color(nsColor: .separatorColor), lineWidth: 1))
    }

    private var remoteFetchBanner: some View {
        let cleanSlug = SimpleIconCatalog.normalizeSlug(search)
        let cardShape = RoundedRectangle(cornerRadius: Radius.md, style: .continuous)

        return VStack(spacing: Space.md) {
            Image(systemName: "cloud.rainbow.half")
                .font(.system(size: 32))
                .foregroundStyle(Palette.accentText)

            Text("Brand '\(search)' not in offline bundle")
                .font(.headline)

            Text("Download official SVG from Simple Icons CDN via slug '**\(cleanSlug)**'?")
                .font(.caption)
                .foregroundStyle(.secondary)

            Button {
                fetchOnline(slug: cleanSlug)
            } label: {
                Label("Download '\(cleanSlug).svg' from CDN", systemImage: "arrow.down.circle.fill")
            }
            .buttonStyle(.borderedProminent)
            .disabled(cleanSlug.isEmpty || isDownloading)
        }
        .frame(maxWidth: .infinity)
        .padding(Space.xl)
        .background(.background.secondary, in: cardShape)
        .overlay(cardShape.strokeBorder(Color(nsColor: .separatorColor), lineWidth: 1))
    }

    // MARK: - SF Symbols Grid
    private var sfSymbolsGrid: some View {
        LazyVGrid(columns: [GridItem(.adaptive(minimum: 120, maximum: 160), spacing: Space.md)], spacing: Space.md) {
            ForEach(filteredSFSymbols, id: \.self) { symName in
                sfSymbolCard(symName)
            }
        }
    }

    private func sfSymbolCard(_ symName: String) -> some View {
        let cardShape = RoundedRectangle(cornerRadius: Radius.md, style: .continuous)

        return VStack(spacing: Space.xs) {
            Image(systemName: symName)
                .font(.system(size: 28))
                .foregroundStyle(Palette.accentText)
                .frame(width: 44, height: 44)
                .background(Palette.accent.opacity(0.12), in: Circle())

            Text(symName)
                .font(.caption2.monospaced())
                .foregroundStyle(.secondary)
                .lineLimit(1)

            Button {
                applySFSymbol(symName)
            } label: {
                Text(currentTargetItem != nil ? "Apply to App" : "Apply")
                    .font(.caption2.weight(.semibold))
                    .foregroundStyle(Palette.accentText)
                    .padding(.horizontal, Space.sm)
                    .padding(.vertical, 2)
                    .background(Palette.accent.opacity(0.14), in: Capsule())
            }
            .buttonStyle(.plain)
            .padding(.top, 2)
        }
        .frame(maxWidth: .infinity)
        .padding(Space.md)
        .background(.background.secondary, in: cardShape)
        .overlay(cardShape.strokeBorder(Color(nsColor: .separatorColor), lineWidth: 1))
    }

    // MARK: - Footer
    private var footerBar: some View {
        HStack {
            if let item = currentTargetItem {
                Text("Target: **\(item.title)**")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            Spacer()

            let count = selectedTab == .simpleIcons ? filteredSimpleIcons.count : filteredSFSymbols.count
            Text("\(count) vector glyphs available")
                .font(.caption2)
                .foregroundStyle(.secondary)
        }
        .padding(Space.md)
        .background(.background.secondary)
    }

    // MARK: - Actions
    private func applySimpleIcon(_ icon: SimpleIcon) {
        guard let item = currentTargetItem else { return }
        model.setCustomArtwork(data: icon.svgData(), for: item)
        item.brandHex = Int(icon.hex)
        try? context.save()
        dismiss()
    }

    private func applySFSymbol(_ symName: String) {
        guard let item = currentTargetItem else { return }
        let size: CGFloat = 512
        let config = NSImage.SymbolConfiguration(pointSize: size * 0.50, weight: .regular)
        guard let sym = NSImage(systemSymbolName: symName, accessibilityDescription: nil)?.withSymbolConfiguration(config) else { return }

        let image = NSImage(size: NSSize(width: size, height: size))
        image.lockFocus()
        NSColor.white.set()
        let r = NSRect(x: (size - sym.size.width) / 2, y: (size - sym.size.height) / 2, width: sym.size.width, height: sym.size.height)
        sym.draw(in: r, from: .zero, operation: .sourceOver, fraction: 1.0)
        image.unlockFocus()

        if let tiff = image.tiffRepresentation,
           let rep = NSBitmapImageRep(data: tiff),
           let png = rep.representation(using: .png, properties: [:]) {
            model.setCustomArtwork(data: png, for: item)
            item.symbolName = symName
            try? context.save()
            dismiss()
        }
    }

    private func fetchOnline(slug: String) {
        isDownloading = true
        downloadStatus = "Querying Simple Icons CDN for '\(slug)'…"

        Task {
            if let icon = await SimpleIconCatalog.fetch(slugOrTitle: slug) {
                await MainActor.run {
                    downloadedIcons.append(icon)
                    isDownloading = false
                    downloadStatus = "Fetched '\(icon.title)' successfully!"
                    if currentTargetItem != nil {
                        applySimpleIcon(icon)
                    }
                }
            } else {
                await MainActor.run {
                    isDownloading = false
                    downloadStatus = "Could not find '\(slug).svg' on Simple Icons CDN."
                }
            }
        }
    }
}
