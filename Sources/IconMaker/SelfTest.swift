import AppKit
import Foundation
import IconMakerKit
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
