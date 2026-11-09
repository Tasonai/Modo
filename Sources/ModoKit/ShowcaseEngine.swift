import AppKit
import CoreGraphics
import Foundation

// MARK: - Showcase Presentation Styles

public enum ShowcaseTheme: String, CaseIterable, Identifiable, Sendable {
    case ambientMesh = "Ambient Mesh"
    case studioDark = "Studio Obsidian"
    case cleanWhite = "Clean Minimal"
    case sunsetGlow = "Sunset Gradient"
    case cyberNeon = "Cyber Glow"

    public var id: String { rawValue }
}

public enum ShowcaseLayout: String, CaseIterable, Identifiable, Sendable {
    case heroSingle = "Hero Icon (Centered)"
    case tripleVariants = "Light, Dark & Tinted Trio"
    case macCard = "macOS Window Card"

    public var id: String { rawValue }
}

public struct ShowcaseExportOptions: Sendable {
    public var theme: ShowcaseTheme
    public var layout: ShowcaseLayout
    public var title: String
    public var subtitle: String
    public var width: Int
    public var height: Int

    public init(
        theme: ShowcaseTheme = .ambientMesh,
        layout: ShowcaseLayout = .heroSingle,
        title: String = "App Title",
        subtitle: String = "Designed with Modo",
        width: Int = 1920,
        height: Int = 1080
    ) {
        self.theme = theme
        self.layout = layout
        self.title = title
        self.subtitle = subtitle
        self.width = width
        self.height = height
    }
}

// MARK: - Showcase Renderer

public enum ShowcaseEngine {
    /// Renders a full presentation graphic for App Store, GitHub README, or portfolio.
    public static func renderShowcase(
        icon: NSImage,
        darkIcon: NSImage? = nil,
        tintedIcon: NSImage? = nil,
        options: ShowcaseExportOptions
    ) -> NSImage {
        let size = NSSize(width: options.width, height: options.height)
        let image = NSImage(size: size)
        image.lockFocus()

        guard let ctx = NSGraphicsContext.current?.cgContext else {
            image.unlockFocus()
            return image
        }

        let rect = CGRect(origin: .zero, size: size)

        // 1. Draw Background
        drawBackground(in: ctx, rect: rect, theme: options.theme)

        // 2. Draw Layout
        switch options.layout {
        case .heroSingle:
            drawHeroLayout(in: ctx, rect: rect, icon: icon, options: options)
        case .tripleVariants:
            let dark = darkIcon ?? icon
            let tinted = tintedIcon ?? icon
            drawTripleLayout(in: ctx, rect: rect, light: icon, dark: dark, tinted: tinted, options: options)
        case .macCard:
            drawCardLayout(in: ctx, rect: rect, icon: icon, options: options)
        }

        image.unlockFocus()
        return image
    }

    private static func drawBackground(in ctx: CGContext, rect: CGRect, theme: ShowcaseTheme) {
        ctx.saveGState()
        let colorSpace = CGColorSpaceCreateDeviceRGB()

        var colors: [CGColor] = []
        var locations: [CGFloat] = [0.0, 1.0]

        switch theme {
        case .ambientMesh:
            colors = [
                CGColor(srgbRed: 0.08, green: 0.10, blue: 0.16, alpha: 1.0),
                CGColor(srgbRed: 0.15, green: 0.18, blue: 0.28, alpha: 1.0),
                CGColor(srgbRed: 0.05, green: 0.06, blue: 0.10, alpha: 1.0)
            ]
            locations = [0.0, 0.55, 1.0]
        case .studioDark:
            colors = [
                CGColor(srgbRed: 0.11, green: 0.12, blue: 0.14, alpha: 1.0),
                CGColor(srgbRed: 0.04, green: 0.04, blue: 0.05, alpha: 1.0)
            ]
        case .cleanWhite:
            colors = [
                CGColor(srgbRed: 0.98, green: 0.98, blue: 0.99, alpha: 1.0),
                CGColor(srgbRed: 0.90, green: 0.91, blue: 0.94, alpha: 1.0)
            ]
        case .sunsetGlow:
            colors = [
                CGColor(srgbRed: 0.15, green: 0.06, blue: 0.18, alpha: 1.0),
                CGColor(srgbRed: 0.32, green: 0.12, blue: 0.22, alpha: 1.0),
                CGColor(srgbRed: 0.08, green: 0.04, blue: 0.10, alpha: 1.0)
            ]
            locations = [0.0, 0.6, 1.0]
        case .cyberNeon:
            colors = [
                CGColor(srgbRed: 0.02, green: 0.08, blue: 0.12, alpha: 1.0),
                CGColor(srgbRed: 0.04, green: 0.16, blue: 0.22, alpha: 1.0),
                CGColor(srgbRed: 0.01, green: 0.03, blue: 0.05, alpha: 1.0)
            ]
            locations = [0.0, 0.5, 1.0]
        }

        if let gradient = CGGradient(colorsSpace: colorSpace, colors: colors as CFArray, locations: locations) {
            ctx.drawLinearGradient(
                gradient,
                start: CGPoint(x: 0, y: rect.height),
                end: CGPoint(x: rect.width, y: 0),
                options: [.drawsBeforeStartLocation, .drawsAfterEndLocation]
            )
        }

        ctx.restoreGState()
    }

    private static func drawHeroLayout(in ctx: CGContext, rect: CGRect, icon: NSImage, options: ShowcaseExportOptions) {
        let isLight = options.theme == .cleanWhite
        let textColor: NSColor = isLight ? NSColor(white: 0.1, alpha: 1.0) : .white
        let subColor: NSColor = isLight ? NSColor(white: 0.4, alpha: 1.0) : NSColor(white: 0.65, alpha: 1.0)

        // Center icon
        let iconDim = min(rect.height * 0.46, rect.width * 0.32)
        let iconX = (rect.width - iconDim) / 2
        let iconY = rect.height * 0.42

        drawIconWithShadow(in: ctx, icon: icon, rect: CGRect(x: iconX, y: iconY, width: iconDim, height: iconDim))

        // Title and Subtitle
        let titleFont = NSFont.systemFont(ofSize: min(rect.height * 0.065, 48), weight: .bold)
        let subFont = NSFont.systemFont(ofSize: min(rect.height * 0.032, 22), weight: .medium)

        let titleStyle = NSMutableParagraphStyle()
        titleStyle.alignment = .center
        let titleAttrs: [NSAttributedString.Key: Any] = [
            .font: titleFont,
            .foregroundColor: textColor,
            .paragraphStyle: titleStyle
        ]
        let titleString = NSAttributedString(string: options.title, attributes: titleAttrs)
        let titleRect = CGRect(x: 40, y: iconY - 70, width: rect.width - 80, height: 60)
        titleString.draw(in: titleRect)

        let subStyle = NSMutableParagraphStyle()
        subStyle.alignment = .center
        let subAttrs: [NSAttributedString.Key: Any] = [
            .font: subFont,
            .foregroundColor: subColor,
            .paragraphStyle: subStyle
        ]
        let subString = NSAttributedString(string: options.subtitle, attributes: subAttrs)
        let subRect = CGRect(x: 40, y: iconY - 110, width: rect.width - 80, height: 36)
        subString.draw(in: subRect)
    }

    private static func drawTripleLayout(
        in ctx: CGContext,
        rect: CGRect,
        light: NSImage,
        dark: NSImage,
        tinted: NSImage,
        options: ShowcaseExportOptions
    ) {
        let isLight = options.theme == .cleanWhite
        let textColor: NSColor = isLight ? NSColor(white: 0.1, alpha: 1.0) : .white
        let subColor: NSColor = isLight ? NSColor(white: 0.4, alpha: 1.0) : NSColor(white: 0.65, alpha: 1.0)

        let iconDim = min(rect.height * 0.38, rect.width * 0.22)
        let spacing = min(rect.width * 0.06, 60.0)
        let totalW = (iconDim * 3) + (spacing * 2)
        let startX = (rect.width - totalW) / 2
        let iconY = rect.height * 0.44

        let icons = [
            ("Light", light, startX),
            ("Dark", dark, startX + iconDim + spacing),
            ("Tinted", tinted, startX + (iconDim + spacing) * 2)
        ]

        let labelFont = NSFont.systemFont(ofSize: 16, weight: .semibold)
        let labelStyle = NSMutableParagraphStyle()
        labelStyle.alignment = .center
        let labelAttrs: [NSAttributedString.Key: Any] = [
            .font: labelFont,
            .foregroundColor: subColor,
            .paragraphStyle: labelStyle
        ]

        for (lbl, img, x) in icons {
            drawIconWithShadow(in: ctx, icon: img, rect: CGRect(x: x, y: iconY, width: iconDim, height: iconDim))
            let lblString = NSAttributedString(string: lbl, attributes: labelAttrs)
            lblString.draw(in: CGRect(x: x, y: iconY - 32, width: iconDim, height: 24))
        }

        // Title and Subtitle at top
        let titleFont = NSFont.systemFont(ofSize: min(rect.height * 0.06, 44), weight: .bold)
        let titleStyle = NSMutableParagraphStyle()
        titleStyle.alignment = .center
        let titleAttrs: [NSAttributedString.Key: Any] = [
            .font: titleFont,
            .foregroundColor: textColor,
            .paragraphStyle: titleStyle
        ]
        let titleString = NSAttributedString(string: options.title, attributes: titleAttrs)
        titleString.draw(in: CGRect(x: 40, y: rect.height - 110, width: rect.width - 80, height: 48))
    }

    private static func drawCardLayout(in ctx: CGContext, rect: CGRect, icon: NSImage, options: ShowcaseExportOptions) {
        let cardW = min(rect.width * 0.65, 800.0)
        let cardH = min(rect.height * 0.70, 560.0)
        let cardX = (rect.width - cardW) / 2
        let cardY = (rect.height - cardH) / 2

        ctx.saveGState()
        // Card shadow
        ctx.setShadow(offset: CGSize(width: 0, height: -20), blur: 50, color: CGColor(gray: 0, alpha: 0.45))
        let cardPath = CGPath(roundedRect: CGRect(x: cardX, y: cardY, width: cardW, height: cardH), cornerWidth: 24, cornerHeight: 24, transform: nil)
        ctx.addPath(cardPath)
        ctx.setFillColor(CGColor(srgbRed: 0.14, green: 0.15, blue: 0.18, alpha: 0.92))
        ctx.fillPath()
        ctx.restoreGState()

        // Card border
        ctx.saveGState()
        ctx.addPath(cardPath)
        ctx.setStrokeColor(CGColor(gray: 1.0, alpha: 0.12))
        ctx.setLineWidth(1.5)
        ctx.strokePath()
        ctx.restoreGState()

        // Inner icon
        let iconDim = min(cardH * 0.45, 220.0)
        let iconX = cardX + (cardW - iconDim) / 2
        let iconY = cardY + cardH * 0.42
        drawIconWithShadow(in: ctx, icon: icon, rect: CGRect(x: iconX, y: iconY, width: iconDim, height: iconDim))

        // Card typography
        let titleFont = NSFont.systemFont(ofSize: 34, weight: .bold)
        let titleStyle = NSMutableParagraphStyle()
        titleStyle.alignment = .center
        let titleAttrs: [NSAttributedString.Key: Any] = [
            .font: titleFont,
            .foregroundColor: NSColor.white,
            .paragraphStyle: titleStyle
        ]
        let titleString = NSAttributedString(string: options.title, attributes: titleAttrs)
        titleString.draw(in: CGRect(x: cardX + 20, y: iconY - 55, width: cardW - 40, height: 40))

        let subFont = NSFont.systemFont(ofSize: 18, weight: .medium)
        let subStyle = NSMutableParagraphStyle()
        subStyle.alignment = .center
        let subAttrs: [NSAttributedString.Key: Any] = [
            .font: subFont,
            .foregroundColor: NSColor(white: 0.65, alpha: 1.0),
            .paragraphStyle: subStyle
        ]
        let subString = NSAttributedString(string: options.subtitle, attributes: subAttrs)
        subString.draw(in: CGRect(x: cardX + 20, y: iconY - 90, width: cardW - 40, height: 32))
    }

    private static func drawIconWithShadow(in ctx: CGContext, icon: NSImage, rect: CGRect) {
        ctx.saveGState()

        // 1. Soft ambient shadow
        ctx.setShadow(
            offset: CGSize(width: 0, height: -rect.height * 0.08),
            blur: rect.height * 0.18,
            color: CGColor(gray: 0, alpha: 0.38)
        )

        // 2. Render icon image
        if let cgImage = icon.cgImage(forProposedRect: nil, context: nil, hints: nil) {
            ctx.draw(cgImage, in: rect)
        } else {
            icon.draw(in: rect)
        }

        ctx.restoreGState()
    }
}
