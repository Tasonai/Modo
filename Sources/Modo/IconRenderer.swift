import AppKit
import CoreImage
import CoreGraphics
import Foundation
import ModoKit
import ImageIO
import UniformTypeIdentifiers

/// Native CoreGraphics / AppKit circular icon renderer and macOS `.app` icon applier.
enum IconRenderer {
    /// Loads the original unmodified icon for an application bundle (reading directly from
    /// `Contents/Resources/*.icns` when available so custom applied icons don't overwrite the source),
    /// or synthesizes a crisp vector icon for seeded/custom entries.
    static func loadOriginalIcon(bundlePath: String, symbolName: String, brandHex: UInt32, canvasSize: Int = 512) -> NSImage {
        if !bundlePath.isEmpty, FileManager.default.fileExists(atPath: bundlePath) {
            if let bundle = Bundle(path: bundlePath),
               let iconFile = bundle.infoDictionary?["CFBundleIconFile"] as? String {
                let fileName = iconFile.hasSuffix(".icns") ? iconFile : "\(iconFile).icns"
                let icnsPath = (bundlePath as NSString).appendingPathComponent("Contents/Resources/\(fileName)")
                if let image = NSImage(contentsOfFile: icnsPath) {
                    return image
                }
            }
            if let image = NSImage(contentsOfFile: bundlePath) {
                return image
            }
            let wsIcon = NSWorkspace.shared.icon(forFile: bundlePath)
            wsIcon.size = NSSize(width: canvasSize, height: canvasSize) // grid: ok bitmap canvas
            return wsIcon
        }
        return synthesizeAppIcon(symbolName: symbolName, brandHex: brandHex, canvasSize: canvasSize)
    }

    /// Synthesizes a rich macOS-style squircle app icon with a gradient background and SF Symbol glyph.
    static func synthesizeAppIcon(symbolName: String, brandHex: UInt32, canvasSize: Int = 512) -> NSImage {
        let dim = CGFloat(canvasSize)
        let image = NSImage(size: NSSize(width: dim, height: dim)) // grid: ok bitmap canvas
        image.lockFocus()
        defer { image.unlockFocus() }

        guard let ctx = NSGraphicsContext.current?.cgContext else { return image }
        ctx.setAllowsAntialiasing(true)
        ctx.setShouldAntialias(true)

        let inset = dim * 0.09
        let rect = CGRect(x: inset, y: inset, width: dim - inset * 2, height: dim - inset * 2) // grid: ok bitmap rect
        let corner = rect.width * 0.225
        let path = CGPath(roundedRect: rect, cornerWidth: corner, cornerHeight: corner, transform: nil)

        ctx.saveGState()
        ctx.setShadow(offset: CGSize(width: 0, height: -dim * 0.02), blur: dim * 0.04, // grid: ok shadow
                      color: NSColor.black.withAlphaComponent(0.28).cgColor)
        ctx.addPath(path)
        ctx.fillPath()
        ctx.restoreGState()

        ctx.saveGState()
        ctx.addPath(path)
        ctx.clip()

        let base = MaterialRGB(hex: brandHex)
        let topColor = NSColor(
            srgbRed: min(CGFloat(base.r) / 255.0 + 0.14, 1.0),
            green: min(CGFloat(base.g) / 255.0 + 0.14, 1.0),
            blue: min(CGFloat(base.b) / 255.0 + 0.14, 1.0),
            alpha: 1.0
        )
        let bottomColor = NSColor(
            srgbRed: max(CGFloat(base.r) / 255.0 - 0.10, 0.0),
            green: max(CGFloat(base.g) / 255.0 - 0.10, 0.0),
            blue: max(CGFloat(base.b) / 255.0 - 0.10, 0.0),
            alpha: 1.0
        )
        let colors = [topColor.cgColor, bottomColor.cgColor] as CFArray
        if let gradient = CGGradient(colorsSpace: CGColorSpaceCreateDeviceRGB(), colors: colors, locations: [0.0, 1.0]) {
            ctx.drawLinearGradient(
                gradient,
                start: CGPoint(x: rect.midX, y: rect.maxY),
                end: CGPoint(x: rect.midX, y: rect.minY),
                options: []
            )
        }
        ctx.restoreGState()

        let pointSize = dim * 0.42
        let config = NSImage.SymbolConfiguration(pointSize: pointSize, weight: .semibold)
            .applying(.init(hierarchicalColor: .white))
        if let symbol = NSImage(systemSymbolName: symbolName, accessibilityDescription: nil)?.withSymbolConfiguration(config) {
            let symSize = symbol.size
            let drawRect = CGRect(
                x: (dim - symSize.width) * 0.5,
                y: (dim - symSize.height) * 0.5,
                width: symSize.width, // grid: ok symbol size
                height: symSize.height // grid: ok symbol size
            )
            symbol.draw(in: drawRect, from: .zero, operation: .sourceOver, fraction: 0.96)
        }
        return image
    }

    /// Renders a Google Pixel / Material You circular icon from `sourceImage` using `style`.
    static func renderRoundIcon(sourceImage: NSImage, style: RoundIconStyle, fallbackHex: UInt32 = 0x1A73E8, canvasSize: Int = 512) -> NSImage {
        let dim = CGFloat(canvasSize)
        guard let sourceCG = cgImage(from: sourceImage, targetSize: canvasSize) else {
            return sourceImage
        }

        let trimmedCG = style.autoTrim ? trimTransparentEdges(sourceCG) : sourceCG
        let dominant = extractColors(from: trimmedCG, fallbackHex: fallbackHex)

        let out = NSImage(size: NSSize(width: dim, height: dim)) // grid: ok bitmap canvas
        out.lockFocus()
        defer { out.unlockFocus() }

        guard let ctx = NSGraphicsContext.current?.cgContext else { return sourceImage }
        ctx.setAllowsAntialiasing(true)
        ctx.setShouldAntialias(true)
        ctx.interpolationQuality = .high

        // Modo adaptive icon specification:
        // Leave breathing room around the plate for the ambient + key drop shadow so it aligns with macOS Dock optical weight.
        let margin = dim * 0.075
        let circleRect = CGRect(x: margin, y: margin, width: dim - margin * 2.0, height: dim - margin * 2.0) // grid: ok canvas bounds
        let circlePath = shapePath(for: style.shape, in: circleRect)

        // 1. Outer elevation drop shadow
        if style.shadowIntensity > 0.01 {
            ctx.saveGState()
            let shadowAlpha = CGFloat(0.36 * style.shadowIntensity)
            let blur = dim * CGFloat(0.042 * style.shadowIntensity + 0.008)
            let yOffset = -dim * CGFloat(0.018 * style.shadowIntensity)
            ctx.setShadow(
                offset: CGSize(width: 0, height: yOffset), // grid: ok shadow offset
                blur: blur,
                color: NSColor.black.withAlphaComponent(shadowAlpha).cgColor
            )
            ctx.setFillColor(NSColor.white.cgColor)
            ctx.addPath(circlePath)
            ctx.fillPath()
            ctx.restoreGState()
        }

        // 2. Circular surface fill
        let (topFill, bottomFill) = surfaceColors(for: style, dominant: dominant.dominant, edge: dominant.edge)
        ctx.saveGState()
        ctx.addPath(circlePath)
        ctx.clip()

        let gradColors = [topFill.cgColor, bottomFill.cgColor] as CFArray
        if let gradient = CGGradient(colorsSpace: CGColorSpaceCreateDeviceRGB(), colors: gradColors, locations: [0.0, 1.0]) {
            ctx.drawLinearGradient(
                gradient,
                start: CGPoint(x: circleRect.midX, y: circleRect.maxY),
                end: CGPoint(x: circleRect.midX, y: circleRect.minY),
                options: []
            )
        }

        // 3. Foreground glyph placement inside the circular mask (supports >100% zoom, X/Y pan, and optical radius normalization)
        let opticalFactor: CGFloat = style.autoTrim
            ? min(max(0.50 / max(dominant.opticalRadiusNorm, 0.42), 0.72), 1.08)
            : 1.0
        var fgScale: CGFloat = CGFloat(style.iconScale) * opticalFactor
        if style.fullBleedSquares && dominant.isSquareLike {
            fgScale = max(fgScale, 1.36)
        }
        let maxGlyphSpan = circleRect.width * fgScale
        let srcW = CGFloat(trimmedCG.width)
        let srcH = CGFloat(trimmedCG.height)
        let aspect = srcW / max(srcH, 1.0)
        let drawW = aspect >= 1.0 ? maxGlyphSpan : maxGlyphSpan * aspect
        let drawH = aspect >= 1.0 ? maxGlyphSpan / aspect : maxGlyphSpan
        let shiftX = CGFloat(style.offsetX) * (circleRect.width * 0.5)
        let shiftY = CGFloat(style.offsetY) * (circleRect.height * 0.5)
        let glyphRect = CGRect(
            x: circleRect.midX + shiftX - drawW * 0.5,
            y: circleRect.midY + shiftY - drawH * 0.5,
            width: drawW, // grid: ok bitmap glyph
            height: drawH // grid: ok bitmap glyph
        )

        ctx.saveGState()
        if style.innerShadow && !(style.fullBleedSquares && dominant.isSquareLike) {
            ctx.setShadow(
                offset: CGSize(width: 0, height: -dim * 0.008), // grid: ok shadow
                blur: dim * 0.018,
                color: NSColor.black.withAlphaComponent(0.20).cgColor
            )
        }

        if style.monochromeTint && dominant.isSquareLike {
            // A full-bleed squircle icon has no transparent glyph to silhouette: a flat tint would
            // paint a solid block. Desaturate instead, so the artwork keeps its detail.
            ctx.draw(desaturated(trimmedCG), in: glyphRect)
        } else if style.monochromeTint {
            let tintRGB: MaterialRGB = switch style.backgroundStyle {
            case .darkMaterial: MaterialRGB(hex: 0xE6E6E8)
            case .customColor: MaterialRGB(hex: style.customHex).materialDeepInk()
            default: dominant.dominant.materialDeepInk()
            }
            let tintColor = NSColor(
                srgbRed: CGFloat(tintRGB.r) / 255.0,
                green: CGFloat(tintRGB.g) / 255.0,
                blue: CGFloat(tintRGB.b) / 255.0,
                alpha: 1.0
            )
            // Draw tinted silhouette using source alpha mask
            ctx.beginTransparencyLayer(auxiliaryInfo: nil)
            ctx.clip(to: glyphRect, mask: trimmedCG)
            ctx.setFillColor(tintColor.cgColor)
            ctx.fill(glyphRect)
            ctx.endTransparencyLayer()
        } else {
            ctx.draw(trimmedCG, in: glyphRect)
        }
        ctx.restoreGState()
        ctx.restoreGState() // End circle clip

        // 4. Crisp subtle rim highlight
        ctx.saveGState()
        let borderWidth = max(1.0, dim * 0.004)
        let rimColor: NSColor = style.backgroundStyle == .darkMaterial
            ? NSColor.white.withAlphaComponent(0.14)
            : NSColor.black.withAlphaComponent(0.08)
        ctx.setStrokeColor(rimColor.cgColor)
        ctx.setLineWidth(borderWidth)
        let insetRect = circleRect.insetBy(dx: borderWidth * 0.5, dy: borderWidth * 0.5)
        ctx.addPath(shapePath(for: style.shape, in: insetRect))
        ctx.strokePath()
        ctx.restoreGState()

        return out
    }

    private static let ciContext = CIContext(options: [.useSoftwareRenderer: false])

    /// Grayscale copy of `image` that keeps its alpha channel (transparent corners stay transparent).
    static func desaturated(_ image: CGImage) -> CGImage {
        let input = CIImage(cgImage: image)
        guard let filter = CIFilter(name: "CIColorControls") else { return image }
        filter.setValue(input, forKey: kCIInputImageKey)
        filter.setValue(0.0, forKey: kCIInputSaturationKey)
        guard let output = filter.outputImage,
              let result = ciContext.createCGImage(output, from: input.extent) else { return image }
        return result
    }

    /// Smooth superellipse |x|^n + |y|^n = 1 (n = 5 is close to the macOS app-icon outline).
    /// Curvature changes continuously, so there is no visible seam where a corner meets an edge.
    static func superellipsePath(in rect: CGRect, exponent n: Double) -> CGPath {
        let path = CGMutablePath()
        let steps = 256
        let power = 2.0 / n
        for i in 0..<steps {
            let t = 2.0 * Double.pi * Double(i) / Double(steps)
            let c = cos(t), s = sin(t)
            let x = (c < 0 ? -1.0 : 1.0) * pow(abs(c), power)
            let y = (s < 0 ? -1.0 : 1.0) * pow(abs(s), power)
            let pt = CGPoint(x: rect.midX + CGFloat(x) * rect.width * 0.5,
                             y: rect.midY + CGFloat(y) * rect.height * 0.5)
            if i == 0 { path.move(to: pt) } else { path.addLine(to: pt) }
        }
        path.closeSubpath()
        return path
    }

    /// Constructs the path geometry for the requested `IconShape`.
    static func shapePath(for shape: IconShape, in rect: CGRect) -> CGPath {
        switch shape {
        case .circle:
            return CGPath(ellipseIn: rect, transform: nil)
        case .squircle:
            return superellipsePath(in: rect, exponent: 5.0)
        case .roundedRect:
            let r = rect.width * 0.16
            return CGPath(roundedRect: rect, cornerWidth: r, cornerHeight: r, transform: nil)
        case .pebble:
            let r = rect.width * 0.38
            return CGPath(roundedRect: rect, cornerWidth: r, cornerHeight: r, transform: nil)
        case .hexagon:
            let path = CGMutablePath()
            let cx = rect.midX
            let cy = rect.midY
            let radius = min(rect.width, rect.height) * 0.5
            for i in 0..<6 {
                let angle = CGFloat(i) * CGFloat.pi / 3.0 - CGFloat.pi / 6.0
                let pt = CGPoint(x: cx + radius * cos(angle), y: cy + radius * sin(angle))
                if i == 0 {
                    path.move(to: pt)
                } else {
                    path.addLine(to: pt)
                }
            }
            path.closeSubpath()
            return path
        }
    }

    /// Applies the rendered circular icon to the target `.app` bundle or creates a desktop/folder alias if read-only.
    @discardableResult
    static func applyIcon(_ icon: NSImage, toAppAt bundlePath: String) -> Bool {
        guard !bundlePath.isEmpty, FileManager.default.fileExists(atPath: bundlePath) else {
            return true // Seeded/virtual sample app in test store
        }
        let applied = NSWorkspace.shared.setIcon(icon, forFile: bundlePath, options: [])
        if applied {
            NSWorkspace.shared.noteFileSystemChanged(bundlePath)
            return true
        }
        // Fallback for SIP/root-owned system apps: apply to an alias in ~/Applications/Round Icons/
        let fallbackDir = (NSHomeDirectory() as NSString).appendingPathComponent("Applications/Round Icons")
        try? FileManager.default.createDirectory(atPath: fallbackDir, withIntermediateDirectories: true)
        let appName = (bundlePath as NSString).lastPathComponent
        let aliasPath = (fallbackDir as NSString).appendingPathComponent(appName)
        if !FileManager.default.fileExists(atPath: aliasPath) {
            try? FileManager.default.createSymbolicLink(atPath: aliasPath, withDestinationPath: bundlePath)
        }
        let ok = NSWorkspace.shared.setIcon(icon, forFile: aliasPath, options: [])
        NSWorkspace.shared.noteFileSystemChanged(aliasPath)
        return ok
    }

    /// Restores the original macOS icon on the target `.app` bundle.
    @discardableResult
    static func restoreOriginalIcon(at bundlePath: String) -> Bool {
        guard !bundlePath.isEmpty, FileManager.default.fileExists(atPath: bundlePath) else {
            return true
        }
        let ok = NSWorkspace.shared.setIcon(nil, forFile: bundlePath, options: [])
        NSWorkspace.shared.noteFileSystemChanged(bundlePath)
        return ok
    }

    /// Encodes an `NSImage` as PNG data at the requested pixel dimension.
    static func pngData(from image: NSImage, pixelSize: Int) -> Data? {
        guard let cg = cgImage(from: image, targetSize: pixelSize) else { return nil }
        let rep = NSBitmapImageRep(cgImage: cg)
        rep.size = NSSize(width: pixelSize, height: pixelSize) // grid: ok bitmap size
        return rep.representation(using: .png, properties: [:])
    }

    /// Writes a multi-resolution `.icns`, `.ico`, and `1024×1024` `.png` pack for an app into `directory`.
    static func exportIconPack(
        for appTitle: String,
        sourceImage: NSImage,
        style: RoundIconStyle,
        fallbackHex: UInt32,
        to directory: URL
    ) throws -> [URL] {
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
        let safeName = ItemTitle.clean(appTitle)
            .replacingOccurrences(of: "/", with: "-")
            .replacingOccurrences(of: ":", with: "-")

        let master = renderRoundIcon(sourceImage: sourceImage, style: style, fallbackHex: fallbackHex, canvasSize: 1024)
        var written: [URL] = []

        // 1. 1024x1024 PNG
        let pngURL = directory.appendingPathComponent("\(safeName).png")
        if let png = pngData(from: master, pixelSize: 1024) {
            try png.write(to: pngURL, options: .atomic)
            written.append(pngURL)
        }

        // 2. Multi-size Windows .ico (16, 32, 48, 64, 128, 256)
        let icoSizes = [16, 32, 48, 64, 128, 256]
        var icoEntries: [ICOBuilder.Entry] = []
        for sz in icoSizes {
            if let data = pngData(from: master, pixelSize: sz) {
                icoEntries.append(.init(size: sz, pngData: data))
            }
        }
        let icoData = ICOBuilder.build(entries: icoEntries)
        let icoURL = directory.appendingPathComponent("\(safeName).ico")
        try icoData.write(to: icoURL, options: .atomic)
        written.append(icoURL)

        // 3. Native Apple .icns via CGImageDestination
        let icnsURL = directory.appendingPathComponent("\(safeName).icns")
        if let dest = CGImageDestinationCreateWithURL(icnsURL as CFURL, UTType.icns.identifier as CFString, 4, nil) {
            for sz in [32, 128, 256, 512, 1024] {
                if let cg = cgImage(from: master, targetSize: sz) {
                    CGImageDestinationAddImage(dest, cg, nil)
                }
            }
            if CGImageDestinationFinalize(dest) {
                written.append(icnsURL)
            }
        }

        return written
    }

    // MARK: - Private Bitmap Helpers

    private static func surfaceColors(
        for style: RoundIconStyle,
        dominant: MaterialRGB,
        edge: MaterialRGB
    ) -> (NSColor, NSColor) {
        switch style.backgroundStyle {
        case .googleWhite:
            return (
                NSColor(srgbRed: 1.0, green: 1.0, blue: 1.0, alpha: 1.0),
                NSColor(srgbRed: 0.955, green: 0.962, blue: 0.976, alpha: 1.0)
            )
        case .materialPastel:
            let top = dominant.materialPastel(whiteMix: 0.82)
            let bottom = dominant.materialPastel(whiteMix: 0.72)
            return (nsColor(top), nsColor(bottom))
        case .darkMaterial:
            return (
                NSColor(srgbRed: 0.155, green: 0.162, blue: 0.180, alpha: 1.0),
                NSColor(srgbRed: 0.105, green: 0.110, blue: 0.125, alpha: 1.0)
            )
        case .adaptiveEdge:
            let top = edge.materialPastel(whiteMix: 0.25)
            return (nsColor(top), nsColor(edge))
        case .customColor:
            let base = MaterialRGB(hex: style.customHex)
            let top = base.materialPastel(whiteMix: 0.18)
            return (nsColor(top), nsColor(base))
        }
    }

    private static func nsColor(_ rgb: MaterialRGB) -> NSColor {
        NSColor(
            srgbRed: CGFloat(rgb.r) / 255.0,
            green: CGFloat(rgb.g) / 255.0,
            blue: CGFloat(rgb.b) / 255.0,
            alpha: 1.0
        )
    }

    private static func cgImage(from image: NSImage, targetSize: Int) -> CGImage? {
        let dim = CGFloat(targetSize)
        var rect = CGRect(x: 0, y: 0, width: dim, height: dim) // grid: ok bitmap rect
        if let cg = image.cgImage(forProposedRect: &rect, context: nil, hints: [.interpolation: NSImageInterpolation.high]) {
            return cg
        }
        guard let ctx = CGContext(
            data: nil,
            width: targetSize, // grid: ok bitmap width
            height: targetSize, // grid: ok bitmap height
            bitsPerComponent: 8,
            bytesPerRow: targetSize * 4,
            space: CGColorSpaceCreateDeviceRGB(),
            bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue
        ) else { return nil }
        NSGraphicsContext.saveGraphicsState()
        NSGraphicsContext.current = NSGraphicsContext(cgContext: ctx, flipped: false)
        image.draw(in: rect)
        NSGraphicsContext.restoreGraphicsState()
        return ctx.makeImage()
    }

    private static func trimTransparentEdges(_ cgImage: CGImage) -> CGImage {
        let w = cgImage.width
        let h = cgImage.height
        guard w > 16, h > 16 else { return cgImage }
        var pixels = [UInt8](repeating: 0, count: w * h * 4)
        guard let ctx = CGContext(
            data: &pixels,
            width: w, // grid: ok bitmap width
            height: h, // grid: ok bitmap height
            bitsPerComponent: 8,
            bytesPerRow: w * 4,
            space: CGColorSpaceCreateDeviceRGB(),
            bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue
        ) else { return cgImage }
        ctx.draw(cgImage, in: CGRect(x: 0, y: 0, width: w, height: h)) // grid: ok bitmap rect

        var minX = w
        var minY = h
        var maxX = 0
        var maxY = 0
        var solidCount = 0

        // Strip soft outer drop shadows (alpha < 108) so icons with shadows have the exact same tight bounds
        for y in 0..<h {
            let rowOffset = y * w * 4
            for x in 0..<w {
                let a = pixels[rowOffset + x * 4 + 3]
                if a >= 108 {
                    if x < minX { minX = x }
                    if x > maxX { maxX = x }
                    if y < minY { minY = y }
                    if y > maxY { maxY = y }
                    solidCount += 1
                }
            }
        }

        if solidCount < 24 || minX >= maxX || minY >= maxY {
            minX = w; minY = h; maxX = 0; maxY = 0
            for y in 0..<h {
                let rowOffset = y * w * 4
                for x in 0..<w {
                    let a = pixels[rowOffset + x * 4 + 3]
                    if a > 28 {
                        if x < minX { minX = x }
                        if x > maxX { maxX = x }
                        if y < minY { minY = y }
                        if y > maxY { maxY = y }
                    }
                }
            }
        }

        guard minX < maxX, minY < maxY else { return cgImage }
        let cropRect = CGRect(x: minX, y: minY, width: maxX - minX + 1, height: maxY - minY + 1) // grid: ok crop rect
        return cgImage.cropping(to: cropRect) ?? cgImage
    }

    private static func extractColors(
        from cgImage: CGImage,
        fallbackHex: UInt32
    ) -> (dominant: MaterialRGB, edge: MaterialRGB, isSquareLike: Bool, opticalRadiusNorm: CGFloat) {
        let sampleSize = 64
        var pixels = [UInt8](repeating: 0, count: sampleSize * sampleSize * 4)
        guard let ctx = CGContext(
            data: &pixels,
            width: sampleSize, // grid: ok sample width
            height: sampleSize, // grid: ok sample height
            bitsPerComponent: 8,
            bytesPerRow: sampleSize * 4,
            space: CGColorSpaceCreateDeviceRGB(),
            bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue
        ) else {
            let fb = MaterialRGB(hex: fallbackHex)
            return (fb, fb, false, 0.50)
        }
        ctx.draw(cgImage, in: CGRect(x: 0, y: 0, width: sampleSize, height: sampleSize)) // grid: ok sample rect

        var totalR = 0.0
        var totalG = 0.0
        var totalB = 0.0
        var totalWeight = 0.0

        var edgeR = 0.0
        var edgeG = 0.0
        var edgeB = 0.0
        var edgeCount = 0.0

        var opaqueCount = 0
        var radii: [CGFloat] = []
        radii.reserveCapacity(sampleSize * sampleSize)
        let center = CGFloat(sampleSize - 1) * 0.5

        for y in 0..<sampleSize {
            for x in 0..<sampleSize {
                let idx = (y * sampleSize + x) * 4
                let a = pixels[idx + 3]
                guard a > 96 else { continue }
                opaqueCount += 1

                let dx = (CGFloat(x) - center) / CGFloat(sampleSize)
                let dy = (CGFloat(y) - center) / CGFloat(sampleSize)
                radii.append(hypot(dx, dy))

                let r = Double(pixels[idx])
                let g = Double(pixels[idx + 1])
                let b = Double(pixels[idx + 2])

                let maxC = max(r, g, b)
                let minC = min(r, g, b)
                let saturation = maxC > 0 ? (maxC - minC) / maxC : 0
                let weight = 0.25 + saturation * 2.5
                totalR += r * weight
                totalG += g * weight
                totalB += b * weight
                totalWeight += weight

                if x < 6 || x >= sampleSize - 6 || y < 6 || y >= sampleSize - 6 {
                    edgeR += r
                    edgeG += g
                    edgeB += b
                    edgeCount += 1.0
                }
            }
        }

        let dominant: MaterialRGB
        if totalWeight > 0 {
            dominant = MaterialRGB(
                r: UInt8(clamping: Int((totalR / totalWeight).rounded())),
                g: UInt8(clamping: Int((totalG / totalWeight).rounded())),
                b: UInt8(clamping: Int((totalB / totalWeight).rounded()))
            )
        } else {
            dominant = MaterialRGB(hex: fallbackHex)
        }

        let edge: MaterialRGB
        if edgeCount > 0 {
            edge = MaterialRGB(
                r: UInt8(clamping: Int((edgeR / edgeCount).rounded())),
                g: UInt8(clamping: Int((edgeG / edgeCount).rounded())),
                b: UInt8(clamping: Int((edgeB / edgeCount).rounded()))
            )
        } else {
            edge = dominant
        }

        var opticalRadius: CGFloat = 0.50
        if !radii.isEmpty {
            radii.sort()
            let idx96 = min(radii.count - 1, (radii.count * 96) / 100)
            opticalRadius = min(max(radii[idx96], 0.42), 0.70)
        }

        let fillRatio = Double(opaqueCount) / Double(sampleSize * sampleSize)
        return (dominant, edge, fillRatio > 0.72, opticalRadius)
    }
}
