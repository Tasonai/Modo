import AppKit
import CryptoKit
import ImageIO
import ModoKit

/// An icon set installed on disk.
nonisolated struct InstalledIconSet: Identifiable, Equatable {
    let manifest: IconSetManifest
    let directory: URL
    var id: String { manifest.id }
}

/// Installs, lists and removes icon sets, and downloads them from a catalog.
/// Everything that arrives from the network or from a zip is treated as hostile: sizes are
/// capped, the checksum must match, paths may not leave the set folder, symlinks are refused
/// and every PNG is checked before it is kept.
nonisolated enum IconSetStore {
    nonisolated static var root: URL {
        if Store.isOverridden { return URL(fileURLWithPath: "/tmp/Modo-iconsets-test", isDirectory: true) }
        return URL.applicationSupportDirectory.appending(path: "Modo/IconSets", directoryHint: .isDirectory)
    }

    private static let maxCatalogBytes = 2 * 1024 * 1024
    private static let maxUnpackedBytes = 400 * 1024 * 1024

    // MARK: Listing

    static func installedSets() -> [InstalledIconSet] {
        let fm = FileManager.default
        let dirs = (try? fm.contentsOfDirectory(at: root, includingPropertiesForKeys: nil)) ?? []
        return dirs.compactMap { dir -> InstalledIconSet? in
            guard let data = try? Data(contentsOf: dir.appendingPathComponent("manifest.json")),
                  let manifest = try? IconSetRules.decodeManifest(data),
                  manifest.id == dir.lastPathComponent else { return nil }
            return InstalledIconSet(manifest: manifest, directory: dir)
        }.sorted { $0.manifest.name.localizedStandardCompare($1.manifest.name) == .orderedAscending }
    }

    static func remove(id: String) {
        guard IconSetRules.isValidID(id) else { return }
        try? FileManager.default.removeItem(at: root.appendingPathComponent(id, isDirectory: true))
    }

    /// The set's artwork for `bundleID`, if it has one.
    static func image(for bundleID: String, in set: InstalledIconSet) -> NSImage? {
        guard let entry = set.manifest.icons.first(where: { $0.bundleID == bundleID }) else { return nil }
        // Re-check at read time: the folder is user-writable, the manifest was validated at install.
        guard IconSetRules.isSafeRelativePNGPath(entry.file) else { return nil }
        let url = set.directory.appendingPathComponent(entry.file)
        guard url.standardizedFileURL.path.hasPrefix(set.directory.standardizedFileURL.path + "/") else { return nil }
        return NSImage(contentsOf: url)
    }

    // MARK: Catalog

    static func fetchCatalog(from url: URL) async throws -> IconCatalog {
        let data = try await loadData(from: url, maxBytes: maxCatalogBytes)
        return try IconSetRules.decodeCatalog(data, allowFileURLs: url.isFileURL)
    }

    /// Downloads, verifies and installs one catalog entry.
    static func download(_ entry: IconCatalog.Entry) async throws -> InstalledIconSet {
        guard let url = URL(string: entry.url) else { throw IconSetError.insecureURL(entry.url) }
        let data = try await loadData(from: url, maxBytes: min(entry.sizeBytes, IconSetRules.maxZipBytes) + 1024)
        let installed = try await Task.detached { try install(zipData: data, expectedSHA256: entry.sha256) }.value
        guard installed.id == entry.id else {
            remove(id: installed.id)
            throw IconSetError.invalidID(installed.id)
        }
        return installed
    }

    private static func loadData(from url: URL, maxBytes: Int) async throws -> Data {
        guard let scheme = url.scheme?.lowercased(), scheme == "https" || scheme == "file" else {
            throw IconSetError.insecureURL(url.absoluteString)
        }
        if url.isFileURL {
            let size = (try? url.resourceValues(forKeys: [.fileSizeKey]).fileSize) ?? 0
            guard size <= maxBytes else { throw IconSetError.tooLarge(size) }
            return try Data(contentsOf: url)
        }
        let (tempFile, response) = try await URLSession.shared.download(from: url)
        defer { try? FileManager.default.removeItem(at: tempFile) }
        if let http = response as? HTTPURLResponse, !(200..<300).contains(http.statusCode) {
            throw URLError(.badServerResponse)
        }
        let size = (try? tempFile.resourceValues(forKeys: [.fileSizeKey]).fileSize) ?? 0
        guard size <= maxBytes else { throw IconSetError.tooLarge(size) }
        return try Data(contentsOf: tempFile)
    }

    // MARK: Installing

    /// Verifies and unpacks a set zip into the store. Runs off the main thread.
    nonisolated static func install(zipData: Data, expectedSHA256: String?) throws -> InstalledIconSet {
        guard zipData.count <= IconSetRules.maxZipBytes else { throw IconSetError.tooLarge(zipData.count) }
        if let expected = expectedSHA256, IconSetRules.sha256Hex(zipData) != expected.lowercased() {
            throw IconSetError.checksumMismatch
        }
        try validateZipCentralDirectory(zipData)

        let fm = FileManager.default
        let work = fm.temporaryDirectory.appendingPathComponent("modo-iconset-\(UUID().uuidString)", isDirectory: true)
        let unpacked = work.appendingPathComponent("unpacked", isDirectory: true)
        try fm.createDirectory(at: unpacked, withIntermediateDirectories: true)
        defer { try? fm.removeItem(at: work) }

        let zipFile = work.appendingPathComponent("set.zip")
        try zipData.write(to: zipFile)
        let ditto = Process()
        ditto.executableURL = URL(fileURLWithPath: "/usr/bin/ditto")
        ditto.arguments = ["-x", "-k", zipFile.path, unpacked.path]
        ditto.standardError = FileHandle.nullDevice
        try ditto.run()
        ditto.waitUntilExit()
        guard ditto.terminationStatus == 0 else { throw IconSetError.notAnIconSet }

        try checkUnpackedTree(unpacked)

        let manifestURL = unpacked.appendingPathComponent("manifest.json")
        guard let manifestSize = try? manifestURL.resourceValues(forKeys: [.fileSizeKey]).fileSize,
              manifestSize <= 2 * 1024 * 1024,
              let manifestData = try? Data(contentsOf: manifestURL) else { throw IconSetError.notAnIconSet }
        let manifest = try IconSetRules.decodeManifest(manifestData)

        for entry in manifest.icons {
            let png = unpacked.appendingPathComponent(entry.file)
            try checkPNG(png, relativePath: entry.file)
        }

        // Keep only the manifest and the PNGs it lists.
        let finalDir = root.appendingPathComponent(manifest.id, isDirectory: true)
        let staging = work.appendingPathComponent("final", isDirectory: true)
        try fm.createDirectory(at: staging, withIntermediateDirectories: true)
        try fm.copyItem(at: manifestURL, to: staging.appendingPathComponent("manifest.json"))
        for entry in manifest.icons {
            let dest = staging.appendingPathComponent(entry.file)
            try fm.createDirectory(at: dest.deletingLastPathComponent(), withIntermediateDirectories: true)
            try fm.copyItem(at: unpacked.appendingPathComponent(entry.file), to: dest)
        }
        try fm.createDirectory(at: root, withIntermediateDirectories: true)
        try? fm.removeItem(at: finalDir)
        try fm.moveItem(at: staging, to: finalDir)
        return InstalledIconSet(manifest: manifest, directory: finalDir)
    }

    /// Inspects the ZIP central directory in memory BEFORE invoking `ditto` so zip-slip paths,
    /// symlinks, and zip-bombs are rejected before touching the filesystem.
    private nonisolated static func validateZipCentralDirectory(_ data: Data) throws {
        guard data.count >= 22 else { throw IconSetError.notAnIconSet }
        let bytes = [UInt8](data)
        func u16(_ idx: Int) -> Int { Int(bytes[idx]) | (Int(bytes[idx + 1]) << 8) }
        func u32(_ idx: Int) -> Int64 {
            Int64(bytes[idx]) | (Int64(bytes[idx + 1]) << 8) | (Int64(bytes[idx + 2]) << 16) | (Int64(bytes[idx + 3]) << 24)
        }

        // Locate End of Central Directory signature 0x06054b50 within last 65557 bytes
        let minEOCD = max(0, bytes.count - 65557)
        var eocdOffset: Int?
        for i in stride(from: bytes.count - 22, through: minEOCD, by: -1) {
            if bytes[i] == 0x50, bytes[i + 1] == 0x4b, bytes[i + 2] == 0x05, bytes[i + 3] == 0x06 {
                eocdOffset = i
                break
            }
        }
        guard let eocd = eocdOffset else { throw IconSetError.notAnIconSet }
        let entryCount = u16(eocd + 10)
        let cdSize = Int(u32(eocd + 12))
        let cdOffset = Int(u32(eocd + 16))
        guard entryCount <= IconSetRules.maxIconsPerSet + 32 else { throw IconSetError.tooManyIcons(entryCount) }
        guard cdOffset >= 0, cdSize >= 0, cdOffset + cdSize <= bytes.count else { throw IconSetError.notAnIconSet }

        var pos = cdOffset
        var totalUncompressed: Int64 = 0
        for _ in 0..<entryCount {
            guard pos + 46 <= bytes.count,
                  bytes[pos] == 0x50, bytes[pos + 1] == 0x4b, bytes[pos + 2] == 0x01, bytes[pos + 3] == 0x02 else {
                throw IconSetError.notAnIconSet
            }
            let uncompressedSize = u32(pos + 24)
            if uncompressedSize == 0xFFFFFFFF {
                throw IconSetError.tooLarge(maxUnpackedBytes + 1)
            }
            let nameLen = u16(pos + 28)
            let extraLen = u16(pos + 30)
            let commentLen = u16(pos + 32)
            let externalAttr = UInt32(truncatingIfNeeded: u32(pos + 38))
            let unixMode = (externalAttr >> 16) & 0o170000
            if unixMode == 0o120000 {
                throw IconSetError.unsafePath("symlink")
            }

            let nameStart = pos + 46
            let nameEnd = nameStart + nameLen
            guard nameEnd + extraLen + commentLen <= bytes.count else { throw IconSetError.notAnIconSet }
            let rawName = String(decoding: bytes[nameStart..<nameEnd], as: UTF8.self)
            if rawName.hasPrefix("/") || rawName.contains("\\") || rawName.contains("\0") {
                throw IconSetError.unsafePath(rawName)
            }
            let segments = rawName.split(separator: "/", omittingEmptySubsequences: false)
            if segments.contains("..") {
                throw IconSetError.unsafePath(rawName)
            }

            totalUncompressed += uncompressedSize
            if totalUncompressed > Int64(maxUnpackedBytes) {
                throw IconSetError.tooLarge(Int(min(totalUncompressed, Int64(Int.max))))
            }
            pos = nameEnd + extraLen + commentLen
        }
    }

    /// Refuses symlinks, anything outside the folder, too many files and zip bombs.
    private nonisolated static func checkUnpackedTree(_ dir: URL) throws {
        let fm = FileManager.default
        let base = dir.standardizedFileURL.path + "/"
        guard let walker = fm.enumerator(at: dir, includingPropertiesForKeys: [.isSymbolicLinkKey, .isRegularFileKey, .fileSizeKey]) else {
            throw IconSetError.notAnIconSet
        }
        var files = 0
        var total = 0
        for case let url as URL in walker {
            let values = try url.resourceValues(forKeys: [.isSymbolicLinkKey, .isRegularFileKey, .fileSizeKey])
            guard values.isSymbolicLink != true,
                  url.standardizedFileURL.path.hasPrefix(base) else { throw IconSetError.unsafePath(url.lastPathComponent) }
            if values.isRegularFile == true {
                files += 1
                total += values.fileSize ?? 0
                guard files <= IconSetRules.maxIconsPerSet + 5 else { throw IconSetError.tooManyIcons(files) }
                guard total <= 400 * 1024 * 1024 else { throw IconSetError.tooLarge(total) }
            }
        }
    }

    /// A listed PNG must exist, stay small, really be a PNG and have sane dimensions.
    private nonisolated static func checkPNG(_ url: URL, relativePath: String) throws {
        guard let size = try? url.resourceValues(forKeys: [.fileSizeKey]).fileSize,
              size > 0, size <= IconSetRules.maxPNGBytes,
              let source = CGImageSourceCreateWithURL(url as CFURL, nil),
              CGImageSourceGetType(source) as String? == "public.png",
              let props = CGImageSourceCopyPropertiesAtIndex(source, 0, nil) as? [CFString: Any],
              let w = props[kCGImagePropertyPixelWidth] as? Int,
              let h = props[kCGImagePropertyPixelHeight] as? Int,
              (1...IconSetRules.maxPixels).contains(w), (1...IconSetRules.maxPixels).contains(h) else {
            throw IconSetError.unsafePath(relativePath)
        }
    }
}
