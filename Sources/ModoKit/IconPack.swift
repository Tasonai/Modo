import Foundation

/// Curated icon design pack for one-click system-wide aesthetic customization.
public struct IconPack: Identifiable, Hashable, Sendable {
    public let id: String
    public let name: String
    public let subtitle: String
    public let category: String
    public let iconSymbol: String
    public let style: RoundIconStyle
    public let accentHex: UInt32

    public init(
        id: String,
        name: String,
        subtitle: String,
        category: String,
        iconSymbol: String,
        style: RoundIconStyle,
        accentHex: UInt32
    ) {
        self.id = id
        self.name = name
        self.subtitle = subtitle
        self.category = category
        self.iconSymbol = iconSymbol
        self.style = style
        self.accentHex = accentHex
    }

    /// Classic crisp white circular disc with soft elevation (Google Pixel style).
    public static let pixelPure = IconPack(
        id: "pixel_pure",
        name: "Pixel Pure",
        subtitle: "Classic crisp white disc with soft drop shadow",
        category: "Google",
        iconSymbol: "circle.fill",
        style: RoundIconStyle(
            backgroundStyle: .googleWhite,
            shape: .circle,
            iconScale: 0.68,
            shadowIntensity: 0.40,
            innerShadow: true,
            monochromeTint: false
        ),
        accentHex: 0xFFFFFF
    )

    /// Dynamic pastel palette adapted to app branding with high-contrast glyph.
    public static let materialPastel = IconPack(
        id: "material_pastel",
        name: "Material You",
        subtitle: "Dynamic pastel palette adapted to app branding",
        category: "Google",
        iconSymbol: "paintpalette.fill",
        style: RoundIconStyle(
            backgroundStyle: .materialPastel,
            shape: .circle,
            iconScale: 0.70,
            shadowIntensity: 0.35,
            innerShadow: true,
            monochromeTint: false
        ),
        accentHex: 0xD3E3FD
    )

    /// Deep matte obsidian disc for dark workspaces and OLED displays.
    public static let modoNoir = IconPack(
        id: "modo_noir",
        name: "Modo Noir",
        subtitle: "Deep matte obsidian disc for dark workspaces",
        category: "Atoma",
        iconSymbol: "moon.fill",
        style: RoundIconStyle(
            backgroundStyle: .darkMaterial,
            shape: .circle,
            iconScale: 0.66,
            shadowIntensity: 0.50,
            innerShadow: true,
            monochromeTint: false
        ),
        accentHex: 0x1E1F22
    )

    /// Strict achromatic slate plate with monochrome ink in the signature Atoma aesthetic.
    public static let atomaMono = IconPack(
        id: "atoma_mono",
        name: "Atoma Mono",
        subtitle: "Achromatic slate plate with high-contrast monochrome ink",
        category: "Atoma",
        iconSymbol: "circle.lefthalf.striped.horizontal",
        style: RoundIconStyle(
            backgroundStyle: .darkMaterial,
            shape: .squircle,
            iconScale: 0.64,
            shadowIntensity: 0.30,
            innerShadow: false,
            monochromeTint: true
        ),
        accentHex: 0x2A2B2E
    )

    /// macOS-like superellipse plate.
    public static let squircleStudio = IconPack(
        id: "squircle_studio",
        name: "Squircle Studio",
        subtitle: "Smooth superellipse plate with balanced inner glyph",
        category: "Apple",
        iconSymbol: "app.fill",
        style: RoundIconStyle(
            backgroundStyle: .googleWhite,
            shape: .squircle,
            iconScale: 0.72,
            shadowIntensity: 0.35,
            innerShadow: true,
            monochromeTint: false
        ),
        accentHex: 0xF3F4F6
    )

    /// Warm ivory parchment (#F6F3EB) with subtle edge contrast and editorial elegance.
    public static let editorialPaper = IconPack(
        id: "editorial_paper",
        name: "Editorial Paper",
        subtitle: "Warm ivory parchment (#F6F3EB) with editorial elegance",
        category: "Editorial",
        iconSymbol: "doc.text.image",
        style: RoundIconStyle(
            backgroundStyle: .customColor,
            shape: .squircle,
            iconScale: 0.68,
            shadowIntensity: 0.25,
            innerShadow: false,
            monochromeTint: false,
            customHex: 0xF6F3EB
        ),
        accentHex: 0xF6F3EB
    )

    /// Futuristic hexagonal plate with dark titanium finish.
    public static let cyberHex = IconPack(
        id: "cyber_hex",
        name: "Cyber Hex",
        subtitle: "Futuristic hexagonal plate with dark titanium finish",
        category: "Tech",
        iconSymbol: "hexagon.fill",
        style: RoundIconStyle(
            backgroundStyle: .darkMaterial,
            shape: .hexagon,
            iconScale: 0.65,
            shadowIntensity: 0.55,
            innerShadow: true,
            monochromeTint: false
        ),
        accentHex: 0x111215
    )

    /// Warm dark palette with soothing pastel lavender (#CBA6F7) from Catppuccin Mocha.
    public static let catppuccinMocha = IconPack(
        id: "catppuccin_mocha",
        name: "Catppuccin",
        subtitle: "Warm dark palette with soothing pastel lavender (#CBA6F7)",
        category: "Community",
        iconSymbol: "cup.and.saucer.fill",
        style: RoundIconStyle(
            backgroundStyle: .customColor,
            shape: .squircle,
            iconScale: 0.68,
            shadowIntensity: 0.35,
            innerShadow: false,
            monochromeTint: false,
            customHex: 0x1E1E2E
        ),
        accentHex: 0xCBA6F7
    )

    /// Arctic cold slate (#2E3440) with polar ice blue glyphs inspired by the Nord theme.
    public static let nordicFrost = IconPack(
        id: "nordic_frost",
        name: "Nordic Frost",
        subtitle: "Arctic cold slate (#2E3440) with polar ice blue (#88C0D0)",
        category: "Community",
        iconSymbol: "snowflake",
        style: RoundIconStyle(
            backgroundStyle: .customColor,
            shape: .squircle,
            iconScale: 0.68,
            shadowIntensity: 0.30,
            innerShadow: false,
            monochromeTint: false,
            customHex: 0x2E3440
        ),
        accentHex: 0x88C0D0
    )

    /// Cyberpunk dark storm (#1A1B26) with vibrant neon glow inspired by Tokyo Night.
    public static let tokyoNight = IconPack(
        id: "tokyo_night",
        name: "Tokyo Night",
        subtitle: "Cyberpunk dark storm (#1A1B26) with electric indigo glow",
        category: "Tech",
        iconSymbol: "sparkles",
        style: RoundIconStyle(
            backgroundStyle: .customColor,
            shape: .hexagon,
            iconScale: 0.66,
            shadowIntensity: 0.50,
            innerShadow: true,
            monochromeTint: false,
            customHex: 0x1A1B26
        ),
        accentHex: 0x7AA2F7
    )

    /// Legendary dark contrast theme (#282A36) with vibrant purple accent.
    public static let draculaVamp = IconPack(
        id: "dracula_vamp",
        name: "Dracula",
        subtitle: "Dark contrast theme (#282A36) with vibrant purple accent",
        category: "Community",
        iconSymbol: "bolt.heart.fill",
        style: RoundIconStyle(
            backgroundStyle: .customColor,
            shape: .squircle,
            iconScale: 0.68,
            shadowIntensity: 0.40,
            innerShadow: false,
            monochromeTint: false,
            customHex: 0x282A36
        ),
        accentHex: 0xBD93F9
    )

    /// Soft organic pebble shape with warm twilight amber (#F97316).
    public static let sunsetPebble = IconPack(
        id: "sunset_pebble",
        name: "Sunset Pebble",
        subtitle: "Soft organic pebble shape with warm twilight amber",
        category: "Aesthetic",
        iconSymbol: "sunset.fill",
        style: RoundIconStyle(
            backgroundStyle: .customColor,
            shape: .pebble,
            iconScale: 0.70,
            shadowIntensity: 0.35,
            innerShadow: false,
            monochromeTint: false,
            customHex: 0x2D1B28
        ),
        accentHex: 0xF97316
    )

    /// Microsoft Fluent-inspired crisp squircle with deep gentle elevation.
    public static let fluentSoft = IconPack(
        id: "fluent_soft",
        name: "Fluent Soft",
        subtitle: "Crisp squircle plate with deep gentle elevation",
        category: "Design",
        iconSymbol: "square.stack.3d.up.fill",
        style: RoundIconStyle(
            backgroundStyle: .googleWhite,
            shape: .squircle,
            iconScale: 0.74,
            shadowIntensity: 0.45,
            innerShadow: true,
            monochromeTint: false
        ),
        accentHex: 0x0078D4
    )

    /// Curated collection of all built-in Modo icon packs.
    public static let allPacks: [IconPack] = [
        .pixelPure,
        .materialPastel,
        .modoNoir,
        .atomaMono,
        .squircleStudio,
        .editorialPaper,
        .cyberHex,
        .catppuccinMocha,
        .nordicFrost,
        .tokyoNight,
        .draculaVamp,
        .sunsetPebble,
        .fluentSoft
    ]

    /// Finds a pack matching the given ID or falls back to Pixel Pure.
    public static func find(id: String) -> IconPack {
        allPacks.first(where: { $0.id == id }) ?? .pixelPure
    }
}
