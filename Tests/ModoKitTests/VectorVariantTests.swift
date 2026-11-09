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
        #expect(variants.contains { $0.kind == .lucide })
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

        let lawn = VectorVariant(
            id: "lawnicons-telegram",
            title: "Telegram Line",
            subtitle: "Lawnicons Themed",
            badge: "Lawnicons",
            kind: .lawnicons,
            rawData: fakeSVG
        )
        #expect(lawn.isActive(customImageData: fakeSVG, currentSymbolName: nil, currentBrandHex: nil))
        #expect(!lawn.isActive(customImageData: nil, currentSymbolName: nil, currentBrandHex: nil))

        let ws = VectorVariant(
            id: "whitesur-telegram",
            title: "Telegram macOS",
            subtitle: "WhiteSur Scalable",
            badge: "WhiteSur",
            kind: .whiteSur,
            rawData: fakeSVG
        )
        #expect(ws.isActive(customImageData: fakeSVG, currentSymbolName: nil, currentBrandHex: nil))
        #expect(!ws.isActive(customImageData: nil, currentSymbolName: nil, currentBrandHex: nil))
    }

    @Test("Candidate slugs generator produces cross-platform open icon slugs")
    func candidateSlugsGenerator() {
        let chromeSlugs = VectorVariantEngine.candidateSlugs(for: "Google Chrome")
        #expect(chromeSlugs.contains("google-chrome"))
        #expect(chromeSlugs.contains("google_chrome"))
        #expect(chromeSlugs.contains("googlechrome"))

        let codeSlugs = VectorVariantEngine.candidateSlugs(for: "Visual Studio Code.app")
        #expect(codeSlugs.contains("visual-studio-code"))
        #expect(codeSlugs.contains("visual_studio_code"))
        #expect(codeSlugs.contains("visualstudiocode"))
        #expect(codeSlugs.contains("vscode"))

        let localizedSlugs = VectorVariantEngine.candidateSlugs(for: "Заметки")
        #expect(localizedSlugs.contains("notes"))
    }

    @Test("Custom Monogram SVG generator produces valid SVGs across frames and fonts")
    func customMonogramSVGGeneration() {
        for frame in MonogramFrameStyle.allCases {
            for font in MonogramFontStyle.allCases {
                let svg = ProceduralGlyphGenerator.generateCustomMonogramSVG(
                    text: "AB",
                    fontStyle: font,
                    frameStyle: frame,
                    primaryHex: 0x1A73E8,
                    secondaryHex: 0x4285F4
                )
                #expect(svg.contains("<svg"))
                #expect(svg.contains("</svg>"))
                #expect(svg.contains("AB"))
                #expect(svg.contains("#1A73E8"))
            }
        }
    }

    @Test("Lucide vector catalog validation and smart selection")
    func lucideCatalogValidation() {
        #expect(!LucideIconCatalog.all.isEmpty)
        #expect(LucideIconCatalog.all.count >= 30)

        // Exact match
        let term = LucideIconCatalog.match(query: "terminal")
        #expect(term != nil)
        #expect(term?.name == "terminal")
        #expect(term?.category == .developer)
        let svg = term?.svgString(strokeColorHex: 0x10B981, strokeWidth: 2.0)
        #expect(svg?.contains("<svg") == true)
        #expect(svg?.contains("10B981") == true)
        #expect(svg?.contains("stroke-width=\"2.0\"") == true)

        // Fuzzy / tag match
        let bash = LucideIconCatalog.match(query: "bash")
        #expect(bash?.name == "terminal")

        let devLucide = VectorVariantEngine.smartLucideIcon(
            forTitle: "Terminal.app",
            bundleIdentifier: "com.apple.Terminal",
            categoryName: "Developer Tools"
        )
        #expect(devLucide?.name == "terminal")

        let chatLucide = VectorVariantEngine.smartLucideIcon(
            forTitle: "Discord",
            bundleIdentifier: "com.discord.app",
            categoryName: "Communication"
        )
        #expect(chatLucide?.name == "message-square")
    }
}
