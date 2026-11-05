import Foundation

/// Procedural Geometric Vector Logo styles.
public enum GeometricGlyphStyle: String, CaseIterable, Identifiable, Codable, Sendable {
    case monogram = "Monogram"
    case sparkle = "Sparkle"
    case aperture = "Aperture"
    case orbit = "Orbit"
    case hexacore = "Hexacore"
    case prism = "Prism"
    case shield = "Shield"
    case terminal = "Terminal"
    case waveform = "Waveform"

    public var id: String { rawValue }

    public var title: String {
        switch self {
        case .monogram: return "Letter Monogram"
        case .sparkle: return "Celestial Sparkle"
        case .aperture: return "Iris Aperture"
        case .orbit: return "Planetary Orbit"
        case .hexacore: return "Hexagon Core"
        case .prism: return "Faceted Prism"
        case .shield: return "Cyber Shield"
        case .terminal: return "Code Terminal"
        case .waveform: return "Sound Waveform"
        }
    }

    public var iconSymbol: String {
        switch self {
        case .monogram: return "character.cursor.ibeam"
        case .sparkle: return "sparkles"
        case .aperture: return "camera.aperture"
        case .orbit: return "atom"
        case .hexacore: return "hexagon.fill"
        case .prism: return "diamond.fill"
        case .shield: return "shield.fill"
        case .terminal: return "chevron.left.forwardslash.chevron.right"
        case .waveform: return "waveform"
        }
    }
}

/// Parameters for procedural vector SVG generation.
public struct ProceduralGlyphOptions: Sendable, Hashable {
    public var title: String
    public var style: GeometricGlyphStyle
    public var primaryHex: UInt32
    public var secondaryHex: UInt32
    public var seed: Int

    public init(
        title: String,
        style: GeometricGlyphStyle = .sparkle,
        primaryHex: UInt32 = 0x3B82F6,
        secondaryHex: UInt32 = 0x60A5FA,
        seed: Int = 1
    ) {
        self.title = title
        self.style = style
        self.primaryHex = primaryHex & 0xFFFFFF
        self.secondaryHex = secondaryHex & 0xFFFFFF
        self.seed = seed
    }
}

/// Algorithmic procedural SVG vector logo generator.
public enum ProceduralGlyphGenerator {

    public static func generateSVG(options: ProceduralGlyphOptions) -> String {
        let pColor = String(format: "#%06X", options.primaryHex)
        let sColor = String(format: "#%06X", options.secondaryHex)

        switch options.style {
        case .monogram:
            let initials = extractInitials(from: options.title)
            return """
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
              <circle cx="50" cy="50" r="42" fill="none" stroke="\(pColor)" stroke-width="4" stroke-dasharray="180 12"/>
              <text x="50" y="62" font-family="-apple-system, BlinkMacSystemFont, 'SF Pro Display', 'Helvetica Neue', sans-serif" font-size="\(initials.count > 1 ? "36" : "44")" font-weight="800" text-anchor="middle" fill="\(pColor)" letter-spacing="-1">\(initials)</text>
              <circle cx="74" cy="28" r="5" fill="\(sColor)"/>
            </svg>
            """

        case .sparkle:
            return """
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
              <path d="M 50 6 Q 50 50 94 50 Q 50 50 50 94 Q 50 50 6 50 Q 50 50 50 6 Z" fill="\(pColor)"/>
              <circle cx="50" cy="50" r="8" fill="\(sColor)"/>
              <circle cx="22" cy="22" r="3" fill="\(sColor)" opacity="0.8"/>
              <circle cx="78" cy="78" r="3" fill="\(sColor)" opacity="0.8"/>
            </svg>
            """

        case .aperture:
            var blades: [String] = []
            for i in 0..<6 {
                let rot = i * 60 + (options.seed % 30)
                blades.append("<g transform=\"rotate(\(rot) 50 50)\"><path d=\"M 50 14 L 78 35 L 62 55 Z\" fill=\"\(i % 2 == 0 ? pColor : sColor)\" opacity=\"0.9\"/></g>")
            }
            return """
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
              <circle cx="50" cy="50" r="44" fill="none" stroke="\(pColor)" stroke-width="3" opacity="0.4"/>
              \(blades.joined(separator: "\n  "))
            </svg>
            """

        case .orbit:
            return """
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
              <ellipse cx="50" cy="50" rx="38" ry="14" fill="none" stroke="\(pColor)" stroke-width="4" transform="rotate(-30 50 50)"/>
              <ellipse cx="50" cy="50" rx="38" ry="14" fill="none" stroke="\(sColor)" stroke-width="4" transform="rotate(30 50 50)"/>
              <ellipse cx="50" cy="50" rx="38" ry="14" fill="none" stroke="\(pColor)" stroke-width="3" stroke-dasharray="6 4" transform="rotate(90 50 50)"/>
              <circle cx="50" cy="50" r="10" fill="\(pColor)"/>
            </svg>
            """

        case .hexacore:
            return """
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
              <polygon points="50,10 85,30 85,70 50,90 15,70 15,30" fill="none" stroke="\(pColor)" stroke-width="5"/>
              <polygon points="50,24 72,37 72,63 50,76 28,63 28,37" fill="none" stroke="\(sColor)" stroke-width="4"/>
              <circle cx="50" cy="50" r="8" fill="\(pColor)"/>
              <line x1="50" y1="10" x2="50" y2="24" stroke="\(sColor)" stroke-width="3"/>
              <line x1="50" y1="76" x2="50" y2="90" stroke="\(sColor)" stroke-width="3"/>
            </svg>
            """

        case .prism:
            return """
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
              <polygon points="50,14 86,40 14,40" fill="\(pColor)"/>
              <polygon points="14,40 86,40 50,86" fill="\(sColor)"/>
              <polygon points="50,14 50,86 14,40" fill="\(pColor)" opacity="0.6"/>
              <polygon points="50,14 50,86 86,40" fill="\(sColor)" opacity="0.4"/>
            </svg>
            """

        case .shield:
            return """
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
              <path d="M 50 12 L 82 24 C 82 58 50 82 50 88 C 50 82 18 58 18 24 Z" fill="\(pColor)"/>
              <path d="M 50 24 L 72 33 C 72 56 50 72 50 76 C 50 72 28 56 28 33 Z" fill="\(sColor)"/>
              <path d="M 45 44 L 50 49 L 62 37" fill="none" stroke="#FFFFFF" stroke-width="5" stroke-linecap="round" stroke-linejoin="round"/>
            </svg>
            """

        case .terminal:
            return """
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
              <rect x="14" y="20" width="72" height="60" rx="8" fill="none" stroke="\(pColor)" stroke-width="4"/>
              <path d="M 28 40 L 40 50 L 28 60" fill="none" stroke="\(pColor)" stroke-width="5" stroke-linecap="round" stroke-linejoin="round"/>
              <line x1="48" y1="60" x2="68" y2="60" stroke="\(sColor)" stroke-width="5" stroke-linecap="round"/>
            </svg>
            """

        case .waveform:
            return """
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
              <rect x="16" y="42" width="6" height="16" rx="3" fill="\(sColor)"/>
              <rect x="28" y="30" width="6" height="40" rx="3" fill="\(pColor)"/>
              <rect x="40" y="18" width="6" height="64" rx="3" fill="\(pColor)"/>
              <rect x="52" y="24" width="6" height="52" rx="3" fill="\(sColor)"/>
              <rect x="64" y="34" width="6" height="32" rx="3" fill="\(pColor)"/>
              <rect x="76" y="44" width="6" height="12" rx="3" fill="\(sColor)"/>
            </svg>
            """
        }
    }

    public static func generateSVGData(options: ProceduralGlyphOptions) -> Data {
        generateSVG(options: options).data(using: .utf8) ?? Data()
    }

    private static func extractInitials(from text: String) -> String {
        let clean = ItemTitle.clean(text)
        let words = clean.split(separator: " ")
        if words.count >= 2 {
            let first = words[0].prefix(1)
            let second = words[1].prefix(1)
            return "\(first)\(second)".uppercased()
        }
        return String(clean.prefix(1)).uppercased()
    }
}

/// AI Image generation styles.
public enum AIImageStylePreset: String, CaseIterable, Identifiable, Codable, Sendable {
    case vectorLogo = "Vector Logo"
    case glass3D = "3D Glass"
    case cyberNeon = "Cyber Neon"
    case retroArcade = "Pixel Arcade"

    public var id: String { rawValue }

    public var promptKeywords: String {
        switch self {
        case .vectorLogo:
            return "minimalist flat vector icon, crisp monochrome symbol, high contrast, clean SVG contours, pure solid background, centered, professional app logo"
        case .glass3D:
            return "3D frosted glass app icon, modern glassmorphism, glossy reflections, subtle gradient, Apple macOS design aesthetic, transparent clean studio background"
        case .cyberNeon:
            return "neon glowing tech glyph, dark cyber wireframe emblem, vibrant edge illumination, high contrast, crisp futuristic logo"
        case .retroArcade:
            return "16-bit retro arcade pixel icon, nostalgic video game emblem, crisp sharp pixels, centered clean icon"
        }
    }
}

/// Prompt builder for cloud neural AI image APIs.
public enum AIImagePromptBuilder {
    public static func buildURL(prompt: String, style: AIImageStylePreset, seed: Int = 42) -> URL? {
        let fullPrompt = "\(prompt.trimmingCharacters(in: .whitespacesAndNewlines)), \(style.promptKeywords)"
        guard let encoded = fullPrompt.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed) else {
            return nil
        }
        return URL(string: "https://image.pollinations.ai/prompt/\(encoded)?width=512&height=512&nologo=true&seed=\(seed)")
    }
}
