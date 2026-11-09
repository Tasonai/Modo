import AppKit
import Foundation

// MARK: - Scalable Vector Graphics (SVG) Export Engine

public enum SVGExportEngine {
    /// Generates pure standalone SVG vector markup for an adaptive icon.
    public static func generateSVG(
        title: String,
        style: RoundIconStyle,
        dominantHex: UInt32 = 0x1A73E8,
        glyphSVGPath: String? = nil,
        rasterBase64PNG: String? = nil,
        canvasSize: Int = 512
    ) -> String {
        let size = Double(canvasSize)
        let pad = size * 0.04
        let innerDim = size - (pad * 2.0)

        // Calculate surface colors
        let (topColor, bottomColor) = surfaceHexColors(for: style, dominantHex: dominantHex)

        var svg = """
        <?xml version="1.0" encoding="UTF-8"?>
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 \(canvasSize) \(canvasSize)" width="\(canvasSize)" height="\(canvasSize)">
          <defs>
            <!-- Drop Shadow Filter -->
            <filter id="modo-shadow" x="-20%" y="-20%" width="140%" height="140%">
              <feDropShadow dx="0" dy="\(Int(size * 0.03))" stdDeviation="\(Int(size * 0.04))" flood-color="#000000" flood-opacity="0.32" />
            </filter>

            <!-- Background Gradient -->
            <linearGradient id="bg-grad" x1="0%" y1="0%" x2="0%" y2="100%">
              <stop offset="0%" stop-color="\(topColor)" />
              <stop offset="100%" stop-color="\(bottomColor)" />
            </linearGradient>

        """

        // Material-specific defs
        switch style.surfaceMaterial {
        case .smooth:
            break
        case .frostedGlass:
            svg += """
                <linearGradient id="glass-sweep" x1="0%" y1="0%" x2="100%" y2="100%">
                  <stop offset="0%" stop-color="#ffffff" stop-opacity="0.32" />
                  <stop offset="45%" stop-color="#ffffff" stop-opacity="0.08" />
                  <stop offset="100%" stop-color="#ffffff" stop-opacity="0.0" />
                </linearGradient>

            """
        case .brushedTitanium:
            svg += """
                <linearGradient id="titanium-grad" x1="0%" y1="0%" x2="100%" y2="100%">
                  <stop offset="0%" stop-color="#ffffff" stop-opacity="0.18" />
                  <stop offset="28%" stop-color="#000000" stop-opacity="0.12" />
                  <stop offset="52%" stop-color="#ffffff" stop-opacity="0.22" />
                  <stop offset="78%" stop-color="#000000" stop-opacity="0.14" />
                  <stop offset="100%" stop-color="#ffffff" stop-opacity="0.10" />
                </linearGradient>

            """
        case .neumorphic:
            svg += """
                <linearGradient id="neu-grad" x1="0%" y1="0%" x2="100%" y2="100%">
                  <stop offset="0%" stop-color="#ffffff" stop-opacity="0.30" />
                  <stop offset="50%" stop-color="#ffffff" stop-opacity="0.0" />
                  <stop offset="100%" stop-color="#000000" stop-opacity="0.22" />
                </linearGradient>

            """
        case .prismGlow:
            svg += """
                <linearGradient id="prism-grad" x1="0%" y1="100%" x2="100%" y2="0%">
                  <stop offset="0%" stop-color="#F43F5E" stop-opacity="0.22" />
                  <stop offset="25%" stop-color="#F59E0B" stop-opacity="0.20" />
                  <stop offset="50%" stop-color="#10B981" stop-opacity="0.22" />
                  <stop offset="75%" stop-color="#3B82F6" stop-opacity="0.25" />
                  <stop offset="100%" stop-color="#8B5CF6" stop-opacity="0.22" />
                </linearGradient>

            """
        case .liquidChrome:
            svg += """
                <linearGradient id="chrome-grad" x1="0%" y1="100%" x2="100%" y2="0%">
                  <stop offset="0%" stop-color="#ffffff" stop-opacity="0.45" />
                  <stop offset="22%" stop-color="#454855" stop-opacity="0.15" />
                  <stop offset="45%" stop-color="#151825" stop-opacity="0.35" />
                  <stop offset="65%" stop-color="#e8f0fe" stop-opacity="0.50" />
                  <stop offset="85%" stop-color="#303340" stop-opacity="0.20" />
                  <stop offset="100%" stop-color="#ffffff" stop-opacity="0.40" />
                </linearGradient>

            """
        case .matteCeramic:
            svg += """
                <linearGradient id="ceramic-grad" x1="50%" y1="0%" x2="50%" y2="100%">
                  <stop offset="0%" stop-color="#ffffff" stop-opacity="0.22" />
                  <stop offset="35%" stop-color="#fdfbf7" stop-opacity="0.08" />
                  <stop offset="70%" stop-color="#e2e4e8" stop-opacity="0.0" />
                  <stop offset="100%" stop-color="#000000" stop-opacity="0.08" />
                </linearGradient>

            """
        case .holographic:
            svg += """
                <linearGradient id="holo-grad" x1="0%" y1="0%" x2="100%" y2="100%">
                  <stop offset="0%" stop-color="#06B6D4" stop-opacity="0.28" />
                  <stop offset="25%" stop-color="#EC4899" stop-opacity="0.30" />
                  <stop offset="50%" stop-color="#F59E0B" stop-opacity="0.24" />
                  <stop offset="75%" stop-color="#10B981" stop-opacity="0.28" />
                  <stop offset="100%" stop-color="#8B5CF6" stop-opacity="0.30" />
                </linearGradient>

            """
        }

        svg += """
          </defs>

          <!-- Background Adaptive Shape -->
          <g filter="url(#modo-shadow)">
        """

        let shapeD = shapePathData(shape: style.shape, x: pad, y: pad, size: innerDim)
        svg += """
            <path d="\(shapeD)" fill="url(#bg-grad)" />

        """

        // Material overlays
        switch style.surfaceMaterial {
        case .smooth:
            break
        case .frostedGlass:
            svg += """
                <path d="\(shapeD)" fill="url(#glass-sweep)" />
                <path d="\(shapeD)" fill="none" stroke="#ffffff" stroke-width="\(max(1.5, size * 0.004))" stroke-opacity="0.28" />

            """
        case .brushedTitanium:
            svg += """
                <path d="\(shapeD)" fill="url(#titanium-grad)" />
                <path d="\(shapeD)" fill="none" stroke="#ffffff" stroke-width="\(max(1.5, size * 0.004))" stroke-opacity="0.20" />

            """
        case .neumorphic:
            svg += """
                <path d="\(shapeD)" fill="url(#neu-grad)" />

            """
        case .prismGlow:
            svg += """
                <path d="\(shapeD)" fill="url(#prism-grad)" />
                <path d="\(shapeD)" fill="none" stroke="#8B5CF6" stroke-width="\(max(2.0, size * 0.006))" stroke-opacity="0.35" />

            """
        case .liquidChrome:
            svg += """
                <path d="\(shapeD)" fill="url(#chrome-grad)" />
                <path d="\(shapeD)" fill="none" stroke="#ffffff" stroke-width="\(max(2.0, size * 0.005))" stroke-opacity="0.40" />

            """
        case .matteCeramic:
            svg += """
                <path d="\(shapeD)" fill="url(#ceramic-grad)" />

            """
        case .holographic:
            svg += """
                <path d="\(shapeD)" fill="url(#holo-grad)" />
                <path d="\(shapeD)" fill="none" stroke="#EC4899" stroke-width="\(max(2.0, size * 0.005))" stroke-opacity="0.30" />

            """
        }

        svg += """
          </g>

          <!-- Foreground Glyph -->
        """

        let glyphScale = style.iconScale
        let glyphSpan = innerDim * glyphScale
        let glyphX = (size - glyphSpan) / 2.0 + (style.offsetX * innerDim * 0.5)
        let glyphY = (size - glyphSpan) / 2.0 - (style.offsetY * innerDim * 0.5)

        if let svgPath = glyphSVGPath, !svgPath.isEmpty {
            let fillHex = style.monochromeTint ? "#FFFFFF" : String(format: "#%06X", dominantHex)
            svg += """
              <g transform="translate(\(glyphX), \(glyphY)) scale(\(glyphSpan / 24.0))">
                <path d="\(svgPath)" fill="\(fillHex)" />
              </g>

            """
        } else if let b64 = rasterBase64PNG, !b64.isEmpty {
            svg += """
              <image href="data:image/png;base64,\(b64)" x="\(glyphX)" y="\(glyphY)" width="\(glyphSpan)" height="\(glyphSpan)" preserveAspectRatio="xMidYMid meet" />

            """
        } else {
            // Default elegant abstract star glyph
            let cx = size / 2.0 + (style.offsetX * innerDim * 0.5)
            let cy = size / 2.0 - (style.offsetY * innerDim * 0.5)
            let r = glyphSpan * 0.45
            svg += """
              <circle cx="\(cx)" cy="\(cy)" r="\(r)" fill="#FFFFFF" fill-opacity="0.95" />

            """
        }

        svg += "</svg>\n"
        return svg
    }

    private static func surfaceHexColors(for style: RoundIconStyle, dominantHex: UInt32) -> (String, String) {
        switch style.backgroundStyle {
        case .googleWhite:
            return ("#FFFFFF", "#F3F4F6")
        case .materialPastel:
            let rgb = MaterialRGB(hex: dominantHex)
            let top = rgb.materialPastel(whiteMix: 0.84).hexString
            let bot = rgb.materialPastel(whiteMix: 0.72).hexString
            return (top, bot)
        case .darkMaterial:
            return ("#262930", "#181A1F")
        case .adaptiveEdge:
            let rgb = MaterialRGB(hex: dominantHex)
            let top = rgb.materialPastel(whiteMix: 0.35).hexString
            return (top, rgb.hexString)
        case .customColor:
            let rgb = MaterialRGB(hex: style.customHex)
            let top = rgb.materialPastel(whiteMix: 0.20).hexString
            return (top, rgb.hexString)
        case .wallpaperDynamic:
            let pal = WallpaperPaletteEngine.currentWallpaperPalette()
            let rgb = MaterialRGB(hex: pal.pastelHex)
            let top = rgb.materialPastel(whiteMix: 0.15).hexString
            return (top, rgb.hexString)
        }
    }

    private static func shapePathData(shape: IconShape, x: Double, y: Double, size: Double) -> String {
        let r = size / 2.0
        let cx = x + r
        let cy = y + r

        switch shape {
        case .circle:
            return "M \(cx) \(y) A \(r) \(r) 0 1 1 \(cx - 0.001) \(y) Z"

        case .squircle:
            // Superellipse n=4 bezier approximation
            let k = r * 0.90
            return """
            M \(cx) \(y)
            C \(cx + k) \(y), \(x + size) \(cy - k), \(x + size) \(cy)
            C \(x + size) \(cy + k), \(cx + k) \(y + size), \(cx) \(y + size)
            C \(cx - k) \(y + size), \(x) \(cy + k), \(x) \(cy)
            C \(x) \(cy - k), \(cx - k) \(y), \(cx) \(y) Z
            """

        case .roundedRect:
            let cr = size * 0.22
            return """
            M \(x + cr) \(y)
            H \(x + size - cr)
            A \(cr) \(cr) 0 0 1 \(x + size) \(y + cr)
            V \(y + size - cr)
            A \(cr) \(cr) 0 0 1 \(x + size - cr) \(y + size)
            H \(x + cr)
            A \(cr) \(cr) 0 0 1 \(x) \(y + size - cr)
            V \(y + cr)
            A \(cr) \(cr) 0 0 1 \(x + cr) \(y) Z
            """

        case .pebble:
            let rTopLeft = size * 0.48
            let rTopRight = size * 0.22
            let rBottomRight = size * 0.48
            let rBottomLeft = size * 0.22
            return """
            M \(x + rTopLeft) \(y)
            H \(x + size - rTopRight)
            A \(rTopRight) \(rTopRight) 0 0 1 \(x + size) \(y + rTopRight)
            V \(y + size - rBottomRight)
            A \(rBottomRight) \(rBottomRight) 0 0 1 \(x + size - rBottomRight) \(y + size)
            H \(x + rBottomLeft)
            A \(rBottomLeft) \(rBottomLeft) 0 0 1 \(x) \(y + size - rBottomLeft)
            V \(y + rTopLeft)
            A \(rTopLeft) \(rTopLeft) 0 0 1 \(x + rTopLeft) \(y) Z
            """

        case .hexagon:
            let h = size
            let w = size * 0.88
            let offX = x + (size - w) / 2.0
            let p1 = "\(offX + w * 0.5),\(y)"
            let p2 = "\(offX + w),\(y + h * 0.25)"
            let p3 = "\(offX + w),\(y + h * 0.75)"
            let p4 = "\(offX + w * 0.5),\(y + h)"
            let p5 = "\(offX),\(y + h * 0.75)"
            let p6 = "\(offX),\(y + h * 0.25)"
            return "M \(p1) L \(p2) L \(p3) L \(p4) L \(p5) L \(p6) Z"
        }
    }
}
