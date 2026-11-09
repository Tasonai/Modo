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

    @Test("SurfaceMaterial defines all 8 tactile textures")
    func surfaceMaterialCases() {
        #expect(SurfaceMaterial.allCases.count == 8)
        let titles = SurfaceMaterial.allCases.map(\.title)
        #expect(titles.contains("Smooth Flat"))
        #expect(titles.contains("Frosted Glass"))
        #expect(titles.contains("Brushed Titanium"))
        #expect(titles.contains("Neumorphic Emboss"))
        #expect(titles.contains("Prism Chromatic"))
        #expect(titles.contains("Liquid Chrome"))
        #expect(titles.contains("Matte Ceramic"))
        #expect(titles.contains("Holographic Foil"))
    }

    @Test("GlyphStyleMode defines all 6 artwork rendering styles")
    func glyphStyleModeCases() {
        #expect(GlyphStyleMode.allCases.count == 6)
        let titles = GlyphStyleMode.allCases.map(\.title)
        #expect(titles.contains("Original"))
        #expect(titles.contains("Silhouette"))
        #expect(titles.contains("Embossed 3D"))
        #expect(titles.contains("Outline"))
        #expect(titles.contains("Duotone"))
        #expect(titles.contains("Neon Glow"))
    }

    @Test("ColorPaletteEngine extracts deterministic 5-color BrandPalette")
    func colorPaletteEngineExtraction() {
        let samples: [ColorPaletteEngine.RGBA] = [
            .init(r: 26, g: 115, b: 232),
            .init(r: 26, g: 115, b: 232),
            .init(r: 66, g: 133, b: 244),
            .init(r: 234, g: 67, b: 53)
        ]
        let palette = ColorPaletteEngine.extractPalette(from: samples, fallbackHex: 0x1A73E8)
        #expect(palette.primary > 0)
        #expect(palette.secondary > 0)
        #expect(palette.backgroundTint > 0)
        #expect(palette.deepTone > 0)
        #expect(palette.highlight == 0xFFFFFF)
    }

    @Test("RoundIconStyle encodes and decodes surfaceMaterial and glyphStyle")
    func roundIconStyleMaterialPersistence() throws {
        var style = RoundIconStyle(backgroundStyle: .wallpaperDynamic, shape: .squircle)
        style.surfaceMaterial = .liquidChrome
        style.glyphStyle = .duotone

        let data = try JSONEncoder().encode(style)
        let decoded = try JSONDecoder().decode(RoundIconStyle.self, from: data)

        #expect(decoded.backgroundStyle == .wallpaperDynamic)
        #expect(decoded.surfaceMaterial == .liquidChrome)
        #expect(decoded.glyphStyle == .duotone)
    }
}
