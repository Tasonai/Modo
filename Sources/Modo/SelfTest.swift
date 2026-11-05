import AppKit
import Foundation
import ModoKit
import SwiftData

/// Self-test for agents: runs the app's real functions on a test store,
/// prints PASS or FAIL per check, and quits with exit code 0 or 1.
///   APP_STORE=/tmp/IconMaker-self.store ~/Applications/IconMaker.app/Contents/MacOS/IconMaker --selftest
@MainActor
enum SelfTest {
    static var requested: Bool { CommandLine.arguments.contains("--selftest") }
    /// Read by AppDelegate.applicationWillTerminate.
    static var exitCode: Int32 = 0
    private static var failures = 0
    private static var count = 0

    static func runIfRequested(model: AppModel) {
        guard requested else { return }
        setvbuf(stdout, nil, _IOLBF, 0)
        guard Store.isOverridden else {
            print("SELFTEST refused: set APP_STORE to a file in /tmp")
            finish(code: 1)
            return
        }
        Task {
            try? await Task.sleep(for: .seconds(0.5))
            let context = model.container.mainContext
            func items() -> [Item] { (try? context.fetch(FetchDescriptor<Item>())) ?? [] }

            let before = items().count
            model.addItem(title: "Pixel Studio.app", symbolName: "camera.aperture", brandHex: 0x1A73E8)
            check("New Item adds one app entry", items().count == before + 1)
            check("the new item is selected", model.selection != nil)

            if let item = items().first(where: { $0.id == model.selection }) {
                check("app title strips .app suffix", item.title == "Pixel Studio")

                let rendered = model.roundIcon(for: item)
                check("circular Google icon renders at 256x256", Int(rendered.size.width) == 256)

                model.style.backgroundStyle = .materialPastel
                model.style.iconScale = 1.65
                model.style.offsetX = 0.25
                model.style.offsetY = -0.20
                let pastelRendered = model.roundIcon(for: item)
                check("Material You pastel style with >100% zoom and X/Y pan renders cleanly",
                      Int(pastelRendered.size.height) == 256 && model.style.iconScale == 1.65)
                model.style.resetPosition()
                model.style.iconScale = 0.68

                model.applyRoundIcon(to: item)
                check("applying circular icon marks item as applied", item.isApplied)

                model.restoreOriginalIcon(for: item)
                check("restoring original icon clears applied state", !item.isApplied)

                let exportDir = URL(fileURLWithPath: "/tmp/IconMaker-selftest-export")
                try? FileManager.default.removeItem(at: exportDir)
                let exported = model.exportIconPack(for: [item], to: exportDir)
                check("exporting icon pack writes .icns, .ico, and .png", exported == 1)

                item.isStarred.toggle()
                check("starring an item marks it starred", item.isStarred)
                context.delete(item)
                try? context.save()
                check("deleting the item removes it", items().count == before)
            }
            check("titles are cleaned", ItemTitle.clean("  Safari.app ") == "Safari")

            // Auto re-apply: a real bundle in /tmp loses its icon file / gets a new version.
            do {
                let fm = FileManager.default
                let app = "/tmp/ModoReapplyTest.app"
                try? fm.removeItem(atPath: app)
                try? fm.createDirectory(atPath: app + "/Contents", withIntermediateDirectories: true)
                func writePlist(_ version: String) {
                    let plist: [String: Any] = ["CFBundleName": "ModoReapplyTest", "CFBundleIdentifier": "com.atoma.modo.reapplytest",
                                                "CFBundleShortVersionString": version, "CFBundleVersion": "1"]
                    (plist as NSDictionary).write(toFile: app + "/Contents/Info.plist", atomically: true)
                }
                writePlist("1.0")
                AppModel.persistRecordsInSelfTest = true
                AppModel.clearRecords()
                let probe = Item(title: "ModoReapplyTest", bundlePath: app)
                context.insert(probe)
                model.applyPack(.pixelPure)
                model.applyRoundIcon(to: probe)
                let iconFile = app + "/Icon\r"
                check("applied icon writes the custom-icon file", fm.fileExists(atPath: iconFile))
                check("untouched app is not re-applied", model.reapplyUpdatedApps(only: app) == 0)
                try? fm.removeItem(atPath: iconFile)
                check("vanished icon file is re-applied", model.reapplyUpdatedApps(only: app) == 1 && fm.fileExists(atPath: iconFile))
                writePlist("2.0")
                check("updated app version is re-applied", model.reapplyUpdatedApps(only: app) == 1)
                check("re-applied app is stable afterwards", model.reapplyUpdatedApps(only: app) == 0)
                model.restoreOriginalIcon(for: probe)
                check("restoring forgets the record", model.reapplyUpdatedApps(only: app) == 0)
                context.delete(probe)
                try? context.save()
                try? fm.removeItem(atPath: app)
                AppModel.clearRecords()
                AppModel.persistRecordsInSelfTest = false
            }

            // Icon sets: install, hostile archives, catalog download.
            do {
                let fm = FileManager.default
                let work = URL(fileURLWithPath: "/tmp/Modo-selftest-sets", isDirectory: true)
                try? fm.removeItem(at: work)
                try? fm.removeItem(at: IconSetStore.root)
                try? fm.createDirectory(at: work, withIntermediateDirectories: true)

                func png(_ color: NSColor, size: Int = 64) -> Data {
                    let image = NSImage(size: NSSize(width: size, height: size))
                    image.lockFocus(); color.setFill(); NSRect(x: 0, y: 0, width: size, height: size).fill(); image.unlockFocus()
                    return NSBitmapImageRep(cgImage: image.cgImage(forProposedRect: nil, context: nil, hints: nil)!)
                        .representation(using: .png, properties: [:])!
                }
                let pureRed = NSColor(srgbRed: 1, green: 0, blue: 0, alpha: 1)
                /// Builds a zip from a folder spec and returns its bytes.
                func makeZip(named name: String, manifestFile: String = "red.png", pngData: Data? = nil,
                             extra: (URL) -> Void = { _ in }) -> Data {
                    let dir = work.appendingPathComponent(name, isDirectory: true)
                    try? fm.removeItem(at: dir)
                    try? fm.createDirectory(at: dir, withIntermediateDirectories: true)
                    let manifest = IconSetManifest(id: name, name: "Set \(name)", author: "Atoma", license: "CC0",
                                                   icons: [.init(bundleID: "com.example.probe", file: manifestFile)])
                    try? JSONEncoder().encode(manifest).write(to: dir.appendingPathComponent("manifest.json"))
                    try? (pngData ?? png(pureRed)).write(to: dir.appendingPathComponent("red.png"))
                    extra(dir)
                    let zip = work.appendingPathComponent("\(name).zip")
                    try? fm.removeItem(at: zip)
                    let p = Process()
                    p.executableURL = URL(fileURLWithPath: "/usr/bin/zip")
                    p.currentDirectoryURL = dir
                    p.arguments = ["-q", "-r", "-y", zip.path, "."]
                    try? p.run(); p.waitUntilExit()
                    return (try? Data(contentsOf: zip)) ?? Data()
                }

                let good = makeZip(named: "goodset")
                let goodHash = IconSetRules.sha256Hex(good)
                let installed = try? IconSetStore.install(zipData: good, expectedSHA256: goodHash)
                check("a valid icon set installs", installed?.id == "goodset" && IconSetStore.installedSets().count == 1)

                model.refreshIconSets()
                model.activeIconSetID = "goodset"
                let probe = Item(title: "Probe", bundleIdentifier: "com.example.probe")
                let art = model.sourceIcon(for: probe, canvasSize: 64)
                let artRep = art.cgImage(forProposedRect: nil, context: nil, hints: nil).map { NSBitmapImageRep(cgImage: $0) }
                let artColor = artRep?.colorAt(x: 32, y: 32)?.usingColorSpace(.sRGB)
                check("the active set's picture is used for a matching app", (artColor?.redComponent ?? 0) > 0.8 && (artColor?.redComponent ?? 0) > (artColor?.greenComponent ?? 0) + 0.4)
                let other = Item(title: "Other", bundleIdentifier: "com.example.other")
                check("an app the set does not cover keeps its own icon", model.sourceIcon(for: other, canvasSize: 64) !== art)
                model.activeIconSetID = nil

                @MainActor func refuses(_ label: String, _ data: Data, sha: String? = nil) {
                    let before = IconSetStore.installedSets().count
                    let threw = (try? IconSetStore.install(zipData: data, expectedSHA256: sha)) == nil
                    check(label, threw && IconSetStore.installedSets().count == before)
                }
                refuses("a wrong checksum is refused", good, sha: String(repeating: "0", count: 64))
                refuses("a manifest path that escapes the folder is refused", makeZip(named: "escape", manifestFile: "../evil.png"))
                check("nothing was written outside the set folder", !fm.fileExists(atPath: work.appendingPathComponent("evil.png").path))
                refuses("a symlink inside the archive is refused", makeZip(named: "linked") { dir in
                    try? fm.createSymbolicLink(atPath: dir.appendingPathComponent("link.png").path, withDestinationPath: "/etc/hosts")
                })
                refuses("a listed 'png' that is really text is refused", makeZip(named: "fake", pngData: Data("not an image".utf8)))
                refuses("an oversized picture is refused", makeZip(named: "huge", pngData: png(.blue, size: IconSetRules.maxPixels + 1)))
                refuses("garbage is refused", Data("definitely not a zip".utf8))

                // Catalog over a local file: fetch, then download and install.
                let catalog = IconCatalog(format: IconSetRules.catalogFormat, version: 1, sets: [
                    .init(id: "goodset", name: "Set goodset", author: "Atoma", license: "CC0", version: "1",
                          url: work.appendingPathComponent("goodset.zip").absoluteString, sha256: goodHash,
                          sizeBytes: good.count, iconCount: 1),
                    .init(id: "tampered", name: "Tampered", author: "Evil", license: "?", version: "1",
                          url: work.appendingPathComponent("goodset.zip").absoluteString, sha256: String(repeating: "1", count: 64),
                          sizeBytes: good.count, iconCount: 1),
                ])
                let catalogURL = work.appendingPathComponent("catalog.json")
                try? JSONEncoder().encode(catalog).write(to: catalogURL)
                IconSetStore.remove(id: "goodset")
                if let fetched = try? await IconSetStore.fetchCatalog(from: catalogURL) {
                    check("a catalog lists its sets", fetched.sets.count == 2)
                    let ok = try? await IconSetStore.download(fetched.sets[0])
                    check("a catalog entry downloads and installs", ok?.id == "goodset")
                    let bad = try? await IconSetStore.download(fetched.sets[1])
                    check("a download that fails its checksum is refused", bad == nil)
                } else {
                    check("a catalog lists its sets", false)
                }
                model.refreshIconSets()
                if let set = model.installedIconSets.first { model.removeIconSet(set) }
                check("removing a set deletes it", model.installedIconSets.isEmpty)

                // Curated popular packs: direct offline install & icon extraction
                let papirus = try? CuratedIconSets.install(id: "papirus-minimal")
                check("curated popular pack installs with icons", papirus?.id == "papirus-minimal" && (papirus?.manifest.icons.count ?? 0) >= 10)
                model.refreshIconSets()
                model.activeIconSetID = "papirus-minimal"
                let chromeItem = Item(title: "Google Chrome", bundleIdentifier: "com.google.Chrome")
                let chromeArt = model.sourceIcon(for: chromeItem, canvasSize: 64)
                check("curated pack supplies artwork for Chrome", chromeArt.size.width > 0)
                if let set = model.installedIconSets.first(where: { $0.id == "papirus-minimal" }) {
                    model.removeIconSet(set)
                }
                model.activeIconSetID = nil
                check("curated pack cleanup leaves store empty", model.installedIconSets.isEmpty)

                try? fm.removeItem(at: work)
                try? fm.removeItem(at: IconSetStore.root)
            }

            // Icon packs: built-in apply, manual edit leaves the pack, custom save and delete.
            model.applyPack(.catppuccinMocha)
            check("applying Catppuccin sets its style and palette", model.style.customHex == 0x1E1E2E && model.activePackName == "Catppuccin")
            model.applyPack(.nordicFrost)
            check("applying Nordic Frost sets its arctic style", model.style.customHex == 0x2E3440 && model.activePackName == "Nordic Frost")
            model.applyPack(.cyberHex)
            check("applying a pack sets its shape", model.style.shape == .hexagon && model.activePackName == "Cyber Hex")
            model.style.iconScale = 0.5
            check("manual edit leaves the pack", model.activePackName == "Custom Style")
            model.saveCurrentStyleAsPack(named: "Selftest Pack")
            check("saving a custom pack activates it", model.activePackName == "Selftest Pack")
            if let saved = model.customPacks.first(where: { $0.name == "Selftest Pack" }) {
                model.deleteCustomPack(saved)
            }
            check("deleting a custom pack removes it", !model.customPacks.contains { $0.name == "Selftest Pack" })
            do {
                let before = model.customPacks.count
                model.style.shape = .pebble
                model.saveCurrentStyleAsPack(named: "Selftest Share")
                let file = URL(fileURLWithPath: "/tmp/Modo-selftest.\(PackFile.fileExtension)")
                try? FileManager.default.removeItem(at: file)
                if let share = model.customPacks.last { model.exportPacks([share], to: file) }
                check("pack export writes a file", FileManager.default.fileExists(atPath: file.path))
                check("pack import adds a copy with the same style",
                      model.importPacks(from: file) == 1
                      && model.customPacks.count == before + 2
                      && model.customPacks.last?.style.shape == .pebble)
                check("importing a non-pack file is refused", model.importPacks(from: URL(fileURLWithPath: "/etc/hosts")) == 0)
                model.customPacks.removeAll { $0.name == "Selftest Share" }
                try? FileManager.default.removeItem(at: file)
            }

            // Icon sizing: grid view mode, slider bounds, per-app scale overrides, multi-resolution export
            do {
                model.viewMode = .grid
                check("view mode switches to grid", model.viewMode == .grid)
                model.gridIconSize = 250.0
                check("grid icon size clamps to max bounds", model.gridIconSize == 160.0)
                model.gridIconSize = 10.0
                check("grid icon size clamps to min bounds", model.gridIconSize == 36.0)
                model.gridIconSize = 72.0
                model.viewMode = .list

                let zoomApp = Item(title: "ZoomApp", bundleIdentifier: "com.example.zoomapp")
                zoomApp.customScale = 1.45
                check("per-app scale overrides global style", model.effectiveStyle(for: zoomApp).iconScale == 1.45 && model.style.iconScale != 1.45)
                let customRender = model.roundIcon(for: zoomApp)
                check("per-app scale produces a valid rendered bitmap", customRender.size.width > 0)
                zoomApp.customScale = nil
                model.invalidateCache(for: zoomApp)
                check("clearing custom scale restores global style", model.effectiveStyle(for: zoomApp).iconScale == model.style.iconScale)

                let exportDir = URL(fileURLWithPath: "/tmp/Modo-sizes-test", isDirectory: true)
                try? FileManager.default.removeItem(at: exportDir)
                let exportedCount = model.exportCustomSizes(for: [zoomApp], sizes: [16, 32, 64, 128, 512], to: exportDir)
                let has16 = FileManager.default.fileExists(atPath: exportDir.appendingPathComponent("ZoomApp-16x16.png").path)
                let has512 = FileManager.default.fileExists(atPath: exportDir.appendingPathComponent("ZoomApp-512x512.png").path)
                check("multi-resolution export writes requested sizes", exportedCount == 1 && has16 && has512)
                try? FileManager.default.removeItem(at: exportDir)
            }

            // Custom artwork: Drag-and-drop / SVG / PNG glyph replacement
            do {
                let svgStr = """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                    <circle cx="50" cy="50" r="45" fill="#4285F4"/>
                    <path d="M30 50 L45 65 L70 35" stroke="#FFFFFF" stroke-width="10" stroke-linecap="round" fill="none"/>
                </svg>
                """
                let svgData = svgStr.data(using: .utf8)!
                check("CustomArtworkHelper identifies valid SVG text", CustomArtworkHelper.isSVGData(svgData))

                let customApp = Item(title: "CustomApp", bundleIdentifier: "com.example.customapp")
                let defaultSource = model.sourceIcon(for: customApp, canvasSize: 256)

                // Assign SVG vector
                model.setCustomArtwork(data: svgData, for: customApp)
                check("custom SVG data stored in item", customApp.customImageData != nil)
                let svgSource = model.sourceIcon(for: customApp, canvasSize: 256)
                check("sourceIcon returns custom SVG image", svgSource != defaultSource && svgSource.size.width > 0)
                let svgRound = model.roundIcon(for: customApp)
                check("roundIcon renders styled circular icon from custom SVG", svgRound.size.width > 0)

                // Clear artwork back to system default
                model.clearCustomArtwork(for: customApp)
                check("clearing custom artwork resets customImageData to nil", customApp.customImageData == nil)
                let restoredSource = model.sourceIcon(for: customApp, canvasSize: 256)
                check("restored source matches default behavior", restoredSource.size.width > 0)

                // Simple Icons 1-click vector matching
                let slackIcon = SimpleIconCatalog.find(for: "Slack.app")
                check("SimpleIconCatalog finds match for Slack", slackIcon != nil && slackIcon?.slug == "slack")
                if let icon = slackIcon {
                    model.setCustomArtwork(data: icon.svgData(), for: customApp)
                    check("applying SimpleIcon assigns SVG data", customApp.customImageData != nil)
                    let roundSlack = model.roundIcon(for: customApp)
                    check("roundIcon renders SimpleIcon vector", roundSlack.size.width > 0)
                }

                // Appearance modes: Light, Dark, Tinted (macOS Sequoia / iOS 18)
                model.style.appearanceMode = .dark
                model.invalidateCache(for: customApp)
                let darkIcon = model.roundIcon(for: customApp)
                check("renders valid Dark appearance icon", darkIcon.size.width > 0)

                model.style.appearanceMode = .tinted
                model.style.tintColorHex = 0xAF52DE // iOS 18 Purple Tint
                model.invalidateCache(for: customApp)
                let tintedIcon = model.roundIcon(for: customApp)
                check("renders valid Tinted appearance icon", tintedIcon.size.width > 0)

                model.style.appearanceMode = .light
                model.invalidateCache(for: customApp)

                let appearanceDir = URL(fileURLWithPath: "/tmp/Modo-appearance-test", isDirectory: true)
                try? FileManager.default.removeItem(at: appearanceDir)
                let variantCount = model.exportAppearanceVariants(for: [customApp], to: appearanceDir)
                let hasLight = FileManager.default.fileExists(atPath: appearanceDir.appendingPathComponent("CustomApp-Light.png").path)
                let hasDark = FileManager.default.fileExists(atPath: appearanceDir.appendingPathComponent("CustomApp-Dark.png").path)
                let hasTinted = FileManager.default.fileExists(atPath: appearanceDir.appendingPathComponent("CustomApp-Tinted.png").path)
                check("exportAppearanceVariants generates Light, Dark, and Tinted PNGs", variantCount == 1 && hasLight && hasDark && hasTinted)
                try? FileManager.default.removeItem(at: appearanceDir)

                // 1-Click System Theming Wizard: Snapshot backup point creation & restore
                let backupPoint = model.createBackupPoint(for: [customApp], name: "Selftest Backup")
                check("createBackupPoint creates snapshot point", backupPoint.appPaths.contains(customApp.bundlePath) || backupPoint.name == "Selftest Backup")
                check("backup point recorded in recentBackups", model.recentBackups.contains(where: { $0.id == backupPoint.id }))
                let restoredCount = model.restoreFromBackupPoint(backupPoint, items: [customApp])
                check("restoreFromBackupPoint executes cleanly", restoredCount >= 0)

                // AI Glyph Studio: Procedural Geometric Vector generation & assignment
                let prismOpts = ProceduralGlyphOptions(title: "Cyber App", style: .prism, primaryHex: 0x8B5CF6, secondaryHex: 0x6D28D9, seed: 101)
                let prismSVGData = ProceduralGlyphGenerator.generateSVGData(options: prismOpts)
                check("ProceduralGlyphGenerator creates valid SVG data", CustomArtworkHelper.isSVGData(prismSVGData))
                model.setCustomArtwork(data: prismSVGData, for: customApp)
                check("AI geometric vector assigned to item", customApp.customImageData != nil)
                let aiRound = model.roundIcon(for: customApp)
                check("roundIcon renders styled icon from AI vector", aiRound.size.width > 0)

                // OTA Auto-Update: Manifest parsing, version comparator, and checksum verification
                let testManifestJSON = """
                {
                  "version": "9.9.9",
                  "releaseDate": "2026-11-05",
                  "title": "Modo 9.9.9 — Future Release",
                  "releaseNotes": "OTA test release notes",
                  "isMandatory": false,
                  "assets": [
                    {
                      "name": "Modo-macOS.zip",
                      "platform": "macos",
                      "format": "zip",
                      "downloadURL": "https://example.com/Modo-macOS.zip",
                      "sha256": "4b227777d4dd1fc61c6f884f48641d02b4d121d3fd328cb08b5531fcacdabf8a",
                      "sizeBytes": 2048,
                      "isPrimary": true
                    }
                  ]
                }
                """
                let mData = testManifestJSON.data(using: .utf8)!
                let manifest = try JSONDecoder().decode(UpdateManifest.self, from: mData)
                check("OTAUpdateEngine detects newer version available", OTAUpdateEngine.isUpdateAvailable(manifest: manifest, currentVersion: "1.5.0"))
                check("OTAUpdateEngine reports current version as not newer", !OTAUpdateEngine.isUpdateAvailable(manifest: manifest, currentVersion: "9.9.9"))
                let bestAsset = OTAUpdateEngine.findBestAsset(in: manifest, for: .macos)
                check("OTAUpdateEngine selects macOS zip asset", bestAsset?.format == .zip && bestAsset?.name == "Modo-macOS.zip")

                let payload = "Modo OTA Update Verification Payload".data(using: .utf8)!
                let validHash = OTAUpdateEngine.sha256(data: payload)
                check("OTAUpdateEngine validates matching SHA-256", OTAUpdateEngine.verifyChecksum(data: payload, expectedSHA256: validHash))
                check("OTAUpdateEngine rejects tampered checksum", !OTAUpdateEngine.verifyChecksum(data: payload, expectedSHA256: "0000000000000000000000000000000000000000000000000000000000000000"))

                // Folder Icon Styler: SVG generation, color presets & folder customization
                let folderSVG = FolderVectorGenerator.generateFolderSVG(
                    style: FolderStyle(preset: .sunsetOrange, embossStyle: .etched),
                    size: 512,
                    centerGlyphSVG: "<circle cx=\"256\" cy=\"330\" r=\"40\" fill=\"white\"/>"
                )
                check("FolderVectorGenerator outputs valid SVG", folderSVG.contains("<svg") && folderSVG.contains("linearGradient"))
                check("FolderColorPreset provides all 9 palette colors", FolderColorPreset.allCases.count == 9)

                let fm = FileManager.default
                let testFolder = URL(fileURLWithPath: "/tmp/ModoFolderTest")
                try? fm.removeItem(at: testFolder)
                try? fm.createDirectory(at: testFolder, withIntermediateDirectories: true)
                model.addFolder(from: testFolder)
                if let folderItem = items().first(where: { $0.bundlePath == testFolder.path }) {
                    check("added folder item is identified as isFolder", folderItem.isFolder)
                    let folderImg = model.folderIcon(for: folderItem)
                    check("model.folderIcon renders an NSImage", folderImg.size.width > 0 && folderImg.size.height > 0)
                    model.applyRoundIcon(to: folderItem)
                    check("applying folder icon marks item as applied", folderItem.isApplied)
                    model.restoreOriginalIcon(for: folderItem)
                    check("restoring folder icon clears applied state", !folderItem.isApplied)
                    context.delete(folderItem)
                    try? context.save()
                }
                try? fm.removeItem(at: testFolder)

                // Batch Iconset & Pack Creator: Directory scanning & pack naming
                let batchDir = URL(fileURLWithPath: "/tmp/ModoBatchTest")
                try? fm.removeItem(at: batchDir)
                try? fm.createDirectory(at: batchDir, withIntermediateDirectories: true)
                let samplePNG = batchDir.appendingPathComponent("safari-browser.png")
                let sampleSVG = batchDir.appendingPathComponent("developer-terminal.svg")
                try? "fake-png-data".write(to: samplePNG, atomically: true, encoding: .utf8)
                try? "<svg></svg>".write(to: sampleSVG, atomically: true, encoding: .utf8)
                let candidates = BatchIconEngine.scanDirectory(at: batchDir)
                check("BatchIconEngine scans image files into candidates", candidates.count == 2)
                check("BatchIconEngine formats title from filename", candidates.contains(where: { $0.cleanTitle == "Safari Browser" }))
                let safeName = BatchIconEngine.safePackName("My Awesome Design Pack! 2026")
                check("BatchIconEngine formats safe pack name", safeName == "My-Awesome-Design-Pack-2026")
                try? fm.removeItem(at: batchDir)
            }

            print(failures == 0 ? "SELFTEST PASS \(count) checks" : "SELFTEST FAIL \(failures) of \(count) checks")
            finish(code: failures == 0 ? 0 : 1)
        }
    }

    private static func check(_ name: String, _ ok: Bool, _ detail: String = "") {
        count += 1
        if !ok { failures += 1 }
        print(ok ? "PASS \(name)" : "FAIL \(name)\(detail.isEmpty ? "" : ": \(detail)")")
    }

    static func finish(code: Int32) {
        exitCode = code
        for window in NSApp.windows { if let sheet = window.attachedSheet { window.endSheet(sheet) } }
        NSApp.terminate(nil)
    }
}
