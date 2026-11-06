import AppKit
import Foundation
@testable import ModoKit
import Testing

@Suite("Developer Export Engine Tests")
struct DeveloperExportTests {

    @Test("Contents.json generator produces valid Asset Catalog structure")
    func contentsJSONValid() throws {
        let jsonStr = DeveloperExportEngine.generateAppIconSetContentsJSON()
        #expect(jsonStr.contains("\"author\" : \"Modo by Atoma\""))
        #expect(jsonStr.contains("\"images\" : ["))
        #expect(jsonStr.contains("\"idiom\" : \"mac\""))
        #expect(jsonStr.contains("\"platform\" : \"ios\""))

        // Must parse as valid JSON
        let data = try #require(jsonStr.data(using: .utf8))
        let obj = try JSONSerialization.jsonObject(with: data) as? [String: Any]
        #expect(obj != nil)
        let images = obj?["images"] as? [[String: Any]]
        #expect(images?.count == DeveloperExportEngine.standardXcodeSpecs.count)
    }

    @Test("Web manifest generator produces valid site.webmanifest")
    func webManifestValid() throws {
        let jsonStr = DeveloperExportEngine.generateWebManifest(name: "Test App")
        #expect(jsonStr.contains("\"name\" : \"Test App\""))
        #expect(jsonStr.contains("android-chrome-192x192.png"))
        #expect(jsonStr.contains("android-chrome-512x512.png"))

        let data = try #require(jsonStr.data(using: .utf8))
        let obj = try JSONSerialization.jsonObject(with: data) as? [String: Any]
        #expect(obj != nil)
    }

    @Test("HTML snippet contains apple-touch-icon and webmanifest tags")
    func htmlSnippetValid() {
        let snippet = DeveloperExportEngine.generateHTMLFaviconSnippet()
        #expect(snippet.contains("apple-touch-icon.png"))
        #expect(snippet.contains("site.webmanifest"))
        #expect(snippet.contains("favicon-32x32.png"))
    }

    @Test("Export AppIcon.appiconset writes all required PNG sizes and Contents.json")
    func exportAppIconSetWritesFiles() throws {
        let tempDir = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        try FileManager.default.createDirectory(at: tempDir, withIntermediateDirectories: true)
        defer { try? FileManager.default.removeItem(at: tempDir) }

        // Create sample 256x256 test image
        let img = NSImage(size: NSSize(width: 256, height: 256))
        img.lockFocus()
        NSColor.systemBlue.setFill()
        NSRect(x: 0, y: 0, width: 256, height: 256).fill()
        img.unlockFocus()

        let setURL = try DeveloperExportEngine.exportAppIconSet(from: img, appName: "Demo App", destination: tempDir)
        #expect(FileManager.default.fileExists(atPath: setURL.path))
        #expect(FileManager.default.fileExists(atPath: setURL.appendingPathComponent("Contents.json").path))
        #expect(FileManager.default.fileExists(atPath: setURL.appendingPathComponent("icon_16x16.png").path))
        #expect(FileManager.default.fileExists(atPath: setURL.appendingPathComponent("icon_512x512@2x.png").path))
        #expect(FileManager.default.fileExists(atPath: setURL.appendingPathComponent("icon_1024x1024.png").path))
    }

    @Test("Export Web Favicon Kit writes multi-size favicon.ico, PNGs, and webmanifest")
    func exportWebFaviconKitWritesFiles() throws {
        let tempDir = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        try FileManager.default.createDirectory(at: tempDir, withIntermediateDirectories: true)
        defer { try? FileManager.default.removeItem(at: tempDir) }

        let img = NSImage(size: NSSize(width: 256, height: 256))
        img.lockFocus()
        NSColor.systemPurple.setFill()
        NSRect(x: 0, y: 0, width: 256, height: 256).fill()
        img.unlockFocus()

        let kitURL = try DeveloperExportEngine.exportWebFaviconKit(from: img, appName: "Web Project", destination: tempDir)
        #expect(FileManager.default.fileExists(atPath: kitURL.path))
        #expect(FileManager.default.fileExists(atPath: kitURL.appendingPathComponent("favicon.ico").path))
        #expect(FileManager.default.fileExists(atPath: kitURL.appendingPathComponent("apple-touch-icon.png").path))
        #expect(FileManager.default.fileExists(atPath: kitURL.appendingPathComponent("android-chrome-192x192.png").path))
        #expect(FileManager.default.fileExists(atPath: kitURL.appendingPathComponent("android-chrome-512x512.png").path))
        #expect(FileManager.default.fileExists(atPath: kitURL.appendingPathComponent("site.webmanifest").path))
        #expect(FileManager.default.fileExists(atPath: kitURL.appendingPathComponent("html_snippet.html").path))
    }
}
