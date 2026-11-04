import Foundation

/// What Modo remembers about an icon it applied to an app bundle.
public struct AppliedIconRecord: Codable, Equatable, Sendable {
    /// `CFBundleShortVersionString (CFBundleVersion)` at the time the icon was applied.
    public var version: String
    /// Whether the bundle contained the custom-icon file right after applying.
    public var hadIconFile: Bool

    public init(version: String, hadIconFile: Bool) {
        self.version = version
        self.hadIconFile = hadIconFile
    }
}

/// Decides when a styled app lost its icon and must be styled again.
public enum ReapplyPolicy {
    /// - Returns: `true` when the app was updated (version changed) or the custom-icon file
    ///   that existed after applying has disappeared. Apps Modo never styled return `false`.
    public static func needsReapply(record: AppliedIconRecord?, currentVersion: String, hasIconFile: Bool) -> Bool {
        guard let record else { return false }
        if record.version != currentVersion { return true }
        return record.hadIconFile && !hasIconFile
    }
}
