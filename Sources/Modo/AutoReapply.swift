import AppKit
import Foundation
import ModoKit
import SwiftData

/// Keeps styled apps styled: macOS drops a custom icon when an app is updated, so Modo
/// notices (on its own launch and whenever an app launches) and applies the style again.
extension AppModel {
    private static let recordsKey = "modo.appliedRecords"
    private static let autoReapplyKey = "autoReapplyIcons"
    private nonisolated(unsafe) static var launchObserver: NSObjectProtocol?
    /// Lets the self-test exercise record persistence on its throwaway store.
    nonisolated(unsafe) static var persistRecordsInSelfTest = false

    /// User preference, on by default. Toggled in Settings.
    static var autoReapplyEnabled: Bool {
        UserDefaults.standard.object(forKey: autoReapplyKey) as? Bool ?? true
    }

    // MARK: Records

    private func loadRecords() -> [String: AppliedIconRecord] {
        guard let data = UserDefaults.standard.data(forKey: Self.recordsKey),
              let records = try? JSONDecoder().decode([String: AppliedIconRecord].self, from: data) else { return [:] }
        return records
    }

    static func clearRecords() { UserDefaults.standard.removeObject(forKey: recordsKey) }

    private func saveRecords(_ records: [String: AppliedIconRecord]) {
        guard !Store.isOverridden || Self.persistRecordsInSelfTest, let data = try? JSONEncoder().encode(records) else { return }
        UserDefaults.standard.set(data, forKey: Self.recordsKey)
    }

    static func bundleVersion(at path: String) -> String {
        // Read the plist from disk: Bundle caches its info dictionary, which goes stale after an update.
        let plistURL = URL(fileURLWithPath: path).appendingPathComponent("Contents/Info.plist")
        let info = NSDictionary(contentsOf: plistURL) as? [String: Any]
        let short = info?["CFBundleShortVersionString"] as? String ?? "?"
        let build = info?["CFBundleVersion"] as? String ?? "?"
        return "\(short) (\(build))"
    }

    private static func hasIconFile(at path: String) -> Bool {
        FileManager.default.fileExists(atPath: (path as NSString).appendingPathComponent("Icon\r"))
    }

    /// Remembers that `item` was just styled.
    func recordApplied(_ item: Item) {
        guard !item.bundlePath.isEmpty, FileManager.default.fileExists(atPath: item.bundlePath) else { return }
        var records = loadRecords()
        records[item.bundlePath] = AppliedIconRecord(
            version: Self.bundleVersion(at: item.bundlePath),
            hadIconFile: Self.hasIconFile(at: item.bundlePath)
        )
        saveRecords(records)
    }

    func forgetApplied(_ item: Item) {
        var records = loadRecords()
        records.removeValue(forKey: item.bundlePath)
        saveRecords(records)
    }

    // MARK: Re-applying

    /// Styles again every applied app (or only the one at `bundlePath`) that lost its icon.
    @discardableResult
    func reapplyUpdatedApps(only bundlePath: String? = nil) -> Int {
        guard Self.autoReapplyEnabled else { return 0 }
        let records = loadRecords()
        guard !records.isEmpty else { return 0 }
        let context = container.mainContext
        let applied = ((try? context.fetch(FetchDescriptor<Item>())) ?? []).filter {
            $0.isApplied && (bundlePath == nil || $0.bundlePath == bundlePath)
        }
        var count = 0
        for item in applied {
            let needs = ReapplyPolicy.needsReapply(
                record: records[item.bundlePath],
                currentVersion: Self.bundleVersion(at: item.bundlePath),
                hasIconFile: Self.hasIconFile(at: item.bundlePath)
            )
            guard needs else { continue }
            let rendered: NSImage
            if item.isFolder {
                rendered = folderIcon(for: item)
            } else {
                let source = sourceIcon(for: item, canvasSize: 512)
                rendered = IconRenderer.renderRoundIcon(
                    sourceImage: source,
                    style: effectiveStyle(for: item),
                    fallbackHex: UInt32(clamping: item.brandHex),
                    canvasSize: 512
                )
            }
            _ = IconRenderer.applyIcon(rendered, toAppAt: item.bundlePath)
            recordApplied(item)
            count += 1
        }
        if count > 0 {
            statusMessage = "Re-applied Modo icons to \(count) updated app\(count == 1 ? "" : "s")"
        }
        return count
    }

    /// Checks once now and again whenever any app launches (updaters relaunch the app they replaced).
    func startAutoReapply() {
        guard !Store.isOverridden, Self.launchObserver == nil else { return }
        Task { @MainActor [weak self] in
            try? await Task.sleep(for: .seconds(2))
            self?.reapplyUpdatedApps()
        }
        Self.launchObserver = NSWorkspace.shared.notificationCenter.addObserver(
            forName: NSWorkspace.didLaunchApplicationNotification,
            object: nil,
            queue: .main
        ) { [weak self] note in
            guard let path = (note.userInfo?[NSWorkspace.applicationUserInfoKey] as? NSRunningApplication)?.bundleURL?.path else { return }
            Task { @MainActor [weak self] in
                try? await Task.sleep(for: .seconds(3))
                self?.reapplyUpdatedApps(only: path)
            }
        }
    }
}
