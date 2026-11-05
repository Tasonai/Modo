import Foundation

// MARK: - Batch Candidate Model

public struct BatchCandidate: Identifiable, Hashable, Sendable {
    public let id: UUID
    public let filename: String
    public let cleanTitle: String
    public let fileURL: URL
    public let isVector: Bool
    public let fileSize: Int64

    public init(fileURL: URL) {
        self.id = UUID()
        self.fileURL = fileURL
        self.filename = fileURL.lastPathComponent

        let baseName = fileURL.deletingPathExtension().lastPathComponent
        // Clean title: replace dashes/underscores with spaces and capitalize if needed
        var normalized = baseName
            .replacingOccurrences(of: "_", with: " ")
            .replacingOccurrences(of: "-", with: " ")
            .replacingOccurrences(of: "  ", with: " ")
        if normalized == normalized.lowercased() {
            normalized = normalized.capitalized
        }
        self.cleanTitle = ItemTitle.clean(normalized)

        let ext = fileURL.pathExtension.lowercased()
        self.isVector = (ext == "svg")
        let attr = try? FileManager.default.attributesOfItem(atPath: fileURL.path)
        self.fileSize = (attr?[.size] as? Int64) ?? 0
    }
}

// MARK: - Batch Export Options

public struct BatchExportOptions: Sendable {
    public var packName: String
    public var author: String
    public var style: RoundIconStyle
    public var exportModoPack: Bool
    public var exportICNS: Bool
    public var exportICO: Bool
    public var exportPNG: Bool
    public var exportZip: Bool
    public var pngSizes: [Int]
    public var outputDirectory: URL

    public init(
        packName: String = "Custom Pack",
        author: String = "Atoma",
        style: RoundIconStyle = RoundIconStyle(),
        exportModoPack: Bool = true,
        exportICNS: Bool = true,
        exportICO: Bool = true,
        exportPNG: Bool = true,
        exportZip: Bool = true,
        pngSizes: [Int] = [1024],
        outputDirectory: URL = FileManager.default.temporaryDirectory
    ) {
        self.packName = packName.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty ? "Custom Pack" : packName
        self.author = author.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty ? "Atoma" : author
        self.style = style
        self.exportModoPack = exportModoPack
        self.exportICNS = exportICNS
        self.exportICO = exportICO
        self.exportPNG = exportPNG
        self.exportZip = exportZip
        self.pngSizes = pngSizes.isEmpty ? [1024] : pngSizes
        self.outputDirectory = outputDirectory
    }
}

// MARK: - Batch Scanner Engine

public enum BatchIconEngine {
    public static let supportedExtensions: Set<String> = [
        "png", "svg", "jpg", "jpeg", "webp", "icns", "ico", "bmp", "tiff", "tif"
    ]

    /// Scans a directory URL for candidate image and vector assets.
    public static func scanDirectory(at folderURL: URL, recursive: Bool = false) -> [BatchCandidate] {
        var results: [BatchCandidate] = []
        let fm = FileManager.default

        let keys: [URLResourceKey] = [.isRegularFileKey, .isDirectoryKey]
        let options: FileManager.DirectoryEnumerationOptions = recursive ? [.skipsHiddenFiles] : [.skipsHiddenFiles, .skipsSubdirectoryDescendants]

        guard let enumerator = fm.enumerator(at: folderURL, includingPropertiesForKeys: keys, options: options) else {
            return []
        }

        for case let fileURL as URL in enumerator {
            let ext = fileURL.pathExtension.lowercased()
            if supportedExtensions.contains(ext) {
                // Ignore macOS resource fork files (._*)
                if !fileURL.lastPathComponent.hasPrefix("._") {
                    results.append(BatchCandidate(fileURL: fileURL))
                }
            }
        }

        return results.sorted { $0.cleanTitle.localizedCaseInsensitiveCompare($1.cleanTitle) == .orderedAscending }
    }

    /// Formats a clean pack filename from pack name.
    public static func safePackName(_ name: String) -> String {
        name.components(separatedBy: CharacterSet.alphanumerics.union(CharacterSet(charactersIn: " -_")).inverted)
            .joined()
            .trimmingCharacters(in: .whitespacesAndNewlines)
            .replacingOccurrences(of: " ", with: "-")
    }
}
