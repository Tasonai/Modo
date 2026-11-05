import AppKit
import Foundation
@testable import ModoKit
import Testing

@Suite("Showcase Engine Tests")
struct CLIShowcaseTests {
    @Test("ShowcaseEngine renders hero layout without crashing")
    func showcaseHeroLayout() {
        let baseImage = NSImage(size: NSSize(width: 256, height: 256))
        baseImage.lockFocus()
        NSColor.systemBlue.setFill()
        NSBezierPath(ovalIn: NSRect(x: 20, y: 20, width: 216, height: 216)).fill()
        baseImage.unlockFocus()

        let options = ShowcaseExportOptions(
            theme: .ambientMesh,
            layout: .heroSingle,
            title: "Test App",
            subtitle: "Subtitle Test",
            width: 400,
            height: 250
        )

        let result = ShowcaseEngine.renderShowcase(icon: baseImage, options: options)
        #expect(result.size.width == 400)
        #expect(result.size.height == 250)
    }

    @Test("ShowcaseEngine renders triple variants layout")
    func showcaseTripleLayout() {
        let baseImage = NSImage(size: NSSize(width: 128, height: 128))
        let options = ShowcaseExportOptions(
            theme: .sunsetGlow,
            layout: .tripleVariants,
            title: "Trio",
            subtitle: "Light Dark Tinted",
            width: 600,
            height: 300
        )

        let result = ShowcaseEngine.renderShowcase(icon: baseImage, options: options)
        #expect(result.size.width == 600)
        #expect(result.size.height == 300)
    }

    @Test("ShowcaseEngine renders mac card layout")
    func showcaseCardLayout() {
        let baseImage = NSImage(size: NSSize(width: 128, height: 128))
        let options = ShowcaseExportOptions(
            theme: .cyberNeon,
            layout: .macCard,
            title: "Card",
            subtitle: "Card Layout Test",
            width: 500,
            height: 400
        )

        let result = ShowcaseEngine.renderShowcase(icon: baseImage, options: options)
        #expect(result.size.width == 500)
        #expect(result.size.height == 400)
    }
}
