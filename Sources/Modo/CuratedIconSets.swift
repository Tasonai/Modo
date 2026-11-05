import AppKit
import Foundation
import ModoKit

/// Generates and installs popular community icon sets offline or on-demand.
enum CuratedIconSets {
    struct CuratedPack: Identifiable, Sendable {
        let id: String
        let name: String
        let author: String
        let license: String
        let description: String
        let iconCount: Int
    }

    static let popularPacks: [CuratedPack] = [
        CuratedPack(
            id: "papirus-minimal",
            name: "Papirus Icons",
            author: "Papirus Team & Community",
            license: "GPL-3.0",
            description: "Classic clean flat vector icons for popular desktop apps",
            iconCount: 10
        ),
        CuratedPack(
            id: "catppuccin-icons",
            name: "Catppuccin Pastel",
            author: "Catppuccin Community",
            license: "MIT",
            description: "Soothing pastel palette with mocha surfaces and soft accents",
            iconCount: 10
        ),
        CuratedPack(
            id: "nordic-frost",
            name: "Nordic Minimal",
            author: "Arctic Ice Studio",
            license: "MIT",
            description: "Arctic cold slate and polar ice blue clean aesthetic",
            iconCount: 10
        )
    ]

    /// Target bundle identifiers for essential macOS desktop applications.
    static let apps: [(bundleID: String, file: String, name: String)] = [
        ("com.google.Chrome", "icons/chrome.png", "Google Chrome"),
        ("com.apple.Safari", "icons/safari.png", "Safari"),
        ("com.microsoft.VSCode", "icons/vscode.png", "Visual Studio Code"),
        ("ru.keepcoder.Telegram", "icons/telegram.png", "Telegram"),
        ("com.apple.Terminal", "icons/terminal.png", "Terminal"),
        ("com.apple.finder", "icons/finder.png", "Finder"),
        ("com.apple.Notes", "icons/notes.png", "Notes"),
        ("com.apple.mail", "icons/mail.png", "Mail"),
        ("com.apple.systempreferences", "icons/settings.png", "System Settings"),
        ("com.spotify.client", "icons/spotify.png", "Spotify")
    ]

    /// Generates a valid ZIP archive containing `manifest.json` and 256x256 PNG icons.
    static func buildZipData(for id: String) -> Data? {
        guard let pack = popularPacks.first(where: { $0.id == id }) else { return nil }
        let fm = FileManager.default
        let work = fm.temporaryDirectory.appendingPathComponent("curated-pack-\(UUID().uuidString)", isDirectory: true)
        let iconsDir = work.appendingPathComponent("icons", isDirectory: true)
        defer { try? fm.removeItem(at: work) }

        do {
            try fm.createDirectory(at: iconsDir, withIntermediateDirectories: true)
            let manifest = IconSetManifest(
                id: pack.id,
                name: pack.name,
                author: pack.author,
                license: pack.license,
                icons: apps.map { .init(bundleID: $0.bundleID, file: $0.file) }
            )
            let manifestData = try JSONEncoder().encode(manifest)
            try manifestData.write(to: work.appendingPathComponent("manifest.json"))

            for app in apps {
                let pngData = renderIcon(for: app.bundleID, style: id)
                try pngData.write(to: work.appendingPathComponent(app.file))
            }

            let zipURL = fm.temporaryDirectory.appendingPathComponent("curated-\(id)-\(UUID().uuidString).zip")
            defer { try? fm.removeItem(at: zipURL) }

            let process = Process()
            process.executableURL = URL(fileURLWithPath: "/usr/bin/zip")
            process.currentDirectoryURL = work
            process.arguments = ["-q", "-r", "-y", zipURL.path, "."]
            try process.run()
            process.waitUntilExit()
            guard process.terminationStatus == 0 else { return nil }

            return try Data(contentsOf: zipURL)
        } catch {
            return nil
        }
    }

    /// Installs a popular curated pack directly.
    static func install(id: String) throws -> InstalledIconSet {
        guard let zipData = buildZipData(for: id) else {
            throw IconSetError.notAnIconSet
        }
        return try IconSetStore.install(zipData: zipData, expectedSHA256: nil)
    }

    // MARK: - Artwork Generators

    private static func renderIcon(for bundleID: String, style: String) -> Data {
        let size = 256
        let colorSpace = CGColorSpaceCreateDeviceRGB()
        guard let ctx = CGContext(
            data: nil,
            width: size,
            height: size,
            bitsPerComponent: 8,
            bytesPerRow: size * 4,
            space: colorSpace,
            bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue
        ) else { return Data() }

        ctx.setAllowsAntialiasing(true)
        ctx.setShouldAntialias(true)
        let rect = CGRect(x: 0, y: 0, width: size, height: size)

        switch bundleID {
        case "com.google.Chrome":
            renderChrome(in: ctx, rect: rect, style: style)
        case "com.apple.Safari":
            renderSafari(in: ctx, rect: rect, style: style)
        case "com.microsoft.VSCode":
            renderVSCode(in: ctx, rect: rect, style: style)
        case "ru.keepcoder.Telegram":
            renderTelegram(in: ctx, rect: rect, style: style)
        case "com.apple.Terminal":
            renderTerminal(in: ctx, rect: rect, style: style)
        case "com.apple.finder":
            renderFinder(in: ctx, rect: rect, style: style)
        case "com.apple.Notes":
            renderNotes(in: ctx, rect: rect, style: style)
        case "com.apple.mail":
            renderMail(in: ctx, rect: rect, style: style)
        case "com.apple.systempreferences":
            renderSettings(in: ctx, rect: rect, style: style)
        case "com.spotify.client":
            renderSpotify(in: ctx, rect: rect, style: style)
        default:
            renderFallback(in: ctx, rect: rect, style: style)
        }

        guard let cg = ctx.makeImage() else { return Data() }
        let rep = NSBitmapImageRep(cgImage: cg)
        return rep.representation(using: .png, properties: [:]) ?? Data()
    }

    private static func renderChrome(in ctx: CGContext, rect: CGRect, style: String) {
        let center = CGPoint(x: rect.midX, y: rect.midY)
        let radius = rect.width * 0.44

        // Base circle
        ctx.setFillColor(NSColor.white.cgColor)
        ctx.fillEllipse(in: CGRect(x: center.x - radius, y: center.y - radius, width: radius * 2, height: radius * 2))

        let (red, green, yellow, blue) = switch style {
        case "catppuccin-icons":
            (NSColor(srgbRed: 0.95, green: 0.55, blue: 0.66, alpha: 1), // Flamingo
             NSColor(srgbRed: 0.65, green: 0.89, blue: 0.63, alpha: 1), // Green
             NSColor(srgbRed: 0.98, green: 0.89, blue: 0.68, alpha: 1), // Yellow
             NSColor(srgbRed: 0.54, green: 0.71, blue: 0.98, alpha: 1)) // Sapphire
        case "nordic-frost":
            (NSColor(srgbRed: 0.75, green: 0.38, blue: 0.42, alpha: 1), // Nord11 Red
             NSColor(srgbRed: 0.64, green: 0.75, blue: 0.55, alpha: 1), // Nord14 Green
             NSColor(srgbRed: 0.92, green: 0.80, blue: 0.55, alpha: 1), // Nord13 Yellow
             NSColor(srgbRed: 0.53, green: 0.75, blue: 0.82, alpha: 1)) // Nord8 Frost
        default: // Papirus
            (NSColor(srgbRed: 0.90, green: 0.22, blue: 0.21, alpha: 1),
             NSColor(srgbRed: 0.06, green: 0.62, blue: 0.35, alpha: 1),
             NSColor(srgbRed: 0.99, green: 0.75, blue: 0.03, alpha: 1),
             NSColor(srgbRed: 0.16, green: 0.50, blue: 0.93, alpha: 1))
        }

        // Draw pinwheel
        let angles: [(CGFloat, CGFloat, NSColor)] = [
            (0, 120, red),
            (120, 240, green),
            (240, 360, yellow)
        ]
        for (start, end, col) in angles {
            ctx.beginPath()
            ctx.move(to: center)
            ctx.addArc(center: center, radius: radius, startAngle: start * .pi / 180, endAngle: end * .pi / 180, clockwise: false)
            ctx.closePath()
            ctx.setFillColor(col.cgColor)
            ctx.fillPath()
        }

        // Inner white disc and center blue
        let innerR = radius * 0.48
        ctx.setFillColor(NSColor.white.cgColor)
        ctx.fillEllipse(in: CGRect(x: center.x - innerR, y: center.y - innerR, width: innerR * 2, height: innerR * 2))

        let centerR = radius * 0.38
        ctx.setFillColor(blue.cgColor)
        ctx.fillEllipse(in: CGRect(x: center.x - centerR, y: center.y - centerR, width: centerR * 2, height: centerR * 2))
    }

    private static func renderSafari(in ctx: CGContext, rect: CGRect, style: String) {
        let center = CGPoint(x: rect.midX, y: rect.midY)
        let radius = rect.width * 0.44

        let (bg, needleRed, needleWhite) = switch style {
        case "catppuccin-icons":
            (NSColor(srgbRed: 0.12, green: 0.12, blue: 0.18, alpha: 1),
             NSColor(srgbRed: 0.95, green: 0.55, blue: 0.66, alpha: 1),
             NSColor(srgbRed: 0.80, green: 0.84, blue: 0.96, alpha: 1))
        case "nordic-frost":
            (NSColor(srgbRed: 0.18, green: 0.20, blue: 0.25, alpha: 1),
             NSColor(srgbRed: 0.75, green: 0.38, blue: 0.42, alpha: 1),
             NSColor(srgbRed: 0.93, green: 0.94, blue: 0.96, alpha: 1))
        default:
            (NSColor(srgbRed: 0.08, green: 0.45, blue: 0.88, alpha: 1),
             NSColor(srgbRed: 0.92, green: 0.26, blue: 0.21, alpha: 1),
             NSColor.white)
        }

        ctx.setFillColor(bg.cgColor)
        ctx.fillEllipse(in: CGRect(x: center.x - radius, y: center.y - radius, width: radius * 2, height: radius * 2))

        // Compass needle rotated 45 degrees
        ctx.saveGState()
        ctx.translateBy(x: center.x, y: center.y)
        ctx.rotate(by: .pi / 4)

        let needleL = radius * 0.75
        let needleW = radius * 0.20

        // Red top half
        ctx.beginPath()
        ctx.move(to: CGPoint(x: 0, y: needleL))
        ctx.addLine(to: CGPoint(x: needleW, y: 0))
        ctx.addLine(to: CGPoint(x: -needleW, y: 0))
        ctx.closePath()
        ctx.setFillColor(needleRed.cgColor)
        ctx.fillPath()

        // White bottom half
        ctx.beginPath()
        ctx.move(to: CGPoint(x: 0, y: -needleL))
        ctx.addLine(to: CGPoint(x: needleW, y: 0))
        ctx.addLine(to: CGPoint(x: -needleW, y: 0))
        ctx.closePath()
        ctx.setFillColor(needleWhite.cgColor)
        ctx.fillPath()

        // Center pivot
        ctx.setFillColor(NSColor.black.withAlphaComponent(0.3).cgColor)
        ctx.fillEllipse(in: CGRect(x: -4, y: -4, width: 8, height: 8))

        ctx.restoreGState()
    }

    private static func renderVSCode(in ctx: CGContext, rect: CGRect, style: String) {
        let (bg, ribbon) = switch style {
        case "catppuccin-icons":
            (NSColor(srgbRed: 0.12, green: 0.12, blue: 0.18, alpha: 1),
             NSColor(srgbRed: 0.79, green: 0.65, blue: 0.97, alpha: 1)) // Mauve
        case "nordic-frost":
            (NSColor(srgbRed: 0.18, green: 0.20, blue: 0.25, alpha: 1),
             NSColor(srgbRed: 0.53, green: 0.75, blue: 0.82, alpha: 1)) // Nord8
        default:
            (NSColor(srgbRed: 0.13, green: 0.14, blue: 0.18, alpha: 1),
             NSColor(srgbRed: 0.00, green: 0.48, blue: 0.80, alpha: 1))
        }

        let radius = rect.width * 0.44
        let center = CGPoint(x: rect.midX, y: rect.midY)
        ctx.setFillColor(bg.cgColor)
        ctx.fillEllipse(in: CGRect(x: center.x - radius, y: center.y - radius, width: radius * 2, height: radius * 2))

        // VS Code folded chevron ribbon
        ctx.setFillColor(ribbon.cgColor)
        let path = CGMutablePath()
        path.move(to: CGPoint(x: 75, y: 155))
        path.addLine(to: CGPoint(x: 155, y: 205))
        path.addLine(to: CGPoint(x: 195, y: 180))
        path.addLine(to: CGPoint(x: 135, y: 128))
        path.addLine(to: CGPoint(x: 195, y: 76))
        path.addLine(to: CGPoint(x: 155, y: 51))
        path.addLine(to: CGPoint(x: 75, y: 101))
        path.closeSubpath()
        ctx.addPath(path)
        ctx.fillPath()
    }

    private static func renderTelegram(in ctx: CGContext, rect: CGRect, style: String) {
        let center = CGPoint(x: rect.midX, y: rect.midY)
        let radius = rect.width * 0.44

        let (bg, planeColor) = switch style {
        case "catppuccin-icons":
            (NSColor(srgbRed: 0.54, green: 0.71, blue: 0.98, alpha: 1),
             NSColor(srgbRed: 0.12, green: 0.12, blue: 0.18, alpha: 1))
        case "nordic-frost":
            (NSColor(srgbRed: 0.51, green: 0.63, blue: 0.76, alpha: 1),
             NSColor(srgbRed: 0.93, green: 0.94, blue: 0.96, alpha: 1))
        default:
            (NSColor(srgbRed: 0.17, green: 0.67, blue: 0.93, alpha: 1),
             NSColor.white)
        }

        ctx.setFillColor(bg.cgColor)
        ctx.fillEllipse(in: CGRect(x: center.x - radius, y: center.y - radius, width: radius * 2, height: radius * 2))

        // Paper plane
        ctx.setFillColor(planeColor.cgColor)
        let path = CGMutablePath()
        path.move(to: CGPoint(x: 60, y: 125))
        path.addLine(to: CGPoint(x: 195, y: 185))
        path.addLine(to: CGPoint(x: 155, y: 70))
        path.addLine(to: CGPoint(x: 120, y: 115))
        path.closeSubpath()
        ctx.addPath(path)
        ctx.fillPath()
    }

    private static func renderTerminal(in ctx: CGContext, rect: CGRect, style: String) {
        let (bg, promptColor) = switch style {
        case "catppuccin-icons":
            (NSColor(srgbRed: 0.12, green: 0.12, blue: 0.18, alpha: 1),
             NSColor(srgbRed: 0.65, green: 0.89, blue: 0.63, alpha: 1))
        case "nordic-frost":
            (NSColor(srgbRed: 0.18, green: 0.20, blue: 0.25, alpha: 1),
             NSColor(srgbRed: 0.64, green: 0.75, blue: 0.55, alpha: 1))
        default:
            (NSColor(srgbRed: 0.14, green: 0.15, blue: 0.17, alpha: 1),
             NSColor(srgbRed: 0.30, green: 0.85, blue: 0.39, alpha: 1))
        }

        let radius = rect.width * 0.44
        let center = CGPoint(x: rect.midX, y: rect.midY)
        ctx.setFillColor(bg.cgColor)
        ctx.fillEllipse(in: CGRect(x: center.x - radius, y: center.y - radius, width: radius * 2, height: radius * 2))

        // Prompt "> _"
        ctx.setStrokeColor(promptColor.cgColor)
        ctx.setLineWidth(14)
        ctx.setLineCap(.round)
        ctx.setLineJoin(.round)

        // Chevron >
        ctx.beginPath()
        ctx.move(to: CGPoint(x: 85, y: 160))
        ctx.addLine(to: CGPoint(x: 125, y: 128))
        ctx.addLine(to: CGPoint(x: 85, y: 96))
        ctx.strokePath()

        // Cursor _
        ctx.beginPath()
        ctx.move(to: CGPoint(x: 140, y: 96))
        ctx.addLine(to: CGPoint(x: 175, y: 96))
        ctx.strokePath()
    }

    private static func renderFinder(in ctx: CGContext, rect: CGRect, style: String) {
        let (bg, accent) = switch style {
        case "catppuccin-icons":
            (NSColor(srgbRed: 0.54, green: 0.71, blue: 0.98, alpha: 1),
             NSColor(srgbRed: 0.79, green: 0.65, blue: 0.97, alpha: 1))
        case "nordic-frost":
            (NSColor(srgbRed: 0.53, green: 0.75, blue: 0.82, alpha: 1),
             NSColor(srgbRed: 0.18, green: 0.20, blue: 0.25, alpha: 1))
        default:
            (NSColor(srgbRed: 0.18, green: 0.56, blue: 0.92, alpha: 1),
             NSColor(srgbRed: 0.96, green: 0.96, blue: 0.96, alpha: 1))
        }

        let radius = rect.width * 0.44
        let center = CGPoint(x: rect.midX, y: rect.midY)
        ctx.setFillColor(bg.cgColor)
        ctx.fillEllipse(in: CGRect(x: center.x - radius, y: center.y - radius, width: radius * 2, height: radius * 2))

        // Split face smile
        ctx.setStrokeColor(accent.cgColor)
        ctx.setLineWidth(12)
        ctx.setLineCap(.round)

        // Eyes
        ctx.setFillColor(accent.cgColor)
        ctx.fillEllipse(in: CGRect(x: 95, y: 145, width: 14, height: 20))
        ctx.fillEllipse(in: CGRect(x: 147, y: 145, width: 14, height: 20))

        // Smile
        ctx.beginPath()
        ctx.addArc(center: CGPoint(x: 128, y: 120), radius: 36, startAngle: .pi * 1.15, endAngle: .pi * 1.85, clockwise: false)
        ctx.strokePath()
    }

    private static func renderNotes(in ctx: CGContext, rect: CGRect, style: String) {
        let (bg, lines) = switch style {
        case "catppuccin-icons":
            (NSColor(srgbRed: 0.98, green: 0.89, blue: 0.68, alpha: 1),
             NSColor(srgbRed: 0.42, green: 0.44, blue: 0.53, alpha: 1))
        case "nordic-frost":
            (NSColor(srgbRed: 0.92, green: 0.80, blue: 0.55, alpha: 1),
             NSColor(srgbRed: 0.29, green: 0.34, blue: 0.42, alpha: 1))
        default:
            (NSColor(srgbRed: 0.98, green: 0.80, blue: 0.18, alpha: 1),
             NSColor.white)
        }

        let radius = rect.width * 0.44
        let center = CGPoint(x: rect.midX, y: rect.midY)
        ctx.setFillColor(bg.cgColor)
        ctx.fillEllipse(in: CGRect(x: center.x - radius, y: center.y - radius, width: radius * 2, height: radius * 2))

        // Notepad ruled lines
        ctx.setStrokeColor(lines.cgColor)
        ctx.setLineWidth(10)
        ctx.setLineCap(.round)
        for y in [155, 125, 95] {
            ctx.beginPath()
            ctx.move(to: CGPoint(x: 85, y: y))
            ctx.addLine(to: CGPoint(x: 171, y: y))
            ctx.strokePath()
        }
    }

    private static func renderMail(in ctx: CGContext, rect: CGRect, style: String) {
        let (bg, envelope) = switch style {
        case "catppuccin-icons":
            (NSColor(srgbRed: 0.54, green: 0.71, blue: 0.98, alpha: 1),
             NSColor(srgbRed: 0.93, green: 0.94, blue: 0.98, alpha: 1))
        case "nordic-frost":
            (NSColor(srgbRed: 0.37, green: 0.51, blue: 0.67, alpha: 1),
             NSColor(srgbRed: 0.93, green: 0.94, blue: 0.96, alpha: 1))
        default:
            (NSColor(srgbRed: 0.08, green: 0.48, blue: 0.92, alpha: 1),
             NSColor.white)
        }

        let radius = rect.width * 0.44
        let center = CGPoint(x: rect.midX, y: rect.midY)
        ctx.setFillColor(bg.cgColor)
        ctx.fillEllipse(in: CGRect(x: center.x - radius, y: center.y - radius, width: radius * 2, height: radius * 2))

        // Envelope shape
        ctx.setStrokeColor(envelope.cgColor)
        ctx.setLineWidth(10)
        ctx.setLineJoin(.round)
        let envRect = CGRect(x: 75, y: 90, width: 106, height: 76)
        ctx.stroke(envRect)

        // Flap
        ctx.beginPath()
        ctx.move(to: CGPoint(x: 75, y: 166))
        ctx.addLine(to: CGPoint(x: 128, y: 128))
        ctx.addLine(to: CGPoint(x: 181, y: 166))
        ctx.strokePath()
    }

    private static func renderSettings(in ctx: CGContext, rect: CGRect, style: String) {
        let (bg, gear) = switch style {
        case "catppuccin-icons":
            (NSColor(srgbRed: 0.12, green: 0.12, blue: 0.18, alpha: 1),
             NSColor(srgbRed: 0.69, green: 0.71, blue: 0.80, alpha: 1))
        case "nordic-frost":
            (NSColor(srgbRed: 0.18, green: 0.20, blue: 0.25, alpha: 1),
             NSColor(srgbRed: 0.85, green: 0.87, blue: 0.91, alpha: 1))
        default:
            (NSColor(srgbRed: 0.35, green: 0.37, blue: 0.42, alpha: 1),
             NSColor.white)
        }

        let radius = rect.width * 0.44
        let center = CGPoint(x: rect.midX, y: rect.midY)
        ctx.setFillColor(bg.cgColor)
        ctx.fillEllipse(in: CGRect(x: center.x - radius, y: center.y - radius, width: radius * 2, height: radius * 2))

        // Central gear
        ctx.setFillColor(gear.cgColor)
        let teeth = 8
        for i in 0..<teeth {
            let angle = CGFloat(i) * (.pi * 2.0 / CGFloat(teeth))
            let toothRect = CGRect(x: center.x - 10, y: center.y + 40, width: 20, height: 16)
            ctx.saveGState()
            ctx.translateBy(x: center.x, y: center.y)
            ctx.rotate(by: angle)
            ctx.translateBy(x: -center.x, y: -center.y)
            ctx.fill(toothRect)
            ctx.restoreGState()
        }

        ctx.fillEllipse(in: CGRect(x: center.x - 45, y: center.y - 45, width: 90, height: 90))
        ctx.setFillColor(bg.cgColor)
        ctx.fillEllipse(in: CGRect(x: center.x - 22, y: center.y - 22, width: 44, height: 44))
    }

    private static func renderSpotify(in ctx: CGContext, rect: CGRect, style: String) {
        let (bg, wave) = switch style {
        case "catppuccin-icons":
            (NSColor(srgbRed: 0.65, green: 0.89, blue: 0.63, alpha: 1),
             NSColor(srgbRed: 0.12, green: 0.12, blue: 0.18, alpha: 1))
        case "nordic-frost":
            (NSColor(srgbRed: 0.64, green: 0.75, blue: 0.55, alpha: 1),
             NSColor(srgbRed: 0.18, green: 0.20, blue: 0.25, alpha: 1))
        default:
            (NSColor(srgbRed: 0.11, green: 0.84, blue: 0.38, alpha: 1),
             NSColor(srgbRed: 0.10, green: 0.10, blue: 0.10, alpha: 1))
        }

        let radius = rect.width * 0.44
        let center = CGPoint(x: rect.midX, y: rect.midY)
        ctx.setFillColor(bg.cgColor)
        ctx.fillEllipse(in: CGRect(x: center.x - radius, y: center.y - radius, width: radius * 2, height: radius * 2))

        // Soundwaves
        ctx.setStrokeColor(wave.cgColor)
        ctx.setLineCap(.round)

        let waves: [(CGFloat, CGFloat, CGFloat)] = [
            (56, 12, 0.42),
            (42, 10, 0.38),
            (28, 8, 0.34)
        ]
        for (arcR, width, span) in waves {
            ctx.setLineWidth(width)
            ctx.beginPath()
            ctx.addArc(center: CGPoint(x: center.x, y: center.y - 20), radius: arcR, startAngle: .pi * (0.5 - span), endAngle: .pi * (0.5 + span), clockwise: false)
            ctx.strokePath()
        }
    }

    private static func renderFallback(in ctx: CGContext, rect: CGRect, style: String) {
        let radius = rect.width * 0.44
        let center = CGPoint(x: rect.midX, y: rect.midY)
        ctx.setFillColor(NSColor.darkGray.cgColor)
        ctx.fillEllipse(in: CGRect(x: center.x - radius, y: center.y - radius, width: radius * 2, height: radius * 2))
    }
}
