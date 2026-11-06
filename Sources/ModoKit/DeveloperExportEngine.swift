import AppKit
import CoreGraphics
import Foundation

// MARK: - Developer Export Engine (Xcode .appiconset & Web Favicon Bundle)

public enum DeveloperExportEngine {

    public struct AssetSpec: Sendable {
        public let size: String
        public let idiom: String
        public let filename: String
        public let scale: String
        public let px: Int
        public var platform: String?

        public init(size: String, idiom: String, filename: String, scale: String, px: Int, platform: String? = nil) {
            self.size = size
            self.idiom = idiom
            self.filename = filename
            self.scale = scale
            self.px = px
            self.platform = platform
        }
    }

    public static let standardXcodeSpecs: [AssetSpec] = [
        .init(size: "16x16", idiom: "mac", filename: "icon_16x16.png", scale: "1x", px: 16),
        .init(size: "16x16", idiom: "mac", filename: "icon_16x16@2x.png", scale: "2x", px: 32),
        .init(size: "32x32", idiom: "mac", filename: "icon_32x32.png", scale: "1x", px: 32),
        .init(size: "32x32", idiom: "mac", filename: "icon_32x32@2x.png", scale: "2x", px: 64),
        .init(size: "128x128", idiom: "mac", filename: "icon_128x128.png", scale: "1x", px: 128),
        .init(size: "128x128", idiom: "mac", filename: "icon_128x128@2x.png", scale: "2x", px: 256),
        .init(size: "256x256", idiom: "mac", filename: "icon_256x256.png", scale: "1x", px: 256),
        .init(size: "256x256", idiom: "mac", filename: "icon_256x256@2x.png", scale: "2x", px: 512),
        .init(size: "512x512", idiom: "mac", filename: "icon_512x512.png", scale: "1x", px: 512),
        .init(size: "512x512", idiom: "mac", filename: "icon_512x512@2x.png", scale: "2x", px: 1024),
        .init(size: "1024x1024", idiom: "universal", filename: "icon_1024x1024.png", scale: "1x", px: 1024, platform: "ios")
    ]

    // MARK: - Bitmap Helpers

    public static func pngData(from image: NSImage, pixelSize: Int) -> Data? {
        var proposed = NSRect(x: 0, y: 0, width: pixelSize, height: pixelSize)
        guard let cg = image.cgImage(forProposedRect: &proposed, context: nil, hints: [
            .interpolation: NSImageInterpolation.high
        ]) else { return nil }

        let rep = NSBitmapImageRep(cgImage: cg)
        rep.size = NSSize(width: pixelSize, height: pixelSize)
        return rep.representation(using: .png, properties: [:])
    }

    // MARK: - Xcode Asset Catalog (.appiconset)

    public static func generateAppIconSetContentsJSON(specs: [AssetSpec] = standardXcodeSpecs) -> String {
        var imagesArray: [[String: String]] = []
        for s in specs {
            var item: [String: String] = [
                "size": s.size,
                "idiom": s.idiom,
                "filename": s.filename,
                "scale": s.scale
            ]
            if let plat = s.platform {
                item["platform"] = plat
            }
            imagesArray.append(item)
        }

        let dict: [String: Any] = [
            "images": imagesArray,
            "info": [
                "version": 1,
                "author": "Modo by Atoma"
            ]
        ]

        if let data = try? JSONSerialization.data(withJSONObject: dict, options: [.prettyPrinted, .sortedKeys]),
           let str = String(data: data, encoding: .utf8) {
            return str
        }

        return "{}"
    }

    @discardableResult
    public static func exportAppIconSet(
        from image: NSImage,
        appName: String,
        destination: URL,
        specs: [AssetSpec] = standardXcodeSpecs
    ) throws -> URL {
        let cleanName = ItemTitle.clean(appName).replacingOccurrences(of: " ", with: "")
        let folderName = "\(cleanName).appiconset"
        let setDir = destination.appendingPathComponent(folderName, isDirectory: true)
        try FileManager.default.createDirectory(at: setDir, withIntermediateDirectories: true)

        for spec in specs {
            let fileURL = setDir.appendingPathComponent(spec.filename)
            if let data = pngData(from: image, pixelSize: spec.px) {
                try data.write(to: fileURL, options: .atomic)
            }
        }

        let contents = generateAppIconSetContentsJSON(specs: specs)
        let contentsURL = setDir.appendingPathComponent("Contents.json")
        try contents.data(using: .utf8)?.write(to: contentsURL, options: .atomic)

        return setDir
    }

    // MARK: - Web Favicon Kit

    public static func generateWebManifest(name: String) -> String {
        let cleanName = ItemTitle.clean(name)
        let dict: [String: Any] = [
            "name": cleanName,
            "short_name": cleanName,
            "icons": [
                [
                    "src": "/android-chrome-192x192.png",
                    "sizes": "192x192",
                    "type": "image/png"
                ],
                [
                    "src": "/android-chrome-512x512.png",
                    "sizes": "512x512",
                    "type": "image/png"
                ]
            ],
            "theme_color": "#ffffff",
            "background_color": "#ffffff",
            "display": "standalone"
        ]

        if let data = try? JSONSerialization.data(withJSONObject: dict, options: [.prettyPrinted, .sortedKeys]),
           let str = String(data: data, encoding: .utf8) {
            return str
        }
        return "{}"
    }

    public static func generateHTMLFaviconSnippet() -> String {
        return """
        <!-- Modo Web Favicon Tags -->
        <link rel="apple-touch-icon" sizes="180x180" href="/apple-touch-icon.png">
        <link rel="icon" type="image/png" sizes="32x32" href="/favicon-32x32.png">
        <link rel="icon" type="image/png" sizes="16x16" href="/favicon-16x16.png">
        <link rel="manifest" href="/site.webmanifest">
        <meta name="theme-color" content="#ffffff">
        """
    }

    @discardableResult
    public static func exportWebFaviconKit(
        from image: NSImage,
        appName: String,
        destination: URL
    ) throws -> URL {
        let cleanName = ItemTitle.clean(appName).replacingOccurrences(of: " ", with: "-").lowercased()
        let folderName = "\(cleanName)-favicons"
        let kitDir = destination.appendingPathComponent(folderName, isDirectory: true)
        try FileManager.default.createDirectory(at: kitDir, withIntermediateDirectories: true)

        // 1. Multi-resolution favicon.ico (16, 32, 48)
        var icoEntries: [ICOBuilder.Entry] = []
        for sz in [16, 32, 48] {
            if let data = pngData(from: image, pixelSize: sz) {
                icoEntries.append(.init(size: sz, pngData: data))
            }
        }
        let icoData = ICOBuilder.build(entries: icoEntries)
        try icoData.write(to: kitDir.appendingPathComponent("favicon.ico"), options: .atomic)

        // 2. Standard Web PNGs
        let pngSpecs: [(String, Int)] = [
            ("favicon-16x16.png", 16),
            ("favicon-32x32.png", 32),
            ("apple-touch-icon.png", 180),
            ("android-chrome-192x192.png", 192),
            ("android-chrome-512x512.png", 512)
        ]
        for (filename, sz) in pngSpecs {
            if let data = pngData(from: image, pixelSize: sz) {
                try data.write(to: kitDir.appendingPathComponent(filename), options: .atomic)
            }
        }

        // 3. site.webmanifest
        let manifest = generateWebManifest(name: appName)
        try manifest.data(using: .utf8)?.write(to: kitDir.appendingPathComponent("site.webmanifest"), options: .atomic)

        // 4. html_snippet.html
        let snippet = generateHTMLFaviconSnippet()
        try snippet.data(using: .utf8)?.write(to: kitDir.appendingPathComponent("html_snippet.html"), options: .atomic)

        return kitDir
    }
}
