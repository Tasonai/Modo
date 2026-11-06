import AppKit
import Foundation
@testable import ModoKit
import Testing

@Suite("SVG Vector Export Tests")
struct SVGExportTests {
    @Test("SVGExportEngine generates valid SVG XML for all shapes")
    func svgShapeGeneration() {
        for shape in IconShape.allCases {
            let style = RoundIconStyle(backgroundStyle: .materialPastel, shape: shape)
            let svg = SVGExportEngine.generateSVG(title: "Test", style: style)
            #expect(svg.contains("<svg"))
            #expect(svg.contains("</svg>"))
            #expect(svg.contains("<path d="))
            #expect(svg.contains("linearGradient"))
        }
    }

    @Test("SVGExportEngine embeds materials and glyph vectors")
    func svgMaterialAndGlyphEmbedding() {
        var style = RoundIconStyle(backgroundStyle: .wallpaperDynamic, shape: .squircle)
        style.surfaceMaterial = .frostedGlass

        let glyph = "M12 2L2 7l10 5 10-5-10-5z"
        let svg = SVGExportEngine.generateSVG(title: "VectorApp", style: style, glyphSVGPath: glyph)

        #expect(svg.contains("glass-sweep"))
        #expect(svg.contains(glyph))
    }
}
