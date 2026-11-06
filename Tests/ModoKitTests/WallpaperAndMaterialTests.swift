import AppKit
import Foundation
@testable import ModoKit
import Testing

@Suite("Wallpaper Dynamic & Surface Materials Tests")
struct WallpaperAndMaterialTests {
    @Test("WallpaperPaletteEngine extracts valid palette from image")
    func wallpaperPaletteExtraction() {
        let sampleImage = NSImage(size: NSSize(width: 128, height: 128))
        sampleImage.lockFocus()
        NSColor(srgbRed: 0.2, green: 0.6, blue: 0.9, alpha: 1.0).setFill()
        NSBezierPath(rect: NSRect(x: 0, y: 0, width: 128, height: 128)).fill()
        sampleImage.unlockFocus()

        let pal = WallpaperPaletteEngine.extractPalette(from: sampleImage)
        #expect(pal.primaryHex > 0)
        #expect(pal.pastelHex > 0)
        #expect(pal.darkHex > 0)
        #expect(pal.accentHex > 0)
    }

    @Test("WallpaperPaletteEngine provides safe default palette")
    func wallpaperDefaultPalette() {
        let def = WallpaperPaletteEngine.defaultPalette()
        #expect(def.primaryHex == 0x2A82E4)
        #expect(def.pastelHex == 0xDCEDFE)
    }

    @Test("SurfaceMaterial defines all 5 tactile textures")
    func surfaceMaterialCases() {
        #expect(SurfaceMaterial.allCases.count == 5)
        let titles = SurfaceMaterial.allCases.map(\.title)
        #expect(titles.contains("Smooth Flat"))
        #expect(titles.contains("Frosted Glass"))
        #expect(titles.contains("Brushed Titanium"))
        #expect(titles.contains("Neumorphic Emboss"))
        #expect(titles.contains("Prism Chromatic"))
    }

    @Test("RoundIconStyle encodes and decodes surfaceMaterial")
    func roundIconStyleMaterialPersistence() throws {
        var style = RoundIconStyle(backgroundStyle: .wallpaperDynamic, shape: .squircle)
        style.surfaceMaterial = .frostedGlass

        let data = try JSONEncoder().encode(style)
        let decoded = try JSONDecoder().decode(RoundIconStyle.self, from: data)

        #expect(decoded.backgroundStyle == .wallpaperDynamic)
        #expect(decoded.surfaceMaterial == .frostedGlass)
    }
}
