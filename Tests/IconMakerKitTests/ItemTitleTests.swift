import Foundation
import Testing
@testable import IconMakerKit

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
