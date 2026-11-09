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

        // Apply Surface Material Shader Pass (Glass, Titanium, Neumorphic, Prism)
        drawSurfaceMaterial(in: ctx, rect: circleRect, path: circlePath, material: style.surfaceMaterial)

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
        // Apply glyph transformations (rotation and mirroring) around glyph center
        let centerX = glyphRect.midX
        let centerY = glyphRect.midY
        ctx.translateBy(x: centerX, y: centerY)
        if style.glyphRotation != 0.0 {
            let rad = CGFloat(style.glyphRotation * .pi / 180.0)
            ctx.rotate(by: rad)
        }
        let scaleX: CGFloat = style.glyphFlipHorizontal ? -1.0 : 1.0
        let scaleY: CGFloat = style.glyphFlipVertical ? -1.0 : 1.0
        if scaleX != 1.0 || scaleY != 1.0 {
            ctx.scaleBy(x: scaleX, y: scaleY)
        }
        ctx.translateBy(x: -centerX, y: -centerY)

        if style.innerShadow && !(style.fullBleedSquares && dominant.isSquareLike) {
            ctx.setShadow(
                offset: CGSize(width: 0, height: -dim * 0.008), // grid: ok shadow
                blur: dim * 0.018,
                color: NSColor.black.withAlphaComponent(0.20).cgColor
            )
        }

        let effMode = effectiveAppearanceMode(for: style)

        if let grad = style.glyphGradientPreset {
            // Dual-color gradient mask across the foreground glyph
            let (startHex, endHex) = grad.colors
            let startCol = NSColor(
                srgbRed: CGFloat((startHex >> 16) & 0xFF) / 255.0,
                green: CGFloat((startHex >> 8) & 0xFF) / 255.0,
                blue: CGFloat(startHex & 0xFF) / 255.0,
                alpha: 1.0
            )
            let endCol = NSColor(
                srgbRed: CGFloat((endHex >> 16) & 0xFF) / 255.0,
                green: CGFloat((endHex >> 8) & 0xFF) / 255.0,
                blue: CGFloat(endHex & 0xFF) / 255.0,
                alpha: 1.0
            )
            let colorSpace = CGColorSpaceCreateDeviceRGB()
            let cgColors = [startCol.cgColor, endCol.cgColor] as CFArray
            if let gradient = CGGradient(colorsSpace: colorSpace, colors: cgColors, locations: [0.0, 1.0]) {
                ctx.beginTransparencyLayer(auxiliaryInfo: nil)
                ctx.clip(to: glyphRect, mask: trimmedCG)
                let p1 = CGPoint(x: glyphRect.minX, y: glyphRect.maxY)
                let p2 = CGPoint(x: glyphRect.maxX, y: glyphRect.minY)
                ctx.drawLinearGradient(gradient, start: p1, end: p2, options: [])
                ctx.endTransparencyLayer()
            } else {
                ctx.draw(trimmedCG, in: glyphRect)
            }
        } else if let customColor = style.glyphCustomColorHex {
            // Flat custom color tint across the foreground glyph
            let tintCol = NSColor(
                srgbRed: CGFloat((customColor >> 16) & 0xFF) / 255.0,
                green: CGFloat((customColor >> 8) & 0xFF) / 255.0,
                blue: CGFloat(customColor & 0xFF) / 255.0,
                alpha: 1.0
            )
            ctx.beginTransparencyLayer(auxiliaryInfo: nil)
            ctx.clip(to: glyphRect, mask: trimmedCG)
            ctx.setFillColor(tintCol.cgColor)
            ctx.fill(glyphRect)
            ctx.endTransparencyLayer()
        } else if effMode == .tinted {
            // macOS Sequoia / iOS 18 Tinted Silhouette
            let tintColor = NSColor(
                srgbRed: CGFloat((style.tintColorHex >> 16) & 0xFF) / 255.0,
                green: CGFloat((style.tintColorHex >> 8) & 0xFF) / 255.0,
                blue: CGFloat(style.tintColorHex & 0xFF) / 255.0,
                alpha: 1.0
            )
            if dominant.isSquareLike {
                ctx.draw(desaturated(trimmedCG), in: glyphRect)
            } else {
                ctx.beginTransparencyLayer(auxiliaryInfo: nil)
                ctx.clip(to: glyphRect, mask: trimmedCG)
                ctx.setFillColor(tintColor.cgColor)
                ctx.fill(glyphRect)
                ctx.endTransparencyLayer()
            }
        } else if style.monochromeTint && dominant.isSquareLike {
            // A full-bleed squircle icon has no transparent glyph to silhouette: a flat tint would
            // paint a solid block. Desaturate instead, so the artwork keeps its detail.
            ctx.draw(desaturated(trimmedCG), in: glyphRect)
        } else if style.monochromeTint {
            let tintRGB: MaterialRGB = switch (effMode == .dark ? RoundBackgroundStyle.darkMaterial : style.backgroundStyle) {
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
            drawGlyph(
                in: ctx,
                image: trimmedCG,
                rect: glyphRect,
                canvasDim: dim,
                style: style.glyphStyle,
                palette: dominant.palette,
                dominantRGB: dominant.dominant,
                isSquareLike: dominant.isSquareLike
            )
        }
        ctx.restoreGState()
        ctx.restoreGState() // End circle clip

        // 4. Crisp subtle rim highlight
        ctx.saveGState()
        let borderWidth = max(1.0, dim * 0.004)
        let isDarkish = (effMode == .dark || effMode == .tinted || style.backgroundStyle == .darkMaterial)
        let rimColor: NSColor = isDarkish
            ? NSColor.white.withAlphaComponent(0.16)
            : NSColor.black.withAlphaComponent(0.08)
        ctx.setStrokeColor(rimColor.cgColor)
        ctx.setLineWidth(borderWidth)
        let insetRect = circleRect.insetBy(dx: borderWidth * 0.5, dy: borderWidth * 0.5)
        ctx.addPath(shapePath(for: style.shape, in: insetRect))
        ctx.strokePath()
        ctx.restoreGState()

        // 5. Status / modifier badge overlay
        drawGlyphBadge(in: ctx, badge: style.glyphBadge, position: style.glyphBadgePosition, canvasDim: dim, circleRect: circleRect)

        return out
    }

    /// Renders a status or modifier badge overlay on top of the icon artwork.
    static func drawGlyphBadge(
        in ctx: CGContext,
        badge: GlyphBadgeOverlay,
        position: GlyphBadgePosition,
        canvasDim dim: CGFloat,
        circleRect: CGRect
    ) {
        guard badge != .none else { return }

        let r = dim * 0.088
        let isPill = (badge == .beta || badge == .pro)
        let pillW = isPill ? (badge == .beta ? r * 2.35 : r * 2.05) : r * 2.0
        let pillH = r * 2.0

        let cx: CGFloat
        let cy: CGFloat
        let marginX = isPill ? pillW * 0.65 : r * 1.35
        let marginY = r * 1.35

        switch position {
        case .topRight:
            cx = circleRect.maxX - marginX
            cy = circleRect.maxY - marginY
        case .bottomRight:
            cx = circleRect.maxX - marginX
            cy = circleRect.minY + marginY
        case .topLeft:
            cx = circleRect.minX + marginX
            cy = circleRect.maxY - marginY
        case .bottomLeft:
            cx = circleRect.minX + marginX
            cy = circleRect.minY + marginY
        }

        ctx.saveGState()

        // 1. Drop shadow
        ctx.setShadow(
            offset: CGSize(width: 0, height: -dim * 0.008),
            blur: dim * 0.022,
            color: NSColor.black.withAlphaComponent(0.38).cgColor
        )

        let badgeRect = CGRect(x: cx - pillW * 0.5, y: cy - pillH * 0.5, width: pillW, height: pillH)
        let badgeCorner = isPill ? pillH * 0.5 : r
        let badgePath = CGPath(roundedRect: badgeRect, cornerWidth: badgeCorner, cornerHeight: badgeCorner, transform: nil)

        // 2. High-contrast outer stroke / ring (white 2.5pt)
        let strokeW = max(2.0, dim * 0.012)
        ctx.saveGState()
        ctx.addPath(badgePath)
        ctx.setStrokeColor(NSColor.white.cgColor)
        ctx.setLineWidth(strokeW)
        ctx.strokePath()
        ctx.restoreGState()

        // 3. Solid badge fill
        let fillColor: NSColor = switch badge {
        case .none: .clear
        case .notificationDot: NSColor(srgbRed: 0.98, green: 0.23, blue: 0.19, alpha: 1.0)
        case .checkmark: NSColor(srgbRed: 0.20, green: 0.78, blue: 0.35, alpha: 1.0)
        case .star: NSColor(srgbRed: 1.00, green: 0.60, blue: 0.00, alpha: 1.0)
        case .lock: NSColor(srgbRed: 0.15, green: 0.20, blue: 0.28, alpha: 1.0)
        case .alert: NSColor(srgbRed: 0.98, green: 0.45, blue: 0.09, alpha: 1.0)
        case .beta: NSColor(srgbRed: 0.55, green: 0.36, blue: 0.96, alpha: 1.0)
        case .pro: NSColor(srgbRed: 0.01, green: 0.52, blue: 0.78, alpha: 1.0)
        }

        ctx.setFillColor(fillColor.cgColor)
        ctx.addPath(badgePath)
        ctx.fillPath()

        // 4. Emblem glyph or text inside badge
        ctx.setShadow(offset: .zero, blur: 0, color: nil)
        switch badge {
        case .none, .notificationDot:
            break

        case .checkmark:
            ctx.setStrokeColor(NSColor.white.cgColor)
            ctx.setLineWidth(r * 0.26)
            ctx.setLineCap(.round)
            ctx.setLineJoin(.round)
            ctx.beginPath()
            ctx.move(to: CGPoint(x: cx - r * 0.42, y: cy - r * 0.04))
            ctx.addLine(to: CGPoint(x: cx - r * 0.10, y: cy - r * 0.36))
            ctx.addLine(to: CGPoint(x: cx + r * 0.42, y: cy + r * 0.30))
            ctx.strokePath()

        case .star:
            ctx.setFillColor(NSColor.white.cgColor)
            let starPath = CGMutablePath()
            let points = 5
            let outerR = r * 0.52
            let innerR = outerR * 0.45
            for i in 0..<(points * 2) {
                let radius = (i % 2 == 0) ? outerR : innerR
                let angle = CGFloat(i) * .pi / CGFloat(points) - .pi / 2.0
                let px = cx + radius * cos(angle)
                let py = cy + radius * sin(angle)
                if i == 0 {
                    starPath.move(to: CGPoint(x: px, y: py))
                } else {
                    starPath.addLine(to: CGPoint(x: px, y: py))
                }
            }
            starPath.closeSubpath()
            ctx.addPath(starPath)
            ctx.fillPath()

        case .lock:
            ctx.setFillColor(NSColor.white.cgColor)
            ctx.setStrokeColor(NSColor.white.cgColor)
            let bodyW = r * 0.82
            let bodyH = r * 0.65
            let bodyRect = CGRect(x: cx - bodyW * 0.5, y: cy - bodyH * 0.85, width: bodyW, height: bodyH)
            let bodyPath = CGPath(roundedRect: bodyRect, cornerWidth: bodyH * 0.25, cornerHeight: bodyH * 0.25, transform: nil)
            ctx.addPath(bodyPath)
            ctx.fillPath()

            ctx.setLineWidth(r * 0.18)
            ctx.setLineCap(.round)
            ctx.beginPath()
            let shackleR = bodyW * 0.32
            ctx.addArc(center: CGPoint(x: cx, y: bodyRect.maxY - 1), radius: shackleR, startAngle: 0, endAngle: .pi, clockwise: false)
            ctx.strokePath()

        case .alert:
            ctx.setStrokeColor(NSColor.white.cgColor)
            ctx.setLineWidth(r * 0.24)
            ctx.setLineCap(.round)
            ctx.beginPath()
            ctx.move(to: CGPoint(x: cx, y: cy + r * 0.42))
            ctx.addLine(to: CGPoint(x: cx, y: cy - r * 0.05))
            ctx.strokePath()

            ctx.setFillColor(NSColor.white.cgColor)
            let dotRect = CGRect(x: cx - r * 0.12, y: cy - r * 0.42, width: r * 0.24, height: r * 0.24)
            ctx.fillEllipse(in: dotRect)

        case .beta, .pro:
            let text = badge == .beta ? "BETA" : "PRO"
            let fontSize = r * 0.84
            let font = NSFont.systemFont(ofSize: fontSize, weight: .bold)
            let attrs: [NSAttributedString.Key: Any] = [
                .font: font,
                .foregroundColor: NSColor.white
            ]
            let str = NSAttributedString(string: text, attributes: attrs)
            let strSize = str.size()
            let textRect = CGRect(
                x: cx - strSize.width * 0.5,
                y: cy - strSize.height * 0.5,
                width: strSize.width,
                height: strSize.height
            )
            str.draw(in: textRect)
        }

        ctx.restoreGState()
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

    /// Outline of `shape` inside `rect` (geometry lives in ModoKit so it is unit-tested).
    static func shapePath(for shape: IconShape, in rect: CGRect) -> CGPath {
        shape.path(in: rect)
    }

    /// Result of attempting to apply an icon to a target application or bundle.
    enum IconApplyStatus: Sendable, Hashable {
        case directSuccess
        case sipProtectedSystemApp(aliasPath: String)
        case aliasCreated(aliasPath: String)
        case permissionDenied(reason: String)
        case failure(reason: String)

        var isSuccess: Bool {
            switch self {
            case .directSuccess, .sipProtectedSystemApp, .aliasCreated:
                return true
            case .permissionDenied, .failure:
                return false
            }
        }
    }

    /// Invalidates macOS LaunchServices cache and refreshes Dock & Finder icon representation.
    static func flushSystemDockAndFinder() {
        guard !Store.isOverridden else { return }
        let killDock = Process()
        killDock.executableURL = URL(fileURLWithPath: "/usr/bin/killall")
        killDock.arguments = ["Dock"]
        try? killDock.run()
    }

    /// Updates bundle timestamps and asks LaunchServices and Finder to re-index the application icon.
    static func notifyLaunchServices(for bundlePath: String) {
        guard !Store.isOverridden, !bundlePath.isEmpty else { return }
        let fm = FileManager.default
        let now = Date()
        try? fm.setAttributes([.modificationDate: now], ofItemAtPath: bundlePath)

        let infoPlist = (bundlePath as NSString).appendingPathComponent("Contents/Info.plist")
        if fm.fileExists(atPath: infoPlist) {
            try? fm.setAttributes([.modificationDate: now], ofItemAtPath: infoPlist)
        }

        let lsregisterPaths = [
            "/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/LaunchServices.framework/Versions/A/Support/lsregister",
            "/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister"
        ]
        for path in lsregisterPaths {
            if fm.fileExists(atPath: path) {
                let task = Process()
                task.executableURL = URL(fileURLWithPath: path)
                task.arguments = ["-f", "-R", bundlePath]
                try? task.run()
                break
            }
        }

        // Inform Finder to refresh its cached view for this file
        let escaped = bundlePath.replacingOccurrences(of: "\"", with: "\\\"")
        let finderScript = "tell application \"Finder\" to update item (POSIX file \"\(escaped)\" as alias)"
        if let script = NSAppleScript(source: finderScript) {
            var error: NSDictionary?
            script.executeAndReturnError(&error)
        }

        NSWorkspace.shared.noteFileSystemChanged(bundlePath)
    }

    /// Requests admin privileges via AppleScript to make a root-owned /Applications bundle writable.
    static func requestAdminWriteAccess(for bundlePath: String) -> Bool {
        guard !Store.isOverridden, !bundlePath.isEmpty else { return false }
        let escaped = bundlePath.replacingOccurrences(of: "'", with: "'\\''")
        let script = "do shell script \"chmod -R a+w '\(escaped)'\" with administrator privileges"
        var error: NSDictionary?
        if let appleScript = NSAppleScript(source: script) {
            appleScript.executeAndReturnError(&error)
            return error == nil && FileManager.default.isWritableFile(atPath: bundlePath)
        }
        return false
    }

    /// Applies the rendered circular icon to the target `.app` bundle or creates a desktop/folder alias if read-only.
    @discardableResult
    static func applyIcon(_ icon: NSImage, toAppAt bundlePath: String) -> Bool {
        applyIconWithDetails(icon, toAppAt: bundlePath, allowAdminPrompt: false).isSuccess
    }

    /// Detailed icon application with SIP detection, administrator privilege escalation, and cache flush.
    static func applyIconWithDetails(
        _ icon: NSImage,
        toAppAt bundlePath: String,
        allowAdminPrompt: Bool = true
    ) -> IconApplyStatus {
        guard !bundlePath.isEmpty, FileManager.default.fileExists(atPath: bundlePath) else {
            return .directSuccess // Seeded/virtual sample app in test store
        }

        let isSystemApp = bundlePath.hasPrefix("/System/")

        // 1. System apps in /System are locked by macOS APFS Sealed System Volume & SIP
        if isSystemApp {
            let aliasPath = createRoundIconsAlias(for: bundlePath, icon: icon)
            return .sipProtectedSystemApp(aliasPath: aliasPath)
        }

        // 2. Check if writable; if owned by root in /Applications, attempt elevation if allowed
        if !FileManager.default.isWritableFile(atPath: bundlePath) {
            if allowAdminPrompt {
                _ = requestAdminWriteAccess(for: bundlePath)
            }
        }

        // 3. Attempt direct application on bundle
        if FileManager.default.isWritableFile(atPath: bundlePath) {
            let applied = NSWorkspace.shared.setIcon(icon, forFile: bundlePath, options: [])
            if applied {
                notifyLaunchServices(for: bundlePath)
                flushSystemDockAndFinder()
                return .directSuccess
            }
        }

        // 4. Fallback for read-only non-system bundles (user declined admin prompt)
        let aliasPath = createRoundIconsAlias(for: bundlePath, icon: icon)
        return .aliasCreated(aliasPath: aliasPath)
    }

    private static func createRoundIconsAlias(for bundlePath: String, icon: NSImage) -> String {
        let fallbackDir = (NSHomeDirectory() as NSString).appendingPathComponent("Applications/Round Icons")
        try? FileManager.default.createDirectory(atPath: fallbackDir, withIntermediateDirectories: true)
        let appName = (bundlePath as NSString).lastPathComponent
        let aliasPath = (fallbackDir as NSString).appendingPathComponent(appName)
        if !FileManager.default.fileExists(atPath: aliasPath) {
            try? FileManager.default.createSymbolicLink(atPath: aliasPath, withDestinationPath: bundlePath)
        }
        _ = NSWorkspace.shared.setIcon(icon, forFile: aliasPath, options: [])
        notifyLaunchServices(for: aliasPath)
        return aliasPath
    }

    /// Restores the original macOS icon on the target `.app` bundle.
    @discardableResult
    static func restoreOriginalIcon(at bundlePath: String) -> Bool {
        guard !bundlePath.isEmpty, FileManager.default.fileExists(atPath: bundlePath) else {
            return true
        }
        let ok = NSWorkspace.shared.setIcon(nil, forFile: bundlePath, options: [])
        notifyLaunchServices(for: bundlePath)
        flushSystemDockAndFinder()
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

    /// Exports PNG icons in specific pixel resolutions (e.g. 16, 32, 48, 64, 128, 256, 512, 1024) into `directory`.
    static func exportPNGSizes(
        for appTitle: String,
        sourceImage: NSImage,
        style: RoundIconStyle,
        fallbackHex: UInt32,
        sizes: [Int],
        to directory: URL
    ) throws -> [URL] {
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
        let safeName = ItemTitle.clean(appTitle)
            .replacingOccurrences(of: "/", with: "-")
            .replacingOccurrences(of: ":", with: "-")

        let master = renderRoundIcon(sourceImage: sourceImage, style: style, fallbackHex: fallbackHex, canvasSize: 1024)
        var written: [URL] = []

        for sz in sizes.sorted() {
            guard sz >= 16 && sz <= 2048 else { continue }
            let fileURL = directory.appendingPathComponent("\(safeName)-\(sz)x\(sz).png")
            if let png = pngData(from: master, pixelSize: sz) {
                try png.write(to: fileURL, options: .atomic)
                written.append(fileURL)
            }
        }
        return written
    }

    /// Exports Light, Dark, and Tinted PNG variants for `appTitle` into `directory` (macOS Sequoia & iOS 18 style).
    static func exportAppearanceVariants(
        for appTitle: String,
        sourceImage: NSImage,
        baseStyle: RoundIconStyle,
        fallbackHex: UInt32,
        to directory: URL
    ) throws -> [URL] {
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
        let safeName = ItemTitle.clean(appTitle)
            .replacingOccurrences(of: "/", with: "-")
            .replacingOccurrences(of: ":", with: "-")

        var written: [URL] = []
        let variants: [(String, IconAppearanceMode)] = [
            ("Light", .light),
            ("Dark", .dark),
            ("Tinted", .tinted)
        ]

        for (suffix, mode) in variants {
            var s = baseStyle
            s.appearanceMode = mode
            let rendered = renderRoundIcon(sourceImage: sourceImage, style: s, fallbackHex: fallbackHex, canvasSize: 1024)
            let fileURL = directory.appendingPathComponent("\(safeName)-\(suffix).png")
            if let png = pngData(from: rendered, pixelSize: 1024) {
                try png.write(to: fileURL, options: .atomic)
                written.append(fileURL)
            }
        }
        return written
    }

    // MARK: - Private Bitmap Helpers

    static func effectiveAppearanceMode(for style: RoundIconStyle) -> IconAppearanceMode {
        if style.appearanceMode == .system {
            let isDark = NSApp?.effectiveAppearance.bestMatch(from: [.darkAqua, .aqua]) == .darkAqua
            return isDark ? .dark : .light
        }
        return style.appearanceMode
    }

    private static func surfaceColors(
        for style: RoundIconStyle,
        dominant: MaterialRGB,
        edge: MaterialRGB
    ) -> (NSColor, NSColor) {
        let mode = effectiveAppearanceMode(for: style)
        switch mode {
        case .dark:
            // macOS Sequoia / iOS 18 Dark Icon slate plate
            return (
                NSColor(srgbRed: 0.145, green: 0.148, blue: 0.160, alpha: 1.0),
                NSColor(srgbRed: 0.082, green: 0.084, blue: 0.092, alpha: 1.0)
            )
        case .tinted:
            // Dual-tone dark plate tinted with user-chosen hue
            let tint = MaterialRGB(hex: style.tintColorHex)
            let topTinted = NSColor(
                srgbRed: 0.08 + (CGFloat(tint.r) / 255.0) * 0.12,
                green: 0.08 + (CGFloat(tint.g) / 255.0) * 0.12,
                blue: 0.09 + (CGFloat(tint.b) / 255.0) * 0.12,
                alpha: 1.0
            )
            let bottomTinted = NSColor(
                srgbRed: 0.04 + (CGFloat(tint.r) / 255.0) * 0.06,
                green: 0.04 + (CGFloat(tint.g) / 255.0) * 0.06,
                blue: 0.05 + (CGFloat(tint.b) / 255.0) * 0.06,
                alpha: 1.0
            )
            return (topTinted, bottomTinted)
        case .light, .system:
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
            case .wallpaperDynamic:
                let pal = WallpaperPaletteEngine.currentWallpaperPalette()
                let pastelRGB = MaterialRGB(hex: pal.pastelHex)
                let top = pastelRGB.materialPastel(whiteMix: 0.12)
                return (nsColor(top), nsColor(pastelRGB))
            }
        }
    }

    private static func drawSurfaceMaterial(
        in ctx: CGContext,
        rect: CGRect,
        path: CGPath,
        material: SurfaceMaterial
    ) {
        guard material != .smooth else { return }
        ctx.saveGState()

        switch material {
        case .smooth:
            break

        case .frostedGlass:
            // 1. Diagonal specular sweep (translucent white highlight)
            let sweepColors = [
                CGColor(srgbRed: 1.0, green: 1.0, blue: 1.0, alpha: 0.32),
                CGColor(srgbRed: 1.0, green: 1.0, blue: 1.0, alpha: 0.08),
                CGColor(srgbRed: 1.0, green: 1.0, blue: 1.0, alpha: 0.0)
            ] as CFArray
            if let sweepGrad = CGGradient(colorsSpace: CGColorSpaceCreateDeviceRGB(), colors: sweepColors, locations: [0.0, 0.45, 1.0]) {
                ctx.drawLinearGradient(
                    sweepGrad,
                    start: CGPoint(x: rect.minX, y: rect.maxY),
                    end: CGPoint(x: rect.maxX, y: rect.minY),
                    options: []
                )
            }
            // 2. Translucent top edge glow
            let topGlow = CGPath(roundedRect: CGRect(x: rect.minX, y: rect.midY, width: rect.width, height: rect.height * 0.5), cornerWidth: rect.width * 0.1, cornerHeight: rect.width * 0.1, transform: nil)
            ctx.addPath(topGlow)
            ctx.setFillColor(CGColor(srgbRed: 1.0, green: 1.0, blue: 1.0, alpha: 0.08))
            ctx.fillPath()

        case .brushedTitanium:
            // Multi-reflection metallic sheen
            let metalColors = [
                CGColor(srgbRed: 1.0, green: 1.0, blue: 1.0, alpha: 0.16),
                CGColor(srgbRed: 0.0, green: 0.0, blue: 0.0, alpha: 0.12),
                CGColor(srgbRed: 1.0, green: 1.0, blue: 1.0, alpha: 0.22),
                CGColor(srgbRed: 0.0, green: 0.0, blue: 0.0, alpha: 0.14),
                CGColor(srgbRed: 1.0, green: 1.0, blue: 1.0, alpha: 0.10)
            ] as CFArray
            if let metalGrad = CGGradient(colorsSpace: CGColorSpaceCreateDeviceRGB(), colors: metalColors, locations: [0.0, 0.28, 0.52, 0.78, 1.0]) {
                ctx.drawLinearGradient(
                    metalGrad,
                    start: CGPoint(x: rect.minX, y: rect.maxY),
                    end: CGPoint(x: rect.maxX, y: rect.minY),
                    options: []
                )
            }
            // Fine horizontal hairline grain
            ctx.setStrokeColor(CGColor(gray: 1.0, alpha: 0.04))
            ctx.setLineWidth(1.0)
            var y = rect.minY + 4
            while y < rect.maxY {
                ctx.move(to: CGPoint(x: rect.minX, y: y))
                ctx.addLine(to: CGPoint(x: rect.maxX, y: y))
                y += 6
            }
            ctx.strokePath()

        case .neumorphic:
            // Soft tactile dual shading: top-left highlight + bottom-right shade
            let innerHighlight = [
                CGColor(srgbRed: 1.0, green: 1.0, blue: 1.0, alpha: 0.28),
                CGColor(srgbRed: 1.0, green: 1.0, blue: 1.0, alpha: 0.0),
                CGColor(srgbRed: 0.0, green: 0.0, blue: 0.0, alpha: 0.22)
            ] as CFArray
            if let neuGrad = CGGradient(colorsSpace: CGColorSpaceCreateDeviceRGB(), colors: innerHighlight, locations: [0.0, 0.5, 1.0]) {
                ctx.drawLinearGradient(
                    neuGrad,
                    start: CGPoint(x: rect.minX, y: rect.maxY),
                    end: CGPoint(x: rect.maxX, y: rect.minY),
                    options: []
                )
            }

        case .prismGlow:
            // Holographic spectral dispersion
            let rainbow = [
                CGColor(srgbRed: 0.95, green: 0.35, blue: 0.45, alpha: 0.18),
                CGColor(srgbRed: 0.95, green: 0.75, blue: 0.25, alpha: 0.16),
                CGColor(srgbRed: 0.30, green: 0.85, blue: 0.55, alpha: 0.18),
                CGColor(srgbRed: 0.25, green: 0.65, blue: 0.95, alpha: 0.20),
                CGColor(srgbRed: 0.75, green: 0.35, blue: 0.95, alpha: 0.18)
            ] as CFArray
            if let rainbowGrad = CGGradient(colorsSpace: CGColorSpaceCreateDeviceRGB(), colors: rainbow, locations: [0.0, 0.25, 0.50, 0.75, 1.0]) {
                ctx.drawLinearGradient(
                    rainbowGrad,
                    start: CGPoint(x: rect.minX, y: rect.minY),
                    end: CGPoint(x: rect.maxX, y: rect.maxY),
                    options: []
                )
            }

        case .liquidChrome:
            // High dynamic range liquid metal specular reflection
            let chromeColors = [
                CGColor(srgbRed: 0.95, green: 0.97, blue: 1.0, alpha: 0.40),
                CGColor(srgbRed: 0.45, green: 0.48, blue: 0.55, alpha: 0.15),
                CGColor(srgbRed: 0.15, green: 0.18, blue: 0.25, alpha: 0.35),
                CGColor(srgbRed: 0.90, green: 0.94, blue: 1.0, alpha: 0.50),
                CGColor(srgbRed: 0.30, green: 0.33, blue: 0.40, alpha: 0.20),
                CGColor(srgbRed: 0.98, green: 0.99, blue: 1.0, alpha: 0.35)
            ] as CFArray
            if let chromeGrad = CGGradient(colorsSpace: CGColorSpaceCreateDeviceRGB(), colors: chromeColors, locations: [0.0, 0.22, 0.45, 0.65, 0.85, 1.0]) {
                ctx.drawLinearGradient(
                    chromeGrad,
                    start: CGPoint(x: rect.minX, y: rect.maxY),
                    end: CGPoint(x: rect.maxX, y: rect.minY),
                    options: []
                )
            }
            // Curved specular liquid highlight arc
            let chromeArc = CGMutablePath()
            chromeArc.move(to: CGPoint(x: rect.minX, y: rect.midY))
            chromeArc.addQuadCurve(to: CGPoint(x: rect.maxX, y: rect.midY + rect.height * 0.15), control: CGPoint(x: rect.midX, y: rect.maxY * 0.95))
            ctx.setStrokeColor(CGColor(srgbRed: 1.0, green: 1.0, blue: 1.0, alpha: 0.30))
            ctx.setLineWidth(max(1.5, rect.width * 0.008))
            ctx.addPath(chromeArc)
            ctx.strokePath()

        case .matteCeramic:
            // Soft diffuse subsurface lighting + velvety matte sheen
            let ceramicSheen = [
                CGColor(srgbRed: 1.0, green: 1.0, blue: 1.0, alpha: 0.22),
                CGColor(srgbRed: 1.0, green: 0.98, blue: 0.95, alpha: 0.08),
                CGColor(srgbRed: 0.90, green: 0.90, blue: 0.92, alpha: 0.0),
                CGColor(srgbRed: 0.0, green: 0.0, blue: 0.0, alpha: 0.08)
            ] as CFArray
            if let ceramicGrad = CGGradient(colorsSpace: CGColorSpaceCreateDeviceRGB(), colors: ceramicSheen, locations: [0.0, 0.35, 0.70, 1.0]) {
                ctx.drawLinearGradient(
                    ceramicGrad,
                    start: CGPoint(x: rect.midX, y: rect.maxY),
                    end: CGPoint(x: rect.midX, y: rect.minY),
                    options: []
                )
            }

        case .holographic:
            // Multi-angle iridescent rainbow prism foil
            let holoColors = [
                CGColor(srgbRed: 0.35, green: 0.85, blue: 1.0, alpha: 0.25),
                CGColor(srgbRed: 0.95, green: 0.40, blue: 0.85, alpha: 0.28),
                CGColor(srgbRed: 1.0, green: 0.85, blue: 0.35, alpha: 0.22),
                CGColor(srgbRed: 0.40, green: 0.95, blue: 0.65, alpha: 0.25),
                CGColor(srgbRed: 0.65, green: 0.45, blue: 1.0, alpha: 0.28)
            ] as CFArray
            if let holoGrad = CGGradient(colorsSpace: CGColorSpaceCreateDeviceRGB(), colors: holoColors, locations: [0.0, 0.25, 0.50, 0.75, 1.0]) {
                ctx.drawLinearGradient(
                    holoGrad,
                    start: CGPoint(x: rect.minX, y: rect.minY),
                    end: CGPoint(x: rect.maxX, y: rect.maxY),
                    options: []
                )
            }
        }

        ctx.restoreGState()
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
    ) -> (dominant: MaterialRGB, edge: MaterialRGB, isSquareLike: Bool, opticalRadiusNorm: CGFloat, palette: BrandPalette) {
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
            return (fb, fb, false, 0.50, ColorPaletteEngine.defaultPalette(fallbackHex: fallbackHex))
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
        var sampleRGBAs: [ColorPaletteEngine.RGBA] = []
        sampleRGBAs.reserveCapacity(sampleSize * sampleSize)
        let center = CGFloat(sampleSize - 1) * 0.5

        for y in 0..<sampleSize {
            for x in 0..<sampleSize {
                let idx = (y * sampleSize + x) * 4
                let a = pixels[idx + 3]
                let r = pixels[idx]
                let g = pixels[idx + 1]
                let b = pixels[idx + 2]
                if a > 30 {
                    sampleRGBAs.append(ColorPaletteEngine.RGBA(r: r, g: g, b: b, a: a))
                }

                guard a > 96 else { continue }
                opaqueCount += 1

                let dx = (CGFloat(x) - center) / CGFloat(sampleSize)
                let dy = (CGFloat(y) - center) / CGFloat(sampleSize)
                radii.append(hypot(dx, dy))

                let maxC = max(Double(r), Double(g), Double(b))
                let minC = min(Double(r), Double(g), Double(b))
                let saturation = maxC > 0 ? (maxC - minC) / maxC : 0
                let weight = 0.25 + saturation * 2.5
                totalR += Double(r) * weight
                totalG += Double(g) * weight
                totalB += Double(b) * weight
                totalWeight += weight

                if x < 6 || x >= sampleSize - 6 || y < 6 || y >= sampleSize - 6 {
                    edgeR += Double(r)
                    edgeG += Double(g)
                    edgeB += Double(b)
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
        let brandPalette = ColorPaletteEngine.extractPalette(from: sampleRGBAs, fallbackHex: fallbackHex)
        return (dominant, edge, fillRatio > 0.72, opticalRadius, brandPalette)
    }

    /// Renders the foreground glyph according to the selected artistic GlyphStyleMode.
    private static func drawGlyph(
        in ctx: CGContext,
        image: CGImage,
        rect: CGRect,
        canvasDim: CGFloat,
        style: GlyphStyleMode,
        palette: BrandPalette,
        dominantRGB: MaterialRGB,
        isSquareLike: Bool
    ) {
        switch style {
        case .original:
            ctx.draw(image, in: rect)

        case .silhouette:
            if isSquareLike {
                ctx.draw(desaturated(image), in: rect)
            } else {
                let col = NSColor(
                    srgbRed: CGFloat(dominantRGB.r) / 255.0,
                    green: CGFloat(dominantRGB.g) / 255.0,
                    blue: CGFloat(dominantRGB.b) / 255.0,
                    alpha: 1.0
                )
                ctx.beginTransparencyLayer(auxiliaryInfo: nil)
                ctx.clip(to: rect, mask: image)
                ctx.setFillColor(col.cgColor)
                ctx.fill(rect)
                ctx.endTransparencyLayer()
            }

        case .embossed:
            let offsetDist = max(1.5, canvasDim * 0.005)
            // 1. Lower-right shadow bevel
            ctx.saveGState()
            ctx.beginTransparencyLayer(auxiliaryInfo: nil)
            ctx.clip(to: rect.offsetBy(dx: 0, dy: -offsetDist), mask: image)
            ctx.setFillColor(CGColor(gray: 0.0, alpha: 0.35))
            ctx.fill(rect.offsetBy(dx: 0, dy: -offsetDist))
            ctx.endTransparencyLayer()
            ctx.restoreGState()

            // 2. Upper-left highlight bevel
            ctx.saveGState()
            ctx.beginTransparencyLayer(auxiliaryInfo: nil)
            ctx.clip(to: rect.offsetBy(dx: 0, dy: offsetDist), mask: image)
            ctx.setFillColor(CGColor(gray: 1.0, alpha: 0.45))
            ctx.fill(rect.offsetBy(dx: 0, dy: offsetDist))
            ctx.endTransparencyLayer()
            ctx.restoreGState()

            // 3. Central glyph
            ctx.draw(image, in: rect)

        case .outline:
            let strokeDist = max(1.5, canvasDim * 0.004)
            let strokeCol = NSColor(
                srgbRed: CGFloat((palette.highlight >> 16) & 0xFF) / 255.0,
                green: CGFloat((palette.highlight >> 8) & 0xFF) / 255.0,
                blue: CGFloat(palette.highlight & 0xFF) / 255.0,
                alpha: 0.85
            )
            ctx.saveGState()
            let offsets: [(CGFloat, CGFloat)] = [
                (-strokeDist, 0), (strokeDist, 0), (0, -strokeDist), (0, strokeDist),
                (-strokeDist * 0.7, -strokeDist * 0.7), (strokeDist * 0.7, strokeDist * 0.7),
                (-strokeDist * 0.7, strokeDist * 0.7), (strokeDist * 0.7, -strokeDist * 0.7)
            ]
            for (ox, oy) in offsets {
                ctx.beginTransparencyLayer(auxiliaryInfo: nil)
                ctx.clip(to: rect.offsetBy(dx: ox, dy: oy), mask: image)
                ctx.setFillColor(strokeCol.cgColor)
                ctx.fill(rect.offsetBy(dx: ox, dy: oy))
                ctx.endTransparencyLayer()
            }
            ctx.restoreGState()
            ctx.draw(image, in: rect)

        case .duotone:
            let deep = NSColor(
                srgbRed: CGFloat((palette.deepTone >> 16) & 0xFF) / 255.0,
                green: CGFloat((palette.deepTone >> 8) & 0xFF) / 255.0,
                blue: CGFloat(palette.deepTone & 0xFF) / 255.0,
                alpha: 1.0
            )
            let light = NSColor(
                srgbRed: CGFloat((palette.primary >> 16) & 0xFF) / 255.0,
                green: CGFloat((palette.primary >> 8) & 0xFF) / 255.0,
                blue: CGFloat(palette.primary & 0xFF) / 255.0,
                alpha: 1.0
            )
            if let duotoneCG = createDuotoneImage(from: image, color0: deep, color1: light) {
                ctx.draw(duotoneCG, in: rect)
            } else {
                ctx.draw(image, in: rect)
            }

        case .glow:
            let glowCol = NSColor(
                srgbRed: CGFloat((palette.primary >> 16) & 0xFF) / 255.0,
                green: CGFloat((palette.primary >> 8) & 0xFF) / 255.0,
                blue: CGFloat(palette.primary & 0xFF) / 255.0,
                alpha: 0.65
            )
            ctx.saveGState()
            ctx.setShadow(
                offset: .zero,
                blur: canvasDim * 0.035,
                color: glowCol.cgColor
            )
            ctx.draw(image, in: rect)
            ctx.restoreGState()
            ctx.draw(image, in: rect)
        }
    }

    private static func createDuotoneImage(from image: CGImage, color0: NSColor, color1: NSColor) -> CGImage? {
        let ciImg = CIImage(cgImage: image)
        guard let falseColor = CIFilter(name: "CIFalseColor") else { return nil }
        falseColor.setValue(ciImg, forKey: kCIInputImageKey)
        falseColor.setValue(CIColor(color: color0), forKey: "inputColor0")
        falseColor.setValue(CIColor(color: color1), forKey: "inputColor1")
        guard let output = falseColor.outputImage else { return nil }

        guard let maskFilter = CIFilter(name: "CIBlendWithAlphaMask") else {
            return ciContext.createCGImage(output, from: ciImg.extent)
        }
        maskFilter.setValue(output, forKey: kCIInputImageKey)
        maskFilter.setValue(CIImage.empty(), forKey: kCIInputBackgroundImageKey)
        maskFilter.setValue(ciImg, forKey: kCIInputMaskImageKey)
        if let maskedOutput = maskFilter.outputImage,
           let cg = ciContext.createCGImage(maskedOutput, from: ciImg.extent) {
            return cg
        }
        return ciContext.createCGImage(output, from: ciImg.extent)
    }

    /// Extracts a 5-color structured BrandPalette (primary, secondary, tint, deep, highlight) from any icon image.
    public static func extractBrandPalette(from image: NSImage, fallbackHex: UInt32 = 0x1A73E8) -> BrandPalette {
        guard let cg = cgImage(from: image, targetSize: 128) else {
            return ColorPaletteEngine.defaultPalette(fallbackHex: fallbackHex)
        }
        return extractColors(from: cg, fallbackHex: fallbackHex).palette
    }
}
