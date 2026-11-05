import AppKit
import ModoKit
import SwiftUI

/// Modal sheet for batch converting a folder of images/SVGs into styled icons and packs.
struct BatchCreatorSheetView: View {
    @Environment(\.dismiss) private var dismiss
    @Environment(AppModel.self) private var model

    @State private var sourceFolderURL: URL?
    @State private var candidates: [BatchCandidate] = []
    @State private var packName: String = "My Icon Pack"
    @State private var authorName: String = "Atoma"
    @State private var exportModoPack: Bool = true
    @State private var exportICNS: Bool = true
    @State private var exportICO: Bool = true
    @State private var exportPNG: Bool = true
    @State private var exportZip: Bool = true
    @State private var isProcessing: Bool = false
    @State private var progressIndex: Int = 0
    @State private var currentItemTitle: String = ""
    @State private var completionMessage: String?
    @State private var outputDirectoryURL: URL?
    @State private var isDropTargeted: Bool = false

    var body: some View {
        VStack(spacing: 0) {
            header
                .padding(.horizontal, 24)
                .padding(.top, 20)
                .padding(.bottom, 16)

            Divider()

            ScrollView {
                VStack(spacing: 20) {
                    folderDropZone
                    if !candidates.isEmpty {
                        configurationSection
                        candidatesPreviewGrid
                    }
                }
                .padding(24)
            }

            Divider()

            footerActions
                .padding(.horizontal, 24)
                .padding(.vertical, 16)
                .background(Color(nsColor: .windowBackgroundColor))
        }
        .frame(width: 620, height: 600)
        .background(Color(nsColor: .controlBackgroundColor))
        .onAppear {
            if let initial = model.batchInitialFolderURL {
                loadFolder(initial)
                model.batchInitialFolderURL = nil
            }
        }
    }

    // MARK: - Header

    private var header: some View {
        HStack(spacing: 16) {
            ZStack {
                RoundedRectangle(cornerRadius: 12, style: .continuous)
                    .fill(
                        LinearGradient(
                            colors: [Palette.accent, Palette.accent.opacity(0.7)],
                            startPoint: .topLeading,
                            endPoint: .bottomTrailing
                        )
                    )
                    .frame(width: 48, height: 48)
                    .shadow(color: Palette.accent.opacity(0.3), radius: 6, x: 0, y: 3)

                Image(systemName: "square.stack.3d.up.fill")
                    .font(.system(size: 22, weight: .semibold))
                    .foregroundStyle(.white)
            }

            VStack(alignment: .leading, spacing: 3) {
                Text("Batch Iconset & Pack Creator")
                    .font(.headline)
                Text("Convert an entire folder of logos & images into styled icon packs in one click.")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            Spacer()
        }
    }

    // MARK: - Drop Zone

    private var folderDropZone: some View {
        VStack(spacing: 12) {
            Image(systemName: sourceFolderURL == nil ? "folder.badge.plus" : "folder.fill")
                .font(.system(size: 38))
                .foregroundStyle(isDropTargeted ? Palette.accent : (sourceFolderURL == nil ? .secondary : Palette.accent))

            if let url = sourceFolderURL {
                VStack(spacing: 4) {
                    Text(url.lastPathComponent)
                        .font(.headline)
                    Text("\(candidates.count) image assets discovered in folder")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            } else {
                VStack(spacing: 4) {
                    Text("Drag & Drop a Folder Here")
                        .font(.headline)
                    Text("or click to choose a folder containing PNG, SVG, or WebP images")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            }

            Button(sourceFolderURL == nil ? "Choose Folder…" : "Change Folder…") {
                chooseFolder()
            }
            .buttonStyle(.bordered)
            .controlSize(.small)
        }
        .frame(maxWidth: .infinity)
        .padding(.vertical, 24)
        .background(
            RoundedRectangle(cornerRadius: 12, style: .continuous)
                .fill(Color(nsColor: .controlBackgroundColor))
                .overlay(
                    RoundedRectangle(cornerRadius: 12, style: .continuous)
                        .strokeBorder(
                            isDropTargeted ? Palette.accent : Color(nsColor: .separatorColor),
                            style: StrokeStyle(lineWidth: 1.5, dash: [6, 4])
                        )
                )
        )
        .onDrop(of: [.fileURL], isTargeted: $isDropTargeted) { providers in
            guard let provider = providers.first else { return false }
            _ = provider.loadObject(ofClass: URL.self) { url, _ in
                guard let url = url else { return }
                var isDir: ObjCBool = false
                if FileManager.default.fileExists(atPath: url.path, isDirectory: &isDir), isDir.boolValue {
                    Task { @MainActor in
                        self.loadFolder(url)
                    }
                }
            }
            return true
        }
    }

    // MARK: - Config Section

    private var configurationSection: some View {
        VStack(alignment: .leading, spacing: 14) {
            Text("Pack Configuration")
                .font(.subheadline.bold())

            HStack(spacing: 16) {
                VStack(alignment: .leading, spacing: 4) {
                    Text("Pack Name")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                    TextField("Pack Name", text: $packName)
                        .textFieldStyle(.roundedBorder)
                }

                VStack(alignment: .leading, spacing: 4) {
                    Text("Author")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                    TextField("Author", text: $authorName)
                        .textFieldStyle(.roundedBorder)
                }
            }

            Text("Export Targets")
                .font(.caption.bold())
                .foregroundStyle(.secondary)
                .padding(.top, 4)

            LazyVGrid(columns: [GridItem(.flexible()), GridItem(.flexible())], spacing: 8) {
                Toggle("Modo Icon Pack (.modopack)", isOn: $exportModoPack)
                Toggle("Apple Icons (.icns)", isOn: $exportICNS)
                Toggle("Windows Icons (.ico)", isOn: $exportICO)
                Toggle("Master PNGs (1024 px)", isOn: $exportPNG)
                Toggle("Compressed Archive (.zip)", isOn: $exportZip)
            }
            .font(.caption)
        }
        .padding(16)
        .background(Color(nsColor: .windowBackgroundColor), in: RoundedRectangle(cornerRadius: 10))
    }

    // MARK: - Preview Grid

    private var candidatesPreviewGrid: some View {
        VStack(alignment: .leading, spacing: 10) {
            HStack {
                Text("Discovered Logos & Assets (\(candidates.count))")
                    .font(.subheadline.bold())
                Spacer()
                Text("Active Style: \(model.activePackName)")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: 14) {
                    ForEach(candidates.prefix(16)) { candidate in
                        VStack(spacing: 6) {
                            if let img = NSImage(contentsOf: candidate.fileURL) {
                                Image(nsImage: IconRenderer.renderRoundIcon(
                                    sourceImage: img,
                                    style: model.style,
                                    fallbackHex: 0x1A73E8,
                                    canvasSize: 64
                                ))
                                .resizable()
                                .frame(width: 52, height: 52)
                                .shadow(color: .black.opacity(0.15), radius: 3, x: 0, y: 1.5)
                            } else {
                                RoundedRectangle(cornerRadius: 10)
                                    .fill(Color.secondary.opacity(0.2))
                                    .frame(width: 52, height: 52)
                            }

                            Text(candidate.cleanTitle)
                                .font(.system(size: 10, weight: .medium))
                                .lineLimit(1)
                                .frame(width: 64)
                        }
                    }
                }
                .padding(.vertical, 8)
                .padding(.horizontal, 4)
            }
        }
    }

    // MARK: - Footer Actions

    private var footerActions: some View {
        HStack {
            if isProcessing {
                HStack(spacing: 10) {
                    ProgressView(value: Double(progressIndex), total: Double(max(1, candidates.count)))
                        .frame(width: 140)
                    Text("\(progressIndex)/\(candidates.count): \(currentItemTitle)")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                        .lineLimit(1)
                }
            } else if let msg = completionMessage {
                HStack(spacing: 6) {
                    Image(systemName: "checkmark.circle.fill").foregroundStyle(.green)
                    Text(msg)
                        .font(.caption.weight(.medium))
                        .foregroundStyle(.green)
                }
                if let out = outputDirectoryURL {
                    Button("Show in Finder") {
                        NSWorkspace.shared.selectFile(nil, inFileViewerRootedAtPath: out.path)
                    }
                    .controlSize(.small)
                }
            }

            Spacer()

            Button("Cancel") {
                dismiss()
            }
            .disabled(isProcessing)

            Button("Start Batch Conversion (\(candidates.count))") {
                startBatchProcess()
            }
            .buttonStyle(.borderedProminent)
            .disabled(candidates.isEmpty || isProcessing)
            .keyboardShortcut(.defaultAction)
        }
    }

    // MARK: - Helper Methods

    private func chooseFolder() {
        let panel = NSOpenPanel()
        panel.title = "Select Folder with Images"
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.allowsMultipleSelection = false
        if panel.runModal() == .OK, let url = panel.url {
            loadFolder(url)
        }
    }

    private func loadFolder(_ url: URL) {
        sourceFolderURL = url
        candidates = BatchIconEngine.scanDirectory(at: url)
        packName = url.lastPathComponent
            .replacingOccurrences(of: "_", with: " ")
            .replacingOccurrences(of: "-", with: " ")
            .capitalized
    }

    private func startBatchProcess() {
        guard let source = sourceFolderURL, !candidates.isEmpty else { return }

        // Let user choose output directory
        let panel = NSOpenPanel()
        panel.title = "Choose Destination Folder for Converted Icons"
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = true
        panel.prompt = "Convert Here"
        guard panel.runModal() == .OK, let outputBase = panel.url else { return }

        isProcessing = true
        progressIndex = 0
        completionMessage = nil

        let safeName = BatchIconEngine.safePackName(packName)
        let targetDir = outputBase.appendingPathComponent("Modo-\(safeName)", isDirectory: true)
        try? FileManager.default.createDirectory(at: targetDir, withIntermediateDirectories: true)
        outputDirectoryURL = targetDir

        let options = BatchExportOptions(
            packName: packName,
            author: authorName,
            style: model.style,
            exportModoPack: exportModoPack,
            exportICNS: exportICNS,
            exportICO: exportICO,
            exportPNG: exportPNG,
            exportZip: exportZip,
            outputDirectory: targetDir
        )

        Task {
            var processed = 0
            for (idx, candidate) in candidates.enumerated() {
                await MainActor.run {
                    progressIndex = idx + 1
                    currentItemTitle = candidate.cleanTitle
                }

                guard let img = NSImage(contentsOf: candidate.fileURL) else { continue }

                // Export icon pack files (.png, .icns, .ico)
                _ = try? IconRenderer.exportIconPack(
                    for: candidate.cleanTitle,
                    sourceImage: img,
                    style: options.style,
                    fallbackHex: 0x1A73E8,
                    to: targetDir
                )
                processed += 1
            }

            // Export .modopack if requested
            if options.exportModoPack {
                let customPack = CustomIconPack(
                    name: options.packName,
                    style: options.style
                )
                let packFileURL = targetDir.appendingPathComponent("\(safeName).modopack")
                model.exportPacks([customPack], to: packFileURL)
            }

            // Create ZIP archive if requested
            if options.exportZip {
                let zipURL = outputBase.appendingPathComponent("Modo-\(safeName).zip")
                let ditto = Process()
                ditto.executableURL = URL(fileURLWithPath: "/usr/bin/ditto")
                ditto.arguments = ["-c", "-k", "--keepParent", targetDir.path, zipURL.path]
                try? ditto.run()
                ditto.waitUntilExit()
            }

            await MainActor.run {
                isProcessing = false
                completionMessage = "Converted \(processed) icons successfully!"
                model.statusMessage = "Batch converted \(processed) icons from \(source.lastPathComponent)"
            }
        }
    }
}
