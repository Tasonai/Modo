import Foundation
#if canImport(AppKit)
import AppKit
#endif

/// Category of vector artwork variant available for an application or icon.
public enum VectorVariantKind: String, Codable, Sendable, CaseIterable {
    case original = "Original"
    case brandMono = "Brand Monochrome"
    case brandColor = "Brand Color"
    case sfSymbol = "SF Symbol"
    case monogram = "Monogram"
    case geometric = "Geometric"
    case remoteCDN = "CDN Brand"
}

/// A specific vector rendering variant discovered or generated for an icon.
public struct VectorVariant: Identifiable, Sendable, Hashable {
    public let id: String
    public let title: String
    public let subtitle: String
    public let badge: String
    public let kind: VectorVariantKind
    public let brandHex: Int?
    public let symbolName: String?
    public let svgMarkup: String?
    public let rawData: Data?

    public init(
        id: String,
        title: String,
        subtitle: String,
        badge: String,
        kind: VectorVariantKind,
        brandHex: Int? = nil,
        symbolName: String? = nil,
        svgMarkup: String? = nil,
        rawData: Data? = nil
    ) {
        self.id = id
        self.title = title
        self.subtitle = subtitle
        self.badge = badge
        self.kind = kind
        self.brandHex = brandHex
        self.symbolName = symbolName
        self.svgMarkup = svgMarkup
        self.rawData = rawData
    }

    /// Binary data representing this vector variant (SVG or high-res bitmap).
    public var applicationData: Data? {
        if let raw = rawData { return raw }
        if let svg = svgMarkup { return svg.data(using: .utf8) }
        return nil
    }

    /// Evaluates if this variant matches the currently applied settings of an item.
    public func isActive(customImageData: Data?, currentSymbolName: String?, currentBrandHex: Int?) -> Bool {
        switch kind {
        case .original:
            return customImageData == nil
        case .brandMono, .brandColor, .remoteCDN:
            guard let activeData = customImageData, let selfData = applicationData else { return false }
            if activeData == selfData { return true }
            if let activeStr = String(data: activeData, encoding: .utf8),
               let selfStr = String(data: selfData, encoding: .utf8) {
                return activeStr.trimmingCharacters(in: .whitespacesAndNewlines) == selfStr.trimmingCharacters(in: .whitespacesAndNewlines)
            }
            return false
        case .sfSymbol:
            if let sym = symbolName, let curSym = currentSymbolName, sym == curSym {
                return true
            }
            if let activeData = customImageData, let selfData = applicationData, activeData == selfData {
                return true
            }
            return false
        case .monogram, .geometric:
            guard let activeData = customImageData, let selfData = applicationData else { return false }
            return activeData == selfData
        }
    }
}

/// Engine that automatically discovers, generates, and loads multiple vector variants for each application.
public enum VectorVariantEngine {

    /// Renders an Apple SF Symbol to crisp white PNG data on macOS.
    public static func renderSFSymbolData(symbolName: String, size: CGFloat = 512) -> Data? {
        #if canImport(AppKit)
        let config = NSImage.SymbolConfiguration(pointSize: size * 0.50, weight: .regular)
        guard let sym = NSImage(systemSymbolName: symbolName, accessibilityDescription: nil)?.withSymbolConfiguration(config) else {
            return nil
        }
        let image = NSImage(size: NSSize(width: size, height: size))
        image.lockFocus()
        NSColor.white.set()
        let rect = NSRect(
            x: (size - sym.size.width) / 2.0,
            y: (size - sym.size.height) / 2.0,
            width: sym.size.width,
            height: sym.size.height
        )
        sym.draw(in: rect, from: .zero, operation: .sourceOver, fraction: 1.0)
        image.unlockFocus()

        guard let tiff = image.tiffRepresentation,
              let rep = NSBitmapImageRep(data: tiff),
              let png = rep.representation(using: .png, properties: [:]) else {
            return nil
        }
        return png
        #else
        return nil
        #endif
    }

    /// Selects contextual SF Symbols based on application category, bundle identifier, and name.
    public static func smartSFSymbols(
        forTitle title: String,
        bundleIdentifier: String,
        categoryName: String
    ) -> [(symbol: String, label: String)] {
        let combined = " \(title) \(bundleIdentifier) \(categoryName) ".lowercased()

        if combined.contains("chat") || combined.contains("messag") || combined.contains("telegram") ||
            combined.contains("slack") || combined.contains("discord") || combined.contains("mail") ||
            combined.contains("talk") || combined.contains("whatsapp") || combined.contains("signal") ||
            combined.contains("communicat") {
            return [
                ("bubble.left.and.bubble.right.fill", "Диалог"),
                ("paperplane.fill", "Отправка"),
                ("message.fill", "Чат"),
                ("envelope.fill", "Почта")
            ]
        }

        if combined.contains("develop") || combined.contains("git") || combined.contains("term") ||
            combined.contains("xcode") || combined.contains("cursor") || combined.contains("script") ||
            combined.contains("docker") || combined.contains("sublime") || combined.contains("vscode") ||
            combined.contains(" code") || combined.contains("code ") || combined.contains(".code") {
            return [
                ("terminal.fill", "Терминал"),
                ("chevron.left.forwardslash.chevron.right", "Код"),
                ("curlybraces", "Скрипт"),
                ("cpu", "Процессор")
            ]
        }

        if combined.contains("browser") || combined.contains("web") || combined.contains("safari") ||
            combined.contains("chrome") || combined.contains("firefox") || combined.contains("arc") ||
            combined.contains("net") {
            return [
                ("globe", "Интернет"),
                ("safari.fill", "Компас"),
                ("network", "Сеть"),
                ("arrow.triangle.2.circlepath", "Синхронизация")
            ]
        }

        if combined.contains("music") || combined.contains("audio") || combined.contains("sound") ||
            combined.contains("spotify") || combined.contains("pod") || combined.contains("vlc") {
            return [
                ("music.note", "Музыка"),
                ("headphones", "Наушники"),
                ("waveform", "Волна"),
                ("play.circle.fill", "Плеер")
            ]
        }

        if combined.contains("design") || combined.contains("photo") || combined.contains("draw") ||
            combined.contains("paint") || combined.contains("figma") || combined.contains("blender") ||
            combined.contains("sketch") || combined.contains("video") || combined.contains("movie") {
            return [
                ("paintpalette.fill", "Палитра"),
                ("wand.and.stars", "Эффекты"),
                ("camera.viewfinder", "Камера"),
                ("film.fill", "Фильм")
            ]
        }

        if combined.contains("note") || combined.contains("doc") || combined.contains("text") ||
            combined.contains("notion") || combined.contains("obsidian") || combined.contains("office") ||
            combined.contains("sheet") || combined.contains("write") {
            return [
                ("doc.text.fill", "Документ"),
                ("folder.fill", "Папка"),
                ("checklist", "Задачи"),
                ("calendar", "Календарь")
            ]
        }

        if combined.contains("pass") || combined.contains("secure") || combined.contains("guard") ||
            combined.contains("vpn") || combined.contains("auth") || combined.contains("1password") {
            return [
                ("shield.fill", "Защита"),
                ("lock.fill", "Замок"),
                ("key.fill", "Ключ"),
                ("touchid", "Биометрия")
            ]
        }

        // General default set
        return [
            ("sparkles", "Искра"),
            ("gearshape.fill", "Система"),
            ("app.fill", "Приложение"),
            ("square.grid.2x2.fill", "Сетка")
        ]
    }

    /// Automatically resolves all available local vector variants for an application.
    public static func resolveLocalVariants(
        title: String,
        bundleIdentifier: String = "",
        categoryName: String = "",
        brandHex: Int = 0x1A73E8
    ) -> [VectorVariant] {
        var variants: [VectorVariant] = []

        // 1. Original system icon
        variants.append(
            VectorVariant(
                id: "original",
                title: "Оригинал",
                subtitle: "По умолчанию",
                badge: "Native",
                kind: .original,
                brandHex: brandHex,
                symbolName: nil,
                svgMarkup: nil,
                rawData: nil
            )
        )

        // 2. Official Brand Icons (from offline Simple Icons catalog)
        if let icon = SimpleIconCatalog.find(for: title) {
            let monoSVG = icon.svgString(fillHex: 0xFFFFFF)
            variants.append(
                VectorVariant(
                    id: "brand-mono-\(icon.slug)",
                    title: "\(icon.title) Mono",
                    subtitle: "Белый вектор",
                    badge: "Simple Icons",
                    kind: .brandMono,
                    brandHex: Int(icon.hex),
                    symbolName: nil,
                    svgMarkup: monoSVG,
                    rawData: icon.svgData(fillHex: 0xFFFFFF)
                )
            )

            let colorSVG = icon.svgString(fillHex: icon.hex)
            variants.append(
                VectorVariant(
                    id: "brand-color-\(icon.slug)",
                    title: "\(icon.title) Brand",
                    subtitle: "Фирменный цвет",
                    badge: "Brand Color",
                    kind: .brandColor,
                    brandHex: Int(icon.hex),
                    symbolName: nil,
                    svgMarkup: colorSVG,
                    rawData: icon.svgData(fillHex: icon.hex)
                )
            )
        }

        // 3. Category & context-matched SF Symbols
        let symbols = smartSFSymbols(forTitle: title, bundleIdentifier: bundleIdentifier, categoryName: categoryName)
        for (symName, label) in symbols {
            let symbolData = renderSFSymbolData(symbolName: symName)
            variants.append(
                VectorVariant(
                    id: "symbol-\(symName)",
                    title: label,
                    subtitle: symName,
                    badge: "SF Symbol",
                    kind: .sfSymbol,
                    brandHex: brandHex,
                    symbolName: symName,
                    svgMarkup: nil,
                    rawData: symbolData
                )
            )
        }

        // 4. Procedural Algorithmic Vector Glyphs
        let primaryColor = UInt32(clamping: brandHex)
        let monogramSVG = ProceduralGlyphGenerator.generateSVG(
            options: ProceduralGlyphOptions(title: title, style: .monogram, primaryHex: primaryColor, secondaryHex: 0xFFFFFF)
        )
        variants.append(
            VectorVariant(
                id: "procedural-monogram",
                title: "Монограмма",
                subtitle: "Инициалы названия",
                badge: "SVG Initials",
                kind: .monogram,
                brandHex: brandHex,
                symbolName: nil,
                svgMarkup: monogramSVG,
                rawData: monogramSVG.data(using: .utf8)
            )
        )

        let sparkleSVG = ProceduralGlyphGenerator.generateSVG(
            options: ProceduralGlyphOptions(title: title, style: .sparkle, primaryHex: primaryColor, secondaryHex: 0x60A5FA)
        )
        variants.append(
            VectorVariant(
                id: "procedural-sparkle",
                title: "Сверхновая",
                subtitle: "Геометрическая звезда",
                badge: "SVG Vector",
                kind: .geometric,
                brandHex: brandHex,
                symbolName: nil,
                svgMarkup: sparkleSVG,
                rawData: sparkleSVG.data(using: .utf8)
            )
        )

        let apertureSVG = ProceduralGlyphGenerator.generateSVG(
            options: ProceduralGlyphOptions(title: title, style: .aperture, primaryHex: primaryColor, secondaryHex: 0x34D399)
        )
        variants.append(
            VectorVariant(
                id: "procedural-aperture",
                title: "Диафрагма",
                subtitle: "Многолепестковый ирис",
                badge: "SVG Vector",
                kind: .geometric,
                brandHex: brandHex,
                symbolName: nil,
                svgMarkup: apertureSVG,
                rawData: apertureSVG.data(using: .utf8)
            )
        )

        let shieldSVG = ProceduralGlyphGenerator.generateSVG(
            options: ProceduralGlyphOptions(title: title, style: .shield, primaryHex: primaryColor, secondaryHex: 0xF59E0B)
        )
        variants.append(
            VectorVariant(
                id: "procedural-shield",
                title: "Кибер-щит",
                subtitle: "Защитный контур",
                badge: "SVG Vector",
                kind: .geometric,
                brandHex: brandHex,
                symbolName: nil,
                svgMarkup: shieldSVG,
                rawData: shieldSVG.data(using: .utf8)
            )
        )

        return variants
    }

    /// Asynchronously fetches remote brand SVG from Simple Icons CDN if not already in local catalog.
    public static func fetchRemoteBrandVariants(for title: String) async -> [VectorVariant] {
        guard let icon = await SimpleIconCatalog.fetch(slugOrTitle: title) else {
            return []
        }

        let monoSVG = icon.svgString(fillHex: 0xFFFFFF)
        let monoVariant = VectorVariant(
            id: "cdn-mono-\(icon.slug)",
            title: "\(icon.title) CDN",
            subtitle: "Вектор Simple Icons",
            badge: "CDN Mono",
            kind: .remoteCDN,
            brandHex: Int(icon.hex),
            symbolName: nil,
            svgMarkup: monoSVG,
            rawData: icon.svgData(fillHex: 0xFFFFFF)
        )

        let colorSVG = icon.svgString(fillHex: icon.hex)
        let colorVariant = VectorVariant(
            id: "cdn-color-\(icon.slug)",
            title: "\(icon.title) Brand CDN",
            subtitle: "Фирменный цвет",
            badge: "CDN Brand",
            kind: .remoteCDN,
            brandHex: Int(icon.hex),
            symbolName: nil,
            svgMarkup: colorSVG,
            rawData: icon.svgData(fillHex: icon.hex)
        )

        return [monoVariant, colorVariant]
    }
}
