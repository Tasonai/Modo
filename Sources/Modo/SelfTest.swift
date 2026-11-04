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

            // Icon packs: built-in apply, manual edit leaves the pack, custom save and delete.
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
