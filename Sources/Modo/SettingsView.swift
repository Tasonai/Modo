import ModoKit
import SwiftUI

/// The Settings window (Command-Comma). Each tab is a grouped Form that sizes
/// to its content so the window shows no unnecessary scroll bar.
struct SettingsView: View {
    @Environment(AppModel.self) private var model

    var body: some View {
        @Bindable var model = model
        TabView(selection: $model.settingsTab) {
            ForEach(SettingsTab.allCases) { tab in
                page(tab)
                    .tabItem { Label(tab.title, systemImage: tab.symbol) }
                    .tag(tab)
            }
        }
        .noInitialFocus()
    }

    @ViewBuilder
    private func page(_ tab: SettingsTab) -> some View {
        Form {
            switch tab {
            case .general: GeneralSettings()
            case .appearance: AppearanceSettings()
            case .updates: UpdatesSettingsView()
            }
        }
        .formStyle(.grouped)
        .fitsContent()
        .frame(width: 520)
    }
}

/// The Settings tabs. Snapshot mode walks through every case.
enum SettingsTab: String, CaseIterable, Identifiable {
    case general, appearance, updates

    var id: String { rawValue }

    var title: String {
        switch self {
        case .general: "General"
        case .appearance: "Appearance"
        case .updates: "Updates"
        }
    }

    var symbol: String {
        switch self {
        case .general: "gearshape"
        case .appearance: "paintpalette"
        case .updates: "arrow.triangle.2.circlepath"
        }
    }
}

struct GeneralSettings: View {
    @Environment(AppModel.self) private var model
    @AppStorage("scanSystemApps") private var scanSystemApps = true
    @AppStorage("createAliasOnReadOnly") private var createAliasOnReadOnly = true
    @AppStorage("exportIncludesICO") private var exportIncludesICO = true
    @AppStorage("autoReapplyIcons") private var autoReapplyIcons = true
    @AppStorage("iconSetCatalogURL") private var iconSetCatalogURL = ""
    @AppStorage("showMenuBarExtra") private var showMenuBarExtra = true
    @State private var cliStatus = ""

    var body: some View {
        @Bindable var model = model
        Section("Circular Icon Rendering") {
            Picker("Default Surface Preset", selection: $model.style.backgroundStyle) {
                ForEach(RoundBackgroundStyle.allCases) { preset in
                    Text(preset.title).tag(preset)
                }
            }
            Toggle("Automatically trim transparent icon margins", isOn: $model.style.autoTrim)
            Toggle("Expand full-bleed square icons to fill circle", isOn: $model.style.fullBleedSquares)
            Toggle("Auto-match official brand vectors on pack selection", isOn: $model.autoVectorizePacks)
                .help("When choosing an icon pack, automatically replaces macOS squircle icons with clean official vector brand logos (Simple Icons).")
        }

        Section("macOS Integration & Menu Bar") {
            Toggle("Show Modo Quick Drop in Menu Bar", isOn: $showMenuBarExtra)
            Toggle("Include /System/Applications when scanning", isOn: $scanSystemApps)
            Toggle("Create ~/Applications/Round Icons alias for read-only apps", isOn: $createAliasOnReadOnly)
            Toggle("Export multi-size Windows .ico alongside Apple .icns", isOn: $exportIncludesICO)
            Toggle("Re-apply Modo icons after an app is updated", isOn: $autoReapplyIcons)
            TextField("Icon set catalog URL", text: $iconSetCatalogURL, prompt: Text("https://…/catalog.json"))
        }

        Section("Command Line Tool (modo)") {
            HStack {
                VStack(alignment: .leading, spacing: 2) {
                    Text("Install 'modo' in terminal")
                        .font(.body)
                    Text("Automate icon styling, batch export, and folder customization from scripts.")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
                Spacer()
                Button("Install CLI…") {
                    installCLITool()
                }
            }
            if !cliStatus.isEmpty {
                Text(cliStatus)
                    .font(.caption)
                    .foregroundStyle(Palette.accent)
            }
        }
    }

    private func installCLITool() {
        guard let execPath = Bundle.main.executablePath else { return }
        let target = "/usr/local/bin/modo"
        let fm = FileManager.default

        // Try direct symlink
        try? fm.removeItem(atPath: target)
        do {
            try fm.createSymbolicLink(atPath: target, withDestinationPath: execPath)
            cliStatus = "✓ Installed 'modo' to \(target)"
        } catch {
            // Suggest terminal command
            let altDir = ("~/.local/bin" as NSString).expandingTildeInPath
            try? fm.createDirectory(atPath: altDir, withIntermediateDirectories: true)
            let altTarget = altDir + "/modo"
            try? fm.removeItem(atPath: altTarget)
            if (try? fm.createSymbolicLink(atPath: altTarget, withDestinationPath: execPath)) != nil {
                cliStatus = "✓ Installed to \(altTarget) (ensure ~/.local/bin is in PATH)"
            } else {
                cliStatus = "Run in terminal: sudo ln -sf \"\(execPath)\" /usr/local/bin/modo"
            }
        }
    }
}
