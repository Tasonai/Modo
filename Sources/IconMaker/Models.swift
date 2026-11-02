import AppKit
import Foundation
import IconMakerKit
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

    init(
        title: String,
        bundlePath: String = "",
        bundleIdentifier: String = "com.example.app",
        categoryName: String = "Application",
        symbolName: String = "app.fill",
        brandHex: Int = 0x1A73E8,
        isStarred: Bool = false,
        isApplied: Bool = false
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
        case .applied: "Round Applied"
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

    var style: RoundIconStyle = RoundIconStyle() {
        didSet {
            roundCache.removeAll()
        }
    }

    private var originalCache: [UUID: NSImage] = [:]
    private var roundCache: [UUID: NSImage] = [:]

    init(container: ModelContainer) {
        self.container = container
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

    /// Returns the cached circular Google Pixel / Material You icon for `item` under the current `style`.
    func roundIcon(for item: Item) -> NSImage {
        if let cached = roundCache[item.id] {
            return cached
        }
        let source = originalIcon(for: item)
        let rendered = IconRenderer.renderRoundIcon(
            sourceImage: source,
            style: style,
            fallbackHex: UInt32(clamping: item.brandHex),
            canvasSize: 256
        )
        roundCache[item.id] = rendered
        return rendered
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

    /// Applies the circular Google Pixel icon to a single `Item`.
    func applyRoundIcon(to item: Item) {
        let source = IconRenderer.loadOriginalIcon(
            bundlePath: item.bundlePath,
            symbolName: item.symbolName,
            brandHex: UInt32(clamping: item.brandHex),
            canvasSize: 1024
        )
        let hiResRound = IconRenderer.renderRoundIcon(
            sourceImage: source,
            style: style,
            fallbackHex: UInt32(clamping: item.brandHex),
            canvasSize: 1024
        )
        _ = IconRenderer.applyIcon(hiResRound, toAppAt: item.bundlePath)
        item.isApplied = true
        try? container.mainContext.save()
        statusMessage = "Applied circular Google icon to \(item.title)"
    }

    /// Applies circular Google Pixel icons to all provided `items`.
    func applyRoundIconsToAll(_ items: [Item]) {
        guard !items.isEmpty else { return }
        for item in items {
            let source = IconRenderer.loadOriginalIcon(
                bundlePath: item.bundlePath,
                symbolName: item.symbolName,
                brandHex: UInt32(clamping: item.brandHex),
                canvasSize: 512
            )
            let round = IconRenderer.renderRoundIcon(
                sourceImage: source,
                style: style,
                fallbackHex: UInt32(clamping: item.brandHex),
                canvasSize: 512
            )
            _ = IconRenderer.applyIcon(round, toAppAt: item.bundlePath)
            item.isApplied = true
        }
        try? container.mainContext.save()
        statusMessage = "Applied circular Google icons to \(items.count) apps"
    }

    /// Restores the original macOS icon for `item`.
    func restoreOriginalIcon(for item: Item) {
        _ = IconRenderer.restoreOriginalIcon(at: item.bundlePath)
        item.isApplied = false
        try? container.mainContext.save()
        statusMessage = "Restored original macOS icon for \(item.title)"
    }

    /// Restores original macOS icons for all provided `items`.
    func restoreAllOriginalIcons(_ items: [Item]) {
        for item in items {
            _ = IconRenderer.restoreOriginalIcon(at: item.bundlePath)
            item.isApplied = false
        }
        try? container.mainContext.save()
        statusMessage = "Restored original macOS icons for \(items.count) apps"
    }

    /// Exports `.icns`, `.ico`, and `1024×1024` `.png` files for `items` into `directory`.
    @discardableResult
    func exportIconPack(for items: [Item], to directory: URL) -> Int {
        var count = 0
        for item in items {
            let source = IconRenderer.loadOriginalIcon(
                bundlePath: item.bundlePath,
                symbolName: item.symbolName,
                brandHex: UInt32(clamping: item.brandHex),
                canvasSize: 1024
            )
            if let urls = try? IconRenderer.exportIconPack(
                for: item.title,
                sourceImage: source,
                style: style,
                fallbackHex: UInt32(clamping: item.brandHex),
                to: directory
            ), !urls.isEmpty {
                count += 1
            }
        }
        statusMessage = "Exported \(count) circular icon packs (.icns, .ico, .png) to \(directory.lastPathComponent)"
        return count
    }
}
