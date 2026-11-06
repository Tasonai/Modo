import AppKit
import Foundation
import ModoKit

// MARK: - Modo Command-Line Interface Engine

public enum ModoCLI {
    public static let version = "1.7.0"

    /// Dispatches command line arguments. Returns process exit code.
    public static func run(arguments: [String]) -> Int32 {
        guard !arguments.isEmpty else {
            printHelp()
            return 0
        }

        let command = arguments[0].lowercased()
        switch command {
        case "--help", "-h", "help":
            printHelp()
            return 0

        case "--version", "-v", "version":
            print("Modo \(version) — Native macOS Adaptive Icon Studio by Atoma")
            return 0

        case "style":
            return handleStyle(args: Array(arguments.dropFirst()))

        case "restore":
            return handleRestore(args: Array(arguments.dropFirst()))

        case "folder":
            return handleFolder(args: Array(arguments.dropFirst()))

        case "batch":
            return handleBatch(args: Array(arguments.dropFirst()))

        case "showcase":
            return handleShowcase(args: Array(arguments.dropFirst()))

        case "export":
            return handleExport(args: Array(arguments.dropFirst()))

        case "theme":
            return handleTheme(args: Array(arguments.dropFirst()))

        default:
            print("Unknown command: '\(command)'. Run 'modo --help' for usage.")
            return 1
        }
    }

    // MARK: - Subcommand Handlers

    private static func handleStyle(args: [String]) -> Int32 {
        guard let targetPath = args.first, !targetPath.hasPrefix("-") else {
            print("Error: Missing target app path. Usage: modo style <app_path> [options]")
            return 1
        }

        let url = URL(fileURLWithPath: (targetPath as NSString).expandingTildeInPath)
        guard FileManager.default.fileExists(atPath: url.path) else {
            print("Error: Target path does not exist: \(url.path)")
            return 1
        }

        var shape: IconShape = .squircle
        var bgStyle: RoundBackgroundStyle = .materialPastel
        var scale: Double = 0.68

        var i = 1
        while i < args.count {
            let flag = args[i]
            if flag == "--shape", i + 1 < args.count {
                if let s = IconShape(rawValue: args[i + 1]) { shape = s }
                i += 2
            } else if flag == "--bg", i + 1 < args.count {
                let name = args[i + 1].lowercased()
                if let match = RoundBackgroundStyle.allCases.first(where: { $0.rawValue.lowercased() == name || $0.title.lowercased() == name }) {
                    bgStyle = match
                }
                i += 2
            } else if flag == "--scale", i + 1 < args.count {
                if let val = Double(args[i + 1]) { scale = min(max(val, 0.4), 1.8) }
                i += 2
            } else {
                i += 1
            }
        }

        let appTitle = url.deletingPathExtension().lastPathComponent
        let sourceImage: NSImage
        if let vector = SimpleIconCatalog.find(for: appTitle), let svgImg = NSImage(data: vector.svgData()) {
            sourceImage = svgImg
        } else {
            sourceImage = NSWorkspace.shared.icon(forFile: url.path)
        }
        let style = RoundIconStyle(
            backgroundStyle: bgStyle,
            shape: shape,
            iconScale: scale
        )

        let rendered = IconRenderer.renderRoundIcon(sourceImage: sourceImage, style: style, fallbackHex: 0x1A73E8)
        let ok = NSWorkspace.shared.setIcon(rendered, forFile: url.path, options: [])
        if ok {
            print("✓ Successfully styled \(url.lastPathComponent)")
            print("  Shape: \(shape.title), Background: \(bgStyle.title), Scale: \(Int(scale * 100))%")
            return 0
        } else {
            print("✗ Failed to apply icon to \(url.path). Ensure you have write permissions.")
            return 1
        }
    }

    private static func handleRestore(args: [String]) -> Int32 {
        guard let targetPath = args.first else {
            print("Error: Missing target path. Usage: modo restore <path>")
            return 1
        }
        let url = URL(fileURLWithPath: (targetPath as NSString).expandingTildeInPath)
        let ok = NSWorkspace.shared.setIcon(nil, forFile: url.path, options: [])
        if ok {
            print("✓ Restored original icon for \(url.lastPathComponent)")
            return 0
        } else {
            print("✗ Failed to restore icon for \(url.path)")
            return 1
        }
    }

    private static func handleFolder(args: [String]) -> Int32 {
        guard let folderPath = args.first, !folderPath.hasPrefix("-") else {
            print("Error: Missing folder path. Usage: modo folder <folder_path> [--color <preset>]")
            return 1
        }
        let url = URL(fileURLWithPath: (folderPath as NSString).expandingTildeInPath)
        guard FileManager.default.fileExists(atPath: url.path) else {
            print("Error: Folder path does not exist: \(url.path)")
            return 1
        }

        var preset: FolderColorPreset = .appleBlue
        var i = 1
        while i < args.count {
            if args[i] == "--color", i + 1 < args.count {
                let colorName = args[i + 1].lowercased()
                if let match = FolderColorPreset.allCases.first(where: {
                    $0.rawValue.lowercased().contains(colorName) || String(describing: $0).lowercased() == colorName
                }) {
                    preset = match
                }
                i += 2
            } else {
                i += 1
            }
        }

        let svgMarkup = FolderVectorGenerator.generateFolderSVG(
            style: FolderStyle(preset: preset, embossStyle: .etched),
            size: 512
        )
        guard let svgData = svgMarkup.data(using: .utf8), let icon = NSImage(data: svgData) else {
            print("✗ Failed to rasterize folder SVG")
            return 1
        }

        let ok = NSWorkspace.shared.setIcon(icon, forFile: url.path, options: [])
        if ok {
            print("✓ Styled folder \(url.lastPathComponent) with preset \(preset.rawValue)")
            return 0
        } else {
            print("✗ Failed to apply custom folder icon to \(url.path)")
            return 1
        }
    }

    private static func handleBatch(args: [String]) -> Int32 {
        guard let inputPath = args.first, !inputPath.hasPrefix("-") else {
            print("Error: Missing input folder. Usage: modo batch <input_folder> --out <output_dir>")
            return 1
        }
        let inputURL = URL(fileURLWithPath: (inputPath as NSString).expandingTildeInPath)
        var outputURL = inputURL.appendingPathComponent("Modo-Icons", isDirectory: true)

        var i = 1
        while i < args.count {
            if (args[i] == "--out" || args[i] == "-o"), i + 1 < args.count {
                outputURL = URL(fileURLWithPath: (args[i + 1] as NSString).expandingTildeInPath)
                i += 2
            } else {
                i += 1
            }
        }

        let candidates = BatchIconEngine.scanDirectory(at: inputURL)
        guard !candidates.isEmpty else {
            print("No image or vector files (.png, .svg, .jpg, .webp, .icns) found in \(inputURL.path)")
            return 1
        }

        try? FileManager.default.createDirectory(at: outputURL, withIntermediateDirectories: true)
        print("Processing \(candidates.count) icons from \(inputURL.lastPathComponent)...")

        var processed = 0
        let style = RoundIconStyle(backgroundStyle: .materialPastel, shape: .squircle, iconScale: 0.68)
        for c in candidates {
            guard let img = NSImage(contentsOf: c.fileURL) else { continue }
            _ = try? IconRenderer.exportIconPack(
                for: c.cleanTitle,
                sourceImage: img,
                style: style,
                fallbackHex: 0x1A73E8,
                to: outputURL
            )
            processed += 1
            print("  [\(processed)/\(candidates.count)] \(c.cleanTitle)")
        }

        print("✓ Successfully exported \(processed) icons to \(outputURL.path)")
        return 0
    }

    private static func handleShowcase(args: [String]) -> Int32 {
        guard let inputPath = args.first, !inputPath.hasPrefix("-") else {
            print("Error: Missing input icon/app. Usage: modo showcase <app_path> --out <banner.png>")
            return 1
        }
        let inputURL = URL(fileURLWithPath: (inputPath as NSString).expandingTildeInPath)
        var outputURL = URL(fileURLWithPath: "./showcase.png")
        var theme: ShowcaseTheme = .ambientMesh
        var layout: ShowcaseLayout = .heroSingle

        var i = 1
        while i < args.count {
            if (args[i] == "--out" || args[i] == "-o"), i + 1 < args.count {
                outputURL = URL(fileURLWithPath: (args[i + 1] as NSString).expandingTildeInPath)
                i += 2
            } else if args[i] == "--theme", i + 1 < args.count {
                if let t = ShowcaseTheme.allCases.first(where: { $0.rawValue.lowercased().contains(args[i + 1].lowercased()) }) {
                    theme = t
                }
                i += 2
            } else if args[i] == "--layout", i + 1 < args.count {
                if let l = ShowcaseLayout.allCases.first(where: { $0.rawValue.lowercased().contains(args[i + 1].lowercased()) }) {
                    layout = l
                }
                i += 2
            } else {
                i += 1
            }
        }

        let baseImg: NSImage
        if FileManager.default.fileExists(atPath: inputURL.path) {
            baseImg = NSImage(contentsOf: inputURL) ?? NSWorkspace.shared.icon(forFile: inputURL.path)
        } else {
            print("Error: Input file not found: \(inputURL.path)")
            return 1
        }

        let appTitle = inputURL.deletingPathExtension().lastPathComponent
        let options = ShowcaseExportOptions(
            theme: theme,
            layout: layout,
            title: appTitle,
            subtitle: "Crafted with Modo Adaptive Icon Studio",
            width: 1920,
            height: 1080
        )

        let showcaseImg = ShowcaseEngine.renderShowcase(icon: baseImg, options: options)
        guard let png = IconRenderer.pngData(from: showcaseImg, pixelSize: 1920) else {
            print("✗ Failed to encode PNG")
            return 1
        }

        do {
            try png.write(to: outputURL)
            print("✓ Saved presentation banner to \(outputURL.path)")
            return 0
        } catch {
            print("✗ Failed to write file: \(error.localizedDescription)")
            return 1
        }
    }

    private static func handleExport(args: [String]) -> Int32 {
        guard !args.isEmpty else {
            print("Usage: modo export <app_or_image_path> [--type <xcode|web|svg>] [--out <output_dir>]")
            return 1
        }

        let inputPath = (args[0] as NSString).expandingTildeInPath
        let inputURL = URL(fileURLWithPath: inputPath)
        guard FileManager.default.fileExists(atPath: inputURL.path) else {
            print("Error: Input path not found: \(inputURL.path)")
            return 1
        }

        var exportType = "xcode"
        var outDir = FileManager.default.currentDirectoryPath

        var i = 1
        while i < args.count {
            switch args[i] {
            case "--type":
                if i + 1 < args.count { exportType = args[i + 1].lowercased(); i += 1 }
            case "--out":
                if i + 1 < args.count { outDir = (args[i + 1] as NSString).expandingTildeInPath; i += 1 }
            default: break
            }
            i += 1
        }

        let destURL = URL(fileURLWithPath: outDir)
        let appName = inputURL.deletingPathExtension().lastPathComponent

        let baseImg: NSImage
        if inputURL.pathExtension.lowercased() == "app" {
            baseImg = NSWorkspace.shared.icon(forFile: inputURL.path)
        } else {
            baseImg = NSImage(contentsOf: inputURL) ?? NSWorkspace.shared.icon(forFile: inputURL.path)
        }

        do {
            switch exportType {
            case "xcode", "appiconset":
                let out = try DeveloperExportEngine.exportAppIconSet(from: baseImg, appName: appName, destination: destURL)
                print("✓ Exported Xcode Asset Catalog: \(out.path)")
                return 0

            case "web", "favicons":
                let out = try DeveloperExportEngine.exportWebFaviconKit(from: baseImg, appName: appName, destination: destURL)
                print("✓ Exported Web Favicon Kit: \(out.path)")
                return 0

            case "svg":
                let svg = SVGExportEngine.generateSVG(
                    title: appName,
                    style: RoundIconStyle(backgroundStyle: .darkMaterial, shape: .squircle, iconScale: 0.68),
                    dominantHex: 0x1A73E8,
                    canvasSize: 512
                )
                let svgURL = destURL.appendingPathComponent("\(appName).svg")
                try svg.write(to: svgURL, atomically: true, encoding: .utf8)
                print("✓ Exported SVG: \(svgURL.path)")
                return 0

            default:
                print("Error: Unknown export type '\(exportType)'. Use 'xcode', 'web', or 'svg'.")
                return 1
            }
        } catch {
            print("✗ Export failed: \(error.localizedDescription)")
            return 1
        }
    }

    private static func handleTheme(args: [String]) -> Int32 {
        guard !args.isEmpty else {
            print("Usage: modo theme <ios18|visionos|titanium|nord|catppuccin|prism> [--apply]")
            return 1
        }

        let themeId = args[0].lowercased()
        let shouldApply = args.contains("--apply")

        let matchedPack: IconPack
        switch themeId {
        case "ios18", "ios-18", "tinted":
            matchedPack = .ios18Tinted
        case "visionos", "glass":
            matchedPack = .visionOSGlass
        case "titanium", "obsidian":
            matchedPack = .obsidianTitanium
        case "nord", "arctic":
            matchedPack = .nordicFrost
        case "catppuccin", "mocha":
            matchedPack = .catppuccinMocha
        case "prism", "cyber":
            matchedPack = .prismGlowCyber
        case "neumorphic", "tactile":
            matchedPack = .neumorphicTactile
        default:
            matchedPack = IconPack.find(id: themeId)
        }

        print("Selected Theme: \(matchedPack.name) (\(matchedPack.subtitle))")
        print("  • Shape: \(matchedPack.style.shape.title)")
        print("  • Background: \(matchedPack.style.backgroundStyle.title)")
        print("  • Surface Material: \(matchedPack.style.surfaceMaterial.title)")

        if shouldApply {
            let appsURL = URL(fileURLWithPath: "/Applications")
            guard let contents = try? FileManager.default.contentsOfDirectory(at: appsURL, includingPropertiesForKeys: nil) else {
                print("Error: Cannot read /Applications")
                return 1
            }
            let apps = contents.filter { $0.pathExtension.lowercased() == "app" }
            print("Applying \(matchedPack.name) to \(apps.count) applications...")

            var count = 0
            var vectorCount = 0
            for appURL in apps {
                let appTitle = appURL.deletingPathExtension().lastPathComponent
                let sourceImage: NSImage
                if let vector = SimpleIconCatalog.find(for: appTitle), let svgImg = NSImage(data: vector.svgData()) {
                    sourceImage = svgImg
                    vectorCount += 1
                } else {
                    sourceImage = NSWorkspace.shared.icon(forFile: appURL.path)
                }
                let rendered = IconRenderer.renderRoundIcon(
                    sourceImage: sourceImage,
                    style: matchedPack.style,
                    fallbackHex: matchedPack.accentHex
                )
                if NSWorkspace.shared.setIcon(rendered, forFile: appURL.path, options: []) {
                    count += 1
                }
            }
            print("✓ Successfully styled \(count)/\(apps.count) applications with \(matchedPack.name) (\(vectorCount) vector brand logos matched)!")
            _ = Process.launchedProcess(launchPath: "/usr/bin/killall", arguments: ["Dock"])
        } else {
            print("Run with '--apply' to apply this theme to installed applications in /Applications.")
        }
        return 0
    }

    private static func printHelp() {
        print("""
        Modo — Native macOS Adaptive Icon Studio (v\(version))
        Command Line Interface (CLI)

        USAGE:
          modo <command> [options]

        COMMANDS:
          style <app_path>       Apply adaptive styling to any .app bundle
                                   --shape <squircle|circle|roundedRect|pebble|hexagon>
                                   --bg <pastel|dark|white|clear>
                                   --scale <0.4..1.8>
          restore <path>         Restore original system icon for an app or folder
          folder <folder_path>   Style a Finder folder with custom color & embossed badge
                                   --color <appleBlue|graphite|purple|teal|sunsetOrange|emerald|coralRed|amberGold|obsidian>
          batch <input_folder>   Batch convert all images in folder into multi-size icons
                                   --out <output_dir>
          showcase <app_or_icon> Generate 1920×1080 App Store & README presentation graphic
                                   --out <banner.png>
                                   --theme <mesh|studio|white|sunset|cyber>
                                   --layout <hero|triple|card>
          export <app_path>      Developer Export: Xcode Asset Catalog or Web Favicon Kit
                                   --type <xcode|web|svg>
                                   --out <output_dir>
          theme <theme_name>     One-Click System Theming (ios18, visionos, titanium, nord, catppuccin, prism)
                                   --apply (apply to all /Applications)
          version                Print Modo CLI version
          help                   Show this help message

        EXAMPLES:
          modo style /Applications/Slack.app --shape squircle --bg pastel
          modo export /Applications/Slack.app --type xcode --out ~/Desktop
          modo export /Applications/Slack.app --type web --out ~/Desktop
          modo theme visionos --apply
          modo folder ~/Projects/Client --color sunsetOrange
          modo batch ~/Design/Logos --out ~/Desktop/Icons
          modo showcase /Applications/Safari.app --out ./safari-banner.png --theme mesh
        """)
    }
}
