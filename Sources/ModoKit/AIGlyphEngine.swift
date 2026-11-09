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

/// Typography font style for custom lettermark monograms.
public enum MonogramFontStyle: String, CaseIterable, Identifiable, Codable, Sendable {
    case modernSans = "Modern Sans"
    case monospaceCode = "Monospace Code"
    case elegantSerif = "Classic Serif"
    case geometricBlack = "Geometric Heavy"

    public var id: String { rawValue }

    public var fontFamily: String {
        switch self {
        case .modernSans:
            return "-apple-system, BlinkMacSystemFont, 'SF Pro Display', 'Helvetica Neue', sans-serif"
        case .monospaceCode:
            return "'SF Mono', Monaco, Menlo, 'Courier New', monospace"
        case .elegantSerif:
            return "New York, Georgia, 'Times New Roman', serif"
        case .geometricBlack:
            return "Futura, 'Avenir Next Heavy', Impact, sans-serif"
        }
    }
}

/// Border and badge geometry framing the monogram lettermark.
public enum MonogramFrameStyle: String, CaseIterable, Identifiable, Codable, Sendable {
    case minimal = "Minimal"
    case circularRing = "Circle Ring"
    case squircleSolid = "Squircle Solid"
    case hexagonFrame = "Hexagon Frame"
    case doubleOrbit = "Double Orbit"
    case diamondCut = "Diamond Cut"

    public var id: String { rawValue }
}

/// Parameters for procedural vector SVG generation.
public struct ProceduralGlyphOptions: Sendable, Hashable {
    public var title: String
    public var style: GeometricGlyphStyle
    public var primaryHex: UInt32
    public var secondaryHex: UInt32
    public var seed: Int
    public var variantIndex: Int

    public init(
        title: String,
        style: GeometricGlyphStyle = .sparkle,
        primaryHex: UInt32 = 0x3B82F6,
        secondaryHex: UInt32 = 0x60A5FA,
        seed: Int = 1,
        variantIndex: Int = 0
    ) {
        self.title = title
        self.style = style
        self.primaryHex = primaryHex & 0xFFFFFF
        self.secondaryHex = secondaryHex & 0xFFFFFF
        self.seed = seed
        self.variantIndex = variantIndex
    }
}

/// Algorithmic procedural SVG vector logo generator.
public enum ProceduralGlyphGenerator {

    public static func variantTitle(style: GeometricGlyphStyle, index: Int) -> String {
        switch style {
        case .monogram:
            switch index % 4 {
            case 0: return "Minimal Ring"
            case 1: return "Solid Badge"
            case 2: return "Double Orbit"
            default: return "Hex Frame"
            }
        case .sparkle:
            switch index % 4 {
            case 0: return "Celestial Flare"
            case 1: return "Radiant 8-Star"
            case 2: return "Halo Sparkle"
            default: return "Twin Flares"
            }
        case .aperture:
            switch index % 4 {
            case 0: return "Iris Shutter"
            case 1: return "Camera Lens"
            case 2: return "Vortex Turbine"
            default: return "Focal Crosshair"
            }
        case .orbit:
            switch index % 4 {
            case 0: return "Atomic Shell"
            case 1: return "Planetary Disc"
            case 2: return "Nested Systems"
            default: return "Slingshot Arc"
            }
        case .hexacore:
            switch index % 4 {
            case 0: return "Dual Hexacore"
            case 1: return "Isometric Cube"
            case 2: return "Cyber Bracket"
            default: return "Knot Triad"
            }
        case .prism:
            switch index % 4 {
            case 0: return "Brilliant Cut"
            case 1: return "Crystal Shard"
            case 2: return "Delta Pyramid"
            default: return "Faceted Pavilion"
            }
        case .shield:
            switch index % 4 {
            case 0: return "Heraldic Crest"
            case 1: return "Cyber Aegis"
            case 2: return "Contour Aegis"
            default: return "Quartered Shield"
            }
        case .terminal:
            switch index % 4 {
            case 0: return "Prompt Window"
            case 1: return "Code Tag Markup"
            case 2: return "Script Braces"
            default: return "Double Chevron"
            }
        case .waveform:
            switch index % 4 {
            case 0: return "Equalizer Spectrum"
            case 1: return "Oscilloscope Wave"
            case 2: return "Radial Visualizer"
            default: return "Acoustic Sonar"
            }
        }
    }

    public static func generateSVG(options: ProceduralGlyphOptions) -> String {
        let pColor = String(format: "#%06X", options.primaryHex)
        let sColor = String(format: "#%06X", options.secondaryHex)

        switch options.style {
        case .monogram:
            let initials = extractInitials(from: options.title)
            let fontSize = initials.count > 1 ? "36" : "44"
            let v = options.variantIndex % 4

            switch v {
            case 0:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <circle cx="50" cy="50" r="42" fill="none" stroke="\(pColor)" stroke-width="4" stroke-dasharray="180 12"/>
                  <text x="50" y="63" font-family="-apple-system, BlinkMacSystemFont, 'SF Pro Display', 'Helvetica Neue', sans-serif" font-size="\(fontSize)" font-weight="800" text-anchor="middle" fill="\(pColor)" letter-spacing="-1">\(initials)</text>
                  <circle cx="74" cy="28" r="5" fill="\(sColor)"/>
                </svg>
                """
            case 1:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <rect x="14" y="14" width="72" height="72" rx="20" fill="\(pColor)"/>
                  <rect x="18" y="18" width="64" height="64" rx="16" fill="none" stroke="\(sColor)" stroke-width="2" opacity="0.6"/>
                  <text x="50" y="63" font-family="-apple-system, BlinkMacSystemFont, 'SF Pro Display', 'Helvetica Neue', sans-serif" font-size="\(fontSize)" font-weight="800" text-anchor="middle" fill="#FFFFFF" letter-spacing="-1">\(initials)</text>
                </svg>
                """
            case 2:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <circle cx="50" cy="50" r="44" fill="none" stroke="\(sColor)" stroke-width="2" stroke-dasharray="8 6"/>
                  <circle cx="50" cy="50" r="35" fill="none" stroke="\(pColor)" stroke-width="3"/>
                  <line x1="50" y1="6" x2="50" y2="15" stroke="\(pColor)" stroke-width="3" stroke-linecap="round"/>
                  <line x1="50" y1="85" x2="50" y2="94" stroke="\(pColor)" stroke-width="3" stroke-linecap="round"/>
                  <line x1="6" y1="50" x2="15" y2="50" stroke="\(pColor)" stroke-width="3" stroke-linecap="round"/>
                  <line x1="85" y1="50" x2="94" y2="50" stroke="\(pColor)" stroke-width="3" stroke-linecap="round"/>
                  <text x="50" y="63" font-family="-apple-system, BlinkMacSystemFont, 'SF Pro Display', 'Helvetica Neue', sans-serif" font-size="\(fontSize)" font-weight="800" text-anchor="middle" fill="\(pColor)" letter-spacing="-1">\(initials)</text>
                </svg>
                """
            default:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <polygon points="50,8 88,30 88,70 50,92 12,70 12,30" fill="none" stroke="\(pColor)" stroke-width="4"/>
                  <polygon points="50,14 82,33 82,67 50,86 18,67 18,33" fill="none" stroke="\(sColor)" stroke-width="1.5" opacity="0.4"/>
                  <rect x="34" y="74" width="32" height="4" rx="2" fill="\(sColor)"/>
                  <text x="50" y="61" font-family="-apple-system, BlinkMacSystemFont, 'SF Pro Display', 'Helvetica Neue', sans-serif" font-size="\(fontSize)" font-weight="800" text-anchor="middle" fill="\(pColor)" letter-spacing="-1">\(initials)</text>
                </svg>
                """
            }

        case .sparkle:
            let v = options.variantIndex % 4
            switch v {
            case 0:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <path d="M 50 6 Q 50 50 94 50 Q 50 50 50 94 Q 50 50 6 50 Q 50 50 50 6 Z" fill="\(pColor)"/>
                  <circle cx="50" cy="50" r="8" fill="\(sColor)"/>
                  <circle cx="22" cy="22" r="3" fill="\(sColor)" opacity="0.8"/>
                  <circle cx="78" cy="78" r="3" fill="\(sColor)" opacity="0.8"/>
                </svg>
                """
            case 1:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <path d="M 50 6 L 56 42 L 94 50 L 56 58 L 50 94 L 44 58 L 6 50 L 44 42 Z" fill="\(pColor)"/>
                  <path d="M 50 20 L 54 44 L 80 50 L 54 56 L 50 80 L 46 56 L 20 50 L 46 44 Z" fill="\(sColor)" opacity="0.75" transform="rotate(45 50 50)"/>
                  <circle cx="50" cy="50" r="7" fill="#FFFFFF"/>
                </svg>
                """
            case 2:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <circle cx="50" cy="50" r="38" fill="none" stroke="\(sColor)" stroke-width="2" stroke-dasharray="6 6"/>
                  <path d="M 50 14 Q 50 50 86 50 Q 50 50 50 86 Q 50 50 14 50 Q 50 50 50 14 Z" fill="\(pColor)"/>
                  <circle cx="50" cy="50" r="6" fill="\(sColor)"/>
                  <circle cx="50" cy="12" r="3" fill="\(pColor)"/>
                  <circle cx="50" cy="88" r="3" fill="\(pColor)"/>
                  <circle cx="12" cy="50" r="3" fill="\(pColor)"/>
                  <circle cx="88" cy="50" r="3" fill="\(pColor)"/>
                </svg>
                """
            default:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <path d="M 42 10 Q 42 42 74 42 Q 42 42 42 74 Q 42 42 10 42 Q 42 42 42 10 Z" fill="\(pColor)"/>
                  <path d="M 72 62 Q 72 78 88 78 Q 72 78 72 94 Q 72 78 56 78 Q 72 78 72 62 Z" fill="\(sColor)"/>
                  <circle cx="42" cy="42" r="6" fill="\(sColor)"/>
                  <circle cx="72" cy="78" r="3.5" fill="#FFFFFF"/>
                  <circle cx="20" cy="78" r="2.5" fill="\(sColor)" opacity="0.6"/>
                  <circle cx="78" cy="22" r="3" fill="\(pColor)" opacity="0.6"/>
                </svg>
                """
            }

        case .aperture:
            let v = options.variantIndex % 4
            switch v {
            case 0:
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
            case 1:
                var blades: [String] = []
                for i in 0..<8 {
                    let rot = i * 45
                    blades.append("<polygon points=\"50,16 68,26 62,48 50,44\" fill=\"\(i % 2 == 0 ? pColor : sColor)\" transform=\"rotate(\(rot) 50 50)\" opacity=\"0.88\"/>")
                }
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <circle cx="50" cy="50" r="42" fill="none" stroke="\(pColor)" stroke-width="4"/>
                  \(blades.joined(separator: "\n  "))
                  <circle cx="50" cy="50" r="10" fill="none" stroke="\(pColor)" stroke-width="2"/>
                </svg>
                """
            case 2:
                var arcs: [String] = []
                for i in 0..<6 {
                    let rot = i * 60
                    arcs.append("<path d=\"M 50 50 Q 65 30 84 40 Q 68 56 50 50 Z\" fill=\"\(i % 2 == 0 ? pColor : sColor)\" transform=\"rotate(\(rot) 50 50)\" opacity=\"0.85\"/>")
                }
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  \(arcs.joined(separator: "\n  "))
                  <circle cx="50" cy="50" r="7" fill="\(pColor)"/>
                </svg>
                """
            default:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <circle cx="50" cy="50" r="42" fill="none" stroke="\(pColor)" stroke-width="3"/>
                  <circle cx="50" cy="50" r="28" fill="none" stroke="\(sColor)" stroke-width="2" stroke-dasharray="12 6"/>
                  <circle cx="50" cy="50" r="14" fill="none" stroke="\(pColor)" stroke-width="3"/>
                  <line x1="50" y1="4" x2="50" y2="96" stroke="\(pColor)" stroke-width="1.5" stroke-dasharray="4 4" opacity="0.6"/>
                  <line x1="4" y1="50" x2="96" y2="50" stroke="\(pColor)" stroke-width="1.5" stroke-dasharray="4 4" opacity="0.6"/>
                  <circle cx="50" cy="50" r="4" fill="\(pColor)"/>
                </svg>
                """
            }

        case .orbit:
            let v = options.variantIndex % 4
            switch v {
            case 0:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <ellipse cx="50" cy="50" rx="38" ry="14" fill="none" stroke="\(pColor)" stroke-width="4" transform="rotate(-30 50 50)"/>
                  <ellipse cx="50" cy="50" rx="38" ry="14" fill="none" stroke="\(sColor)" stroke-width="4" transform="rotate(30 50 50)"/>
                  <ellipse cx="50" cy="50" rx="38" ry="14" fill="none" stroke="\(pColor)" stroke-width="3" stroke-dasharray="6 4" transform="rotate(90 50 50)"/>
                  <circle cx="50" cy="50" r="10" fill="\(pColor)"/>
                </svg>
                """
            case 1:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <ellipse cx="50" cy="50" rx="46" ry="14" fill="none" stroke="\(sColor)" stroke-width="5" transform="rotate(-22 50 50)"/>
                  <circle cx="50" cy="50" r="24" fill="\(pColor)"/>
                  <path d="M 12 60 C 26 66 68 60 88 40" fill="none" stroke="\(sColor)" stroke-width="5" transform="rotate(-22 50 50)"/>
                  <circle cx="78" cy="28" r="4.5" fill="\(sColor)"/>
                </svg>
                """
            case 2:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <circle cx="50" cy="50" r="42" fill="none" stroke="\(pColor)" stroke-width="2" stroke-dasharray="6 6"/>
                  <circle cx="50" cy="50" r="28" fill="none" stroke="\(sColor)" stroke-width="2"/>
                  <circle cx="50" cy="50" r="16" fill="none" stroke="\(pColor)" stroke-width="2" stroke-dasharray="4 4"/>
                  <circle cx="50" cy="50" r="8" fill="\(pColor)"/>
                  <circle cx="50" cy="8" r="4" fill="\(sColor)"/>
                  <circle cx="74" cy="64" r="3.5" fill="\(pColor)"/>
                  <circle cx="38" cy="40" r="2.5" fill="\(sColor)"/>
                </svg>
                """
            default:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <circle cx="38" cy="50" r="18" fill="\(pColor)"/>
                  <path d="M 10 88 C 24 70 38 18 84 20" fill="none" stroke="\(sColor)" stroke-width="5" stroke-linecap="round"/>
                  <circle cx="84" cy="20" r="6" fill="\(sColor)"/>
                  <circle cx="68" cy="26" r="3.5" fill="\(sColor)" opacity="0.6"/>
                  <circle cx="54" cy="38" r="2.5" fill="\(sColor)" opacity="0.4"/>
                </svg>
                """
            }

        case .hexacore:
            let v = options.variantIndex % 4
            switch v {
            case 0:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <polygon points="50,10 85,30 85,70 50,90 15,70 15,30" fill="none" stroke="\(pColor)" stroke-width="5"/>
                  <polygon points="50,24 72,37 72,63 50,76 28,63 28,37" fill="none" stroke="\(sColor)" stroke-width="4"/>
                  <circle cx="50" cy="50" r="8" fill="\(pColor)"/>
                  <line x1="50" y1="10" x2="50" y2="24" stroke="\(sColor)" stroke-width="3"/>
                  <line x1="50" y1="76" x2="50" y2="90" stroke="\(sColor)" stroke-width="3"/>
                </svg>
                """
            case 1:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <polygon points="50,12 84,32 50,52 16,32" fill="\(sColor)"/>
                  <polygon points="16,32 50,52 50,88 16,68" fill="\(pColor)"/>
                  <polygon points="50,52 84,32 84,68 50,88" fill="\(pColor)" opacity="0.75"/>
                  <polyline points="16,32 50,52 84,32" fill="none" stroke="#FFFFFF" stroke-width="1.5" opacity="0.6"/>
                  <line x1="50" y1="52" x2="50" y2="88" stroke="#FFFFFF" stroke-width="1.5" opacity="0.6"/>
                </svg>
                """
            case 2:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <path d="M 38 14 L 50 7 L 62 14" fill="none" stroke="\(pColor)" stroke-width="4" stroke-linecap="round"/>
                  <path d="M 76 26 L 88 33 L 88 47" fill="none" stroke="\(pColor)" stroke-width="4" stroke-linecap="round"/>
                  <path d="M 88 53 L 88 67 L 76 74" fill="none" stroke="\(pColor)" stroke-width="4" stroke-linecap="round"/>
                  <path d="M 62 86 L 50 93 L 38 86" fill="none" stroke="\(pColor)" stroke-width="4" stroke-linecap="round"/>
                  <path d="M 24 74 L 12 67 L 12 53" fill="none" stroke="\(pColor)" stroke-width="4" stroke-linecap="round"/>
                  <path d="M 12 47 L 12 33 L 24 26" fill="none" stroke="\(pColor)" stroke-width="4" stroke-linecap="round"/>
                  <circle cx="50" cy="50" r="14" fill="\(sColor)"/>
                  <circle cx="50" cy="50" r="6" fill="#FFFFFF"/>
                </svg>
                """
            default:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <polygon points="50,14 74,28 74,56 50,70 26,56 26,28" fill="none" stroke="\(pColor)" stroke-width="3"/>
                  <polygon points="62,34 86,48 86,76 62,90 38,76 38,48" fill="none" stroke="\(sColor)" stroke-width="3" opacity="0.8"/>
                  <polygon points="38,34 62,48 62,76 38,90 14,76 14,48" fill="none" stroke="\(pColor)" stroke-width="3" opacity="0.6"/>
                </svg>
                """
            }

        case .prism:
            let v = options.variantIndex % 4
            switch v {
            case 0:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <polygon points="50,14 86,40 14,40" fill="\(pColor)"/>
                  <polygon points="14,40 86,40 50,86" fill="\(sColor)"/>
                  <polygon points="50,14 50,86 14,40" fill="\(pColor)" opacity="0.6"/>
                  <polygon points="50,14 50,86 86,40" fill="\(sColor)" opacity="0.4"/>
                </svg>
                """
            case 1:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <polygon points="50,8 70,30 50,88" fill="\(sColor)"/>
                  <polygon points="50,8 30,30 50,88" fill="\(pColor)"/>
                  <polygon points="30,30 18,48 50,88" fill="\(pColor)" opacity="0.75"/>
                  <polygon points="70,30 82,48 50,88" fill="\(sColor)" opacity="0.75"/>
                  <line x1="50" y1="8" x2="50" y2="88" stroke="#FFFFFF" stroke-width="1.5" opacity="0.7"/>
                </svg>
                """
            case 2:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <polygon points="50,12 84,80 50,66" fill="\(sColor)"/>
                  <polygon points="50,12 16,80 50,66" fill="\(pColor)"/>
                  <polygon points="16,80 84,80 50,66" fill="\(pColor)" opacity="0.6"/>
                  <line x1="50" y1="12" x2="50" y2="66" stroke="#FFFFFF" stroke-width="1.5" opacity="0.8"/>
                </svg>
                """
            default:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <polygon points="50,12 84,31 84,69 50,88 16,69 16,31" fill="none" stroke="\(pColor)" stroke-width="3"/>
                  <polygon points="50,26 72,39 72,61 50,74 28,61 28,39" fill="\(sColor)" opacity="0.85"/>
                  <line x1="50" y1="12" x2="50" y2="26" stroke="\(pColor)" stroke-width="2"/>
                  <line x1="84" y1="31" x2="72" y2="39" stroke="\(pColor)" stroke-width="2"/>
                  <line x1="84" y1="69" x2="72" y2="61" stroke="\(pColor)" stroke-width="2"/>
                  <line x1="50" y1="88" x2="50" y2="74" stroke="\(pColor)" stroke-width="2"/>
                  <line x1="16" y1="69" x2="28" y2="61" stroke="\(pColor)" stroke-width="2"/>
                  <line x1="16" y1="31" x2="28" y2="39" stroke="\(pColor)" stroke-width="2"/>
                </svg>
                """
            }

        case .shield:
            let v = options.variantIndex % 4
            switch v {
            case 0:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <path d="M 50 12 L 82 24 C 82 58 50 82 50 88 C 50 82 18 58 18 24 Z" fill="\(pColor)"/>
                  <path d="M 50 24 L 72 33 C 72 56 50 72 50 76 C 50 72 28 56 28 33 Z" fill="\(sColor)"/>
                  <path d="M 45 44 L 50 49 L 62 37" fill="none" stroke="#FFFFFF" stroke-width="5" stroke-linecap="round" stroke-linejoin="round"/>
                </svg>
                """
            case 1:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <polygon points="50,10 84,24 76,70 50,90 24,70 16,24" fill="none" stroke="\(pColor)" stroke-width="4"/>
                  <polygon points="50,22 72,32 66,62 50,78 34,62 28,32" fill="\(sColor)"/>
                  <circle cx="50" cy="48" r="8" fill="#FFFFFF"/>
                </svg>
                """
            case 2:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <path d="M 50 10 L 84 22 C 84 62 50 88 50 88 C 50 88 16 62 16 22 Z" fill="none" stroke="\(pColor)" stroke-width="4"/>
                  <rect x="38" y="46" width="24" height="18" rx="4" fill="\(sColor)"/>
                  <path d="M 42 46 L 42 38 C 42 33 58 33 58 38 L 58 46" fill="none" stroke="\(pColor)" stroke-width="3" stroke-linecap="round"/>
                  <circle cx="50" cy="54" r="2.5" fill="#FFFFFF"/>
                </svg>
                """
            default:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <path d="M 50 12 L 82 24 C 82 58 50 82 50 88 C 50 82 18 58 18 24 Z" fill="none" stroke="\(pColor)" stroke-width="4"/>
                  <path d="M 50 14 L 80 25 C 80 52 50 50 50 50 Z" fill="\(pColor)"/>
                  <path d="M 20 25 L 50 14 L 50 50 C 50 50 20 52 20 25 Z" fill="\(sColor)"/>
                  <path d="M 50 50 C 50 50 80 52 74 72 L 50 86 Z" fill="\(sColor)"/>
                  <path d="M 26 72 C 20 52 50 50 50 50 L 50 86 Z" fill="\(pColor)"/>
                  <line x1="50" y1="12" x2="50" y2="88" stroke="#FFFFFF" stroke-width="2"/>
                  <line x1="18" y1="48" x2="82" y2="48" stroke="#FFFFFF" stroke-width="2"/>
                </svg>
                """
            }

        case .terminal:
            let v = options.variantIndex % 4
            switch v {
            case 0:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <rect x="14" y="20" width="72" height="60" rx="8" fill="none" stroke="\(pColor)" stroke-width="4"/>
                  <circle cx="24" cy="28" r="2.5" fill="\(sColor)"/>
                  <circle cx="32" cy="28" r="2.5" fill="\(sColor)" opacity="0.7"/>
                  <circle cx="40" cy="28" r="2.5" fill="\(sColor)" opacity="0.4"/>
                  <line x1="14" y1="36" x2="86" y2="36" stroke="\(pColor)" stroke-width="1.5" opacity="0.5"/>
                  <path d="M 26 48 L 36 56 L 26 64" fill="none" stroke="\(pColor)" stroke-width="4" stroke-linecap="round" stroke-linejoin="round"/>
                  <line x1="44" y1="64" x2="60" y2="64" stroke="\(sColor)" stroke-width="4" stroke-linecap="round"/>
                </svg>
                """
            case 1:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <path d="M 32 32 L 16 50 L 32 68" fill="none" stroke="\(pColor)" stroke-width="5" stroke-linecap="round" stroke-linejoin="round"/>
                  <path d="M 68 32 L 84 50 L 68 68" fill="none" stroke="\(pColor)" stroke-width="5" stroke-linecap="round" stroke-linejoin="round"/>
                  <line x1="56" y1="26" x2="44" y2="74" stroke="\(sColor)" stroke-width="5" stroke-linecap="round"/>
                </svg>
                """
            case 2:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <circle cx="50" cy="50" r="42" fill="none" stroke="\(pColor)" stroke-width="2" stroke-dasharray="4 4" opacity="0.4"/>
                  <path d="M 34 24 C 24 24 24 38 24 44 C 24 48 18 50 14 50 C 18 50 24 52 24 56 C 24 62 24 76 34 76" fill="none" stroke="\(pColor)" stroke-width="4.5" stroke-linecap="round"/>
                  <path d="M 66 24 C 76 24 76 38 76 44 C 76 48 82 50 86 50 C 82 50 76 52 76 56 C 76 62 76 76 66 76" fill="none" stroke="\(pColor)" stroke-width="4.5" stroke-linecap="round"/>
                  <circle cx="50" cy="50" r="5" fill="\(sColor)"/>
                </svg>
                """
            default:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <path d="M 20 30 L 40 50 L 20 70" fill="none" stroke="\(pColor)" stroke-width="6" stroke-linecap="round" stroke-linejoin="round"/>
                  <path d="M 44 30 L 64 50 L 44 70" fill="none" stroke="\(sColor)" stroke-width="6" stroke-linecap="round" stroke-linejoin="round"/>
                  <rect x="74" y="62" width="14" height="8" rx="2" fill="\(pColor)"/>
                </svg>
                """
            }

        case .waveform:
            let v = options.variantIndex % 4
            switch v {
            case 0:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <rect x="14" y="42" width="6" height="16" rx="3" fill="\(sColor)"/>
                  <rect x="26" y="30" width="6" height="40" rx="3" fill="\(pColor)"/>
                  <rect x="38" y="18" width="6" height="64" rx="3" fill="\(pColor)"/>
                  <rect x="50" y="24" width="6" height="52" rx="3" fill="\(sColor)"/>
                  <rect x="62" y="16" width="6" height="68" rx="3" fill="\(pColor)"/>
                  <rect x="74" y="34" width="6" height="32" rx="3" fill="\(sColor)"/>
                  <rect x="86" y="44" width="6" height="12" rx="3" fill="\(pColor)"/>
                </svg>
                """
            case 1:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <circle cx="50" cy="50" r="42" fill="none" stroke="\(pColor)" stroke-width="2" stroke-dasharray="4 4" opacity="0.4"/>
                  <path d="M 12 50 L 32 50 L 40 24 L 48 76 L 56 36 L 64 58 L 70 50 L 88 50" fill="none" stroke="\(sColor)" stroke-width="4.5" stroke-linecap="round" stroke-linejoin="round"/>
                  <circle cx="48" cy="76" r="4" fill="\(pColor)"/>
                </svg>
                """
            case 2:
                var rays: [String] = []
                for i in 0..<12 {
                    let rot = i * 30
                    let h = 8 + ((i * 7 + options.seed) % 18)
                    rays.append("<line x1=\"50\" y1=\"\(24 - h)\" x2=\"50\" y2=\"24\" stroke=\"\(i % 2 == 0 ? pColor : sColor)\" stroke-width=\"3.5\" stroke-linecap=\"round\" transform=\"rotate(\(rot) 50 50)\"/>")
                }
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <circle cx="50" cy="50" r="22" fill="none" stroke="\(pColor)" stroke-width="3"/>
                  <circle cx="50" cy="50" r="8" fill="\(sColor)"/>
                  \(rays.joined(separator: "\n  "))
                </svg>
                """
            default:
                return """
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
                  <circle cx="36" cy="50" r="8" fill="\(pColor)"/>
                  <path d="M 48 34 A 20 20 0 0 1 48 66" fill="none" stroke="\(sColor)" stroke-width="4" stroke-linecap="round"/>
                  <path d="M 60 22 A 36 36 0 0 1 60 78" fill="none" stroke="\(pColor)" stroke-width="4.5" stroke-linecap="round"/>
                  <path d="M 72 10 A 52 52 0 0 1 72 90" fill="none" stroke="\(sColor)" stroke-width="5" stroke-linecap="round" opacity="0.8"/>
                </svg>
                """
            }
        }
    }

    public static func generateSVGData(options: ProceduralGlyphOptions) -> Data {
        generateSVG(options: options).data(using: .utf8) ?? Data()
    }

    public static func extractInitials(from text: String) -> String {
        let clean = ItemTitle.clean(text)
        let words = clean.split(separator: " ")
        if words.count >= 2 {
            let first = words[0].prefix(1)
            let second = words[1].prefix(1)
            return "\(first)\(second)".uppercased()
        }
        return String(clean.prefix(1)).uppercased()
    }

    public static func generateCustomMonogramSVG(
        text: String,
        fontStyle: MonogramFontStyle = .modernSans,
        frameStyle: MonogramFrameStyle = .circularRing,
        primaryHex: UInt32 = 0x1A73E8,
        secondaryHex: UInt32 = 0x4285F4
    ) -> String {
        let cleanText = text.trimmingCharacters(in: .whitespacesAndNewlines).prefix(4)
        let letters = cleanText.isEmpty ? "M" : String(cleanText)
        let pColor = String(format: "#%06X", primaryHex & 0xFFFFFF)
        let sColor = String(format: "#%06X", secondaryHex & 0xFFFFFF)
        let ff = fontStyle.fontFamily
        let fontSize: String = switch letters.count {
        case 1: "48"
        case 2: "36"
        case 3: "26"
        default: "20"
        }

        switch frameStyle {
        case .minimal:
            return """
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
              <text x="50" y="64" font-family="\(ff)" font-size="\(fontSize)" font-weight="900" text-anchor="middle" fill="\(pColor)" letter-spacing="-1">\(letters)</text>
            </svg>
            """
        case .circularRing:
            return """
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
              <circle cx="50" cy="50" r="42" fill="none" stroke="\(pColor)" stroke-width="4" stroke-dasharray="180 12"/>
              <text x="50" y="63" font-family="\(ff)" font-size="\(fontSize)" font-weight="800" text-anchor="middle" fill="\(pColor)" letter-spacing="-1">\(letters)</text>
              <circle cx="74" cy="28" r="5" fill="\(sColor)"/>
            </svg>
            """
        case .squircleSolid:
            return """
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
              <rect x="14" y="14" width="72" height="72" rx="20" fill="\(pColor)"/>
              <rect x="18" y="18" width="64" height="64" rx="16" fill="none" stroke="\(sColor)" stroke-width="2" opacity="0.6"/>
              <text x="50" y="63" font-family="\(ff)" font-size="\(fontSize)" font-weight="800" text-anchor="middle" fill="#FFFFFF" letter-spacing="-1">\(letters)</text>
            </svg>
            """
        case .hexagonFrame:
            return """
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
              <polygon points="50,8 88,30 88,70 50,92 12,70 12,30" fill="none" stroke="\(pColor)" stroke-width="4"/>
              <polygon points="50,14 82,33 82,67 50,86 18,67 18,33" fill="none" stroke="\(sColor)" stroke-width="1.5" opacity="0.4"/>
              <rect x="34" y="74" width="32" height="4" rx="2" fill="\(sColor)"/>
              <text x="50" y="61" font-family="\(ff)" font-size="\(fontSize)" font-weight="800" text-anchor="middle" fill="\(pColor)" letter-spacing="-1">\(letters)</text>
            </svg>
            """
        case .doubleOrbit:
            return """
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
              <circle cx="50" cy="50" r="44" fill="none" stroke="\(sColor)" stroke-width="2" stroke-dasharray="8 6"/>
              <circle cx="50" cy="50" r="35" fill="none" stroke="\(pColor)" stroke-width="3"/>
              <line x1="50" y1="6" x2="50" y2="15" stroke="\(pColor)" stroke-width="3" stroke-linecap="round"/>
              <line x1="50" y1="85" x2="50" y2="94" stroke="\(pColor)" stroke-width="3" stroke-linecap="round"/>
              <line x1="6" y1="50" x2="15" y2="50" stroke="\(pColor)" stroke-width="3" stroke-linecap="round"/>
              <line x1="85" y1="50" x2="94" y2="50" stroke="\(pColor)" stroke-width="3" stroke-linecap="round"/>
              <text x="50" y="63" font-family="\(ff)" font-size="\(fontSize)" font-weight="800" text-anchor="middle" fill="\(pColor)" letter-spacing="-1">\(letters)</text>
            </svg>
            """
        case .diamondCut:
            return """
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
              <polygon points="50,6 94,50 50,94 6,50" fill="none" stroke="\(pColor)" stroke-width="3.5"/>
              <polygon points="50,14 86,50 50,86 14,50" fill="none" stroke="\(sColor)" stroke-width="1.5" opacity="0.5"/>
              <text x="50" y="63" font-family="\(ff)" font-size="\(fontSize)" font-weight="800" text-anchor="middle" fill="\(pColor)" letter-spacing="-1">\(letters)</text>
            </svg>
            """
        }
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
