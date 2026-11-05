import Foundation
import Testing
@testable import ModoKit

@Test("FolderColorPreset defines valid colors and accents")
func folderColors() {
    for preset in FolderColorPreset.allCases {
        #expect(preset.hex > 0)
        #expect(preset.accentHex > 0)
        #expect(preset.hex != preset.accentHex)
        #expect(!preset.rawValue.isEmpty)
    }
}

@Test("Folder vector SVG generator produces valid SVG markup")
func folderSVGGeneration() {
    let style = FolderStyle(
        colorHex: FolderColorPreset.purple.hex,
        accentHex: FolderColorPreset.purple.accentHex,
        embossStyle: .etched,
        badgeSymbolName: "terminal.fill",
        badgeScale: 0.4
    )

    let svg = FolderVectorGenerator.generateFolderSVG(
        style: style,
        size: 512,
        centerGlyphSVG: "<circle cx=\"256\" cy=\"256\" r=\"100\" fill=\"white\"/>"
    )

    #expect(svg.contains("<svg"))
    #expect(svg.contains("</svg>"))
    #expect(svg.contains("folderFrontGrad"))
    #expect(svg.contains("folderBackGrad"))
    #expect(svg.contains("256"))
}

@Test("BatchIconEngine scans directory and identifies candidate assets")
func batchScanner() throws {
    let tempDir = URL(fileURLWithPath: NSTemporaryDirectory()).appending(path: "ModoBatchTest_\(UUID().uuidString)")
    try FileManager.default.createDirectory(at: tempDir, withIntermediateDirectories: true)
    defer { try? FileManager.default.removeItem(at: tempDir) }

    // Create mock files
    try "dummy png".write(to: tempDir.appending(path: "google-chrome.png"), atomically: true, encoding: .utf8)
    try "dummy svg".write(to: tempDir.appending(path: "visual_studio_code.svg"), atomically: true, encoding: .utf8)
    try "dummy txt".write(to: tempDir.appending(path: "readme.txt"), atomically: true, encoding: .utf8)
    try "dummy hidden".write(to: tempDir.appending(path: "._hidden.png"), atomically: true, encoding: .utf8)

    let candidates = BatchIconEngine.scanDirectory(at: tempDir)
    #expect(candidates.count == 2)

    let titles = Set(candidates.map(\.cleanTitle))
    #expect(titles.contains("Google Chrome"))
    #expect(titles.contains("Visual Studio Code"))

    let vectorItem = candidates.first { $0.filename.hasSuffix(".svg") }
    #expect(vectorItem?.isVector == true)

    let rasterItem = candidates.first { $0.filename.hasSuffix(".png") }
    #expect(rasterItem?.isVector == false)

    #expect(BatchIconEngine.safePackName("My Awesome Icons! #1") == "My-Awesome-Icons-1")
}
