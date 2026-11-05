import AppKit
import Foundation
import UniformTypeIdentifiers

/// Utilities for importing, validating, and rasterizing custom app icons and SVG vectors.
enum CustomArtworkHelper {
    static let supportedExtensions: Set<String> = [
        "svg", "png", "jpg", "jpeg", "webp", "tiff", "tif", "icns", "ico", "bmp", "pdf", "app"
    ]

    static let allowedContentTypes: [UTType] = [
        .svg, .png, .jpeg, .webP, .tiff, .icns, .ico, .pdf, .application
    ]

    /// Checks if binary data represents an SVG vector image (UTF-8 text containing `<svg`).
    static func isSVGData(_ data: Data) -> Bool {
        guard let text = String(data: data.prefix(2048), encoding: .utf8)?.lowercased() else { return false }
        return text.contains("<svg")
    }

    /// Loads artwork data from a local file URL or application bundle.
    static func loadArtworkData(from url: URL) -> Data? {
        let ext = url.pathExtension.lowercased()

        // Handle macOS .app bundle
        if ext == "app" || (url.hasDirectoryPath && url.path.hasSuffix(".app")) {
            let img = NSWorkspace.shared.icon(forFile: url.path)
            img.size = NSSize(width: 512, height: 512)
            guard let tiff = img.tiffRepresentation,
                  let rep = NSBitmapImageRep(data: tiff),
                  let png = rep.representation(using: .png, properties: [:]) else { return nil }
            return png
        }

        guard let data = try? Data(contentsOf: url) else { return nil }

        // If it's SVG, verify NSImage can parse it or it has valid svg syntax
        if ext == "svg" || isSVGData(data) {
            guard NSImage(data: data) != nil || isSVGData(data) else { return nil }
            return data
        }

        // Other image types: ensure NSImage can read it
        guard NSImage(data: data) != nil else { return nil }
        return data
    }

    /// Loads artwork data from the system pasteboard (SVG text, clipboard image, or copied file URL).
    static func loadArtworkData(from pasteboard: NSPasteboard = .general) -> Data? {
        // 1. Text containing raw SVG code (common when copying from Figma / browser / code editor)
        if let str = pasteboard.string(forType: .string) {
            let trimmed = str.trimmingCharacters(in: .whitespacesAndNewlines)
            if trimmed.lowercased().contains("<svg") && trimmed.lowercased().contains("</svg>") {
                return trimmed.data(using: .utf8)
            }
        }

        // 2. Copied file URL(s)
        if let urls = pasteboard.readObjects(forClasses: [NSURL.self], options: nil) as? [URL],
           let firstURL = urls.first {
            if let data = loadArtworkData(from: firstURL) {
                return data
            }
        }

        // 3. Direct clipboard image bitmap (e.g. screenshot or Cmd+C on image)
        if let img = NSImage(pasteboard: pasteboard) {
            guard let tiff = img.tiffRepresentation,
                  let rep = NSBitmapImageRep(data: tiff),
                  let png = rep.representation(using: .png, properties: [:]) else { return nil }
            return png
        }

        return nil
    }

    /// Checks if the clipboard currently has pasteable artwork (SVG, image, or image file).
    static func canPasteArtwork(from pasteboard: NSPasteboard = .general) -> Bool {
        if let str = pasteboard.string(forType: .string) {
            let trimmed = str.trimmingCharacters(in: .whitespacesAndNewlines)
            if trimmed.lowercased().contains("<svg") && trimmed.lowercased().contains("</svg>") {
                return true
            }
        }
        if let urls = pasteboard.readObjects(forClasses: [NSURL.self], options: nil) as? [URL],
           let firstURL = urls.first {
            let ext = firstURL.pathExtension.lowercased()
            if supportedExtensions.contains(ext) { return true }
        }
        return NSImage.canInit(with: pasteboard)
    }

    /// Prompts user to select an icon, vector SVG, or app bundle from Finder.
    static func promptUserForArtwork() -> Data? {
        let panel = NSOpenPanel()
        panel.canChooseFiles = true
        panel.canChooseDirectories = false
        panel.allowsMultipleSelection = false
        panel.allowedContentTypes = allowedContentTypes
        panel.prompt = "Choose Icon"
        panel.message = "Select an SVG vector, image (PNG, JPEG, ICNS, WebP), or .app bundle"

        guard panel.runModal() == .OK, let url = panel.url else { return nil }
        return loadArtworkData(from: url)
    }
}
