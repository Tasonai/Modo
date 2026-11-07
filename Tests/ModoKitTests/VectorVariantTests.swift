import Foundation
@testable import ModoKit
import Testing

@Suite("Vector Variant Engine Tests")
struct VectorVariantTests {

    @Test("Resolves full vector variants suite for Slack app")
    func resolveSlackVariants() {
        let variants = VectorVariantEngine.resolveLocalVariants(
            title: "Slack",
            bundleIdentifier: "com.tinyspeck.slackmacgap",
            categoryName: "Communication",
            brandHex: 0x4A154B
        )

        #expect(!variants.isEmpty)
        #expect(variants.contains { $0.kind == .original })
        #expect(variants.contains { $0.kind == .brandMono })
        #expect(variants.contains { $0.kind == .brandColor })
        #expect(variants.contains { $0.kind == .sfSymbol })
        #expect(variants.contains { $0.kind == .monogram })
        #expect(variants.contains { $0.kind == .geometric })

        // Check Slack brand vector
        let mono = variants.first { $0.kind == .brandMono }
        #expect(mono != nil)
        #expect(mono?.badge == "Simple Icons")
        #expect(mono?.svgMarkup?.contains("<svg") == true)
        #expect(mono?.brandHex == 0x4A154B)

        let color = variants.first { $0.kind == .brandColor }
        #expect(color != nil)
        #expect(color?.svgMarkup?.contains("4A154B") == true)
    }

    @Test("Contextual SF Symbol matching chooses relevant symbols")
    func contextualSFSymbolMatching() {
        let devSymbols = VectorVariantEngine.smartSFSymbols(
            forTitle: "Visual Studio Code",
            bundleIdentifier: "com.microsoft.VSCode",
            categoryName: "Developer Tools"
        )
        #expect(devSymbols.contains { $0.symbol == "terminal.fill" || $0.symbol == "chevron.left.forwardslash.chevron.right" })

        let chatSymbols = VectorVariantEngine.smartSFSymbols(
            forTitle: "Telegram",
            bundleIdentifier: "ru.keepcoder.Telegram",
            categoryName: "Communication"
        )
        #expect(chatSymbols.contains { $0.symbol == "paperplane.fill" || $0.symbol.contains("bubble") })

        let browserSymbols = VectorVariantEngine.smartSFSymbols(
            forTitle: "Safari",
            bundleIdentifier: "com.apple.Safari",
            categoryName: "Web Browser"
        )
        #expect(browserSymbols.contains { $0.symbol == "globe" || $0.symbol == "safari.fill" })
    }

    @Test("Procedural SVG variants generate valid SVG markup")
    func proceduralSVGGeneration() {
        let variants = VectorVariantEngine.resolveLocalVariants(
            title: "Modo Studio",
            bundleIdentifier: "com.atoma.modo",
            categoryName: "Utility",
            brandHex: 0x3B82F6
        )

        let monogram = variants.first { $0.kind == .monogram }
        #expect(monogram != nil)
        #expect(monogram?.svgMarkup?.contains("<svg") == true)
        #expect(monogram?.svgMarkup?.contains("</svg>") == true)

        let sparkle = variants.first { $0.id == "procedural-sparkle" }
        #expect(sparkle != nil)
        #expect(sparkle?.svgMarkup?.contains("<path") == true)

        let aperture = variants.first { $0.id == "procedural-aperture" }
        #expect(aperture != nil)
        #expect(aperture?.svgMarkup?.contains("rotate") == true)
    }

    @Test("Variant active state evaluation matches item configurations")
    func variantActiveEvaluation() {
        let original = VectorVariant(
            id: "original",
            title: "Original",
            subtitle: "Default",
            badge: "Native",
            kind: .original
        )
        #expect(original.isActive(customImageData: nil, currentSymbolName: nil, currentBrandHex: 0x111111))
        #expect(!original.isActive(customImageData: Data([1, 2, 3]), currentSymbolName: nil, currentBrandHex: 0x111111))

        let fakeSVG = "<svg>sample</svg>".data(using: .utf8)!
        let brand = VectorVariant(
            id: "brand",
            title: "Brand",
            subtitle: "Logo",
            badge: "Simple Icons",
            kind: .brandMono,
            rawData: fakeSVG
        )
        #expect(brand.isActive(customImageData: fakeSVG, currentSymbolName: nil, currentBrandHex: nil))
        #expect(!brand.isActive(customImageData: nil, currentSymbolName: nil, currentBrandHex: nil))

        let sym = VectorVariant(
            id: "sym",
            title: "Terminal",
            subtitle: "Code",
            badge: "SF Symbol",
            kind: .sfSymbol,
            symbolName: "terminal.fill"
        )
        #expect(sym.isActive(customImageData: nil, currentSymbolName: "terminal.fill", currentBrandHex: nil))
        #expect(!sym.isActive(customImageData: nil, currentSymbolName: "gear", currentBrandHex: nil))
    }
}
