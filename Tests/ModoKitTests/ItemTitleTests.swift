import Foundation
import Testing
@testable import ModoKit

@Test("Clean app titles", arguments: [
    ("Safari.app", "Safari"),
    ("  Google   Chrome.app \n", "Google Chrome"),
    ("Visual Studio Code", "Visual Studio Code"),
    ("", "Untitled App"),
    ("   ", "Untitled App"),
])
func cleans(_ input: String, expected: String) {
    #expect(ItemTitle.clean(input) == expected)
}

@Test("Material You pastel color blending and hex parsing")
func colorMath() {
    let googleBlue = MaterialRGB(hex: 0x1A73E8)
    #expect(googleBlue.r == 0x1A)
    #expect(googleBlue.g == 0x73)
    #expect(googleBlue.b == 0xE8)
    #expect(googleBlue.hexString == "#1A73E8")

    let pastel = googleBlue.materialPastel(whiteMix: 0.78)
    #expect(pastel.r > googleBlue.r)
    #expect(pastel.g > googleBlue.g)
    #expect(pastel.b > googleBlue.b)

    let parsed = MaterialRGB.parseHex("#4285F4")
    #expect(parsed == MaterialRGB(hex: 0x4285F4))
    #expect(MaterialRGB.parseHex("invalid") == nil)
}

@Test("RoundIconStyle supports >100% scale and clamps scale, offsets, and shadow bounds")
func styleClamping() {
    let zoomed = RoundIconStyle(iconScale: 1.85, offsetX: 0.4, offsetY: -0.35)
    #expect(zoomed.iconScale == 1.85)
    #expect(zoomed.offsetX == 0.4)
    #expect(zoomed.offsetY == -0.35)

    var clamped = RoundIconStyle(iconScale: 3.5, offsetX: -2.0, offsetY: 2.0, shadowIntensity: -0.2)
    #expect(clamped.iconScale == 2.5)
    #expect(clamped.offsetX == -1.0)
    #expect(clamped.offsetY == 1.0)
    #expect(clamped.shadowIntensity == 0.0)

    clamped.resetPosition()
    #expect(clamped.offsetX == 0.0)
    #expect(clamped.offsetY == 0.0)
}

@Test("ICO binary header packs multi-size PNG entries")
func icoBinaryFormat() {
    let fakePNG16 = Data([0x89, 0x50, 0x4E, 0x47])
    let fakePNG256 = Data([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A])
    let ico = ICOBuilder.build(entries: [
        .init(size: 16, pngData: fakePNG16),
        .init(size: 256, pngData: fakePNG256),
    ])
    // 6-byte header + 2*16 directory entries + 4 + 8 bytes payload = 50 bytes
    #expect(ico.count == 50)
    #expect(ico[2] == 1) // type = 1 (ICO)
    #expect(ico[4] == 2) // count = 2
    #expect(ico[6] == 16) // first entry width = 16
    #expect(ico[22] == 0) // second entry width = 0 (256px)
}

@Test("IconShape and IconPack catalogs verification")
func iconPacksCatalog() {
    #expect(IconShape.allCases.count == 5)
    #expect(IconPack.allPacks.count == 7)

    let pixel = IconPack.find(id: "pixel_pure")
    #expect(pixel.style.shape == .circle)
    #expect(pixel.style.backgroundStyle == .googleWhite)

    let atoma = IconPack.find(id: "atoma_mono")
    #expect(atoma.style.shape == .squircle)
    #expect(atoma.style.monochromeTint == true)

    let cyber = IconPack.find(id: "cyber_hex")
    #expect(cyber.style.shape == .hexagon)

    let squircle = IconPack.find(id: "squircle_studio")
    #expect(squircle.style.shape == .squircle)

    let fallback = IconPack.find(id: "nonexistent_pack")
    #expect(fallback.id == "pixel_pure")
}

@Test("CustomIconPack round-trips through JSON and falls back to a default name")
func customPackCodable() throws {
    let style = RoundIconStyle(backgroundStyle: .darkMaterial, shape: .hexagon, iconScale: 0.9)
    let pack = CustomIconPack(name: "  Night  ", style: style)
    #expect(pack.name == "Night")
    #expect(pack.id.hasPrefix("custom_"))

    let data = try JSONEncoder().encode([pack])
    let decoded = try JSONDecoder().decode([CustomIconPack].self, from: data)
    #expect(decoded == [pack])
    #expect(decoded[0].style.shape == .hexagon)

    #expect(CustomIconPack(name: "   ", style: style).name == "My Pack")
}

@Test("ReapplyPolicy re-styles updated apps and apps whose icon file vanished")
func reapplyPolicy() {
    let record = AppliedIconRecord(version: "1.0 (10)", hadIconFile: true)
    #expect(!ReapplyPolicy.needsReapply(record: nil, currentVersion: "1.0 (10)", hasIconFile: false))
    #expect(!ReapplyPolicy.needsReapply(record: record, currentVersion: "1.0 (10)", hasIconFile: true))
    #expect(ReapplyPolicy.needsReapply(record: record, currentVersion: "1.1 (11)", hasIconFile: true))
    #expect(ReapplyPolicy.needsReapply(record: record, currentVersion: "1.0 (10)", hasIconFile: false))
    // Icons applied through the read-only alias never had a file in the bundle: only a version change counts.
    let aliasRecord = AppliedIconRecord(version: "1.0 (10)", hadIconFile: false)
    #expect(!ReapplyPolicy.needsReapply(record: aliasRecord, currentVersion: "1.0 (10)", hasIconFile: false))
    #expect(ReapplyPolicy.needsReapply(record: aliasRecord, currentVersion: "2.0 (20)", hasIconFile: false))
}
