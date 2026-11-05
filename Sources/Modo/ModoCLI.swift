import AppKit
import Foundation
import ModoKit

// MARK: - Modo Command-Line Interface Engine

public enum ModoCLI {
    public static let version = "1.6.0"

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

        let sourceImage = NSWorkspace.shared.icon(forFile: url.path)
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
          version                Print Modo CLI version
          help                   Show this help message

        EXAMPLES:
          modo style /Applications/Slack.app --shape squircle --bg pastel
          modo folder ~/Projects/Client --color sunsetOrange
          modo batch ~/Design/Logos --out ~/Desktop/Icons
          modo showcase /Applications/Safari.app --out ./safari-banner.png --theme mesh
        """)
    }
}
