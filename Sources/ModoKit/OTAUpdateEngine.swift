import CryptoKit
import Foundation

// MARK: - Semantic Version

/// A type-safe semantic version parser and comparator (SemVer 2.0 compliant subset).
public struct SemanticVersion: Comparable, Hashable, CustomStringConvertible, Codable, Sendable {
    public let major: Int
    public let minor: Int
    public let patch: Int
    public let prerelease: String?
    public let build: String?

    public var description: String {
        var result = "\(major).\(minor).\(patch)"
        if let prerelease = prerelease, !prerelease.isEmpty {
            result += "-\(prerelease)"
        }
        if let build = build, !build.isEmpty {
            result += "+\(build)"
        }
        return result
    }

    public init(major: Int, minor: Int, patch: Int, prerelease: String? = nil, build: String? = nil) {
        self.major = max(0, major)
        self.minor = max(0, minor)
        self.patch = max(0, patch)
        self.prerelease = prerelease?.trimmingCharacters(in: .whitespacesAndNewlines)
        self.build = build?.trimmingCharacters(in: .whitespacesAndNewlines)
    }

    /// Parse a version string like "1.5.0", "v1.6.2", "2.0.0-beta.1+20261105".
    public init?(_ string: String) {
        var raw = string.trimmingCharacters(in: .whitespacesAndNewlines)
        if raw.lowercased().hasPrefix("v") {
            raw.removeFirst()
        }
        guard !raw.isEmpty else { return nil }

        // Split build metadata
        var buildMeta: String?
        if let plusIdx = raw.firstIndex(of: "+") {
            buildMeta = String(raw[raw.index(after: plusIdx)...])
            raw = String(raw[..<plusIdx])
        }

        // Split prerelease
        var preMeta: String?
        if let dashIdx = raw.firstIndex(of: "-") {
            preMeta = String(raw[raw.index(after: dashIdx)...])
            raw = String(raw[..<dashIdx])
        }

        let parts = raw.split(separator: ".", omittingEmptySubsequences: false)
        guard parts.count >= 1 && parts.count <= 3 else { return nil }

        guard let major = Int(parts[0]) else { return nil }
        let minor = parts.count > 1 ? (Int(parts[1]) ?? 0) : 0
        let patch = parts.count > 2 ? (Int(parts[2]) ?? 0) : 0

        self.init(major: major, minor: minor, patch: patch, prerelease: preMeta, build: buildMeta)
    }

    public static func < (lhs: SemanticVersion, rhs: SemanticVersion) -> Bool {
        if lhs.major != rhs.major { return lhs.major < rhs.major }
        if lhs.minor != rhs.minor { return lhs.minor < rhs.minor }
        if lhs.patch != rhs.patch { return lhs.patch < rhs.patch }

        // A version without prerelease is higher than one with prerelease (e.g. 1.0.0 > 1.0.0-beta)
        switch (lhs.prerelease, rhs.prerelease) {
        case (nil, nil):
            return false
        case (nil, .some):
            return false
        case (.some, nil):
            return true
        case let (.some(lPre), .some(rPre)):
            return lPre.localizedStandardCompare(rPre) == .orderedAscending
        }
    }

    public static func == (lhs: SemanticVersion, rhs: SemanticVersion) -> Bool {
        lhs.major == rhs.major &&
            lhs.minor == rhs.minor &&
            lhs.patch == rhs.patch &&
            lhs.prerelease == rhs.prerelease
    }

    public func isNewerThan(_ other: SemanticVersion) -> Bool {
        self > other
    }

    public func isNewerThan(_ versionString: String) -> Bool {
        guard let other = SemanticVersion(versionString) else { return true }
        return self > other
    }
}

// MARK: - Update Manifest Models

public enum UpdatePlatform: String, Codable, Sendable, CaseIterable {
    case macos
    case windows
    case universal
}

public enum UpdateAssetFormat: String, Codable, Sendable, CaseIterable {
    case zip
    case dmg
    case pkg
    case exe
}

public struct UpdateAsset: Codable, Sendable, Identifiable, Hashable {
    public var id: String { name }
    public let name: String
    public let platform: UpdatePlatform
    public let format: UpdateAssetFormat
    public let downloadURL: URL
    public let sha256: String
    public let sizeBytes: Int64
    public let isPrimary: Bool

    public init(
        name: String,
        platform: UpdatePlatform,
        format: UpdateAssetFormat,
        downloadURL: URL,
        sha256: String,
        sizeBytes: Int64,
        isPrimary: Bool = false
    ) {
        self.name = name
        self.platform = platform
        self.format = format
        self.downloadURL = downloadURL
        self.sha256 = sha256.lowercased().trimmingCharacters(in: .whitespacesAndNewlines)
        self.sizeBytes = sizeBytes
        self.isPrimary = isPrimary
    }
}

public struct UpdateManifest: Codable, Sendable {
    public let version: String
    public let build: String?
    public let releaseDate: String
    public let title: String
    public let releaseNotes: String
    public let minOSVersion: String?
    public let isMandatory: Bool
    public let assets: [UpdateAsset]

    public init(
        version: String,
        build: String? = nil,
        releaseDate: String,
        title: String,
        releaseNotes: String,
        minOSVersion: String? = nil,
        isMandatory: Bool = false,
        assets: [UpdateAsset]
    ) {
        self.version = version
        self.build = build
        self.releaseDate = releaseDate
        self.title = title
        self.releaseNotes = releaseNotes
        self.minOSVersion = minOSVersion
        self.isMandatory = isMandatory
        self.assets = assets
    }

    public var semanticVersion: SemanticVersion? {
        SemanticVersion(version)
    }
}

// MARK: - OTA Update Engine

public enum OTAUpdateEngine {
    /// Check if manifest represents a newer version than currentVersion.
    public static func isUpdateAvailable(manifest: UpdateManifest, currentVersion: String) -> Bool {
        guard let manifestVer = manifest.semanticVersion,
              let currentVer = SemanticVersion(currentVersion) else {
            return false
        }
        return manifestVer > currentVer
    }

    /// Select the best asset for the specified platform and preferred format.
    public static func findBestAsset(
        in manifest: UpdateManifest,
        for platform: UpdatePlatform,
        preferredFormat: UpdateAssetFormat? = nil
    ) -> UpdateAsset? {
        let matching = manifest.assets.filter { $0.platform == platform || $0.platform == .universal }
        guard !matching.isEmpty else { return nil }

        if let preferred = preferredFormat,
           let direct = matching.first(where: { $0.format == preferred }) {
            return direct
        }

        // Return primary asset if designated
        if let primary = matching.first(where: { $0.isPrimary }) {
            return primary
        }

        // Default platform priority:
        // macOS: zip (for seamless in-place install) > dmg > pkg
        // Windows: exe > zip
        switch platform {
        case .macos:
            return matching.first(where: { $0.format == .zip })
                ?? matching.first(where: { $0.format == .dmg })
                ?? matching.first(where: { $0.format == .pkg })
                ?? matching.first
        case .windows:
            return matching.first(where: { $0.format == .exe })
                ?? matching.first(where: { $0.format == .zip })
                ?? matching.first
        case .universal:
            return matching.first
        }
    }

    /// Compute SHA256 hex string for given Data.
    public static func sha256(data: Data) -> String {
        let digest = SHA256.hash(data: data)
        return digest.map { String(format: "%02x", $0) }.joined()
    }

    /// Compute SHA256 hex string by streaming file from URL.
    public static func sha256(fileURL: URL) throws -> String {
        let handle = try FileHandle(forReadingFrom: fileURL)
        defer { try? handle.close() }

        var hasher = SHA256()
        let bufferSize = 64 * 1024 // 64 KB chunks
        while true {
            let chunk = handle.readData(ofLength: bufferSize)
            if chunk.isEmpty { break }
            hasher.update(data: chunk)
        }
        let digest = hasher.finalize()
        return digest.map { String(format: "%02x", $0) }.joined()
    }

    /// Verify that file data matches expected SHA256 checksum.
    public static func verifyChecksum(data: Data, expectedSHA256: String) -> Bool {
        let cleanExpected = expectedSHA256.lowercased().trimmingCharacters(in: .whitespacesAndNewlines)
        return sha256(data: data) == cleanExpected
    }

    /// Verify that file at URL matches expected SHA256 checksum.
    public static func verifyChecksum(fileURL: URL, expectedSHA256: String) -> Bool {
        let cleanExpected = expectedSHA256.lowercased().trimmingCharacters(in: .whitespacesAndNewlines)
        guard let computed = try? sha256(fileURL: fileURL) else { return false }
        return computed == cleanExpected
    }

    /// Format byte size into human-readable string (KB, MB, GB).
    public static func formatBytes(_ bytes: Int64) -> String {
        let formatter = ByteCountFormatter()
        formatter.allowedUnits = [.useBytes, .useKB, .useMB, .useGB]
        formatter.countStyle = .file
        return formatter.string(fromByteCount: bytes)
    }
}
