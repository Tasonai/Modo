import AppKit
import ModoKit
import SwiftUI

/// Modal sheet for generating presentation graphics and App Store / README hero banners.
struct ShowcaseSheetView: View {
    @Environment(AppModel.self) private var model
    @Environment(\.dismiss) private var dismiss

    let item: Item

    @State private var theme: ShowcaseTheme = .ambientMesh
    @State private var layout: ShowcaseLayout = .heroSingle
    @State private var title: String = ""
    @State private var subtitle: String = "Designed with Modo Adaptive Icon Studio"
    @State private var selectedResolution: ResolutionPreset = .fullHD
    @State private var previewImage: NSImage?
    @State private var isExporting = false

    enum ResolutionPreset: String, CaseIterable, Identifiable {
        case fullHD = "1920 × 1080 (Full HD / App Store)"
        case gitHub = "1280 × 640 (GitHub README Banner)"
        case square = "1024 × 1024 (Square Hero)"

        var id: String { rawValue }

        var dimensions: (w: Int, h: Int) {
            switch self {
            case .fullHD: return (1920, 1080)
            case .gitHub: return (1280, 640)
            case .square: return (1024, 1024)
            }
        }
    }

    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                VStack(alignment: .leading, spacing: 2) {
                    Text("App Showcase & Presentation Studio")
                        .font(.headline)
                    Text("Generate high-resolution hero graphics for App Store, GitHub README, and portfolios.")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
                Spacer()
                Button {
                    dismiss()
                } label: {
                    Image(systemName: "xmark.circle.fill")
                        .font(.system(size: 18))
                        .foregroundStyle(.secondary)
                }
                .buttonStyle(.plain)
            }
            .padding(Space.xl)
            .background(Color(nsColor: .windowBackgroundColor))

            Divider()

            HStack(spacing: 20) {
                // Controls Column
                VStack(alignment: .leading, spacing: Space.lg) {
                    // Theme picker
                    VStack(alignment: .leading, spacing: Space.sm) {
                        Text("Backdrop Theme")
                            .font(.system(size: 11, weight: .bold))
                            .foregroundStyle(.secondary)
                        Picker("", selection: $theme) {
                            ForEach(ShowcaseTheme.allCases) { t in
                                Text(t.rawValue).tag(t)
                            }
                        }
                        .labelsHidden()
                        .onChange(of: theme) { updatePreview() }
                    }

                    // Layout picker
                    VStack(alignment: .leading, spacing: Space.sm) {
                        Text("Scene Composition")
                            .font(.system(size: 11, weight: .bold))
                            .foregroundStyle(.secondary)
                        Picker("", selection: $layout) {
                            ForEach(ShowcaseLayout.allCases) { l in
                                Text(l.rawValue).tag(l)
                            }
                        }
                        .labelsHidden()
                        .onChange(of: layout) { updatePreview() }
                    }

                    // Resolution
                    VStack(alignment: .leading, spacing: Space.sm) {
                        Text("Dimensions")
                            .font(.system(size: 11, weight: .bold))
                            .foregroundStyle(.secondary)
                        Picker("", selection: $selectedResolution) {
                            ForEach(ResolutionPreset.allCases) { r in
                                Text(r.rawValue).tag(r)
                            }
                        }
                        .labelsHidden()
                        .onChange(of: selectedResolution) { updatePreview() }
                    }

                    Divider()

                    // Title & Subtitle
                    VStack(alignment: .leading, spacing: Space.sm) {
                        Text("Title")
                            .font(.system(size: 11, weight: .bold))
                            .foregroundStyle(.secondary)
                        TextField("App Title", text: $title)
                            .textFieldStyle(.roundedBorder)
                            .onChange(of: title) { updatePreview() }

                        Text("Subtitle / Tagline")
                            .font(.system(size: 11, weight: .bold))
                            .foregroundStyle(.secondary)
                        TextField("Subtitle", text: $subtitle)
                            .textFieldStyle(.roundedBorder)
                            .onChange(of: subtitle) { updatePreview() }
                    }

                    Spacer()

                    // Export button
                    Button {
                        exportShowcase()
                    } label: {
                        HStack {
                            Image(systemName: "square.and.arrow.up.fill")
                            Text("Export High-Res Banner…")
                        }
                        .frame(maxWidth: .infinity)
                    }
                    .buttonStyle(.borderedProminent)
                    .tint(Palette.accent)
                    .controlSize(.large)
                }
                .frame(width: 280)
                .padding(Space.xl)

                Divider()

                // Live Preview Column
                VStack {
                    ZStack {
                        RoundedRectangle(cornerRadius: 12)
                            .fill(Color.black.opacity(0.15))

                        if let img = previewImage {
                            Image(nsImage: img)
                                .resizable()
                                .scaledToFit()
                                .clipShape(RoundedRectangle(cornerRadius: 8))
                                .shadow(color: .black.opacity(0.3), radius: 10, y: 5)
                                .padding(12)
                        } else {
                            ProgressView()
                        }
                    }
                    .frame(maxWidth: .infinity, maxHeight: .infinity)

                    Text("Live preview scaled to fit. Export renders in uncompressed full resolution.")
                        .font(.system(size: 10))
                        .foregroundStyle(.secondary)
                        .padding(.top, 4)
                }
                .padding(Space.xl)
            }
        }
        .frame(width: 820, height: 520)
        .onAppear {
            title = item.title
            updatePreview()
        }
    }

    private func updatePreview() {
        let (w, h) = selectedResolution.dimensions
        let scale = 0.5 // scale down preview for performance
        let options = ShowcaseExportOptions(
            theme: theme,
            layout: layout,
            title: title.isEmpty ? item.title : title,
            subtitle: subtitle,
            width: max(Int(Double(w) * scale), 480),
            height: max(Int(Double(h) * scale), 270)
        )

        let baseIcon = model.roundIcon(for: item)
        let darkIcon = getAppearanceIcon(mode: .dark, size: 512)
        let tintedIcon = getAppearanceIcon(mode: .tinted, size: 512)

        previewImage = ShowcaseEngine.renderShowcase(
            icon: baseIcon,
            darkIcon: darkIcon,
            tintedIcon: tintedIcon,
            options: options
        )
    }

    private func getAppearanceIcon(mode: IconAppearanceMode, size: Int) -> NSImage {
        let source = model.sourceIcon(for: item, canvasSize: size)
        var variantStyle = model.effectiveStyle(for: item)
        variantStyle.appearanceMode = mode
        return IconRenderer.renderRoundIcon(
            sourceImage: source,
            style: variantStyle,
            fallbackHex: UInt32(clamping: item.brandHex),
            canvasSize: size
        )
    }

    private func exportShowcase() {
        let panel = NSSavePanel()
        panel.canCreateDirectories = true
        panel.allowedContentTypes = [.png]
        panel.nameFieldStringValue = "\(item.title)-Showcase.png"

        guard panel.runModal() == .OK, let url = panel.url else { return }

        let (w, h) = selectedResolution.dimensions
        let options = ShowcaseExportOptions(
            theme: theme,
            layout: layout,
            title: title.isEmpty ? item.title : title,
            subtitle: subtitle,
            width: w,
            height: h
        )

        let baseIcon = model.roundIcon(for: item)
        let darkIcon = getAppearanceIcon(mode: .dark, size: 1024)
        let tintedIcon = getAppearanceIcon(mode: .tinted, size: 1024)

        let fullResImage = ShowcaseEngine.renderShowcase(
            icon: baseIcon,
            darkIcon: darkIcon,
            tintedIcon: tintedIcon,
            options: options
        )

        if let png = IconRenderer.pngData(from: fullResImage, pixelSize: max(w, h)) {
            try? png.write(to: url)
            NSWorkspace.shared.activateFileViewerSelecting([url])
            dismiss()
        }
    }
}
