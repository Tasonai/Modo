import Foundation

// MARK: - Folder Colors

/// Preset colors for macOS folder customization.
public enum FolderColorPreset: String, CaseIterable, Identifiable, Sendable {
    case appleBlue = "macOS Blue"
    case graphite = "Graphite"
    case purple = "Deep Purple"
    case teal = "Teal Ocean"
    case sunsetOrange = "Sunset Orange"
    case emerald = "Emerald Mint"
    case coralRed = "Coral Red"
    case amberGold = "Amber Gold"
    case obsidian = "Obsidian Dark"

    public var id: String { rawValue }

    public var hex: Int {
        switch self {
        case .appleBlue: return 0x2A82E4
        case .graphite: return 0x5C6370
        case .purple: return 0x8A5CF6
        case .teal: return 0x0EA5E9
        case .sunsetOrange: return 0xF97316
        case .emerald: return 0x10B981
        case .coralRed: return 0xEF4444
        case .amberGold: return 0xF59E0B
        case .obsidian: return 0x1E293B
        }
    }

    public var accentHex: Int {
        switch self {
        case .appleBlue: return 0x1C64B4
        case .graphite: return 0x3E4451
        case .purple: return 0x6D28D9
        case .teal: return 0x0284C7
        case .sunsetOrange: return 0xC2410C
        case .emerald: return 0x047857
        case .coralRed: return 0xB91C1C
        case .amberGold: return 0xB45309
        case .obsidian: return 0x0F172A
        }
    }
}

// MARK: - Folder Emboss Style

public enum FolderEmbossStyle: String, CaseIterable, Identifiable, Codable, Sendable {
    case etched = "Engraved / Etched"
    case flatWhite = "Clean Flat White"
    case tintedAccent = "Color Accent"
    case adaptiveShape = "Modo Adaptive Badge"

    public var id: String { rawValue }
}

// MARK: - Folder Style Model

public struct FolderStyle: Codable, Equatable, Sendable {
    public var colorHex: Int
    public var accentHex: Int
    public var embossStyle: FolderEmbossStyle
    public var badgeSymbolName: String
    public var badgeScale: Double // 0.2 to 0.6
    public var customSVGText: String?

    public init(
        colorHex: Int = FolderColorPreset.appleBlue.hex,
        accentHex: Int = FolderColorPreset.appleBlue.accentHex,
        embossStyle: FolderEmbossStyle = .etched,
        badgeSymbolName: String = "folder.fill",
        badgeScale: Double = 0.38,
        customSVGText: String? = nil
    ) {
        self.colorHex = colorHex
        self.accentHex = accentHex
        self.embossStyle = embossStyle
        self.badgeSymbolName = badgeSymbolName
        self.badgeScale = min(max(badgeScale, 0.15), 0.7)
        self.customSVGText = customSVGText
    }

    public init(
        preset: FolderColorPreset,
        embossStyle: FolderEmbossStyle = .etched,
        badgeSymbolName: String = "folder.fill",
        badgeScale: Double = 0.38,
        customSVGText: String? = nil
    ) {
        self.init(
            colorHex: preset.hex,
            accentHex: preset.accentHex,
            embossStyle: embossStyle,
            badgeSymbolName: badgeSymbolName,
            badgeScale: badgeScale,
            customSVGText: customSVGText
        )
    }
}

// MARK: - Vector Folder Geometry Generator

public enum FolderVectorGenerator {
    /// Generates pure vector SVG markup representing a tinted macOS folder with optional embossed glyph.
    public static func generateFolderSVG(
        style: FolderStyle,
        size: Int = 512,
        centerGlyphSVG: String? = nil
    ) -> String {
        let hexColor = String(format: "#%06X", style.colorHex)
        let hexAccent = String(format: "#%06X", style.accentHex)

        // Geometric dimensions for folder flap and tab
        let w = Double(size)
        let h = Double(size)

        // Folder back tab
        let tabX = w * 0.12
        let tabY = h * 0.18
        let tabW = w * 0.36
        let tabH = h * 0.14
        let tabR = w * 0.04

        // Folder body
        let bodyX = w * 0.10
        let bodyY = h * 0.25
        let bodyW = w * 0.80
        let bodyH = h * 0.60
        let bodyR = w * 0.05

        // Front flap (slightly lower and wider for 3D perspective)
        let frontY = h * 0.36
        let frontH = h * 0.50

        var glyphMarkup = ""
        if let glyph = centerGlyphSVG, !glyph.isEmpty {
            let glyphScale = style.badgeScale
            let gw = w * glyphScale
            let gh = h * glyphScale
            let gx = (w - gw) / 2.0
            let gy = frontY + (frontH - gh) / 2.0 - (h * 0.02)

            let fillStyle: String
            switch style.embossStyle {
            case .etched:
                fillStyle = "fill=\"#000000\" fill-opacity=\"0.35\" style=\"mix-blend-mode: overlay;\""
            case .flatWhite:
                fillStyle = "fill=\"#FFFFFF\" fill-opacity=\"0.95\" filter=\"drop-shadow(0 2px 4px rgba(0,0,0,0.25))\""
            case .tintedAccent:
                fillStyle = "fill=\"\(hexAccent)\" filter=\"drop-shadow(0 2px 3px rgba(0,0,0,0.15))\""
            case .adaptiveShape:
                fillStyle = "fill=\"#FFFFFF\" filter=\"drop-shadow(0 3px 6px rgba(0,0,0,0.3))\""
            }

            glyphMarkup = """
            <g transform="translate(\(gx), \(gy))" \(fillStyle)>
              <svg width="\(gw)" height="\(gh)" viewBox="0 0 512 512">
                \(glyph)
              </svg>
            </g>
            """
        }

        return """
        <svg xmlns="http://www.w3.org/2000/svg" width="\(size)" height="\(size)" viewBox="0 0 \(size) \(size)">
          <defs>
            <linearGradient id="folderBackGrad" x1="0%" y1="0%" x2="0%" y2="100%">
              <stop offset="0%" stop-color="\(hexColor)" stop-opacity="0.95"/>
              <stop offset="100%" stop-color="\(hexAccent)" stop-opacity="1.0"/>
            </linearGradient>
            <linearGradient id="folderFrontGrad" x1="0%" y1="0%" x2="0%" y2="100%">
              <stop offset="0%" stop-color="\(hexColor)" stop-opacity="1.0"/>
              <stop offset="60%" stop-color="\(hexColor)" stop-opacity="0.95"/>
              <stop offset="100%" stop-color="\(hexAccent)" stop-opacity="1.0"/>
            </linearGradient>
            <filter id="folderDropShadow" x="-10%" y="-10%" width="120%" height="130%">
              <feDropShadow dx="0" dy="\(size / 32)" stdDeviation="\(size / 40)" flood-color="#000000" flood-opacity="0.25"/>
            </filter>
            <filter id="flapShadow" x="-5%" y="-5%" width="110%" height="120%">
              <feDropShadow dx="0" dy="\(size / 80)" stdDeviation="\(size / 80)" flood-color="#000000" flood-opacity="0.20"/>
            </filter>
          </defs>

          <!-- Back Tab -->
          <rect x="\(tabX)" y="\(tabY)" width="\(tabW)" height="\(tabH)" rx="\(tabR)" fill="url(#folderBackGrad)" filter="url(#folderDropShadow)" />

          <!-- Back Body -->
          <rect x="\(bodyX)" y="\(bodyY)" width="\(bodyW)" height="\(bodyH)" rx="\(bodyR)" fill="url(#folderBackGrad)" filter="url(#folderDropShadow)" />

          <!-- Paper Sheen Line -->
          <line x1="\(bodyX + w * 0.05)" y1="\(bodyY + h * 0.08)" x2="\(bodyX + bodyW - w * 0.05)" y2="\(bodyY + h * 0.08)" stroke="#FFFFFF" stroke-opacity="0.35" stroke-width="\(w * 0.01)" stroke-linecap="round" />

          <!-- Front Flap with Depth Shadow -->
          <rect x="\(bodyX)" y="\(frontY)" width="\(bodyW)" height="\(frontH)" rx="\(bodyR)" fill="url(#folderFrontGrad)" filter="url(#flapShadow)" />

          <!-- Top Rim Highlight -->
          <path d="M \(bodyX + bodyR) \(frontY) L \(bodyX + bodyW - bodyR) \(frontY)" stroke="#FFFFFF" stroke-opacity="0.4" stroke-width="\(w * 0.008)" stroke-linecap="round" />

          <!-- Center Emblem / Badge -->
          \(glyphMarkup)
        </svg>
        """
    }
}
