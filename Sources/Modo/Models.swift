import AppKit
import Foundation
import ModoKit
import Observation
import SwiftData

/// A macOS application (or custom icon entry) managed by IconMaker.
@Model
final class Item {
    @Attribute(.unique) var id: UUID
    var title: String
    var bundlePath: String
    var bundleIdentifier: String
    var categoryName: String
    var symbolName: String
    var brandHex: Int
    var isStarred: Bool
    var isApplied: Bool
    var createdAt: Date
    var customScale: Double?
    var customOffsetX: Double?
    var customOffsetY: Double?
    @Attribute(.externalStorage) var customImageData: Data?

    init(
        title: String,
        bundlePath: String = "",
        bundleIdentifier: String = "com.example.app",
        categoryName: String = "Application",
        symbolName: String = "app.fill",
        brandHex: Int = 0x1A73E8,
        isStarred: Bool = false,
        isApplied: Bool = false,
        customScale: Double? = nil,
        customOffsetX: Double? = nil,
        customOffsetY: Double? = nil,
        customImageData: Data? = nil
    ) {
        self.id = UUID()
        self.title = ItemTitle.clean(title)
        self.bundlePath = bundlePath
        self.bundleIdentifier = bundleIdentifier
        self.categoryName = categoryName
        self.symbolName = symbolName
        self.brandHex = brandHex
        self.isStarred = isStarred
        self.isApplied = isApplied
        self.createdAt = .now
        self.customScale = customScale
        self.customOffsetX = customOffsetX
        self.customOffsetY = customOffsetY
        self.customImageData = customImageData
    }

    /// Whether this entry is an ordinary filesystem folder/directory (rather than a .app bundle).
    var isFolder: Bool {
        categoryName == "Folder" || (!bundlePath.isEmpty && !bundlePath.hasSuffix(".app") && (try? URL(fileURLWithPath: bundlePath).resourceValues(forKeys: [.isDirectoryKey]).isDirectory) == true)
    }
}

/// Sidebar filter pages. Snapshot mode walks through every case automatically.
enum SidebarItem: String, CaseIterable, Identifiable {
    case all
    case applied
    case starred

    var id: String { rawValue }

    var title: String {
        switch self {
        case .all: "All Apps"
        case .applied: "Styled"
        case .starred: "Favorites"
        }
    }

    var symbol: String {
        switch self {
        case .all: "square.grid.2x2"
        case .applied: "checkmark.circle"
        case .starred: "star"
        }
    }
}

/// Filter pills/tags for fast category filtering in the list
enum AppCategoryFilter: String, CaseIterable, Identifiable {
    case all = "All"
    case customized = "Customized"
    case system = "System"
    case thirdParty = "Third-Party"

    var id: String { rawValue }
}

/// View presentation mode in the main app list (Compact List vs Responsive Gallery Grid).
enum AppViewMode: String, CaseIterable, Identifiable {
    case list = "List"
    case grid = "Grid"

    var id: String { rawValue }
}

/// Shared UI and icon rendering state for the main window and menu commands.
@Observable
final class AppModel {
    let container: ModelContainer
    var sidebar: SidebarItem = .all
    var categoryFilter: AppCategoryFilter = .all
    var selection: UUID?
    var search = ""
    var settingsTab: SettingsTab = .general
    var statusMessage = "Ready"
    var showUpdateSheet: Bool = false
    var showBatchCreator: Bool = false
    var showShowcaseStudio: Bool = false
    var batchInitialFolderURL: URL? = nil

    var viewMode: AppViewMode = .list {
        didSet {
            if persistsPreferences { UserDefaults.standard.set(viewMode.rawValue, forKey: Self.viewModeKey) }
        }
    }

    var gridIconSize: Double = 64.0 {
        didSet {
            let clamped = min(max(gridIconSize, 36.0), 160.0)
            if clamped != gridIconSize { gridIconSize = clamped }
            if persistsPreferences { UserDefaults.standard.set(gridIconSize, forKey: Self.gridIconSizeKey) }
        }
    }

    private static let viewModeKey = "modo.viewMode"
    private static let gridIconSizeKey = "modo.gridIconSize"

    var activePackId: String = "pixel_pure" {
        didSet {
            if persistsPreferences { UserDefaults.standard.set(activePackId, forKey: Self.packKey) }
        }
    }

    var style: RoundIconStyle = RoundIconStyle() {
        didSet {
            roundCache.removeAll()
            persistStyle()
            // Manual tweaks leave the pack: the UI then reads "Custom Style".
            if let packStyle = styleOfPack(id: activePackId), packStyle != style {
                activePackId = Self.customPackId
            }
        }
    }

    static let customPackId = "custom"

    /// Display name of the active pack, or "Custom Style" after manual edits.
    var activePackName: String {
        IconPack.allPacks.first(where: { $0.id == activePackId })?.name
            ?? customPacks.first(where: { $0.id == activePackId })?.name
            ?? "Custom Style"
    }

    /// User-saved packs, persisted in UserDefaults.
    var customPacks: [CustomIconPack] = [] {
        didSet {
            guard persistsPreferences, let data = try? JSONEncoder().encode(customPacks) else { return }
            UserDefaults.standard.set(data, forKey: Self.customPacksKey)
        }
    }

    private func styleOfPack(id: String) -> RoundIconStyle? {
        IconPack.allPacks.first(where: { $0.id == id })?.style
            ?? customPacks.first(where: { $0.id == id })?.style
    }

    /// Saves the current style as a new custom pack and makes it active.
    func saveCurrentStyleAsPack(named name: String) {
        let pack = CustomIconPack(name: name, style: style)
        customPacks.append(pack)
        activePackId = pack.id
        statusMessage = "Saved pack \(pack.name)"
    }

    func applyCustomPack(_ pack: CustomIconPack) {
        style = pack.style
        activePackId = pack.id
        statusMessage = "Activated \(pack.name) pack"
    }

    /// Writes `packs` to a `.modopack` file.
    func exportPacks(_ packs: [CustomIconPack], to url: URL) {
        do {
            try PackFile.encode(packs).write(to: url, options: .atomic)
            statusMessage = "Exported \(packs.count) pack\(packs.count == 1 ? "" : "s") to \(url.lastPathComponent)"
        } catch {
            statusMessage = "Export failed: \(error.localizedDescription)"
        }
    }

    /// Adds the packs in a `.modopack` file to My Packs.
    @discardableResult
    func importPacks(from url: URL) -> Int {
        do {
            let packs = try PackFile.decode(Data(contentsOf: url))
            customPacks.append(contentsOf: packs)
            statusMessage = "Imported \(packs.count) pack\(packs.count == 1 ? "" : "s") from \(url.lastPathComponent)"
            return packs.count
        } catch PackFile.PackFileError.unsupportedVersion(let version) {
            statusMessage = "Import failed: pack file version \(version) needs a newer Modo"
        } catch PackFile.PackFileError.empty {
            statusMessage = "Import failed: the file contains no packs"
        } catch {
            statusMessage = "Import failed: not a Modo pack file"
        }
        return 0
    }

    func deleteCustomPack(_ pack: CustomIconPack) {
        customPacks.removeAll { $0.id == pack.id }
        if activePackId == pack.id { activePackId = Self.customPackId }
        statusMessage = "Deleted pack \(pack.name)"
    }

    /// Selects and activates an icon design pack, updating all rendering parameters.
    func applyPack(_ pack: IconPack) {
        style = pack.style
        activePackId = pack.id
        statusMessage = "Activated \(pack.name) pack"
    }

    private var originalCache: [UUID: NSImage] = [:]
    private var roundCache: [UUID: NSImage] = [:]

    init(container: ModelContainer) {
        self.container = container
        restoreStyle()
        startAutoReapply()
    }

    private static let styleKey = "modo.style"
    private static let packKey = "modo.activePack"
    private static let customPacksKey = "modo.customPacks"
    private static let backupsKey = "modo.systemBackups"

    var recentBackups: [SystemBackupPoint] = []

    /// Creates an immutable snapshot restore point of selected applications before styling.
    @discardableResult
    func createBackupPoint(for items: [Item], name: String? = nil) -> SystemBackupPoint {
        let appPaths = items.map(\.bundlePath).filter { !$0.isEmpty }
        let pointName = name ?? "Backup \(Date.now.formatted(date: .abbreviated, time: .shortened))"
        let point = SystemBackupPoint(
            name: pointName,
            appPaths: appPaths,
            packName: activePackName
        )
        recentBackups.insert(point, at: 0)
        if recentBackups.count > 10 { recentBackups = Array(recentBackups.prefix(10)) }
        if persistsPreferences, let data = try? JSONEncoder().encode(recentBackups) {
            UserDefaults.standard.set(data, forKey: Self.backupsKey)
        }
        return point
    }

    /// Restores all applications in `backup` back to original system icons.
    @discardableResult
    func restoreFromBackupPoint(_ backup: SystemBackupPoint, items: [Item]) -> Int {
        var restored = 0
        let pathSet = Set(backup.appPaths)
        for item in items where pathSet.contains(item.bundlePath) {
            restoreOriginalIcon(for: item)
            restored += 1
        }
        statusMessage = "Restored \(restored) apps from backup \(backup.name)"
        return restored
    }

    /// Restarts the macOS Dock process so new icon caches display immediately.
    func restartDock() {
        let proc = Process()
        proc.executableURL = URL(fileURLWithPath: "/usr/bin/killall")
        proc.arguments = ["Dock"]
        try? proc.run()
    }

    /// Tests and snapshots run on a throwaway store and must not touch saved preferences.
    private var persistsPreferences: Bool { !Store.isOverridden }

    private func persistStyle() {
        guard persistsPreferences, let data = try? JSONEncoder().encode(style) else { return }
        UserDefaults.standard.set(data, forKey: Self.styleKey)
    }

    private func restoreStyle() {
        guard persistsPreferences else { return }
        let defaults = UserDefaults.standard
        if let data = defaults.data(forKey: Self.backupsKey),
           let saved = try? JSONDecoder().decode([SystemBackupPoint].self, from: data) {
            recentBackups = saved
        }
        if let data = defaults.data(forKey: Self.styleKey),
           let saved = try? JSONDecoder().decode(RoundIconStyle.self, from: data) {
            style = saved
        }
        if let data = defaults.data(forKey: Self.customPacksKey),
           let saved = try? JSONDecoder().decode([CustomIconPack].self, from: data) {
            customPacks = saved
        }
        if let modeStr = defaults.string(forKey: Self.viewModeKey), let mode = AppViewMode(rawValue: modeStr) {
            viewMode = mode
        }
        let savedGridSize = defaults.double(forKey: Self.gridIconSizeKey)
        if savedGridSize >= 36.0 && savedGridSize <= 160.0 {
            gridIconSize = savedGridSize
        }
        activeIconSetID = defaults.string(forKey: Self.iconSetKey)
        activePackId = defaults.string(forKey: Self.packKey) ?? activePackId
        if styleOfPack(id: activePackId) != style {
            activePackId = Self.customPackId
        }
    }

    // MARK: Icon sets

    /// Sets installed on disk (refreshed after install / remove).
    var installedIconSets: [InstalledIconSet] = IconSetStore.installedSets()

    /// The set whose artwork replaces each app's own icon; `nil` means the apps' original icons.
    var activeIconSetID: String? = nil {
        didSet {
            roundCache.removeAll()
            if persistsPreferences { UserDefaults.standard.set(activeIconSetID, forKey: Self.iconSetKey) }
        }
    }

    private static let iconSetKey = "modo.activeIconSet"

    var activeIconSet: InstalledIconSet? {
        guard let id = activeIconSetID else { return nil }
        return installedIconSets.first { $0.id == id }
    }

    func refreshIconSets() {
        installedIconSets = IconSetStore.installedSets()
        if let id = activeIconSetID, !installedIconSets.contains(where: { $0.id == id }) {
            activeIconSetID = nil
        }
    }

    func removeIconSet(_ set: InstalledIconSet) {
        IconSetStore.remove(id: set.id)
        refreshIconSets()
        statusMessage = "Removed icon set \(set.manifest.name)"
    }

    /// The artwork Modo styles for `item`: custom user artwork, or active set's picture, or the app's own icon.
    func sourceIcon(for item: Item, canvasSize: Int) -> NSImage {
        if let customData = item.customImageData, let img = NSImage(data: customData) {
            return img
        }
        if let set = activeIconSet, let image = IconSetStore.image(for: item.bundleIdentifier, in: set) {
            return image
        }
        return IconRenderer.loadOriginalIcon(
            bundlePath: item.bundlePath,
            symbolName: item.symbolName,
            brandHex: UInt32(clamping: item.brandHex),
            canvasSize: canvasSize
        )
    }

    /// Returns the cached original macOS squircle icon for `item`.
    func originalIcon(for item: Item) -> NSImage {
        if let cached = originalCache[item.id] {
            return cached
        }
        let img = IconRenderer.loadOriginalIcon(
            bundlePath: item.bundlePath,
            symbolName: item.symbolName,
            brandHex: UInt32(clamping: item.brandHex),
            canvasSize: 256
        )
        originalCache[item.id] = img
        return img
    }

    /// The effective style for `item`: blends global studio style with any item-specific scale/offset overrides.
    func effectiveStyle(for item: Item) -> RoundIconStyle {
        var s = style
        if let scale = item.customScale { s.iconScale = scale }
        if let ox = item.customOffsetX { s.offsetX = ox }
        if let oy = item.customOffsetY { s.offsetY = oy }
        return s
    }

    /// Clears the cached rendered bitmap for a specific item.
    func invalidateCache(for item: Item) {
        roundCache[item.id] = nil
        originalCache[item.id] = nil
    }

    /// Assigns custom vector SVG or raster image data as the glyph for `item`.
    func setCustomArtwork(data: Data, for item: Item) {
        item.customImageData = data
        invalidateCache(for: item)
        let isSVG = CustomArtworkHelper.isSVGData(data)
        statusMessage = "Custom \(isSVG ? "vector SVG" : "image") assigned to \(item.title)"
    }

    /// Clears any custom icon for `item`, restoring the system default icon.
    func clearCustomArtwork(for item: Item) {
        item.customImageData = nil
        invalidateCache(for: item)
        statusMessage = "Restored default icon for \(item.title)"
    }

    /// Assigns custom artwork from clipboard if available.
    @discardableResult
    func pasteCustomArtwork(for item: Item) -> Bool {
        guard let data = CustomArtworkHelper.loadArtworkData(from: .general) else {
            statusMessage = "Clipboard does not contain an image or SVG"
            return false
        }
        setCustomArtwork(data: data, for: item)
        return true
    }

    /// Prompts user with open panel to select custom artwork for `item`.
    func promptCustomArtwork(for item: Item) {
        if let data = CustomArtworkHelper.promptUserForArtwork() {
            setCustomArtwork(data: data, for: item)
        }
    }

    /// Prompts user with open panel to select custom artwork for the currently selected item.
    func promptCustomArtworkForSelection() {
        guard let id = selection, let item = (try? container.mainContext.fetch(FetchDescriptor<Item>()))?.first(where: { $0.id == id }) else { return }
        promptCustomArtwork(for: item)
        try? container.mainContext.save()
    }

    /// Assigns custom artwork from clipboard to the currently selected item.
    func pasteCustomArtworkForSelection() {
        guard let id = selection, let item = (try? container.mainContext.fetch(FetchDescriptor<Item>()))?.first(where: { $0.id == id }) else { return }
        if pasteCustomArtwork(for: item) {
            try? container.mainContext.save()
        }
    }

    /// Returns the cached circular Google Pixel / Material You icon for `item` under its effective style.
    func roundIcon(for item: Item) -> NSImage {
        if let cached = roundCache[item.id] {
            return cached
        }
        let source = sourceIcon(for: item, canvasSize: 256)
        let rendered = IconRenderer.renderRoundIcon(
            sourceImage: source,
            style: effectiveStyle(for: item),
            fallbackHex: UInt32(clamping: item.brandHex),
            canvasSize: 256
        )
        roundCache[item.id] = rendered
        return rendered
    }

    /// Adds or imports an application by its file URL (e.g. via drag-and-drop or file picker)
    func addApplication(from url: URL) {
        let fullPath = url.path
        let bundle = Bundle(url: url)
        let displayName = (bundle?.object(forInfoDictionaryKey: "CFBundleDisplayName") as? String)
            ?? (bundle?.object(forInfoDictionaryKey: "CFBundleName") as? String)
            ?? ItemTitle.clean(url.deletingPathExtension().lastPathComponent)
        let bundleID = bundle?.bundleIdentifier ?? "com.custom.\(ItemTitle.clean(url.deletingPathExtension().lastPathComponent).lowercased())"
        let category = fullPath.hasPrefix("/System") ? "macOS System App" : "Custom / Dropped Application"
        
        let palette: [Int] = [
            0x1A73E8, 0xEA4335, 0x34A853, 0xFBBC04,
            0x7B5CFF, 0x06B6D4, 0xE84A93, 0xF97316,
        ]
        let hex = palette[abs(displayName.hashValue) % palette.count]

        let context = container.mainContext
        let existing = (try? context.fetch(FetchDescriptor<Item>())) ?? []
        if let match = existing.first(where: { $0.bundlePath == fullPath }) {
            selection = match.id
            statusMessage = "\(match.title) is already in library"
            return
        }

        let item = Item(
            title: displayName,
            bundlePath: fullPath,
            bundleIdentifier: bundleID,
            categoryName: category,
            symbolName: "app.dashed",
            brandHex: hex
        )
        context.insert(item)
        try? context.save()
        selection = item.id
        statusMessage = "Imported \(item.title)"
    }

    /// Adds or imports a directory folder by its URL to customize its Finder icon.
    func addFolder(from url: URL) {
        let fullPath = url.path
        let folderName = url.lastPathComponent
        let context = container.mainContext
        let existing = (try? context.fetch(FetchDescriptor<Item>())) ?? []
        if let match = existing.first(where: { $0.bundlePath == fullPath }) {
            selection = match.id
            statusMessage = "Folder \(match.title) is already in library"
            return
        }

        let item = Item(
            title: folderName,
            bundlePath: fullPath,
            bundleIdentifier: "com.folder.\(UUID().uuidString.prefix(6).lowercased())",
            categoryName: "Folder",
            symbolName: "folder.fill",
            brandHex: FolderColorPreset.appleBlue.hex
        )
        context.insert(item)
        try? context.save()
        selection = item.id
        statusMessage = "Imported folder \(item.title)"
    }

    /// Prompts user to pick a folder from disk to style.
    func promptFolderPicker() {
        let panel = NSOpenPanel()
        panel.title = "Choose Folder to Style"
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.allowsMultipleSelection = false
        panel.prompt = "Choose Folder"
        if panel.runModal() == .OK, let url = panel.url {
            addFolder(from: url)
        }
    }

    /// Adds a custom sample application item (used by File > New Item and self-test).
    func addItem(
        title: String = "Custom App",
        bundlePath: String = "",
        symbolName: String = "sparkles.rectangle.stack.fill",
        brandHex: Int = 0x1A73E8
    ) {
        let context = container.mainContext
        let item = Item(
            title: title,
            bundlePath: bundlePath,
            bundleIdentifier: "com.custom.\(UUID().uuidString.prefix(6).lowercased())",
            categoryName: bundlePath.isEmpty ? "Custom Icon" : "macOS Application",
            symbolName: symbolName,
            brandHex: brandHex
        )
        context.insert(item)
        try? context.save()
        selection = item.id
        statusMessage = "Added \(item.title)"
    }

    /// Scans `/Applications` and `~/Applications` for installed `.app` bundles when not running in an isolated test store.
    func scanInstalledApplications(limit: Int = 40) {
        let context = container.mainContext
        let existing = (try? context.fetch(FetchDescriptor<Item>())) ?? []
        var existingPaths = Set(existing.map(\.bundlePath).filter { !$0.isEmpty })

        let roots = [
            "/Applications",
            (NSHomeDirectory() as NSString).appendingPathComponent("Applications"),
            "/System/Applications",
        ]

        let palette: [Int] = [
            0x1A73E8, 0xEA4335, 0x34A853, 0xFBBC04,
            0x7B5CFF, 0x06B6D4, 0xE84A93, 0xF97316,
        ]
        var addedCount = 0

        for root in roots {
            guard let entries = try? FileManager.default.contentsOfDirectory(atPath: root) else { continue }
            for entry in entries.sorted() where entry.hasSuffix(".app") {
                let fullPath = (root as NSString).appendingPathComponent(entry)
                guard !existingPaths.contains(fullPath) else { continue }
                let bundle = Bundle(path: fullPath)
                let displayName = (bundle?.object(forInfoDictionaryKey: "CFBundleDisplayName") as? String)
                    ?? (bundle?.object(forInfoDictionaryKey: "CFBundleName") as? String)
                    ?? ItemTitle.clean(entry)
                let bundleID = bundle?.bundleIdentifier ?? "com.apple.\(ItemTitle.clean(entry).lowercased())"
                let category = root.hasPrefix("/System") ? "macOS System App" : "Installed Application"
                let hex = palette[abs(displayName.hashValue) % palette.count]

                let item = Item(
                    title: displayName,
                    bundlePath: fullPath,
                    bundleIdentifier: bundleID,
                    categoryName: category,
                    symbolName: "app.badge.checkmark.fill",
                    brandHex: hex
                )
                context.insert(item)
                existingPaths.insert(fullPath)
                addedCount += 1
                if existing.count + addedCount >= limit { break }
            }
            if existing.count + addedCount >= limit { break }
        }

        try? context.save()
        if selection == nil {
            let all = (try? context.fetch(FetchDescriptor<Item>(sortBy: [SortDescriptor(\.title)]))) ?? []
            selection = all.first?.id
        }
        statusMessage = addedCount > 0
            ? "Scanned macOS applications (\(addedCount) added)"
            : "All installed applications are up to date"
    }

    /// Returns the rendered customized folder icon for `item`.
    func folderIcon(for item: Item) -> NSImage {
        if let cached = roundCache[item.id] { return cached }
        let preset = FolderColorPreset.allCases.first(where: { $0.hex == item.brandHex }) ?? .appleBlue
        let folderStyle = FolderStyle(
            colorHex: preset.hex,
            accentHex: preset.accentHex,
            embossStyle: .etched,
            badgeSymbolName: item.symbolName
        )
        var glyphSVG: String? = nil
        if let data = item.customImageData, let text = String(data: data, encoding: .utf8), CustomArtworkHelper.isSVGData(data) {
            glyphSVG = text
        }
        let svg = FolderVectorGenerator.generateFolderSVG(style: folderStyle, size: 512, centerGlyphSVG: glyphSVG)
        if let data = svg.data(using: .utf8), let img = NSImage(data: data) {
            roundCache[item.id] = img
            return img
        }
        return roundIcon(for: item)
    }

    /// Applies the adaptive Modo icon to a single `Item`.
    func applyRoundIcon(to item: Item) {
        let hiRes: NSImage
        if item.isFolder {
            hiRes = folderIcon(for: item)
        } else {
            let source = sourceIcon(for: item, canvasSize: 1024)
            hiRes = IconRenderer.renderRoundIcon(
                sourceImage: source,
                style: effectiveStyle(for: item),
                fallbackHex: UInt32(clamping: item.brandHex),
                canvasSize: 1024
            )
        }
        _ = IconRenderer.applyIcon(hiRes, toAppAt: item.bundlePath)
        item.isApplied = true
        recordApplied(item)
        try? container.mainContext.save()
        let name = item.isFolder ? "Folder Style" : activePackName
        statusMessage = "Applied \(name) to \(item.title)"
    }

    /// Applies adaptive Modo icons to all provided `items`.
    func applyRoundIconsToAll(_ items: [Item]) {
        guard !items.isEmpty else { return }
        for item in items {
            let source = sourceIcon(for: item, canvasSize: 512)
            let round = IconRenderer.renderRoundIcon(
                sourceImage: source,
                style: effectiveStyle(for: item),
                fallbackHex: UInt32(clamping: item.brandHex),
                canvasSize: 512
            )
            _ = IconRenderer.applyIcon(round, toAppAt: item.bundlePath)
            item.isApplied = true
            recordApplied(item)
        }
        try? container.mainContext.save()
        let packName = activePackName
        statusMessage = "Applied \(packName) pack to \(items.count) apps"
    }

    /// Restores the original macOS icon for `item`.
    func restoreOriginalIcon(for item: Item) {
        _ = IconRenderer.restoreOriginalIcon(at: item.bundlePath)
        item.isApplied = false
        forgetApplied(item)
        try? container.mainContext.save()
        statusMessage = "Restored original macOS icon for \(item.title)"
    }

    /// Restores original macOS icons for all provided `items`.
    func restoreAllOriginalIcons(_ items: [Item]) {
        for item in items {
            _ = IconRenderer.restoreOriginalIcon(at: item.bundlePath)
            item.isApplied = false
            forgetApplied(item)
        }
        try? container.mainContext.save()
        statusMessage = "Restored original macOS icons for \(items.count) apps"
    }

    /// Exports `.icns`, `.ico`, and `1024×1024` `.png` files for `items` into `directory`.
    @discardableResult
    func exportIconPack(for items: [Item], to directory: URL) -> Int {
        var count = 0
        for item in items {
            let source = sourceIcon(for: item, canvasSize: 1024)
            if let urls = try? IconRenderer.exportIconPack(
                for: item.title,
                sourceImage: source,
                style: effectiveStyle(for: item),
                fallbackHex: UInt32(clamping: item.brandHex),
                to: directory
            ), !urls.isEmpty {
                count += 1
            }
        }
        statusMessage = "Exported \(count) icon sets (.icns, .ico, .png) to \(directory.lastPathComponent)"
        return count
    }

    /// Exports PNG icons in specific pixel resolutions for `items` into `directory`.
    @discardableResult
    func exportCustomSizes(for items: [Item], sizes: [Int], to directory: URL) -> Int {
        var count = 0
        for item in items {
            let source = sourceIcon(for: item, canvasSize: 1024)
            if let urls = try? IconRenderer.exportPNGSizes(
                for: item.title,
                sourceImage: source,
                style: effectiveStyle(for: item),
                fallbackHex: UInt32(clamping: item.brandHex),
                sizes: sizes,
                to: directory
            ), !urls.isEmpty {
                count += 1
            }
        }
        statusMessage = "Exported \(sizes.count) resolutions for \(count) apps to \(directory.lastPathComponent)"
        return count
    }

    /// Exports Light, Dark, and Tinted PNG variants for `items` into `directory` (macOS Sequoia & iOS 18 style).
    @discardableResult
    func exportAppearanceVariants(for items: [Item], to directory: URL) -> Int {
        var count = 0
        for item in items {
            let source = sourceIcon(for: item, canvasSize: 1024)
            if let urls = try? IconRenderer.exportAppearanceVariants(
                for: item.title,
                sourceImage: source,
                baseStyle: effectiveStyle(for: item),
                fallbackHex: UInt32(clamping: item.brandHex),
                to: directory
            ), !urls.isEmpty {
                count += 1
            }
        }
        statusMessage = "Exported Light, Dark & Tinted variants for \(count) apps to \(directory.lastPathComponent)"
        return count
    }
}
