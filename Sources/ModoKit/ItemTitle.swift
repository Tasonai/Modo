import Foundation

/// An immutable snapshot restore point of applied application icons.
public struct SystemBackupPoint: Identifiable, Codable, Sendable, Hashable {
    public let id: UUID
    public let date: Date
    public let name: String
    public let appPaths: [String]
    public let packName: String

    public init(id: UUID = UUID(), date: Date = .now, name: String, appPaths: [String], packName: String) {
        self.id = id
        self.date = date
        self.name = name
        self.appPaths = appPaths
        self.packName = packName
    }
}

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

/// Visual appearance theme mode (Light, Dark, Tinted, or Auto/System in macOS Sequoia & iOS 18 style).
public enum IconAppearanceMode: String, CaseIterable, Identifiable, Codable, Sendable {
    case light = "Light"
    case dark = "Dark"
    case tinted = "Tinted"
    case system = "Auto"

    public var id: String { rawValue }
}

/// Value type holding all parameters for rendering a Modo adaptive icon.
public struct RoundIconStyle: Codable, Hashable, Sendable {
    public var backgroundStyle: RoundBackgroundStyle
    /// Icon geometry shape (circle, squircle, rounded rect, pebble, hexagon).
    public var shape: IconShape
    /// Foreground glyph scale inside the canvas disc (0.30 ... 2.50, i.e. 30% to 250%).
    public var iconScale: Double
    /// Horizontal pan offset inside the shape (-1.0 ... 1.0, fraction of radius; negative = left, positive = right).
    public var offsetX: Double
    /// Vertical pan offset inside the shape (-1.0 ... 1.0, fraction of radius; negative = down, positive = up).
    public var offsetY: Double
    /// Outer drop shadow intensity (0.0 ... 1.0).
    public var shadowIntensity: Double
    /// Adds a subtle drop shadow directly under the foreground glyph.
    public var innerShadow: Bool
    /// Expands square/squircle full-bleed icons to fill the canvas seamlessly.
    public var fullBleedSquares: Bool
    /// Applies a Material You monochrome tint to the foreground glyph.
    public var monochromeTint: Bool
    /// Automatically crops transparent padding around the original icon before scaling.
    public var autoTrim: Bool
    /// Custom background hex color (0xRRGGBB).
    public var customHex: UInt32
    /// Visual appearance theme mode (macOS Sequoia / iOS 18 style).
    public var appearanceMode: IconAppearanceMode
    /// Tint color used in Tinted appearance mode (0xRRGGBB).
    public var tintColorHex: UInt32

    enum CodingKeys: String, CodingKey {
        case backgroundStyle, shape, iconScale, offsetX, offsetY
        case shadowIntensity, innerShadow, fullBleedSquares, monochromeTint
        case autoTrim, customHex, appearanceMode, tintColorHex
    }

    public init(
        backgroundStyle: RoundBackgroundStyle = .googleWhite,
        shape: IconShape = .circle,
        iconScale: Double = 0.68,
        offsetX: Double = 0.0,
        offsetY: Double = 0.0,
        shadowIntensity: Double = 0.45,
        innerShadow: Bool = true,
        fullBleedSquares: Bool = false,
        monochromeTint: Bool = false,
        autoTrim: Bool = true,
        customHex: UInt32 = 0xE8F0FE,
        appearanceMode: IconAppearanceMode = .light,
        tintColorHex: UInt32 = 0x007AFF
    ) {
        self.backgroundStyle = backgroundStyle
        self.shape = shape
        self.iconScale = min(max(iconScale, 0.30), 2.50)
        self.offsetX = min(max(offsetX, -1.0), 1.0)
        self.offsetY = min(max(offsetY, -1.0), 1.0)
        self.shadowIntensity = min(max(shadowIntensity, 0.0), 1.0)
        self.innerShadow = innerShadow
        self.fullBleedSquares = fullBleedSquares
        self.monochromeTint = monochromeTint
        self.autoTrim = autoTrim
        self.customHex = customHex & 0xFFFFFF
        self.appearanceMode = appearanceMode
        self.tintColorHex = tintColorHex & 0xFFFFFF
    }

    public init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        self.backgroundStyle = try container.decode(RoundBackgroundStyle.self, forKey: .backgroundStyle)
        self.shape = try container.decodeIfPresent(IconShape.self, forKey: .shape) ?? .circle
        let scale = try container.decodeIfPresent(Double.self, forKey: .iconScale) ?? 0.68
        self.iconScale = min(max(scale, 0.30), 2.50)
        let ox = try container.decodeIfPresent(Double.self, forKey: .offsetX) ?? 0.0
        self.offsetX = min(max(ox, -1.0), 1.0)
        let oy = try container.decodeIfPresent(Double.self, forKey: .offsetY) ?? 0.0
        self.offsetY = min(max(oy, -1.0), 1.0)
        let shadow = try container.decodeIfPresent(Double.self, forKey: .shadowIntensity) ?? 0.45
        self.shadowIntensity = min(max(shadow, 0.0), 1.0)
        self.innerShadow = try container.decodeIfPresent(Bool.self, forKey: .innerShadow) ?? true
        self.fullBleedSquares = try container.decodeIfPresent(Bool.self, forKey: .fullBleedSquares) ?? false
        self.monochromeTint = try container.decodeIfPresent(Bool.self, forKey: .monochromeTint) ?? false
        self.autoTrim = try container.decodeIfPresent(Bool.self, forKey: .autoTrim) ?? true
        self.customHex = try container.decodeIfPresent(UInt32.self, forKey: .customHex) ?? 0xE8F0FE
        self.appearanceMode = try container.decodeIfPresent(IconAppearanceMode.self, forKey: .appearanceMode) ?? .light
        self.tintColorHex = try container.decodeIfPresent(UInt32.self, forKey: .tintColorHex) ?? 0x007AFF
    }

    public mutating func resetPosition() {
        offsetX = 0.0
        offsetY = 0.0
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
