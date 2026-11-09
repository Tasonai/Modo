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
    case lucide = "Lucide Vector"
    case monogram = "Monogram"
    case geometric = "Geometric"
    case remoteCDN = "CDN Brand"
    case lawnicons = "Lawnicons"
    case whiteSur = "WhiteSur Vector"
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
        case .brandMono, .brandColor, .remoteCDN, .lawnicons, .whiteSur, .lucide:
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

    /// Selects a matching cross-platform Lucide vector icon based on application context.
    public static func smartLucideIcon(
        forTitle title: String,
        bundleIdentifier: String,
        categoryName: String
    ) -> LucideIcon? {
        let combined = " \(title) \(bundleIdentifier) \(categoryName) ".lowercased()
        if combined.contains("term") || combined.contains("bash") || combined.contains("zsh") || combined.contains("iterm") || combined.contains("warp") {
            return LucideIconCatalog.match(query: "terminal")
        }
        if combined.contains("code") || combined.contains("dev") || combined.contains("xcode") || combined.contains("cursor") {
            return LucideIconCatalog.match(query: "code")
        }
        if combined.contains("git") {
            return LucideIconCatalog.match(query: "git-branch")
        }
        if combined.contains("mail") || combined.contains("post") {
            return LucideIconCatalog.match(query: "mail")
        }
        if combined.contains("chat") || combined.contains("messag") || combined.contains("slack") || combined.contains("telegram") || combined.contains("discord") {
            return LucideIconCatalog.match(query: "message-square")
        }
        if combined.contains("music") || combined.contains("audio") || combined.contains("spotify") {
            return LucideIconCatalog.match(query: "music")
        }
        if combined.contains("camera") || combined.contains("photo") {
            return LucideIconCatalog.match(query: "camera")
        }
        if combined.contains("video") || combined.contains("film") || combined.contains("movie") || combined.contains("vlc") {
            return LucideIconCatalog.match(query: "video")
        }
        if combined.contains("design") || combined.contains("figma") || combined.contains("sketch") || combined.contains("paint") || combined.contains("art") {
            return LucideIconCatalog.match(query: "palette")
        }
        if combined.contains("secur") || combined.contains("protect") || combined.contains("guard") || combined.contains("antivirus") {
            return LucideIconCatalog.match(query: "shield-check")
        }
        if combined.contains("data") || combined.contains("sql") || combined.contains("base") {
            return LucideIconCatalog.match(query: "database")
        }
        if combined.contains("cloud") || combined.contains("sync") || combined.contains("drive") || combined.contains("dropbox") {
            return LucideIconCatalog.match(query: "cloud")
        }
        if combined.contains("setting") || combined.contains("pref") || combined.contains("config") {
            return LucideIconCatalog.match(query: "settings")
        }
        if combined.contains("browser") || combined.contains("web") || combined.contains("safari") || combined.contains("chrome") || combined.contains("firefox") {
            return LucideIconCatalog.match(query: "globe")
        }
        return LucideIconCatalog.match(query: "sparkles")
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

        // 4. Cross-platform Lucide Vector Icon
        if let lucide = smartLucideIcon(forTitle: title, bundleIdentifier: bundleIdentifier, categoryName: categoryName) {
            let svgStr = lucide.svgString(strokeColorHex: 0xFFFFFF, strokeWidth: 2.0)
            variants.append(
                VectorVariant(
                    id: "lucide-\(lucide.name)",
                    title: lucide.title,
                    subtitle: "Lucide: \(lucide.category.rawValue)",
                    badge: "Lucide",
                    kind: .lucide,
                    brandHex: brandHex,
                    symbolName: nil,
                    svgMarkup: svgStr,
                    rawData: svgStr.data(using: .utf8)
                )
            )
        }

        // 5. Procedural Algorithmic Vector Glyphs
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

        let terminalSVG = ProceduralGlyphGenerator.generateSVG(
            options: ProceduralGlyphOptions(title: title, style: .terminal, primaryHex: primaryColor, secondaryHex: 0x10B981)
        )
        variants.append(
            VectorVariant(
                id: "procedural-terminal",
                title: "Терминал",
                subtitle: "Код и консоль",
                badge: "SVG Vector",
                kind: .geometric,
                brandHex: brandHex,
                symbolName: nil,
                svgMarkup: terminalSVG,
                rawData: terminalSVG.data(using: .utf8)
            )
        )

        let waveformSVG = ProceduralGlyphGenerator.generateSVG(
            options: ProceduralGlyphOptions(title: title, style: .waveform, primaryHex: primaryColor, secondaryHex: 0xEC4899)
        )
        variants.append(
            VectorVariant(
                id: "procedural-waveform",
                title: "Звук",
                subtitle: "Частотная волна",
                badge: "SVG Vector",
                kind: .geometric,
                brandHex: brandHex,
                symbolName: nil,
                svgMarkup: waveformSVG,
                rawData: waveformSVG.data(using: .utf8)
            )
        )

        let hexacoreSVG = ProceduralGlyphGenerator.generateSVG(
            options: ProceduralGlyphOptions(title: title, style: .hexacore, primaryHex: primaryColor, secondaryHex: 0x6366F1)
        )
        variants.append(
            VectorVariant(
                id: "procedural-hexacore",
                title: "Гексакор",
                subtitle: "Изометрический куб",
                badge: "SVG Vector",
                kind: .geometric,
                brandHex: brandHex,
                symbolName: nil,
                svgMarkup: hexacoreSVG,
                rawData: hexacoreSVG.data(using: .utf8)
            )
        )

        let orbitSVG = ProceduralGlyphGenerator.generateSVG(
            options: ProceduralGlyphOptions(title: title, style: .orbit, primaryHex: primaryColor, secondaryHex: 0x06B6D4)
        )
        variants.append(
            VectorVariant(
                id: "procedural-orbit",
                title: "Орбиталь",
                subtitle: "Атомная модель",
                badge: "SVG Vector",
                kind: .geometric,
                brandHex: brandHex,
                symbolName: nil,
                svgMarkup: orbitSVG,
                rawData: orbitSVG.data(using: .utf8)
            )
        )

        let prismSVG = ProceduralGlyphGenerator.generateSVG(
            options: ProceduralGlyphOptions(title: title, style: .prism, primaryHex: primaryColor, secondaryHex: 0x8B5CF6)
        )
        variants.append(
            VectorVariant(
                id: "procedural-prism",
                title: "Призма",
                subtitle: "Граненый кристалл",
                badge: "SVG Vector",
                kind: .geometric,
                brandHex: brandHex,
                symbolName: nil,
                svgMarkup: prismSVG,
                rawData: prismSVG.data(using: .utf8)
            )
        )

        return variants
    }

    // MARK: - Candidate Slugs Generator
    /// Derives candidate slugs for finding vector assets across various remote open icon ecosystems.
    public static func candidateSlugs(for title: String) -> [String] {
        var list: [String] = []
        let trimmed = title.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !trimmed.isEmpty else { return [] }

        var base = trimmed.lowercased()
        if base.hasSuffix(".app") { base.removeLast(4) }

        // Localized mapping for standard macOS apps
        switch base {
        case "заметки": base = "notes"
        case "сообщения": base = "messages"
        case "музыка": base = "music"
        case "калькулятор": base = "calculator"
        case "терминал": base = "terminal"
        case "календарь": base = "calendar"
        case "почта": base = "mail"
        case "карты": base = "maps"
        case "фото": base = "photos"
        case "настройки", "системные настройки": base = "preferences"
        default: break
        }

        let words = base.split { !$0.isLetter && !$0.isNumber }
        let alphanumeric = words.joined()
        let hyphenated = words.joined(separator: "-")
        let underscored = words.joined(separator: "_")

        if !hyphenated.isEmpty { list.append(hyphenated) }
        if !underscored.isEmpty && underscored != hyphenated { list.append(underscored) }
        if !alphanumeric.isEmpty && alphanumeric != hyphenated && alphanumeric != underscored { list.append(alphanumeric) }

        switch alphanumeric {
        case "code", "vscode", "visualstudiocode":
            for alias in ["visual-studio-code", "visualstudiocode", "visual_studio_code", "code", "vscode"] {
                if !list.contains(alias) { list.append(alias) }
            }
        case "chrome", "googlechrome":
            for alias in ["google-chrome", "google_chrome", "googlechrome", "chrome"] {
                if !list.contains(alias) { list.append(alias) }
            }
        case "telegram", "tg", "telegramdesktop":
            for alias in ["telegram", "telegram-desktop", "telegram_desktop"] {
                if !list.contains(alias) { list.append(alias) }
            }
        case "spotify":
            for alias in ["spotify", "spotify-client"] {
                if !list.contains(alias) { list.append(alias) }
            }
        case "sublime", "sublimetext":
            for alias in ["sublime-text", "sublime_text", "sublimetext"] {
                if !list.contains(alias) { list.append(alias) }
            }
        case "discord":
            for alias in ["discord"] {
                if !list.contains(alias) { list.append(alias) }
            }
        case "slack":
            for alias in ["slack"] {
                if !list.contains(alias) { list.append(alias) }
            }
        case "github", "githubdesktop":
            for alias in ["github-desktop", "github_desktop", "github"] {
                if !list.contains(alias) { list.append(alias) }
            }
        default:
            break
        }

        return list
    }

    private static let cacheBaseDirectory: URL = {
        let appSupport = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first
            ?? URL(fileURLWithPath: NSTemporaryDirectory())
        let dir = appSupport.appendingPathComponent("Modo/VectorVariantCache", isDirectory: true)
        try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
        return dir
    }()

    /// Fetches vector line art from Lawnicons GitHub repository.
    public static func fetchLawniconsVariant(title: String, slugs: [String]) async -> VectorVariant? {
        let lawnDir = cacheBaseDirectory.appendingPathComponent("lawnicons", isDirectory: true)
        try? FileManager.default.createDirectory(at: lawnDir, withIntermediateDirectories: true)

        for slug in slugs {
            let cached = lawnDir.appendingPathComponent("\(slug).svg")
            if let cachedData = try? Data(contentsOf: cached),
               let cachedText = String(data: cachedData, encoding: .utf8),
               cachedText.contains("<svg") {
                return VectorVariant(
                    id: "lawnicons-\(slug)",
                    title: "\(title) Line",
                    subtitle: "Lawnicons Themed",
                    badge: "Lawnicons",
                    kind: .lawnicons,
                    brandHex: nil,
                    symbolName: nil,
                    svgMarkup: cachedText,
                    rawData: cachedData
                )
            }

            let urlString = "https://raw.githubusercontent.com/LawnchairLauncher/lawnicons/develop/svgs/\(slug).svg"
            guard let url = URL(string: urlString) else { continue }
            var req = URLRequest(url: url)
            req.timeoutInterval = 4.0
            if let (data, resp) = try? await URLSession.shared.data(for: req),
               let http = resp as? HTTPURLResponse, http.statusCode == 200,
               let text = String(data: data, encoding: .utf8),
               text.contains("<svg") {
                try? data.write(to: cached, options: .atomic)
                return VectorVariant(
                    id: "lawnicons-\(slug)",
                    title: "\(title) Line",
                    subtitle: "Lawnicons Themed",
                    badge: "Lawnicons",
                    kind: .lawnicons,
                    brandHex: nil,
                    symbolName: nil,
                    svgMarkup: text,
                    rawData: data
                )
            }
        }
        return nil
    }

    /// Fetches macOS-native scalable vector from WhiteSur icon theme repository.
    public static func fetchWhiteSurVariant(title: String, slugs: [String]) async -> VectorVariant? {
        let whiteSurDir = cacheBaseDirectory.appendingPathComponent("whitesur", isDirectory: true)
        try? FileManager.default.createDirectory(at: whiteSurDir, withIntermediateDirectories: true)

        for slug in slugs {
            let cached = whiteSurDir.appendingPathComponent("\(slug).svg")
            if let cachedData = try? Data(contentsOf: cached),
               let cachedText = String(data: cachedData, encoding: .utf8),
               cachedText.contains("<svg") {
                return VectorVariant(
                    id: "whitesur-\(slug)",
                    title: "\(title) macOS",
                    subtitle: "WhiteSur Scalable",
                    badge: "WhiteSur",
                    kind: .whiteSur,
                    brandHex: nil,
                    symbolName: nil,
                    svgMarkup: cachedText,
                    rawData: cachedData
                )
            }

            let urlString = "https://raw.githubusercontent.com/vinceliuice/WhiteSur-icon-theme/master/src/apps/scalable/\(slug).svg"
            guard let url = URL(string: urlString) else { continue }
            var req = URLRequest(url: url)
            req.timeoutInterval = 4.0
            if let (data, resp) = try? await URLSession.shared.data(for: req),
               let http = resp as? HTTPURLResponse, http.statusCode == 200,
               let text = String(data: data, encoding: .utf8),
               text.contains("<svg") {
                try? data.write(to: cached, options: .atomic)
                return VectorVariant(
                    id: "whitesur-\(slug)",
                    title: "\(title) macOS",
                    subtitle: "WhiteSur Scalable",
                    badge: "WhiteSur",
                    kind: .whiteSur,
                    brandHex: nil,
                    symbolName: nil,
                    svgMarkup: text,
                    rawData: data
                )
            }
        }
        return nil
    }

    /// Asynchronously fetches remote brand SVG variants across Simple Icons, Lawnicons, and WhiteSur CDN endpoints.
    public static func fetchRemoteBrandVariants(for title: String) async -> [VectorVariant] {
        let slugs = candidateSlugs(for: title)
        guard !slugs.isEmpty else { return [] }

        async let simpleIconsTask: [VectorVariant] = {
            guard let icon = await SimpleIconCatalog.fetch(slugOrTitle: title) else {
                return []
            }

            let monoSVG = icon.svgString(fillHex: 0xFFFFFF)
            let monoVariant = VectorVariant(
                id: "cdn-mono-\(icon.slug)",
                title: "\(icon.title) CDN",
                subtitle: "Simple Icons Mono",
                badge: "Simple Icons",
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
                badge: "Brand Color",
                kind: .remoteCDN,
                brandHex: Int(icon.hex),
                symbolName: nil,
                svgMarkup: colorSVG,
                rawData: icon.svgData(fillHex: icon.hex)
            )

            return [monoVariant, colorVariant]
        }()

        async let lawniconsTask: VectorVariant? = fetchLawniconsVariant(title: title, slugs: slugs)
        async let whiteSurTask: VectorVariant? = fetchWhiteSurVariant(title: title, slugs: slugs)

        var results: [VectorVariant] = []
        let simpleResults = await simpleIconsTask
        results.append(contentsOf: simpleResults)

        if let lawn = await lawniconsTask {
            results.append(lawn)
        }
        if let ws = await whiteSurTask {
            results.append(ws)
        }

        return results
    }
}
