import AppKit
import ModoKit
import SwiftData
import SwiftUI

/// AI Glyph Studio: Interactive creator for algorithmic geometric vectors and cloud neural AI icons.
struct AIGlyphStudioView: View {
    @Environment(AppModel.self) private var model
    @Environment(\.modelContext) private var context
    @Environment(\.dismiss) private var dismiss

    let item: Item

    enum StudioTab: String, CaseIterable, Identifiable {
        case geometric = "Geometric Vector (0ms Offline)"
        case neural = "Neural AI Generator"

        var id: String { rawValue }
    }

    @State private var activeTab: StudioTab = .geometric

    // Tab 1: Geometric Vector State
    @State private var selectedGeometricStyle: GeometricGlyphStyle = .sparkle
    @State private var primaryColor: Color = Color(hex: 0x3B82F6)
    @State private var secondaryColor: Color = Color(hex: 0x60A5FA)
    @State private var geometricSeed: Int = 1

    // Tab 2: Neural AI State
    @State private var neuralPrompt: String = ""
    @State private var neuralStyle: AIImageStylePreset = .vectorLogo
    @State private var isGeneratingNeural: Bool = false
    @State private var neuralResults: [Data] = []
    @State private var neuralError: String?

    var body: some View {
        VStack(spacing: 0) {
            headerBar
            Divider()

            Picker("Studio Mode", selection: $activeTab) {
                ForEach(StudioTab.allCases) { tab in
                    Text(tab.rawValue).tag(tab)
                }
            }
            .pickerStyle(.segmented)
            .padding(.horizontal, Space.md)
            .padding(.vertical, Space.sm)
            .background(.background.secondary)

            Divider()

            Group {
                switch activeTab {
                case .geometric:
                    geometricView
                case .neural:
                    neuralView
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
        .frame(minWidth: 640, idealWidth: 720, minHeight: 520, idealHeight: 600)
        .onAppear {
            if neuralPrompt.isEmpty {
                neuralPrompt = "minimalist modern \(item.title) logo emblem"
            }
            let brandRGB = MaterialRGB(hex: UInt32(clamping: item.brandHex))
            primaryColor = Color(
                red: Double(brandRGB.r) / 255.0,
                green: Double(brandRGB.g) / 255.0,
                blue: Double(brandRGB.b) / 255.0
            )
            let pastel = brandRGB.materialPastel(whiteMix: 0.35)
            secondaryColor = Color(
                red: Double(pastel.r) / 255.0,
                green: Double(pastel.g) / 255.0,
                blue: Double(pastel.b) / 255.0
            )
        }
    }

    // MARK: - Header

    private var headerBar: some View {
        HStack {
            VStack(alignment: .leading, spacing: 2) {
                HStack(spacing: Space.xs) {
                    Image(systemName: "wand.and.stars")
                        .foregroundStyle(Palette.accentText)
                    Text("AI Glyph Studio")
                        .font(.headline)
                }
                Text("Design procedural vector marks or neural emblems for \(item.title)")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
            Spacer()
            Button("Done") {
                dismiss()
            }
            .keyboardShortcut(.defaultAction)
        }
        .padding(Space.md)
    }

    // MARK: - Tab 1: Geometric Vector

    private var geometricView: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: Space.lg) {
                // Style Picker
                VStack(alignment: .leading, spacing: Space.xs) {
                    Text("Vector Geometry Style")
                        .font(.headline)

                    ScrollView(.horizontal, showsIndicators: false) {
                        HStack(spacing: Space.sm) {
                            ForEach(GeometricGlyphStyle.allCases) { style in
                                let isSelected = selectedGeometricStyle == style
                                Button {
                                    selectedGeometricStyle = style
                                } label: {
                                    HStack(spacing: 6) {
                                        Image(systemName: style.iconSymbol)
                                        Text(style.title)
                                    }
                                    .font(.callout)
                                    .padding(.horizontal, Space.sm)
                                    .padding(.vertical, Space.xs)
                                    .background(isSelected ? Palette.accent.opacity(0.15) : Color.clear, in: Capsule())
                                    .overlay(
                                        Capsule()
                                            .strokeBorder(isSelected ? Palette.accent : Color.secondary.opacity(0.3), lineWidth: isSelected ? 1.5 : 1)
                                    )
                                }
                                .buttonStyle(.plain)
                            }
                        }
                    }
                }

                // Color controls
                HStack(spacing: Space.lg) {
                    HStack(spacing: Space.xs) {
                        Text("Primary:")
                            .font(.caption)
                        ColorPicker("", selection: $primaryColor, supportsOpacity: false)
                            .labelsHidden()
                    }

                    HStack(spacing: Space.xs) {
                        Text("Secondary:")
                            .font(.caption)
                        ColorPicker("", selection: $secondaryColor, supportsOpacity: false)
                            .labelsHidden()
                    }

                    Spacer()

                    Button {
                        geometricSeed = Int.random(in: 1...9999)
                    } label: {
                        Label("Shuffle Seed", systemImage: "dice")
                    }
                    .controlSize(.small)
                }
                .padding(Space.sm)
                .background(.background.secondary, in: RoundedRectangle(cornerRadius: Radius.sm))

                // 4 Generated Variations
                VStack(alignment: .leading, spacing: Space.sm) {
                    Text("Instant Procedural Variations (SVG)")
                        .font(.headline)

                    LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())], spacing: Space.md) {
                        ForEach(0..<4, id: \.self) { index in
                            let variationSeed = geometricSeed + index * 17
                            let opts = ProceduralGlyphOptions(
                                title: item.title,
                                style: selectedGeometricStyle,
                                primaryHex: EyedropperPicker.hex(from: primaryColor),
                                secondaryHex: EyedropperPicker.hex(from: secondaryColor),
                                seed: variationSeed
                            )
                            let svg = ProceduralGlyphGenerator.generateSVG(options: opts)
                            geometricCard(svg: svg, index: index, primaryHex: opts.primaryHex)
                        }
                    }
                }
            }
            .padding(Space.md)
        }
    }

    private func geometricCard(svg: String, index: Int, primaryHex: UInt32) -> some View {
        let cardShape = RoundedRectangle(cornerRadius: Radius.md, style: .continuous)
        let data = svg.data(using: .utf8) ?? Data()

        return VStack(spacing: Space.sm) {
            if let img = NSImage(data: data) {
                Image(nsImage: img)
                    .resizable()
                    .interpolation(.high)
                    .frame(width: 80, height: 80)
                    .padding(Space.md)
                    .background(Color.black.opacity(0.04), in: Circle())
            }

            HStack {
                Text("Variant \(index + 1)")
                    .font(.caption.weight(.medium))
                Spacer()
                Button("Apply Vector") {
                    model.setCustomArtwork(data: data, for: item)
                    item.brandHex = Int(primaryHex)
                    try? context.save()
                    dismiss()
                }
                .controlSize(.small)
                .buttonStyle(.borderedProminent)
            }
        }
        .padding(Space.md)
        .background(.background.secondary, in: cardShape)
        .overlay(cardShape.strokeBorder(Color(nsColor: .separatorColor), lineWidth: 1))
    }

    // MARK: - Tab 2: Neural AI Generator

    private var neuralView: some View {
        VStack(spacing: 0) {
            VStack(alignment: .leading, spacing: Space.md) {
                // Prompt input
                VStack(alignment: .leading, spacing: 4) {
                    Text("Natural Language Prompt")
                        .font(.headline)
                    HStack(spacing: Space.sm) {
                        TextField("Describe your desired logo (e.g. glowing cyber rocket)…", text: $neuralPrompt)
                            .textFieldStyle(.roundedBorder)

                        Button {
                            generateNeural()
                        } label: {
                            Label(isGeneratingNeural ? "Generating…" : "Generate AI", systemImage: "sparkles")
                        }
                        .buttonStyle(.borderedProminent)
                        .disabled(neuralPrompt.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty || isGeneratingNeural)
                    }
                }

                // Prompt Inspiration Chips
                ScrollView(.horizontal, showsIndicators: false) {
                    HStack(spacing: Space.xs) {
                        inspirationChip("Sleek neon rocket launch")
                        inspirationChip("Minimalist camera prism")
                        inspirationChip("Cyberpunk glowing cat face")
                        inspirationChip("Futuristic crystal compiler")
                        inspirationChip("Vintage cassette audio tape")
                    }
                }

                // AI Style Preset
                HStack(spacing: Space.md) {
                    Text("Visual Style:")
                        .font(.caption.weight(.medium))
                    Picker("Style", selection: $neuralStyle) {
                        ForEach(AIImageStylePreset.allCases) { st in
                            Text(st.rawValue).tag(st)
                        }
                    }
                    .pickerStyle(.segmented)
                }

                if let err = neuralError {
                    Text(err)
                        .font(.caption)
                        .foregroundStyle(.red)
                }
            }
            .padding(Space.md)

            Divider()

            // Neural Results Area
            ScrollView {
                if isGeneratingNeural {
                    VStack(spacing: Space.md) {
                        ProgressView()
                            .controlSize(.large)
                        Text("Dreaming up icons via neural diffusion…")
                            .font(.callout)
                            .foregroundStyle(.secondary)
                    }
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                    .padding(.top, 60)
                } else if neuralResults.isEmpty {
                    VStack(spacing: Space.sm) {
                        Image(systemName: "sparkles.rectangle.stack")
                            .font(.system(size: 44))
                            .foregroundStyle(.secondary)
                        Text("No neural icons generated yet")
                            .font(.headline)
                        Text("Type a prompt above and click 'Generate AI' to produce custom neural artwork.")
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }
                    .frame(maxWidth: .infinity)
                    .padding(.top, 60)
                } else {
                    LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())], spacing: Space.md) {
                        ForEach(Array(neuralResults.enumerated()), id: \.offset) { index, imgData in
                            neuralResultCard(data: imgData, index: index)
                        }
                    }
                    .padding(Space.md)
                }
            }
        }
    }

    private func inspirationChip(_ text: String) -> some View {
        Button {
            neuralPrompt = text
        } label: {
            Text(text)
                .font(.caption2)
                .padding(.horizontal, Space.sm)
                .padding(.vertical, 4)
                .background(.background.secondary, in: Capsule())
                .overlay(Capsule().strokeBorder(Color.secondary.opacity(0.3), lineWidth: 1))
        }
        .buttonStyle(.plain)
    }

    private func neuralResultCard(data: Data, index: Int) -> some View {
        let cardShape = RoundedRectangle(cornerRadius: Radius.md, style: .continuous)

        return VStack(spacing: Space.sm) {
            if let img = NSImage(data: data) {
                Image(nsImage: img)
                    .resizable()
                    .interpolation(.high)
                    .scaledToFit()
                    .frame(width: 140, height: 140)
                    .clipShape(RoundedRectangle(cornerRadius: Radius.sm))
            }

            HStack {
                Text("Result \(index + 1)")
                    .font(.caption.weight(.medium))
                Spacer()
                Button("Apply to \(item.title)") {
                    model.setCustomArtwork(data: data, for: item)
                    try? context.save()
                    dismiss()
                }
                .controlSize(.small)
                .buttonStyle(.borderedProminent)
            }
        }
        .padding(Space.md)
        .background(.background.secondary, in: cardShape)
        .overlay(cardShape.strokeBorder(Color(nsColor: .separatorColor), lineWidth: 1))
    }

    private func generateNeural() {
        let prompt = neuralPrompt.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !prompt.isEmpty else { return }

        isGeneratingNeural = true
        neuralError = nil

        Task {
            do {
                // Generate 2 variations in parallel with distinct seeds
                async let v1 = AIGlyphService.generateNeuralImage(prompt: prompt, style: neuralStyle, seed: Int.random(in: 1000...499999))
                async let v2 = AIGlyphService.generateNeuralImage(prompt: prompt, style: neuralStyle, seed: Int.random(in: 500000...999999))

                let (r1, r2) = try await (v1, v2)
                await MainActor.run {
                    self.neuralResults = [r1, r2]
                    self.isGeneratingNeural = false
                }
            } catch {
                await MainActor.run {
                    self.neuralError = "Generation error: \(error.localizedDescription)"
                    self.isGeneratingNeural = false
                }
            }
        }
    }
}
