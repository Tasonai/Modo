import Foundation

/// Pure business logic for IconMaker: title cleanup, Google Pixel / Material You
/// circular icon style parameters, pastel tint extraction math, and multi-size `.ico` builder.
public enum ItemTitle {
    /// Trims spaces, `.app` extension, and line breaks, collapsing inner runs of spaces.
    public static func clean(_ text: String) -> String {
        var trimmed = text.trimmingCharacters(in: .whitespacesAndNewlines)
        if trimmed.lowercased().hasSuffix(".app") {
            trimmed = String(trimmed.dropLast(4))
        }
        let words = trimmed.split(whereSeparator: \.isWhitespace)
        return words.isEmpty ? "Untitled App" : words.joined(separator: " ")
    }
}

/// Background style presets for Google Pixel / Material You circular icons.
public enum RoundBackgroundStyle: String, CaseIterable, Codable, Sendable, Identifiable {
    case googleWhite = "google_white"
    case materialPastel = "material_pastel"
    case darkMaterial = "dark_material"
    case adaptiveEdge = "adaptive_edge"
    case customColor = "custom_color"

    public var id: String { rawValue }

    public var title: String {
        switch self {
        case .googleWhite: "Google White"
        case .materialPastel: "Material You Pastel"
        case .darkMaterial: "Dark Material"
        case .adaptiveEdge: "Adaptive Edge"
        case .customColor: "Custom Color"
        }
    }

    public var subtitle: String {
        switch self {
        case .googleWhite: "Classic crisp white circular disc with subtle elevation"
        case .materialPastel: "Soft pastel surface tinted from each app's dominant color"
        case .darkMaterial: "Deep Pixel dark surface (#1E1F22) with luminous rim"
        case .adaptiveEdge: "Samples the outer edge of the original icon"
        case .customColor: "Uses your chosen hex surface color"
        }
    }

    public var symbol: String {
        switch self {
        case .googleWhite: "circle.fill"
        case .materialPastel: "paintpalette.fill"
        case .darkMaterial: "moon.circle.fill"
        case .adaptiveEdge: "eyedropper.halffull"
        case .customColor: "slider.horizontal.below.circle.lefthalf.filled"
        }
    }
}

/// Value type holding all parameters for rendering a Google-style circular icon.
public struct RoundIconStyle: Codable, Hashable, Sendable {
    public var backgroundStyle: RoundBackgroundStyle
    /// Foreground glyph scale inside the circular disc (0.50 ... 1.00).
    public var iconScale: Double
    /// Outer drop shadow intensity (0.0 ... 1.0).
    public var shadowIntensity: Double
    /// Adds a subtle drop shadow directly under the foreground glyph.
    public var innerShadow: Bool
    /// Expands square/squircle full-bleed icons to fill the circle seamlessly.
    public var fullBleedSquares: Bool
    /// Applies a Material You monochrome tint to the foreground glyph.
    public var monochromeTint: Bool
    /// Automatically crops transparent padding around the original icon before scaling.
    public var autoTrim: Bool
    /// Custom background hex color (0xRRGGBB).
    public var customHex: UInt32

    public init(
        backgroundStyle: RoundBackgroundStyle = .googleWhite,
        iconScale: Double = 0.68,
        shadowIntensity: Double = 0.45,
        innerShadow: Bool = true,
        fullBleedSquares: Bool = false,
        monochromeTint: Bool = false,
        autoTrim: Bool = true,
        customHex: UInt32 = 0xE8F0FE
    ) {
        self.backgroundStyle = backgroundStyle
        self.iconScale = min(max(iconScale, 0.50), 1.00)
        self.shadowIntensity = min(max(shadowIntensity, 0.0), 1.0)
        self.innerShadow = innerShadow
        self.fullBleedSquares = fullBleedSquares
        self.monochromeTint = monochromeTint
        self.autoTrim = autoTrim
        self.customHex = customHex & 0xFFFFFF
    }
}

/// Pure RGB color math for Material You pastel extraction and contrast calculation.
public struct MaterialRGB: Equatable, Hashable, Sendable {
    public var r: UInt8
    public var g: UInt8
    public var b: UInt8

    public init(r: UInt8, g: UInt8, b: UInt8) {
        self.r = r
        self.g = g
        self.b = b
    }

    public init(hex: UInt32) {
        self.r = UInt8((hex >> 16) & 0xFF)
        self.g = UInt8((hex >> 8) & 0xFF)
        self.b = UInt8(hex & 0xFF)
    }

    public var hex: UInt32 {
        (UInt32(r) << 16) | (UInt32(g) << 8) | UInt32(b)
    }

    public var hexString: String {
        String(format: "#%02X%02X%02X", r, g, b)
    }

    /// Blends this color toward white by `whiteMix` (0...1) to produce a soft Material You pastel disc.
    public func materialPastel(whiteMix: Double = 0.78) -> MaterialRGB {
        let mix = min(max(whiteMix, 0.0), 1.0)
        let nr = UInt8(clamping: Int((Double(r) * (1.0 - mix) + 255.0 * mix).rounded()))
        let ng = UInt8(clamping: Int((Double(g) * (1.0 - mix) + 255.0 * mix).rounded()))
        let nb = UInt8(clamping: Int((Double(b) * (1.0 - mix) + 255.0 * mix).rounded()))
        return MaterialRGB(r: nr, g: ng, b: nb)
    }

    /// Produces a deep high-contrast Material You foreground tint for monochrome mode.
    public func materialDeepInk() -> MaterialRGB {
        let nr = UInt8(clamping: Int((Double(r) * 0.42).rounded()))
        let ng = UInt8(clamping: Int((Double(g) * 0.42).rounded()))
        let nb = UInt8(clamping: Int((Double(b) * 0.42).rounded()))
        return MaterialRGB(r: nr, g: ng, b: nb)
    }

    /// Parses a `#RRGGBB` or `RRGGBB` string into a `MaterialRGB`.
    public static func parseHex(_ text: String) -> MaterialRGB? {
        let cleaned = text.trimmingCharacters(in: .whitespacesAndNewlines)
            .replacingOccurrences(of: "#", with: "")
        guard cleaned.count == 6, let value = UInt32(cleaned, radix: 16) else { return nil }
        return MaterialRGB(hex: value)
    }
}

/// Packs PNG image buffers into a standard Windows `.ico` binary container.
public enum ICOBuilder {
    public struct Entry: Sendable {
        public let size: Int
        public let pngData: Data

        public init(size: Int, pngData: Data) {
            self.size = size
            self.pngData = pngData
        }
    }

    public static func build(entries: [Entry]) -> Data {
        guard !entries.isEmpty else { return Data() }
        var out = Data()
        // ICONDIR header (6 bytes): reserved=0, type=1 (icon), count=entries.count
        out.append(contentsOf: [0, 0, 1, 0])
        let count = UInt16(entries.count)
        out.append(UInt8(count & 0xFF))
        out.append(UInt8((count >> 8) & 0xFF))

        var dataOffset = 6 + entries.count * 16
        for entry in entries {
            let dimByte: UInt8 = entry.size >= 256 ? 0 : UInt8(entry.size)
            out.append(dimByte) // width
            out.append(dimByte) // height
            out.append(0)       // color count
            out.append(0)       // reserved
            out.append(contentsOf: [1, 0]) // color planes = 1
            out.append(contentsOf: [32, 0]) // bits per pixel = 32
            let length = UInt32(entry.pngData.count)
            out.append(contentsOf: [
                UInt8(length & 0xFF),
                UInt8((length >> 8) & 0xFF),
                UInt8((length >> 16) & 0xFF),
                UInt8((length >> 24) & 0xFF),
            ])
            let offset = UInt32(dataOffset)
            out.append(contentsOf: [
                UInt8(offset & 0xFF),
                UInt8((offset >> 8) & 0xFF),
                UInt8((offset >> 16) & 0xFF),
                UInt8((offset >> 24) & 0xFF),
            ])
            dataOffset += entry.pngData.count
        }

        for entry in entries {
            out.append(entry.pngData)
        }
        return out
    }
}
