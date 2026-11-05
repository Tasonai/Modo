import AppKit
import ModoKit
import SwiftData
import SwiftUI

struct DetailInspectorView: View {
    @Environment(AppModel.self) private var model
    @Environment(\.modelContext) private var context
    @Bindable var item: Item
    let allVisibleItems: [Item]
    @State private var dragStartOffset: CGPoint?
    @State private var adjustThisAppOnly: Bool = false
    @State private var isDropTargeted: Bool = false
    @State private var showSimpleIconsSheet: Bool = false

    private var effectiveScale: Double {
        item.customScale ?? model.style.iconScale
    }

    private var itemScaleBinding: Binding<Double> {
        Binding(
            get: { item.customScale ?? model.style.iconScale },
            set: { newScale in
                if adjustThisAppOnly {
                    item.customScale = newScale
                    try? context.save()
                    model.invalidateCache(for: item)
                } else {
                    item.customScale = nil
                    try? context.save()
                    model.style.iconScale = newScale
                }
            }
        )
    }

    private var customColorBinding: Binding<Color> {
        Binding(
            get: { EyedropperPicker.color(from: model.style.customHex) },
            set: { newColor in
                model.style.customHex = EyedropperPicker.hex(from: newColor)
                model.style.backgroundStyle = .customColor
            }
        )
    }

    private var tintColorBinding: Binding<Color> {
        Binding(
            get: { EyedropperPicker.color(from: model.style.tintColorHex) },
            set: { newColor in
                model.style.tintColorHex = EyedropperPicker.hex(from: newColor)
            }
        )
    }

    var body: some View {
        @Bindable var model = model
        ScrollView {
            VStack(alignment: .leading, spacing: Space.lg) {
                headerSection
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .gridProbe("header", area: "detail")

                comparisonCard
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .gridProbe("comparison", area: "detail")

                customArtworkCard
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .gridProbe("custom_artwork", area: "detail")

                iconPacksCard
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .gridProbe("packs", area: "detail")

                styleControlsCard
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .gridProbe("controls", area: "detail")

                actionFooter
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .gridProbe("actions", area: "detail")
            }
            .padding(Space.margin)
            .frame(maxWidth: .infinity, alignment: .topLeading)
            .gridArea("detail", inset: Space.margin)
        }
        .onAppear {
            adjustThisAppOnly = (item.customScale != nil)
        }
        .onChange(of: item.id) {
            adjustThisAppOnly = (item.customScale != nil)
        }
        .sheet(isPresented: $showSimpleIconsSheet) {
            SimpleIconsBrowserView(item: item)
        }
    }

    private var headerSection: some View {
        HStack(alignment: .center, spacing: Space.md) {
            VStack(alignment: .leading, spacing: Space.xs) {
                HStack(spacing: Space.sm) {
                    Text(item.title)
                        .font(.title2.weight(.semibold))
                    if item.isApplied {
                        Label("Modo Icon Active", systemImage: "checkmark.seal.fill")
                            .font(.caption.weight(.semibold))
                            .foregroundStyle(Palette.accentText)
                            .padding(.horizontal, Space.sm)
                            .padding(.vertical, Space.xs)
                            .background(Palette.accent.opacity(0.14), in: Capsule())
                    }
                }
                Text("\(item.categoryName) · \(item.bundleIdentifier)")
                    .font(.subheadline)
                    .foregroundStyle(.secondary)
            }
            Spacer()
            Button {
                item.isStarred.toggle()
                try? context.save()
            } label: {
                Label(item.isStarred ? "Starred" : "Favorite",
                      systemImage: item.isStarred ? "star.fill" : "star")
            }
            .help("Toggle Favorite status")
        }
    }

    private var comparisonCard: some View {
        let shape = RoundedRectangle(cornerRadius: Radius.md, style: .continuous)
        let hasCustom = (item.customImageData != nil)
        let isSVG = hasCustom && CustomArtworkHelper.isSVGData(item.customImageData!)

        return HStack(spacing: Space.xxl) {
            Spacer(minLength: 0)
            VStack(spacing: Space.sm) {
                Image(nsImage: model.sourceIcon(for: item, canvasSize: 256))
                    .resizable()
                    .interpolation(.high)
                    .frame(width: 96, height: 96)
                if hasCustom {
                    Text("Custom Glyph")
                        .font(.callout.weight(.medium))
                    HStack(spacing: 4) {
                        Text(isSVG ? "Vector SVG" : "Custom Image")
                            .font(.caption)
                            .foregroundStyle(Palette.accentText)
                        Button {
                            model.clearCustomArtwork(for: item)
                            try? context.save()
                        } label: {
                            Image(systemName: "xmark.circle.fill")
                                .font(.caption)
                                .foregroundStyle(.secondary)
                        }
                        .buttonStyle(.plain)
                        .help("Restore default macOS icon")
                    }
                } else {
                    Text("Original")
                        .font(.callout.weight(.medium))
                    Text("macOS Default")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            }

            VStack(spacing: Space.xs) {
                Image(systemName: "arrow.right.circle.fill")
                    .font(.title2)
                    .foregroundStyle(Palette.accentText)
                Text("\(Int((effectiveScale * 100).rounded()))% Scale")
                    .font(.caption2.monospacedDigit())
                    .foregroundStyle(.secondary)
                if item.customScale != nil {
                    Text("Per-App")
                        .font(.system(size: 9, weight: .bold))
                        .foregroundStyle(Palette.accentText)
                        .padding(.horizontal, 4)
                        .padding(.vertical, 1)
                        .background(Palette.accent.opacity(0.14), in: Capsule())
                }
            }

            VStack(spacing: Space.sm) {
                Image(nsImage: model.roundIcon(for: item))
                    .resizable()
                    .interpolation(.high)
                    .frame(width: 96, height: 96)
                    .animation(.interactiveSpring(response: 0.28, dampingFraction: 0.86), value: model.style.offsetX)
                    .animation(.interactiveSpring(response: 0.28, dampingFraction: 0.86), value: model.style.offsetY)
                    .gesture(
                        DragGesture(minimumDistance: 1)
                            .onChanged { value in
                                let base = dragStartOffset ?? CGPoint(x: model.style.offsetX, y: model.style.offsetY)
                                if dragStartOffset == nil { dragStartOffset = base }
                                let nx = min(max(base.x + Double(value.translation.width) / 48.0, -1.0), 1.0)
                                let ny = min(max(base.y - Double(value.translation.height) / 48.0, -1.0), 1.0)
                                model.style.offsetX = nx
                                model.style.offsetY = ny
                            }
                            .onEnded { _ in
                                dragStartOffset = nil
                            }
                    )
                    .onTapGesture(count: 2) {
                        NSHapticFeedbackManager.defaultPerformer.perform(.alignment, performanceTime: .default)
                        model.style.resetPosition()
                    }
                    .help("Drag to pan icon · Double click to center")
                Text(model.activePackName)
                    .font(.callout.weight(.semibold))
                Text("Drag to Pan · Double click to Center")
                    .font(.caption)
                    .foregroundStyle(Palette.accentText)
            }
            Spacer(minLength: 0)
        }
        .padding(Space.lg)
        .background(.background.secondary, in: shape)
        .overlay(shape.strokeBorder(isDropTargeted ? Palette.accent : Color(nsColor: .separatorColor), lineWidth: isDropTargeted ? 2 : 1))
        .dropDestination(for: URL.self) { urls, _ in
            guard let url = urls.first, let data = CustomArtworkHelper.loadArtworkData(from: url) else { return false }
            model.setCustomArtwork(data: data, for: item)
            try? context.save()
            return true
        } isTargeted: { targeted in
            isDropTargeted = targeted
        }
    }

    private var customArtworkCard: some View {
        let cardShape = RoundedRectangle(cornerRadius: Radius.md, style: .continuous)
        let hasCustom = (item.customImageData != nil)
        let isSVG = hasCustom && CustomArtworkHelper.isSVGData(item.customImageData!)

        return VStack(alignment: .leading, spacing: Space.sm) {
            HStack {
                Label("Custom Glyph & SVG", systemImage: isSVG ? "sparkles" : "photo.on.rectangle.angled")
                    .font(.headline)
                Spacer()
                if hasCustom {
                    Text(isSVG ? "Vector SVG Active" : "Custom Artwork Active")
                        .font(.caption.weight(.semibold))
                        .foregroundStyle(Palette.accentText)
                        .padding(.horizontal, Space.sm)
                        .padding(.vertical, 2)
                        .background(Palette.accent.opacity(0.14), in: Capsule())
                }
            }

            if let suggested = SimpleIconCatalog.find(for: item.title), !hasCustom {
                HStack(spacing: Space.sm) {
                    Image(systemName: "sparkles")
                        .foregroundStyle(Palette.accentText)
                    VStack(alignment: .leading, spacing: 1) {
                        Text("Simple Icon match: **\(suggested.title)**")
                            .font(.caption.weight(.medium))
                        Text("Official monochrome vector SVG")
                            .font(.caption2)
                            .foregroundStyle(.secondary)
                    }
                    Spacer()
                    Button("Apply SVG") {
                        model.setCustomArtwork(data: suggested.svgData(), for: item)
                        item.brandHex = Int(suggested.hex)
                        try? context.save()
                    }
                    .controlSize(.small)
                    .buttonStyle(.borderedProminent)
                }
                .padding(Space.sm)
                .background(Palette.accent.opacity(0.10), in: RoundedRectangle(cornerRadius: Radius.sm))
            }

            Text("Drop an SVG vector or image directly onto this card, or choose a file:")
                .font(.caption)
                .foregroundStyle(.secondary)

            HStack(spacing: Space.sm) {
                Button {
                    showSimpleIconsSheet = true
                } label: {
                    Label("Simple Icons…", systemImage: "sparkles.rectangle.stack")
                }
                .controlSize(.small)

                Button {
                    if let data = CustomArtworkHelper.promptUserForArtwork() {
                        model.setCustomArtwork(data: data, for: item)
                        try? context.save()
                    }
                } label: {
                    Label("Choose File…", systemImage: "photo.badge.plus")
                }
                .controlSize(.small)

                Button {
                    if model.pasteCustomArtwork(for: item) {
                        try? context.save()
                    }
                } label: {
                    Label("Paste Image", systemImage: "doc.on.clipboard")
                }
                .controlSize(.small)
                .disabled(!CustomArtworkHelper.canPasteArtwork())

                if hasCustom {
                    Spacer()
                    Button(role: .destructive) {
                        model.clearCustomArtwork(for: item)
                        try? context.save()
                    } label: {
                        Label("Reset to Default", systemImage: "arrow.uturn.backward")
                    }
                    .controlSize(.small)
                }
            }
        }
        .padding(Space.md)
        .background(.background.secondary, in: cardShape)
        .overlay(cardShape.strokeBorder(isDropTargeted ? Palette.accent : Color(nsColor: .separatorColor), lineWidth: isDropTargeted ? 2 : 1))
        .dropDestination(for: URL.self) { urls, _ in
            guard let url = urls.first, let data = CustomArtworkHelper.loadArtworkData(from: url) else { return false }
            model.setCustomArtwork(data: data, for: item)
            try? context.save()
            return true
        } isTargeted: { targeted in
            isDropTargeted = targeted
        }
    }

    private var iconPacksCard: some View {
        let cardShape = RoundedRectangle(cornerRadius: Radius.md, style: .continuous)
        return VStack(alignment: .leading, spacing: Space.sm) {
            HStack {
                Text("Curated Icon Packs")
                    .font(.headline)
                Spacer()
                Text("One-click style presets")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: Space.sm) {
                    ForEach(IconPack.allPacks) { pack in
                        packButton(pack)
                    }
                }
                .padding(.vertical, 2)
            }
        }
        .padding(Space.md)
        .background(.background.secondary, in: cardShape)
        .overlay(cardShape.strokeBorder(.separator, lineWidth: 1))
    }

    private func packButton(_ pack: IconPack) -> some View {
        let isSelected = model.activePackId == pack.id
        let btnShape = RoundedRectangle(cornerRadius: Radius.sm, style: .continuous)
        return Button {
            model.applyPack(pack)
        } label: {
            VStack(alignment: .leading, spacing: 4) {
                HStack(spacing: 6) {
                    Image(systemName: pack.iconSymbol)
                        .font(.subheadline)
                    Text(pack.name)
                        .font(.subheadline.weight(.semibold))
                }
                Text(pack.subtitle)
                    .font(.caption2)
                    .foregroundStyle(.secondary)
                    .lineLimit(1)
            }
            .padding(.horizontal, Space.sm)
            .padding(.vertical, Space.xs)
            .frame(width: 156, alignment: .leading)
            .background(isSelected ? Palette.accent.opacity(0.12) : Color.clear, in: btnShape)
            .overlay(
                btnShape.strokeBorder(
                    isSelected ? Palette.accent : Color.primary.opacity(0.15),
                    lineWidth: isSelected ? 1.5 : 1
                )
            )
        }
        .buttonStyle(.plain)
        .help("\(pack.name) (\(pack.category)): \(pack.subtitle)")
    }

    private var styleControlsCard: some View {
        @Bindable var model = model
        let shape = RoundedRectangle(cornerRadius: Radius.md, style: .continuous)
        return VStack(alignment: .leading, spacing: Space.md) {
            Text("Modo Adaptive Studio")
                .font(.headline)

            LabeledContent("Appearance Theme") {
                Picker("Appearance Theme", selection: $model.style.appearanceMode) {
                    Label("Light", systemImage: "sun.max.fill").tag(IconAppearanceMode.light)
                    Label("Dark", systemImage: "moon.stars.fill").tag(IconAppearanceMode.dark)
                    Label("Tinted", systemImage: "paintpalette.fill").tag(IconAppearanceMode.tinted)
                    Label("Auto", systemImage: "circle.lefthalf.filled").tag(IconAppearanceMode.system)
                }
                .pickerStyle(.segmented)
                .frame(width: 260)
            }

            if model.style.appearanceMode == .tinted {
                LabeledContent("Accent Tint (iOS 18)") {
                    HStack(spacing: Space.sm) {
                        ColorPicker("Tint Color", selection: tintColorBinding, supportsOpacity: false)
                            .labelsHidden()

                        Text(MaterialRGB(hex: model.style.tintColorHex).hexString)
                            .font(.caption.monospaced())
                            .foregroundStyle(.secondary)

                        HStack(spacing: Space.xs) {
                            ForEach([0x007AFF, 0x34C759, 0xAF52DE, 0xFF9500, 0xFF2D55, 0x5856D6], id: \.self) { (hex: Int) in
                                Button {
                                    model.style.tintColorHex = UInt32(hex)
                                } label: {
                                    Circle()
                                        .fill(EyedropperPicker.color(from: UInt32(hex)))
                                        .frame(width: 16, height: 16)
                                        .overlay(
                                            Circle()
                                                .strokeBorder(Color.white.opacity(0.8), lineWidth: model.style.tintColorHex == UInt32(hex) ? 2 : 0)
                                        )
                                }
                                .buttonStyle(.plain)
                            }
                        }
                    }
                }
            }

            LabeledContent("Shape Geometry") {
                Picker("Shape Geometry", selection: $model.style.shape) {
                    ForEach(IconShape.allCases) { item in
                        Label(item.title, systemImage: item.symbol).tag(item)
                    }
                }
                .labelsHidden()
                .frame(width: 240)
            }

            LabeledContent("Surface Preset") {
                Picker("Surface Preset", selection: $model.style.backgroundStyle) {
                    ForEach(RoundBackgroundStyle.allCases) { preset in
                        Text(preset.title).tag(preset)
                    }
                }
                .labelsHidden()
                .frame(width: 240)
            }

            LabeledContent("Color & Eyedropper") {
                HStack(spacing: Space.sm) {
                    Button {
                        EyedropperPicker.sampleScreenColor { hex in
                            model.style.customHex = hex
                            model.style.backgroundStyle = .customColor
                        }
                    } label: {
                        Label("Sample Color", systemImage: "eyedropper.halffull")
                    }
                    .controlSize(.small)
                    .help("Pick any color from the icon or screen with macOS Eyedropper")

                    ColorPicker("Custom Surface Color", selection: customColorBinding, supportsOpacity: false)
                        .labelsHidden()
                        .help("Choose custom surface color")

                    Text(MaterialRGB(hex: model.style.customHex).hexString)
                        .font(.caption.monospaced())
                        .foregroundStyle(.secondary)

                    HStack(spacing: Space.xs) {
                        ForEach(ColorSwatchToken.presets.prefix(5)) { swatch in
                            Button {
                                model.style.customHex = swatch.hex
                                model.style.backgroundStyle = .customColor
                            } label: {
                                Circle()
                                    .fill(swatch.color)
                                    .frame(width: 16, height: 16)
                                    .overlay(
                                        Circle()
                                            .strokeBorder(.primary.opacity(model.style.customHex == swatch.hex ? 0.65 : 0.18),
                                                          lineWidth: 1)
                                    )
                            }
                            .buttonStyle(.plain)
                            .help(swatch.name)
                        }
                    }
                }
            }

            Divider()

            LabeledContent {
                VStack(alignment: .leading, spacing: 6) {
                    HStack(spacing: Space.sm) {
                        Slider(value: itemScaleBinding, in: 0.30...2.50)
                            .frame(width: 176)
                        Button("100%") { setScale(1.0) }
                            .controlSize(.small)
                            .help("Set scale to 100%")
                        Button("135%") { setScale(1.35) }
                            .controlSize(.small)
                            .help("Zoom icon to 135% inside the icon shape")
                    }

                    HStack(spacing: Space.sm) {
                        Toggle("This app only", isOn: $adjustThisAppOnly)
                            .toggleStyle(.checkbox)
                            .font(.caption)

                        if let custom = item.customScale {
                            Text("Custom override: \(Int((custom * 100).rounded()))%")
                                .font(.caption2.monospacedDigit())
                                .foregroundStyle(Palette.accentText)
                            Button {
                                item.customScale = nil
                                adjustThisAppOnly = false
                                try? context.save()
                                model.invalidateCache(for: item)
                            } label: {
                                Image(systemName: "xmark.circle.fill")
                                    .font(.caption)
                                    .foregroundStyle(.secondary)
                            }
                            .buttonStyle(.plain)
                            .help("Reset to global style scale")
                        }
                    }
                }
            } label: {
                Text("Glyph Scale (\(Int((effectiveScale * 100).rounded()))%)")
            }

            LabeledContent("Horizontal X (\(Int((model.style.offsetX * 100).rounded()))%)") {
                HStack(spacing: Space.xs) {
                    Button {
                        model.style.offsetX = max(model.style.offsetX - 0.05, -1.0)
                    } label: {
                        Image(systemName: "arrow.left")
                    }
                    .controlSize(.small)
                    .help("Move Left")

                    Slider(value: $model.style.offsetX, in: -1.0...1.0)
                        .frame(width: 160)

                    Button {
                        model.style.offsetX = min(model.style.offsetX + 0.05, 1.0)
                    } label: {
                        Image(systemName: "arrow.right")
                    }
                    .controlSize(.small)
                    .help("Move Right")
                }
            }

            LabeledContent("Vertical Y (\(Int((model.style.offsetY * 100).rounded()))%)") {
                HStack(spacing: Space.xs) {
                    Button {
                        model.style.offsetY = max(model.style.offsetY - 0.05, -1.0)
                    } label: {
                        Image(systemName: "arrow.down")
                    }
                    .controlSize(.small)
                    .help("Move Down")

                    Slider(value: $model.style.offsetY, in: -1.0...1.0)
                        .frame(width: 160)

                    Button {
                        model.style.offsetY = min(model.style.offsetY + 0.05, 1.0)
                    } label: {
                        Image(systemName: "arrow.up")
                    }
                    .controlSize(.small)
                    .help("Move Up")

                    Button("Center") {
                        NSHapticFeedbackManager.defaultPerformer.perform(.alignment, performanceTime: .default)
                        model.style.resetPosition()
                    }
                    .controlSize(.small)
                    .disabled(abs(model.style.offsetX) < 0.005 && abs(model.style.offsetY) < 0.005)
                    .help("Center icon inside circle (0%, 0%)")
                }
            }

            LabeledContent("Elevation Shadow (\(Int((model.style.shadowIntensity * 100).rounded()))%)") {
                Slider(value: $model.style.shadowIntensity, in: 0.0...1.0)
                    .frame(width: 240)
            }

            Divider()

            HStack(spacing: Space.xl) {
                Toggle("Auto-Trim Margins", isOn: $model.style.autoTrim)
                Toggle("Inner Glyph Shadow", isOn: $model.style.innerShadow)
            }
            HStack(spacing: Space.xl) {
                Toggle("Full-Bleed Square Mask", isOn: $model.style.fullBleedSquares)
                Toggle("Material Monochrome Tint", isOn: $model.style.monochromeTint)
            }
        }
        .padding(Space.lg)
        .background(.background.secondary, in: shape)
        .overlay(shape.strokeBorder(.separator, lineWidth: 1))
    }

    private var actionFooter: some View {
        HStack(spacing: Space.md) {
            Button {
                model.applyRoundIcon(to: item)
            } label: {
                Label(item.isApplied ? "Update Round Icon" : "Apply Round Icon",
                      systemImage: "checkmark.circle.fill")
            }
            .buttonStyle(.borderedProminent)
            .keyboardShortcut(.return, modifiers: .command)

            Button {
                model.restoreOriginalIcon(for: item)
            } label: {
                Label("Restore Original", systemImage: "arrow.uturn.backward")
            }
            .disabled(!item.isApplied)

            Menu {
                Button("Standard Icon Pack (.icns, .ico, 1024px .png)") {
                    exportPack(for: item)
                }
                Button("Light, Dark & Tinted Kit (macOS 15 / iOS 18)") {
                    exportAppearanceVariants(for: item)
                }
                Button("All Resolutions (16 to 1024 px PNG)") {
                    exportCustomSizes(for: item, sizes: [16, 32, 48, 64, 128, 256, 512, 1024], title: "All Resolutions")
                }
                Button("Web & Favicon Kit (16, 32, 180, 192, 512 px)") {
                    exportCustomSizes(for: item, sizes: [16, 32, 180, 192, 512], title: "Favicon Kit")
                }
            } label: {
                Label("Export…", systemImage: "square.and.arrow.up")
            }

            Spacer()

            Text(model.statusMessage)
                .font(.caption)
                .foregroundStyle(.secondary)
                .lineLimit(1)
        }
    }

    private func setScale(_ value: Double) {
        if adjustThisAppOnly {
            item.customScale = value
            try? context.save()
            model.invalidateCache(for: item)
        } else {
            item.customScale = nil
            try? context.save()
            model.style.iconScale = value
        }
    }

    private func exportPack(for item: Item) {
        let panel = NSOpenPanel()
        panel.title = "Export Icon Pack for \(item.title)"
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = true
        panel.prompt = "Export"
        if panel.runModal() == .OK, let dir = panel.url {
            _ = model.exportIconPack(for: [item], to: dir)
        }
    }

    private func exportAppearanceVariants(for item: Item) {
        let panel = NSOpenPanel()
        panel.title = "Export Light, Dark & Tinted PNGs for \(item.title)"
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = true
        panel.prompt = "Export"
        if panel.runModal() == .OK, let dir = panel.url {
            _ = model.exportAppearanceVariants(for: [item], to: dir)
        }
    }

    private func exportCustomSizes(for item: Item, sizes: [Int], title: String) {
        let panel = NSOpenPanel()
        panel.title = "Export \(title) for \(item.title)"
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = true
        panel.prompt = "Export"
        if panel.runModal() == .OK, let dir = panel.url {
            _ = model.exportCustomSizes(for: [item], sizes: sizes, to: dir)
        }
    }
}
