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

    // Appearance mode and tinting
    #expect(clamped.appearanceMode == .light)
    #expect(clamped.tintColorHex == 0x007AFF)

    let tintedStyle = RoundIconStyle(appearanceMode: .tinted, tintColorHex: 0x34C759)
    #expect(tintedStyle.appearanceMode == .tinted)
    #expect(tintedStyle.tintColorHex == 0x34C759)

    // Transformations: rotation and mirroring
    let transformedStyle = RoundIconStyle(glyphRotation: 90.0, glyphFlipHorizontal: true, glyphFlipVertical: false)
    #expect(transformedStyle.glyphRotation == 90.0)
    #expect(transformedStyle.glyphFlipHorizontal == true)
    #expect(transformedStyle.glyphFlipVertical == false)

    if let encTrans = try? JSONEncoder().encode(transformedStyle),
       let decTrans = try? JSONDecoder().decode(RoundIconStyle.self, from: encTrans) {
        #expect(decTrans.glyphRotation == 90.0)
        #expect(decTrans.glyphFlipHorizontal == true)
        #expect(decTrans.glyphFlipVertical == false)
    } else {
        #expect(Bool(false), "Failed to encode/decode transformed RoundIconStyle")
    }

    // Round-trip encoding
    if let encoded = try? JSONEncoder().encode(tintedStyle),
       let decoded = try? JSONDecoder().decode(RoundIconStyle.self, from: encoded) {
        #expect(decoded.appearanceMode == .tinted)
        #expect(decoded.tintColorHex == 0x34C759)
    } else {
        #expect(Bool(false), "Failed to encode/decode RoundIconStyle")
    }

    // SystemBackupPoint serialization
    let backup = SystemBackupPoint(name: "Test Point", appPaths: ["/Applications/Safari.app"], packName: "Pixel Pure")
    #expect(backup.appPaths.count == 1)
    if let enc = try? JSONEncoder().encode(backup),
       let dec = try? JSONDecoder().decode(SystemBackupPoint.self, from: enc) {
        #expect(dec.name == "Test Point")
        #expect(dec.appPaths == ["/Applications/Safari.app"])
        #expect(dec.packName == "Pixel Pure")
    } else {
        #expect(Bool(false), "Failed to encode/decode SystemBackupPoint")
    }
}

@Test("Procedural geometric SVG vector generator and AI prompt builder")
func proceduralGlyphs() {
    #expect(GeometricGlyphStyle.allCases.count == 9)
    for style in GeometricGlyphStyle.allCases {
        var variantsSVG: Set<String> = []
        for vIndex in 0..<4 {
            let opts = ProceduralGlyphOptions(title: "Keep", style: style, primaryHex: 0x10B981, secondaryHex: 0x059669, seed: 42, variantIndex: vIndex)
            let svg = ProceduralGlyphGenerator.generateSVG(options: opts)
            #expect(svg.contains("<svg"))
            #expect(svg.contains("</svg>"))
            #expect(!ProceduralGlyphGenerator.generateSVGData(options: opts).isEmpty)
            variantsSVG.insert(svg)

            let title = ProceduralGlyphGenerator.variantTitle(style: style, index: vIndex)
            #expect(!title.isEmpty)
        }
        // Every style must produce 4 unique, visually distinct procedural variants
        #expect(variantsSVG.count == 4)
    }

    #expect(AIImageStylePreset.allCases.count == 4)
    let url = AIImagePromptBuilder.buildURL(prompt: "minimalist camera", style: .vectorLogo, seed: 123)
    #expect(url != nil)
    #expect(url?.host == "image.pollinations.ai")
    #expect(url?.absoluteString.contains("nologo=true") == true)
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
    #expect(IconPack.allPacks.count == 26)

    let simpleIcons = IconPack.find(id: "simple_icons")
    #expect(simpleIcons.style.monochromeTint == true)
    #expect(simpleIcons.style.shape == .squircle)

    let pixel = IconPack.find(id: "pixel_pure")
    #expect(pixel.style.shape == .circle)
    #expect(pixel.style.backgroundStyle == .googleWhite)

    let catppuccin = IconPack.find(id: "catppuccin_mocha")
    #expect(catppuccin.style.shape == .squircle)
    #expect(catppuccin.style.customHex == 0x1E1E2E)

    let nord = IconPack.find(id: "nordic_frost")
    #expect(nord.style.shape == .squircle)
    #expect(nord.accentHex == 0x88C0D0)

    let vision = IconPack.find(id: "visionos_glass")
    #expect(vision.style.surfaceMaterial == .frostedGlass)

    let ios18 = IconPack.find(id: "ios18_tinted")
    #expect(ios18.style.appearanceMode == .tinted)

    let tokyo = IconPack.find(id: "tokyo_night")
    #expect(tokyo.style.shape == .hexagon)

    let dracula = IconPack.find(id: "dracula_vamp")
    #expect(dracula.accentHex == 0xBD93F9)

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

// MARK: - Shape geometry

private let unitRect = CGRect(x: 0, y: 0, width: 100, height: 100)

@Test("Every shape stays inside its rect and contains the center", arguments: IconShape.allCases)
func shapeBounds(_ shape: IconShape) {
    let path = shape.path(in: unitRect)
    let box = path.boundingBoxOfPath
    #expect(box.minX >= -0.001 && box.minY >= -0.001 && box.maxX <= 100.001 && box.maxY <= 100.001)
    #expect(path.contains(CGPoint(x: 50, y: 50)))
    #expect(!path.contains(CGPoint(x: 1, y: 1)), "the rect corner is outside \(shape)")
    if shape != .hexagon {
        #expect(abs(box.width - 100) < 0.01 && abs(box.height - 100) < 0.01)
    }
}

@Test("Squircle is a superellipse: fuller than a circle, symmetric on all four sides")
func squircleGeometry() {
    let squircle = IconShape.squircle.path(in: unitRect)
    let circle = IconShape.circle.path(in: unitRect)
    let diagonal = CGPoint(x: 50 + 37, y: 50 + 37) // outside the circle (r = 50), inside the superellipse
    #expect(!circle.contains(diagonal))
    #expect(squircle.contains(diagonal))
    for p in [CGPoint(x: 13, y: 13), CGPoint(x: 87, y: 13), CGPoint(x: 13, y: 87), CGPoint(x: 87, y: 87)] {
        #expect(squircle.contains(p))
    }
}

@Test("Hexagon has six vertices and is narrower than tall")
func hexagonGeometry() {
    let path = IconShape.hexagon.path(in: unitRect)
    var points = 0
    path.applyWithBlock { element in
        if element.pointee.type == .moveToPoint || element.pointee.type == .addLineToPoint { points += 1 }
    }
    #expect(points == 6)
    #expect(path.boundingBoxOfPath.width < path.boundingBoxOfPath.height)
}

// MARK: - Pack files

@Test("Pack file round-trips and imported packs get fresh ids")
func packFileRoundTrip() throws {
    let original = CustomIconPack(name: "Night", style: RoundIconStyle(shape: .hexagon, iconScale: 0.9))
    let data = try PackFile.encode([original])
    let imported = try PackFile.decode(data)
    #expect(imported.count == 1)
    #expect(imported[0].name == "Night")
    #expect(imported[0].style == original.style)
    #expect(imported[0].id != original.id)
}

@Test("Pack file rejects foreign, empty and future files")
func packFileRejects() throws {
    #expect(throws: PackFile.PackFileError.notAModoPackFile) { try PackFile.decode(Data("hello".utf8)) }
    #expect(throws: PackFile.PackFileError.notAModoPackFile) { try PackFile.decode(Data(#"{"format":"other","version":1,"packs":[]}"#.utf8)) }
    #expect(throws: PackFile.PackFileError.empty) { try PackFile.decode(Data(#"{"format":"modo-icon-packs","version":1,"packs":[]}"#.utf8)) }
    #expect(throws: PackFile.PackFileError.unsupportedVersion(9)) { try PackFile.decode(Data(#"{"format":"modo-icon-packs","version":9,"packs":[]}"#.utf8)) }
}

@Test("Pack file import clamps out-of-range styles and caps long names")
func packFileSanitizes() throws {
    var wild = RoundIconStyle()
    wild.iconScale = 99
    wild.offsetX = -50
    wild.shadowIntensity = 7
    let longName = String(repeating: "x", count: 200)
    let data = try PackFile.encode([CustomIconPack(name: longName, style: wild)])
    let pack = try PackFile.decode(data)[0]
    #expect(pack.style.iconScale == 2.5)
    #expect(pack.style.offsetX == -1.0)
    #expect(pack.style.shadowIntensity == 1.0)
    #expect(pack.name.count == 60)
}

// MARK: - Icon sets

@Test("Set ids are folder-safe")
func iconSetIDs() {
    for ok in ["atoma-basics", "papirus_1.2", "a"] { #expect(IconSetRules.isValidID(ok)) }
    for bad in ["", "Atoma", "../x", "a/b", ".hidden", "a..b", "sp ace", String(repeating: "a", count: 65)] {
        #expect(!IconSetRules.isValidID(bad), "\(bad) must be rejected")
    }
}

@Test("Only safe relative PNG paths are accepted")
func iconSetPaths() {
    for ok in ["a.png", "icons/safari.PNG", "x/y/z.png"] { #expect(IconSetRules.isSafeRelativePNGPath(ok)) }
    for bad in ["", "/etc/x.png", "../x.png", "a/../b.png", "a//b.png", ".hidden.png", "a/.b.png", "a\\b.png", "a.jpg", "a.png/"] {
        #expect(!IconSetRules.isSafeRelativePNGPath(bad), "\(bad) must be rejected")
    }
}

private func catalogJSON(_ entries: String) -> Data {
    Data(#"{"format":"modo-icon-catalog","version":1,"sets":[\#(entries)]}"#.utf8)
}

private let goodHash = String(repeating: "a", count: 64)

private func entryJSON(id: String = "basics", url: String = "https://example.com/b.zip", sha: String = goodHash, size: Int = 1000) -> String {
    #"{"id":"\#(id)","name":"Basics","author":"Atoma","license":"CC0","version":"1","url":"\#(url)","sha256":"\#(sha)","sizeBytes":\#(size),"iconCount":3}"#
}

@Test("Catalog keeps valid entries and drops hostile ones")
func catalogFiltering() throws {
    let entries = [
        entryJSON(id: "good"),
        entryJSON(id: "http-link", url: "http://example.com/b.zip"),
        entryJSON(id: "file-link", url: "file:///etc/passwd"),
        entryJSON(id: "ftp-link", url: "ftp://example.com/b.zip"),
        entryJSON(id: "../escape"),
        entryJSON(id: "short-hash", sha: "abc"),
        entryJSON(id: "upper-hash", sha: String(repeating: "A", count: 64)),
        entryJSON(id: "huge", size: IconSetRules.maxZipBytes + 1),
        entryJSON(id: "good"),
    ].joined(separator: ",")
    let catalog = try IconSetRules.decodeCatalog(catalogJSON(entries))
    #expect(catalog.sets.map(\.id) == ["good"])
}

@Test("File URLs are accepted in a catalog only when explicitly allowed")
func catalogFileURLs() throws {
    let data = catalogJSON(entryJSON(url: "file:///tmp/b.zip"))
    #expect(try IconSetRules.decodeCatalog(data).sets.isEmpty)
    #expect(try IconSetRules.decodeCatalog(data, allowFileURLs: true).sets.count == 1)
}

@Test("Catalog rejects other formats and newer versions")
func catalogRejects() {
    #expect(throws: IconSetError.notACatalog) { try IconSetRules.decodeCatalog(Data("[]".utf8)) }
    #expect(throws: IconSetError.notACatalog) { try IconSetRules.decodeCatalog(Data(#"{"format":"x","version":1,"sets":[]}"#.utf8)) }
    #expect(throws: IconSetError.unsupportedVersion(5)) {
        try IconSetRules.decodeCatalog(Data(#"{"format":"modo-icon-catalog","version":5,"sets":[]}"#.utf8))
    }
}

@Test("Manifest validation refuses path escapes, bad ids and duplicate bundle ids")
func manifestRules() throws {
    func manifest(id: String = "ok", icons: [IconSetManifest.Entry]) throws -> Data {
        try JSONEncoder().encode(IconSetManifest(id: id, name: "N", author: "A", license: "L", icons: icons))
    }
    let good = try manifest(icons: [.init(bundleID: "com.apple.Safari", file: "safari.png")])
    #expect(try IconSetRules.decodeManifest(good).icons.count == 1)
    #expect(throws: IconSetError.unsafePath("../x.png")) {
        try IconSetRules.decodeManifest(try manifest(icons: [.init(bundleID: "a.b", file: "../x.png")]))
    }
    #expect(throws: IconSetError.invalidID("Bad Id")) {
        try IconSetRules.decodeManifest(try manifest(id: "Bad Id", icons: []))
    }
    #expect(throws: IconSetError.duplicateBundleID("a.b")) {
        try IconSetRules.decodeManifest(try manifest(icons: [.init(bundleID: "a.b", file: "1.png"), .init(bundleID: "a.b", file: "2.png")]))
    }
    #expect(throws: IconSetError.duplicateBundleID("a b")) {
        try IconSetRules.decodeManifest(try manifest(icons: [.init(bundleID: "a b", file: "1.png")]))
    }
    #expect(throws: IconSetError.notAnIconSet) { try IconSetRules.decodeManifest(Data("{}".utf8)) }
}

@Test("SHA-256 matches the known digest of 'abc'")
func sha256Known() {
    #expect(IconSetRules.sha256Hex(Data("abc".utf8)) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")
}

@Test("Simple Icons catalog and slug resolution")
func simpleIconsCatalog() {
    #expect(SimpleIconCatalog.curated.count >= 50)

    let chrome = SimpleIconCatalog.find(for: "Google Chrome.app")
    #expect(chrome != nil)
    #expect(chrome?.slug == "googlechrome")
    #expect(chrome?.svgString().contains("<svg") == true)
    #expect(chrome?.svgString().contains("path") == true)

    let slack = SimpleIconCatalog.find(for: "Slack")
    #expect(slack?.slug == "slack")

    let vscode = SimpleIconCatalog.find(for: "Visual Studio Code")
    #expect(vscode?.slug == "visualstudiocode")

    let cursor = SimpleIconCatalog.find(for: "Cursor.app")
    #expect(cursor?.slug == "cursor")

    let ps = SimpleIconCatalog.find(for: "Photoshop")
    #expect(ps?.slug == "adobephotoshop")

    let nvim = SimpleIconCatalog.find(for: "nvim")
    #expect(nvim?.slug == "neovim")

    let canva = SimpleIconCatalog.find(for: "Canva")
    #expect(canva?.slug == "canva")

    let search = SimpleIconCatalog.search(query: "tele")
    #expect(search.contains(where: { $0.slug == "telegram" }))

    let remote = SimpleIconCatalog.remoteURL(for: "linear")
    #expect(remote?.absoluteString == "https://cdn.jsdelivr.net/npm/simple-icons/icons/linear.svg")
}
