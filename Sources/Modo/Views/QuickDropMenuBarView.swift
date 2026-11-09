import AppKit
import ModoKit
import SwiftUI

/// Compact MenuBar Extra popover for quick drag-and-drop icon styling on macOS.
struct QuickDropMenuBarView: View {
    @Environment(AppModel.self) private var model
    @State private var isTargeted = false
    @State private var lastStyledName: String?
    @State private var lastStyledURL: URL?
    @State private var lastStyledImage: NSImage?
    @State private var statusMessage = "Drop any App or Folder to instant-style"
    @State private var statusIsError = false

    var body: some View {
        @Bindable var model = model
        VStack(spacing: 12) {
            // Header
            HStack {
                Image(systemName: "circle.grid.2x2.fill")
                    .foregroundStyle(Palette.accent)
                Text("Modo Quick Drop")
                    .font(.system(size: 13, weight: .bold))
                Spacer()
                Button {
                    NSApp.activate(ignoringOtherApps: true)
                    for window in NSApp.windows where window.identifier?.rawValue == "main" || window.title == "Modo" {
                        window.makeKeyAndOrderFront(nil)
                    }
                } label: {
                    Image(systemName: "macwindow.badge.plus")
                        .font(.system(size: 12))
                        .foregroundStyle(.secondary)
                }
                .buttonStyle(.plain)
                .help("Open Main Studio Window")
            }

            Divider()

            // Quick Drop Zone
            ZStack {
                RoundedRectangle(cornerRadius: 12, style: .continuous)
                    .strokeBorder(
                        isTargeted ? Palette.accent : Color.secondary.opacity(0.3),
                        style: StrokeStyle(lineWidth: isTargeted ? 2 : 1.5, dash: [6, 4])
                    )
                    .background(
                        RoundedRectangle(cornerRadius: 12, style: .continuous)
                            .fill(isTargeted ? Palette.accent.opacity(0.12) : Color.primary.opacity(0.03))
                    )

                VStack(spacing: Space.sm) {
                    if let img = lastStyledImage, let name = lastStyledName {
                        HStack(spacing: Space.md) {
                            Image(nsImage: img)
                                .resizable()
                                .scaledToFit()
                                .frame(width: 44, height: 44)
                                .shadow(color: .black.opacity(0.25), radius: 3, y: 1)

                            VStack(alignment: .leading, spacing: 2) {
                                Text(name)
                                    .font(.system(size: 12, weight: .semibold))
                                    .lineLimit(1)
                                Text("Styled Successfully!")
                                    .font(.system(size: 10))
                                    .foregroundStyle(Palette.accent)
                            }
                            Spacer()

                            Button {
                                if let url = lastStyledURL {
                                    _ = NSWorkspace.shared.setIcon(nil, forFile: url.path, options: [])
                                    lastStyledImage = nil
                                    lastStyledName = nil
                                    lastStyledURL = nil
                                    statusMessage = "Restored original icon"
                                }
                            } label: {
                                Image(systemName: "arrow.uturn.backward.circle.fill")
                                    .font(.system(size: 18))
                                    .foregroundStyle(.secondary)
                            }
                            .buttonStyle(.plain)
                            .help("Restore Original Icon")
                        }
                        .padding(.horizontal, Space.md)
                    } else {
                        Image(systemName: isTargeted ? "arrow.down.circle.fill" : "square.and.arrow.down")
                            .font(.system(size: 26))
                            .foregroundStyle(isTargeted ? Palette.accent : .secondary)

                        Text(statusMessage)
                            .font(.system(size: 11, weight: .medium))
                            .foregroundStyle(statusIsError ? .red : (isTargeted ? Palette.accent : .secondary))
                            .multilineTextAlignment(.center)
                            .padding(.horizontal, 8)
                    }
                }
                .padding(.vertical, Space.lg)
            }
            .frame(height: 84)
            .onDrop(of: [.fileURL], isTargeted: $isTargeted) { providers in
                handleQuickDrop(providers: providers)
            }

            // Quick Shape & Background Presets
            VStack(alignment: .leading, spacing: Space.sm) {
                Text("Active Style")
                    .font(.system(size: 10, weight: .bold))
                    .foregroundStyle(.secondary)
                    .textCase(.uppercase)

                HStack(spacing: 4) {
                    ForEach([IconShape.squircle, .circle, .roundedRect, .pebble, .hexagon], id: \.self) { shape in
                        Button {
                            model.style.shape = shape
                        } label: {
                            Text(shape.title)
                                .font(.system(size: 10, weight: model.style.shape == shape ? .bold : .regular))
                                .padding(.horizontal, Space.sm)
                                .padding(.vertical, Space.xs)
                                .background(
                                    RoundedRectangle(cornerRadius: 8, style: .continuous)
                                        .fill(model.style.shape == shape ? Palette.accent : Color.primary.opacity(0.06))
                                )
                                .foregroundStyle(model.style.shape == shape ? .white : .primary)
                        }
                        .buttonStyle(.plain)
                    }
                }

                HStack(spacing: 4) {
                    ForEach([RoundBackgroundStyle.materialPastel, .darkMaterial, .googleWhite, .adaptiveEdge], id: \.self) { bg in
                        Button {
                            model.style.backgroundStyle = bg
                        } label: {
                            Text(bg.title)
                                .font(.system(size: 10, weight: model.style.backgroundStyle == bg ? .bold : .regular))
                                .padding(.horizontal, Space.sm)
                                .padding(.vertical, Space.xs)
                                .background(
                                    RoundedRectangle(cornerRadius: 8, style: .continuous)
                                        .fill(model.style.backgroundStyle == bg ? Palette.accent.opacity(0.85) : Color.primary.opacity(0.06))
                                )
                                .foregroundStyle(model.style.backgroundStyle == bg ? .white : .primary)
                        }
                        .buttonStyle(.plain)
                    }
                }

                // Surface Materials
                HStack(spacing: Space.xs) {
                    ForEach(SurfaceMaterial.allCases) { mat in
                        Button {
                            model.style.surfaceMaterial = mat
                        } label: {
                            Text(mat.title)
                                .font(.system(size: 9, weight: model.style.surfaceMaterial == mat ? .bold : .regular))
                                .padding(.horizontal, Space.xs)
                                .padding(.vertical, Space.xs)
                                .background(
                                    RoundedRectangle(cornerRadius: 8, style: .continuous)
                                        .fill(model.style.surfaceMaterial == mat ? Palette.accent : Color.primary.opacity(0.06))
                                )
                                .foregroundStyle(model.style.surfaceMaterial == mat ? .white : .primary)
                        }
                        .buttonStyle(.plain)
                    }
                }

                // 1-Click Signature Presets & Wallpaper
                HStack(spacing: 4) {
                    Button {
                        if let screen = NSScreen.main,
                           let url = NSWorkspace.shared.desktopImageURL(for: screen),
                           let img = NSImage(contentsOf: url) {
                            let pal = WallpaperPaletteEngine.extractPalette(from: img)
                            model.style.backgroundStyle = .customColor
                            model.style.customHex = pal.darkHex
                            statusMessage = "Synced with Wallpaper"
                        }
                    } label: {
                        HStack(spacing: Space.xs) {
                            Image(systemName: "sparkles")
                            Text("Wallpaper")
                        }
                        .font(.system(size: 9, weight: .medium))
                        .padding(.horizontal, Space.xs)
                        .padding(.vertical, Space.xs)
                        .background(RoundedRectangle(cornerRadius: 8).fill(Color.primary.opacity(0.06)))
                    }
                    .buttonStyle(.plain)

                    ForEach([IconPack.visionOSGlass, .ios18Tinted, .obsidianTitanium]) { pack in
                        Button {
                            model.applyPack(pack)
                            statusMessage = "Applied \(pack.name)"
                        } label: {
                            Text(pack.name)
                                .font(.system(size: 9, weight: model.activePackId == pack.id ? .bold : .regular))
                                .padding(.horizontal, Space.xs)
                                .padding(.vertical, Space.xs)
                                .background(RoundedRectangle(cornerRadius: 8).fill(model.activePackId == pack.id ? Palette.accent : Color.primary.opacity(0.06)))
                                .foregroundStyle(model.activePackId == pack.id ? .white : .primary)
                        }
                        .buttonStyle(.plain)
                    }
                }

                Button {
                    NSApp.activate(ignoringOtherApps: true)
                    for window in NSApp.windows where window.identifier?.rawValue == "main" || window.title == "Modo" {
                        window.makeKeyAndOrderFront(nil)
                    }
                    model.showSystemTheming = true
                } label: {
                    HStack(spacing: 4) {
                        Image(systemName: "wand.and.stars")
                        Text("1-Click System Theming Wizard…")
                    }
                    .font(.system(size: 11, weight: .semibold))
                    .frame(maxWidth: .infinity)
                    .padding(.vertical, 4)
                    .background(RoundedRectangle(cornerRadius: 8).fill(Palette.accent.opacity(0.12)))
                    .foregroundStyle(Palette.accentText)
                }
                .buttonStyle(.plain)
            }

            Divider()

            // Footer Actions
            HStack {
                Button("Check for Updates") {
                    model.showUpdateSheet = true
                    Task { await OTAUpdateService.shared.checkForUpdates(manual: true) }
                }
                .font(.system(size: 11))
                .buttonStyle(.plain)
                .foregroundStyle(.secondary)

                Spacer()

                Button("Quit") {
                    NSApp.terminate(nil)
                }
                .font(.system(size: 11))
                .buttonStyle(.plain)
                .foregroundStyle(.secondary)
            }
        }
        .padding(Space.lg)
        .frame(width: 320)
    }

    private func handleQuickDrop(providers: [NSItemProvider]) -> Bool {
        guard let provider = providers.first else { return false }
        provider.loadItem(forTypeIdentifier: "public.file-url", options: nil) { data, _ in
            guard let data = data as? Data,
                  let url = URL(dataRepresentation: data, relativeTo: nil) else { return }

            DispatchQueue.main.async {
                self.processDroppedURL(url)
            }
        }
        return true
    }

    private func processDroppedURL(_ url: URL) {
        var isDir: ObjCBool = false
        guard FileManager.default.fileExists(atPath: url.path, isDirectory: &isDir) else {
            statusMessage = "File not found"
            statusIsError = true
            return
        }

        let isApp = url.pathExtension.lowercased() == "app"

        if isApp {
            let sourceImage = NSWorkspace.shared.icon(forFile: url.path)
            let rendered = IconRenderer.renderRoundIcon(sourceImage: sourceImage, style: model.style, fallbackHex: 0x1A73E8)
            let ok = NSWorkspace.shared.setIcon(rendered, forFile: url.path, options: [])
            if ok {
                lastStyledName = url.deletingPathExtension().lastPathComponent
                lastStyledURL = url
                lastStyledImage = rendered
                statusMessage = "Styled \(url.lastPathComponent)!"
                statusIsError = false
                NSSound.beep()
            } else {
                statusMessage = "Failed: permission denied"
                statusIsError = true
            }
        } else if isDir.boolValue {
            // Folder styling
            let svgMarkup = FolderVectorGenerator.generateFolderSVG(
                style: FolderStyle(preset: .sunsetOrange, embossStyle: .etched),
                size: 512
            )
            if let svgData = svgMarkup.data(using: .utf8), let icon = NSImage(data: svgData) {
                let ok = NSWorkspace.shared.setIcon(icon, forFile: url.path, options: [])
                if ok {
                    lastStyledName = url.lastPathComponent
                    lastStyledURL = url
                    lastStyledImage = icon
                    statusMessage = "Styled Folder!"
                    statusIsError = false
                    NSSound.beep()
                } else {
                    statusMessage = "Permission denied for folder"
                    statusIsError = true
                }
            }
        } else {
            statusMessage = "Drop an .app or Folder"
            statusIsError = true
        }
    }
}
