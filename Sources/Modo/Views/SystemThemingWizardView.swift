import AppKit
import ModoKit
import SwiftData
import SwiftUI

/// 1-Click System Theming Wizard: batch themes all installed macOS apps with automatic backup and rollback.
struct SystemThemingWizardView: View {
    @Environment(AppModel.self) private var model
    @Environment(\.modelContext) private var context
    @Environment(\.dismiss) private var dismiss

    let allItems: [Item]

    enum WizardStep: Int, CaseIterable {
        case selectApps = 1
        case selectTheme = 2
        case applyAndBackup = 3
        case completed = 4
    }

    @State private var currentStep: WizardStep = .selectApps
    @State private var selectedItemIDs: Set<UUID> = []
    @State private var autoMatchSimpleIcons: Bool = true
    @State private var restartDockAfterApply: Bool = true
    @State private var isProcessing: Bool = false
    @State private var currentProgress: Double = 0.0
    @State private var progressStatus: String = "Ready"
    @State private var backupPoint: SystemBackupPoint?
    @State private var appliedCount: Int = 0

    private var selectedItems: [Item] {
        allItems.filter { selectedItemIDs.contains($0.id) }
    }

    private var simpleIconsMatchCount: Int {
        selectedItems.filter { SimpleIconCatalog.find(for: $0.title) != nil }.count
    }

    var body: some View {
        VStack(spacing: 0) {
            headerBar
            Divider()

            Group {
                switch currentStep {
                case .selectApps:
                    stepSelectApps
                case .selectTheme:
                    stepSelectTheme
                case .applyAndBackup:
                    stepApplyAndBackup
                case .completed:
                    stepCompleted
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)

            Divider()
            navigationFooter
        }
        .frame(minWidth: 640, idealWidth: 700, minHeight: 520, idealHeight: 580)
        .onAppear {
            if selectedItemIDs.isEmpty {
                // Select all apps with valid bundles by default
                selectedItemIDs = Set(allItems.map(\.id))
            }
        }
    }

    // MARK: - Header Bar

    private var headerBar: some View {
        HStack {
            VStack(alignment: .leading, spacing: 2) {
                HStack(spacing: Space.xs) {
                    Image(systemName: "wand.and.stars")
                        .foregroundStyle(Palette.accentText)
                    Text("System Theming Wizard")
                        .font(.headline)
                }
                Text("Step \(currentStep.rawValue) of 4: \(stepTitle)")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
            Spacer()
            stepIndicators
        }
        .padding(Space.md)
        .background(.background.secondary)
    }

    private var stepTitle: String {
        switch currentStep {
        case .selectApps: return "Select Applications"
        case .selectTheme: return "Choose Theme & Appearance"
        case .applyAndBackup: return "Backup & Apply"
        case .completed: return "Done & Rollback Point"
        }
    }

    private var stepIndicators: some View {
        HStack(spacing: Space.xs) {
            ForEach(WizardStep.allCases, id: \.self) { step in
                Circle()
                    .fill(step.rawValue <= currentStep.rawValue ? Palette.accent : Color.secondary.opacity(0.3))
                    .frame(width: 8, height: 8)
            }
        }
    }

    // MARK: - Step 1: Select Apps

    private var stepSelectApps: some View {
        VStack(alignment: .leading, spacing: Space.md) {
            HStack(spacing: Space.md) {
                summaryStatCard(title: "Apps Selected", value: "\(selectedItemIDs.count) of \(allItems.count)", icon: "app.badge.checkmark")
                summaryStatCard(title: "Simple Icons Match", value: "\(simpleIconsMatchCount)", icon: "sparkles")
                summaryStatCard(title: "Backup Safety", value: "100% Reversible", icon: "shield.checkerboard")
            }

            HStack {
                Button("Select All") {
                    selectedItemIDs = Set(allItems.map(\.id))
                }
                .controlSize(.small)

                Button("Deselect All") {
                    selectedItemIDs.removeAll()
                }
                .controlSize(.small)

                Spacer()

                Toggle("Auto-match official Simple Icons vectors", isOn: $autoMatchSimpleIcons)
                    .font(.caption)
            }

            List(allItems) { item in
                HStack(spacing: Space.sm) {
                    Toggle("", isOn: Binding(
                        get: { selectedItemIDs.contains(item.id) },
                        set: { isChecked in
                            if isChecked { selectedItemIDs.insert(item.id) }
                            else { selectedItemIDs.remove(item.id) }
                        }
                    ))
                    .labelsHidden()

                    Image(nsImage: model.originalIcon(for: item))
                        .resizable()
                        .interpolation(.high)
                        .frame(width: 28, height: 28)

                    VStack(alignment: .leading, spacing: 1) {
                        Text(item.title)
                            .font(.body.weight(.medium))
                        Text(item.bundlePath.isEmpty ? item.bundleIdentifier : item.bundlePath)
                            .font(.caption2)
                            .foregroundStyle(.secondary)
                            .lineLimit(1)
                    }

                    Spacer()

                    if SimpleIconCatalog.find(for: item.title) != nil {
                        Text("Vector Available")
                            .font(.system(size: 9, weight: .semibold))
                            .foregroundStyle(Palette.accentText)
                            .padding(.horizontal, Space.xs)
                            .padding(.vertical, 2)
                            .background(Palette.accent.opacity(0.12), in: Capsule())
                    }
                }
                .padding(.vertical, 2)
            }
            .clipShape(RoundedRectangle(cornerRadius: Radius.sm))
        }
        .padding(Space.md)
    }

    private func summaryStatCard(title: String, value: String, icon: String) -> some View {
        let shape = RoundedRectangle(cornerRadius: Radius.md, style: .continuous)
        return HStack(spacing: Space.sm) {
            Image(systemName: icon)
                .font(.title2)
                .foregroundStyle(Palette.accentText)
            VStack(alignment: .leading, spacing: 1) {
                Text(value)
                    .font(.headline)
                Text(title)
                    .font(.caption2)
                    .foregroundStyle(.secondary)
            }
            Spacer()
        }
        .padding(Space.sm)
        .frame(maxWidth: .infinity)
        .background(.background.secondary, in: shape)
        .overlay(shape.strokeBorder(Color(nsColor: .separatorColor), lineWidth: 1))
    }

    // MARK: - Step 2: Select Theme

    private var stepSelectTheme: some View {
        @Bindable var model = model
        return ScrollView {
            VStack(alignment: .leading, spacing: Space.lg) {
                // Appearance mode
                VStack(alignment: .leading, spacing: Space.xs) {
                    Text("Appearance Theme (macOS Sequoia / iOS 18)")
                        .font(.headline)
                    Picker("Appearance", selection: $model.style.appearanceMode) {
                        Label("Light", systemImage: "sun.max.fill").tag(IconAppearanceMode.light)
                        Label("Dark", systemImage: "moon.stars.fill").tag(IconAppearanceMode.dark)
                        Label("Tinted", systemImage: "paintpalette.fill").tag(IconAppearanceMode.tinted)
                        Label("Auto", systemImage: "circle.lefthalf.filled").tag(IconAppearanceMode.system)
                    }
                    .pickerStyle(.segmented)
                    .frame(maxWidth: 360)
                }

                // Preset packs
                VStack(alignment: .leading, spacing: Space.xs) {
                    Text("Icon Pack Preset")
                        .font(.headline)
                    ScrollView(.horizontal, showsIndicators: false) {
                        HStack(spacing: Space.sm) {
                            ForEach(IconPack.allPacks) { pack in
                                let isSelected = model.activePackId == pack.id
                                Button {
                                    model.applyPack(pack)
                                } label: {
                                    VStack(alignment: .leading, spacing: 2) {
                                        Text(pack.name)
                                            .font(.callout.weight(.medium))
                                            .foregroundStyle(isSelected ? Palette.accentText : .primary)
                                        Text(pack.subtitle)
                                            .font(.caption2)
                                            .foregroundStyle(.secondary)
                                            .lineLimit(1)
                                    }
                                    .padding(.horizontal, Space.sm)
                                    .padding(.vertical, Space.xs)
                                    .frame(width: 140, alignment: .leading)
                                    .background(isSelected ? Palette.accent.opacity(0.15) : Color.clear, in: RoundedRectangle(cornerRadius: Radius.sm))
                                    .overlay(
                                        RoundedRectangle(cornerRadius: Radius.sm)
                                            .strokeBorder(isSelected ? Palette.accent : Color.secondary.opacity(0.3), lineWidth: isSelected ? 1.5 : 1)
                                    )
                                }
                                .buttonStyle(.plain)
                            }
                        }
                    }
                }

                // Shape
                VStack(alignment: .leading, spacing: Space.xs) {
                    Text("Mask Shape")
                        .font(.headline)
                    Picker("Shape", selection: $model.style.shape) {
                        ForEach(IconShape.allCases) { s in
                            Label(s.title, systemImage: s.symbol).tag(s)
                        }
                    }
                    .pickerStyle(.segmented)
                    .frame(maxWidth: 420)
                }

                // Surface Material Texture
                VStack(alignment: .leading, spacing: Space.xs) {
                    Text("Surface Material")
                        .font(.headline)
                    Picker("Material", selection: $model.style.surfaceMaterial) {
                        ForEach(SurfaceMaterial.allCases) { m in
                            Text(m.title).tag(m)
                        }
                    }
                    .pickerStyle(.segmented)
                    .frame(maxWidth: 480)
                }

                // Live Preview of selected apps
                VStack(alignment: .leading, spacing: Space.xs) {
                    Text("Live Preview (\(min(selectedItems.count, 6)) Apps)")
                        .font(.headline)
                    HStack(spacing: Space.md) {
                        ForEach(selectedItems.prefix(6)) { appItem in
                            VStack(spacing: 4) {
                                Image(nsImage: model.roundIcon(for: appItem))
                                    .resizable()
                                    .interpolation(.high)
                                    .frame(width: 56, height: 56)
                                Text(appItem.title)
                                    .font(.caption2)
                                    .lineLimit(1)
                                    .frame(width: 64)
                            }
                        }
                    }
                    .padding(Space.md)
                    .frame(maxWidth: .infinity, alignment: .center)
                    .background(.background.secondary, in: RoundedRectangle(cornerRadius: Radius.md))
                }
            }
            .padding(Space.md)
        }
    }

    // MARK: - Step 3: Apply & Backup Confirmation

    private var stepApplyAndBackup: some View {
        VStack(spacing: Space.xl) {
            Spacer()

            Image(systemName: isProcessing ? "gearshape.2.fill" : "shield.lefthalf.filled.badge.checkmark")
                .font(.system(size: 48))
                .foregroundStyle(Palette.accentText)
                .rotationEffect(.degrees(isProcessing ? 360 : 0))
                .animation(isProcessing ? .linear(duration: 2).repeatForever(autoreverses: false) : .default, value: isProcessing)

            VStack(spacing: Space.xs) {
                Text(isProcessing ? "Styling Applications…" : "Ready to Theme System")
                    .font(.title2.weight(.semibold))

                Text(progressStatus)
                    .font(.callout)
                    .foregroundStyle(.secondary)
                    .multilineTextAlignment(.center)
            }

            if isProcessing {
                ProgressView(value: currentProgress, total: 1.0)
                    .progressViewStyle(.linear)
                    .frame(width: 320)
            } else {
                VStack(alignment: .leading, spacing: Space.sm) {
                    HStack(spacing: Space.sm) {
                        Image(systemName: "checkmark.circle.fill")
                            .foregroundStyle(.green)
                        Text("Automatic snapshot restore point will be saved before changes.")
                            .font(.caption)
                    }
                    HStack(spacing: Space.sm) {
                        Image(systemName: "checkmark.circle.fill")
                            .foregroundStyle(.green)
                        Text("Selected theme: **\(model.activePackName)** (\(model.style.appearanceMode.rawValue) mode).")
                            .font(.caption)
                    }
                    if autoMatchSimpleIcons && simpleIconsMatchCount > 0 {
                        HStack(spacing: Space.sm) {
                            Image(systemName: "sparkles")
                                .foregroundStyle(Palette.accentText)
                            Text("\(simpleIconsMatchCount) apps will receive official Simple Icons SVG vectors.")
                                .font(.caption)
                        }
                    }
                    HStack(spacing: Space.sm) {
                        Toggle("Restart macOS Dock after theming to show icons immediately", isOn: $restartDockAfterApply)
                            .font(.caption)
                    }
                }
                .padding(Space.md)
                .background(.background.secondary, in: RoundedRectangle(cornerRadius: Radius.md))
            }

            Spacer()
        }
        .padding(Space.xl)
    }

    // MARK: - Step 4: Completed

    private var stepCompleted: some View {
        VStack(spacing: Space.lg) {
            Spacer()

            Image(systemName: "checkmark.seal.fill")
                .font(.system(size: 56))
                .foregroundStyle(.green)

            VStack(spacing: Space.xs) {
                Text("System Theming Complete!")
                    .font(.title.weight(.bold))

                Text("Successfully applied **\(model.activePackName)** style to \(appliedCount) applications.")
                    .font(.callout)
                    .foregroundStyle(.secondary)
            }

            if let backup = backupPoint {
                HStack(spacing: Space.sm) {
                    Image(systemName: "clock.arrow.circlepath")
                        .foregroundStyle(Palette.accentText)
                    VStack(alignment: .leading, spacing: 1) {
                        Text("Backup Point: **\(backup.name)**")
                            .font(.caption.weight(.medium))
                        Text("Saved at \(backup.date.formatted(date: .abbreviated, time: .shortened))")
                            .font(.caption2)
                            .foregroundStyle(.secondary)
                    }
                    Spacer()
                    Button("Restore All to Original Now", role: .destructive) {
                        restoreFromBackup(backup)
                    }
                    .controlSize(.small)
                }
                .padding(Space.md)
                .frame(maxWidth: 480)
                .background(.background.secondary, in: RoundedRectangle(cornerRadius: Radius.md))
            }

            Spacer()
        }
        .padding(Space.xl)
    }

    // MARK: - Navigation Footer

    private var navigationFooter: some View {
        HStack {
            if currentStep != .completed && currentStep != .applyAndBackup {
                Button("Cancel") {
                    dismiss()
                }
            }

            if currentStep == .selectTheme {
                Button("Back") {
                    currentStep = .selectApps
                }
            }

            Spacer()

            switch currentStep {
            case .selectApps:
                Button("Next: Choose Theme") {
                    currentStep = .selectTheme
                }
                .buttonStyle(.borderedProminent)
                .disabled(selectedItemIDs.isEmpty)

            case .selectTheme:
                Button("Next: Review & Apply (\(selectedItems.count) Apps)") {
                    currentStep = .applyAndBackup
                }
                .buttonStyle(.borderedProminent)

            case .applyAndBackup:
                if !isProcessing {
                    Button("Start 1-Click Theming") {
                        startThemingProcess()
                    }
                    .buttonStyle(.borderedProminent)
                }

            case .completed:
                Button("Done") {
                    dismiss()
                }
                .buttonStyle(.borderedProminent)
            }
        }
        .padding(Space.md)
        .background(.background.secondary)
    }

    // MARK: - Processing Logic

    private func startThemingProcess() {
        isProcessing = true
        progressStatus = "Creating snapshot backup point…"

        // 1. Create backup point
        let targetItems = selectedItems
        let backup = model.createBackupPoint(for: targetItems, name: "Modo System Theme (\(model.activePackName))")
        self.backupPoint = backup

        Task {
            let total = Double(targetItems.count)
            var count = 0

            for (index, item) in targetItems.enumerated() {
                await MainActor.run {
                    progressStatus = "Styling \(item.title) (\(index + 1) of \(targetItems.count))…"
                    currentProgress = Double(index) / max(total, 1.0)
                }

                // If auto-match Simple Icons is enabled, assign vector if available
                if autoMatchSimpleIcons, let vector = SimpleIconCatalog.find(for: item.title), item.customImageData == nil {
                    await MainActor.run {
                        model.setCustomArtwork(data: vector.svgData(), for: item)
                    }
                }

                // Apply round icon
                await MainActor.run {
                    model.applyRoundIcon(to: item)
                    count += 1
                }

                // Tiny yield for smooth UI animation
                try? await Task.sleep(nanoseconds: 30_000_000)
            }

            if restartDockAfterApply {
                await MainActor.run {
                    progressStatus = "Restarting macOS Dock…"
                    model.restartDock()
                }
                try? await Task.sleep(nanoseconds: 300_000_000)
            }

            await MainActor.run {
                self.appliedCount = count
                self.isProcessing = false
                self.currentStep = .completed
            }
        }
    }

    private func restoreFromBackup(_ backup: SystemBackupPoint) {
        let restored = model.restoreFromBackupPoint(backup, items: allItems)
        model.restartDock()
        model.statusMessage = "Restored \(restored) applications back to original icons"
        dismiss()
    }
}
