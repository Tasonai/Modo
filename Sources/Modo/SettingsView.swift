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
            }
        }
        .formStyle(.grouped)
        .fitsContent()
        .frame(width: 520)
    }
}

/// The Settings tabs. Snapshot mode walks through every case.
enum SettingsTab: String, CaseIterable, Identifiable {
    case general, appearance

    var id: String { rawValue }

    var title: String {
        switch self {
        case .general: "General"
        case .appearance: "Appearance"
        }
    }

    var symbol: String {
        switch self {
        case .general: "gearshape"
        case .appearance: "paintpalette"
        }
    }
}

struct GeneralSettings: View {
    @Environment(AppModel.self) private var model
    @AppStorage("scanSystemApps") private var scanSystemApps = true
    @AppStorage("createAliasOnReadOnly") private var createAliasOnReadOnly = true
    @AppStorage("exportIncludesICO") private var exportIncludesICO = true

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
        }

        Section("macOS Integration & Export") {
            Toggle("Include /System/Applications when scanning", isOn: $scanSystemApps)
            Toggle("Create ~/Applications/Round Icons alias for read-only apps", isOn: $createAliasOnReadOnly)
            Toggle("Export multi-size Windows .ico alongside Apple .icns", isOn: $exportIncludesICO)
        }
    }
}
