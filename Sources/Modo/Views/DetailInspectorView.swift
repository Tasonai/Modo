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
    @State private var showAIGlyphStudio: Bool = false
    @State private var vectorVariants: [VectorVariant] = []
    @State private var isLoadingCDNVariants: Bool = false

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

                vectorVariantsCard
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .gridProbe("vector_variants", area: "detail")

                if item.isFolder {
                    folderCustomizationCard
                        .frame(maxWidth: .infinity, alignment: .leading)
                }

                customArtworkCard
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .gridProbe("custom_artwork", area: "detail")

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
            refreshVectorVariants()
        }
        .onChange(of: item.id) {
            adjustThisAppOnly = (item.customScale != nil)
            refreshVectorVariants()
        }
        .onChange(of: item.title) {
            refreshVectorVariants()
        }
        .sheet(isPresented: $showSimpleIconsSheet) {
            GlyphLibraryView(targetItem: item, allItems: allVisibleItems)
                .environment(model)
        }
        .sheet(isPresented: $showAIGlyphStudio) {
            AIGlyphStudioView(item: item)
        }
    }

    private var headerSection: some View {
        HStack(alignment: .center, spacing: Space.md) {
            VStack(alignment: .leading, spacing: Space.xs) {
                HStack(spacing: Space.sm) {
                    Text(item.title)
                        .font(.title2.weight(.semibold))
                    if item.isApplied {
                        let isSystem = item.bundlePath.hasPrefix("/System/")
                        Label(isSystem ? "Alias Active (SIP)" : "Modo Icon Active", systemImage: isSystem ? "link.circle.fill" : "checkmark.seal.fill")
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
                Image(nsImage: item.isFolder ? model.folderIcon(for: item) : model.roundIcon(for: item))
                    .resizable()
                    .interpolation(.high)
                    .parallaxTilt(maxTilt: 16.0, enableGlare: true)
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
                Text(item.isFolder ? "macOS Tinted Folder" : model.activePackName)
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

    // MARK: - Vector Variants Card & Selection
    private var vectorVariantsCard: some View {
        let cardShape = RoundedRectangle(cornerRadius: Radius.md, style: .continuous)

        return VStack(alignment: .leading, spacing: Space.sm) {
            HStack(alignment: .center) {
                VStack(alignment: .leading, spacing: 2) {
                    HStack(spacing: Space.xs) {
                        Image(systemName: "square.stack.3d.up.fill")
                            .foregroundStyle(Palette.accentText)
                        Text("Векторные варианты оформления")
                            .font(.headline)
                    }
                    Text("Автоматический подбор и генерация векторных стилей для \(item.title)")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }

                Spacer()

                if isLoadingCDNVariants {
                    HStack(spacing: Space.sm) {
                        ProgressView()
                            .controlSize(.small)
                        Text("Поиск в CDN…")
                            .font(.caption2)
                            .foregroundStyle(.secondary)
                    }
                    .padding(.trailing, 4)
                }

                Button {
                    showSimpleIconsSheet = true
                } label: {
                    Label("Все глифы…", systemImage: "magnifyingglass")
                }
                .controlSize(.small)
            }

            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: Space.sm) {
                    ForEach(vectorVariants) { variant in
                        variantTile(variant)
                    }
                }
                .padding(.vertical, 4)
            }
        }
        .padding(Space.md)
        .background(.background.secondary, in: cardShape)
        .overlay(cardShape.strokeBorder(Color(nsColor: .separatorColor), lineWidth: 1))
    }

    private func variantTile(_ variant: VectorVariant) -> some View {
        let isSelected = variant.isActive(
            customImageData: item.customImageData,
            currentSymbolName: item.symbolName,
            currentBrandHex: item.brandHex
        )
        let tileShape = RoundedRectangle(cornerRadius: Radius.md, style: .continuous)

        return Button {
            applyVectorVariant(variant)
        } label: {
            VStack(spacing: Space.sm) {
                HStack {
                    Text(variant.badge)
                        .font(.system(size: 8, weight: .bold))
                        .padding(.horizontal, Space.xs)
                        .padding(.vertical, 2)
                        .background(
                            isSelected ? Palette.accent.opacity(0.18) : Color.primary.opacity(0.06),
                            in: Capsule()
                        )
                        .foregroundStyle(isSelected ? Palette.accentText : .secondary)

                    Spacer(minLength: 0)

                    if isSelected {
                        Image(systemName: "checkmark.circle.fill")
                            .font(.system(size: 12, weight: .bold))
                            .foregroundStyle(Palette.accentText)
                    }
                }

                // Center Icon Preview
                ZStack {
                    Circle()
                        .fill(Color.primary.opacity(0.04))
                        .frame(width: 48, height: 48)

                    variantPreviewImage(variant)
                        .frame(width: 36, height: 36)
                }
                .frame(height: 48)

                VStack(spacing: 1) {
                    Text(variant.title)
                        .font(.caption.weight(.semibold))
                        .lineLimit(1)
                        .foregroundStyle(.primary)

                    Text(variant.subtitle)
                        .font(.system(size: 10))
                        .foregroundStyle(.secondary)
                        .lineLimit(1)
                }

                if isSelected {
                    Text("Выбрано")
                        .font(.system(size: 9, weight: .bold))
                        .foregroundStyle(Palette.accentText)
                        .padding(.horizontal, Space.sm)
                        .padding(.vertical, 1)
                        .background(Palette.accent.opacity(0.12), in: Capsule())
                } else {
                    Text("Применить")
                        .font(.system(size: 9, weight: .medium))
                        .foregroundStyle(.secondary)
                        .padding(.horizontal, Space.sm)
                        .padding(.vertical, 1)
                }
            }
            .padding(Space.sm)
            .frame(width: 124, height: 140)
            .background(isSelected ? Palette.accent.opacity(0.08) : Color(nsColor: .controlBackgroundColor).opacity(0.6), in: tileShape)
            .overlay(
                tileShape.strokeBorder(
                    isSelected ? Palette.accent : Color(nsColor: .separatorColor).opacity(0.7),
                    lineWidth: isSelected ? 2 : 1
                )
            )
        }
        .buttonStyle(.plain)
    }

    @ViewBuilder
    private func variantPreviewImage(_ variant: VectorVariant) -> some View {
        switch variant.kind {
        case .original:
            Image(nsImage: model.originalIcon(for: item))
                .resizable()
                .scaledToFit()

        case .brandMono:
            if let data = variant.applicationData, let img = NSImage(data: data) {
                Image(nsImage: img)
                    .resizable()
                    .renderingMode(.template)
                    .foregroundStyle(.white)
                    .padding(Space.xs)
                    .background(Color.black.opacity(0.85), in: Circle())
            } else {
                Image(systemName: "sparkles")
                    .font(.title3)
            }

        case .brandColor:
            let hex = UInt32(variant.brandHex ?? 0x1A73E8)
            let bColor = Color(hex: hex)
            if let data = variant.applicationData, let img = NSImage(data: data) {
                Image(nsImage: img)
                    .resizable()
                    .scaledToFit()
                    .padding(Space.xs)
                    .background(bColor.opacity(0.16), in: Circle())
            } else {
                Image(systemName: "app.fill")
                    .font(.title3)
                    .foregroundStyle(bColor)
            }

        case .sfSymbol:
            if let sym = variant.symbolName {
                Image(systemName: sym)
                    .font(.system(size: 20, weight: .medium))
                    .foregroundStyle(.primary)
            } else if let data = variant.applicationData, let img = NSImage(data: data) {
                Image(nsImage: img)
                    .resizable()
                    .scaledToFit()
            }

        case .monogram, .geometric:
            if let data = variant.applicationData, let img = NSImage(data: data) {
                Image(nsImage: img)
                    .resizable()
                    .scaledToFit()
                    .padding(2)
            } else {
                Image(systemName: "sparkle")
                    .font(.title3)
            }

        case .remoteCDN, .lawnicons, .whiteSur:
            if let data = variant.applicationData, let img = NSImage(data: data) {
                Image(nsImage: img)
                    .resizable()
                    .scaledToFit()
                    .padding(Space.xs)
                    .background(Palette.accent.opacity(0.12), in: Circle())
            } else {
                Image(systemName: "arrow.down.circle")
                    .font(.title3)
            }
        }
    }

    private func applyVectorVariant(_ variant: VectorVariant) {
        NSHapticFeedbackManager.defaultPerformer.perform(.alignment, performanceTime: .default)

        switch variant.kind {
        case .original:
            model.clearCustomArtwork(for: item)
        case .brandMono, .brandColor, .remoteCDN, .lawnicons, .whiteSur:
            if let data = variant.applicationData {
                model.setCustomArtwork(data: data, for: item)
            }
            if let brandHex = variant.brandHex {
                item.brandHex = brandHex
            }
        case .sfSymbol:
            if let data = variant.applicationData {
                model.setCustomArtwork(data: data, for: item)
            }
            if let sym = variant.symbolName {
                item.symbolName = sym
            }
        case .monogram, .geometric:
            if let data = variant.applicationData {
                model.setCustomArtwork(data: data, for: item)
            }
            if let brandHex = variant.brandHex {
                item.brandHex = brandHex
            }
        }

        model.invalidateCache(for: item)
        try? context.save()
    }

    private func refreshVectorVariants() {
        let local = VectorVariantEngine.resolveLocalVariants(
            title: item.title,
            bundleIdentifier: item.bundleIdentifier,
            categoryName: item.categoryName,
            brandHex: item.brandHex
        )
        vectorVariants = local

        guard !item.title.isEmpty else { return }

        isLoadingCDNVariants = true
        let title = item.title
        Task {
            let remote = await VectorVariantEngine.fetchRemoteBrandVariants(for: title)
            await MainActor.run {
                if !remote.isEmpty && self.item.title == title {
                    var updated = self.vectorVariants
                    let existingIDs = Set(updated.map(\.id))
                    let newVariants = remote.filter { !existingIDs.contains($0.id) }
                    let insertIndex = min(1, updated.count)
                    updated.insert(contentsOf: newVariants, at: insertIndex)
                    self.vectorVariants = updated
                }
                self.isLoadingCDNVariants = false
            }
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

            Text("Drop an SVG vector or image directly onto this card, or choose a file:")
                .font(.caption)
                .foregroundStyle(.secondary)

            HStack(spacing: Space.sm) {
                Button {
                    showAIGlyphStudio = true
                } label: {
                    Label("AI Studio…", systemImage: "wand.and.stars")
                }
                .controlSize(.small)
                .buttonStyle(.borderedProminent)

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

    private var folderCustomizationCard: some View {
        let cardShape = RoundedRectangle(cornerRadius: Radius.md, style: .continuous)
        return VStack(alignment: .leading, spacing: Space.md) {
            Label("macOS Folder Tint & Styling", systemImage: "folder.fill.badge.gearshape")
                .font(.headline)

            Text("Pick a folder color and combine it with your chosen badge to style this folder in Finder.")
                .font(.caption)
                .foregroundStyle(.secondary)

            // Preset Colors Grid
            VStack(alignment: .leading, spacing: 8) {
                Text("Folder Color Preset")
                    .font(.caption.bold())
                    .foregroundStyle(.secondary)

                LazyVGrid(columns: Array(repeating: GridItem(.flexible()), count: 5), spacing: Space.md) {
                    ForEach(FolderColorPreset.allCases) { preset in
                        Button {
                            item.brandHex = preset.hex
                            model.invalidateCache(for: item)
                            try? context.save()
                        } label: {
                            VStack(spacing: 4) {
                                ZStack {
                                    Circle()
                                        .fill(Color(hex: UInt32(preset.hex)))
                                        .frame(width: 24, height: 24)
                                        .shadow(color: Color(hex: UInt32(preset.hex)).opacity(0.3), radius: 2)

                                    if item.brandHex == preset.hex {
                                        Image(systemName: "checkmark")
                                            .font(.system(size: 11, weight: .bold))
                                            .foregroundStyle(.white)
                                    }
                                }
                                Text(preset.rawValue)
                                    .font(.system(size: 9))
                                    .lineLimit(1)
                                    .foregroundStyle(item.brandHex == preset.hex ? Palette.accent : .secondary)
                            }
                            .frame(maxWidth: .infinity)
                        }
                        .buttonStyle(.plain)
                    }
                }
            }

            HStack(spacing: 12) {
                Button {
                    model.applyRoundIcon(to: item)
                } label: {
                    Label("Apply to Folder in Finder", systemImage: "folder.badge.checkmark")
                }
                .buttonStyle(.borderedProminent)

                if item.isApplied {
                    Button("Restore Original") {
                        model.restoreOriginalIcon(for: item)
                    }
                    .buttonStyle(.bordered)
                }

                if !item.bundlePath.isEmpty {
                    Button("Reveal in Finder") {
                        NSWorkspace.shared.selectFile(nil, inFileViewerRootedAtPath: item.bundlePath)
                    }
                    .buttonStyle(.bordered)
                }
            }
        }
        .padding(Space.md)
        .background(.background.secondary, in: cardShape)
        .overlay(cardShape.strokeBorder(Color(nsColor: .separatorColor), lineWidth: 1))
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

            LabeledContent("Surface Finish & Texture") {
                Picker("Surface Material", selection: $model.style.surfaceMaterial) {
                    ForEach(SurfaceMaterial.allCases) { mat in
                        Label(mat.title, systemImage: mat.symbol).tag(mat)
                    }
                }
                .labelsHidden()
                .frame(width: 240)
            }

            LabeledContent("Glyph Artwork Style") {
                Picker("Glyph Style", selection: $model.style.glyphStyle) {
                    ForEach(GlyphStyleMode.allCases) { mode in
                        Label(mode.title, systemImage: mode.symbol).tag(mode)
                    }
                }
                .labelsHidden()
                .frame(width: 240)
            }

            let brandPalette = IconRenderer.extractBrandPalette(from: model.sourceIcon(for: item, canvasSize: 128), fallbackHex: UInt32(clamping: item.brandHex))

            LabeledContent("Brand Color Harmony") {
                HStack(spacing: Space.sm) {
                    HStack(spacing: Space.xs) {
                        ForEach([
                            ("Primary", brandPalette.primary),
                            ("Secondary", brandPalette.secondary),
                            ("Pastel Tint", brandPalette.backgroundTint),
                            ("Deep Ink", brandPalette.deepTone),
                            ("Highlight", brandPalette.highlight)
                        ], id: \.0) { name, hex in
                            Button {
                                model.style.customHex = hex
                                model.style.backgroundStyle = .customColor
                            } label: {
                                Circle()
                                    .fill(Color(
                                        red: Double((hex >> 16) & 0xFF) / 255.0,
                                        green: Double((hex >> 8) & 0xFF) / 255.0,
                                        blue: Double(hex & 0xFF) / 255.0
                                    ))
                                    .frame(width: 20, height: 20)
                                    .overlay(
                                        Circle()
                                            .strokeBorder(.primary.opacity(model.style.customHex == hex ? 0.85 : 0.20), lineWidth: model.style.customHex == hex ? 2 : 1)
                                    )
                            }
                            .buttonStyle(.plain)
                            .help("\(name): \(MaterialRGB(hex: hex).hexString)")
                        }
                    }

                    Button {
                        model.style.customHex = brandPalette.backgroundTint
                        model.style.backgroundStyle = .customColor
                        model.style.glyphStyle = .duotone
                    } label: {
                        Label("Apply Duotone", systemImage: "sparkles")
                    }
                    .controlSize(.small)
                    .help("Set pastel tint substrate and duotone glyph style")
                }
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

                    Button {
                        let pal = WallpaperPaletteEngine.currentWallpaperPalette()
                        model.style.customHex = pal.pastelHex
                        model.style.backgroundStyle = .wallpaperDynamic
                        model.statusMessage = "Matched Desktop Wallpaper palette"
                    } label: {
                        Label("Match Wallpaper", systemImage: "sparkles.rectangle.stack")
                    }
                    .controlSize(.small)
                    .help("Extract harmonic Material You palette from macOS desktop wallpaper")

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
                VStack(alignment: .leading, spacing: Space.sm) {
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

            if item.bundlePath.hasPrefix("/System/") {
                let aliasPath = (NSHomeDirectory() as NSString).appendingPathComponent("Applications/Round Icons/\((item.bundlePath as NSString).lastPathComponent)")
                if FileManager.default.fileExists(atPath: aliasPath) {
                    Button {
                        NSWorkspace.shared.selectFile(aliasPath, inFileViewerRootedAtPath: "")
                    } label: {
                        Label("Show Styled Alias", systemImage: "arrow.up.forward.app")
                    }
                    .help("Reveal styled alias in ~/Applications/Round Icons")
                }
            }

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
                Divider()
                Button("Xcode Asset Catalog (AppIcon.appiconset)…") {
                    exportXcodeAppIconSet(for: item)
                }
                Button("Web Favicon Kit (.ico, manifest, HTML)…") {
                    exportWebFaviconKit(for: item)
                }
                Button("Scalable Vector Graphic (.svg)…") {
                    exportSVG(for: item)
                }
                Divider()
                Button("App Showcase & README Hero Banner…") {
                    model.showShowcaseStudio = true
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

    private func exportXcodeAppIconSet(for item: Item) {
        let panel = NSOpenPanel()
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = true
        panel.prompt = "Export AppIcon.appiconset"
        panel.message = "Choose destination folder for \(item.title).appiconset"
        guard panel.runModal() == .OK, let dir = panel.url else { return }

        let icon = model.roundIcon(for: item)
        do {
            let outURL = try DeveloperExportEngine.exportAppIconSet(from: icon, appName: item.title, destination: dir)
            model.statusMessage = "Exported Xcode Asset Catalog to \(outURL.lastPathComponent)"
            NSWorkspace.shared.activateFileViewerSelecting([outURL])
        } catch {
            model.statusMessage = "Export failed: \(error.localizedDescription)"
        }
    }

    private func exportWebFaviconKit(for item: Item) {
        let panel = NSOpenPanel()
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = true
        panel.prompt = "Export Favicon Kit"
        panel.message = "Choose destination folder for \(item.title) Web Favicon Bundle"
        guard panel.runModal() == .OK, let dir = panel.url else { return }

        let icon = model.roundIcon(for: item)
        do {
            let outURL = try DeveloperExportEngine.exportWebFaviconKit(from: icon, appName: item.title, destination: dir)
            model.statusMessage = "Exported Web Favicon Kit to \(outURL.lastPathComponent)"
            NSWorkspace.shared.activateFileViewerSelecting([outURL])
        } catch {
            model.statusMessage = "Export failed: \(error.localizedDescription)"
        }
    }

    private func exportSVG(for item: Item) {
        let panel = NSSavePanel()
        panel.canCreateDirectories = true
        panel.allowedContentTypes = [.svg]
        panel.nameFieldStringValue = "\(item.title).svg"
        guard panel.runModal() == .OK, let url = panel.url else { return }

        let style = model.effectiveStyle(for: item)
        var glyphSVG: String? = nil
        var base64PNG: String? = nil

        if let data = item.customImageData {
            if CustomArtworkHelper.isSVGData(data), let str = String(data: data, encoding: .utf8) {
                glyphSVG = str
            } else {
                base64PNG = data.base64EncodedString()
            }
        } else {
            let src = model.sourceIcon(for: item, canvasSize: 512)
            if let pngData = IconRenderer.pngData(from: src, pixelSize: 512) {
                base64PNG = pngData.base64EncodedString()
            }
        }

        let svgContent = SVGExportEngine.generateSVG(
            title: item.title,
            style: style,
            dominantHex: UInt32(clamping: item.brandHex),
            glyphSVGPath: glyphSVG,
            rasterBase64PNG: base64PNG,
            canvasSize: 512
        )

        try? svgContent.write(to: url, atomically: true, encoding: String.Encoding.utf8)
        model.statusMessage = "Exported SVG to \(url.lastPathComponent)"
        NSWorkspace.shared.activateFileViewerSelecting([url])
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
