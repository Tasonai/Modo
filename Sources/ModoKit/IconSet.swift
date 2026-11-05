import CryptoKit
import Foundation

/// An icon set: one ready-made PNG per app (matched by bundle identifier) that Modo can use as the
/// artwork instead of an app's own icon. Stored as a zip with `manifest.json` plus PNG files.
public struct IconSetManifest: Codable, Equatable, Sendable {
    public struct Entry: Codable, Equatable, Sendable {
        public var bundleID: String
        public var file: String
        public init(bundleID: String, file: String) {
            self.bundleID = bundleID
            self.file = file
        }
    }

    public var format: String
    public var version: Int
    public var id: String
    public var name: String
    public var author: String
    public var license: String
    public var icons: [Entry]

    public init(id: String, name: String, author: String, license: String, icons: [Entry]) {
        self.format = IconSetRules.setFormat
        self.version = IconSetRules.currentVersion
        self.id = id
        self.name = name
        self.author = author
        self.license = license
        self.icons = icons
    }
}

/// The online catalog: a JSON list of downloadable icon sets.
public struct IconCatalog: Codable, Equatable, Sendable {
    public struct Entry: Codable, Equatable, Identifiable, Sendable {
        public var id: String
        public var name: String
        public var author: String
        public var license: String
        public var version: String
        /// Direct download link of the set's zip (https).
        public var url: String
        /// Lowercase hex SHA-256 of the zip. Modo refuses a download that does not match.
        public var sha256: String
        public var sizeBytes: Int
        public var iconCount: Int

        public init(id: String, name: String, author: String, license: String, version: String,
                    url: String, sha256: String, sizeBytes: Int, iconCount: Int) {
            self.id = id
            self.name = name
            self.author = author
            self.license = license
            self.version = version
            self.url = url
            self.sha256 = sha256
            self.sizeBytes = sizeBytes
            self.iconCount = iconCount
        }
    }

    public var format: String
    public var version: Int
    public var sets: [Entry]

    public init(format: String = IconSetRules.catalogFormat, version: Int = IconSetRules.currentVersion, sets: [Entry]) {
        self.format = format
        self.version = version
        self.sets = sets
    }
}

public enum IconSetError: Error, Equatable {
    case notACatalog
    case notAnIconSet
    case unsupportedVersion(Int)
    case invalidID(String)
    case unsafePath(String)
    case tooManyIcons(Int)
    case duplicateBundleID(String)
    case insecureURL(String)
    case tooLarge(Int)
    case checksumMismatch
}

/// Validation rules shared by the catalog reader and the installer. Everything here treats the
/// data as untrusted: it comes from the internet.
public enum IconSetRules {
    public static let setFormat = "modo-icon-set"
    public static let catalogFormat = "modo-icon-catalog"
    public static let defaultCatalogURL = "https://raw.githubusercontent.com/atoma/modo-icon-sets/main/catalog.json"
    public static let currentVersion = 1
    public static let maxIconsPerSet = 3000
    public static let maxZipBytes = 80 * 1024 * 1024
    public static let maxPNGBytes = 6 * 1024 * 1024
    public static let maxPixels = 2048

    /// Set ids become folder names, so only a tight character set is allowed.
    public static func isValidID(_ id: String) -> Bool {
        guard (1...64).contains(id.count) else { return false }
        return id.allSatisfy { $0.isASCII && ($0.isLowercase || $0.isNumber || $0 == "-" || $0 == "_" || $0 == ".") }
            && !id.hasPrefix(".")
            && !id.contains("..")
    }

    /// A relative path to a PNG inside the set: no absolute paths, no `..`, no hidden files.
    public static func isSafeRelativePNGPath(_ path: String) -> Bool {
        guard !path.isEmpty, path.count <= 200, !path.hasPrefix("/"), !path.contains("\\"),
              path.lowercased().hasSuffix(".png") else { return false }
        let parts = path.split(separator: "/", omittingEmptySubsequences: false)
        return parts.allSatisfy { !$0.isEmpty && $0 != "." && $0 != ".." && !$0.hasPrefix(".") }
    }

    /// Bundle identifiers are reverse-DNS-ish; reject anything exotic.
    public static func isValidBundleID(_ id: String) -> Bool {
        guard (1...200).contains(id.count) else { return false }
        return id.allSatisfy { $0.isASCII && ($0.isLetter || $0.isNumber || $0 == "." || $0 == "-" || $0 == "_") }
    }

    public static func sha256Hex(_ data: Data) -> String {
        SHA256.hash(data: data).map { String(format: "%02x", $0) }.joined()
    }

    // MARK: Decoding

    /// Parses a catalog. Entries that fail validation are dropped; a file that is not a catalog throws.
    public static func decodeCatalog(_ data: Data, allowFileURLs: Bool = false) throws -> IconCatalog {
        guard var catalog = try? JSONDecoder().decode(IconCatalog.self, from: data),
              catalog.format == catalogFormat else { throw IconSetError.notACatalog }
        guard catalog.version <= currentVersion else { throw IconSetError.unsupportedVersion(catalog.version) }
        var seen = Set<String>()
        catalog.sets = catalog.sets.filter { entry in
            guard isValidID(entry.id), seen.insert(entry.id).inserted,
                  entry.sizeBytes > 0, entry.sizeBytes <= maxZipBytes,
                  entry.sha256.count == 64, entry.sha256.allSatisfy({ $0.isHexDigit && !$0.isUppercase }),
                  let url = URL(string: entry.url), let scheme = url.scheme?.lowercased(),
                  scheme == "https" || (allowFileURLs && scheme == "file") else { return false }
            return true
        }
        return catalog
    }

    /// Parses and validates a set manifest read from a downloaded zip.
    public static func decodeManifest(_ data: Data) throws -> IconSetManifest {
        guard let manifest = try? JSONDecoder().decode(IconSetManifest.self, from: data),
              manifest.format == setFormat else { throw IconSetError.notAnIconSet }
        guard manifest.version <= currentVersion else { throw IconSetError.unsupportedVersion(manifest.version) }
        guard isValidID(manifest.id) else { throw IconSetError.invalidID(manifest.id) }
        guard manifest.icons.count <= maxIconsPerSet else { throw IconSetError.tooManyIcons(manifest.icons.count) }
        var seen = Set<String>()
        for entry in manifest.icons {
            guard isSafeRelativePNGPath(entry.file) else { throw IconSetError.unsafePath(entry.file) }
            guard isValidBundleID(entry.bundleID), seen.insert(entry.bundleID).inserted else {
                throw IconSetError.duplicateBundleID(entry.bundleID)
            }
        }
        return manifest
    }
}
