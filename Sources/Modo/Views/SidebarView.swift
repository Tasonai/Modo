import ModoKit
import SwiftData
import SwiftUI

struct SidebarView: View {
    @Environment(AppModel.self) private var model
    let items: [Item]

    var body: some View {
        @Bindable var model = model
        List(selection: $model.sidebar) {
            Section("Library") {
                ForEach(SidebarItem.allCases) { page in
                    Label(page.title, systemImage: page.symbol)
                        .badge(count(for: page))
                        .tag(page)
                }
            }

            Section("Icon Packs") {
                ForEach(IconPack.allPacks) { pack in
                    Button {
                        model.applyPack(pack)
                    } label: {
                        HStack(spacing: Space.sm) {
                            Image(systemName: pack.iconSymbol)
                                .foregroundStyle(model.activePackId == pack.id ? Palette.accentText : .secondary)
                            Text(pack.name)
                                .foregroundStyle(.primary)
                            Spacer()
                            if model.activePackId == pack.id {
                                Image(systemName: "checkmark")
                                    .font(.caption.weight(.bold))
                                    .foregroundStyle(Palette.accentText)
                            }
                        }
                    }
                    .buttonStyle(.plain)
                    .help("\(pack.name) — \(pack.subtitle)")
                }
            }

            Section("Surface Presets") {
                ForEach(RoundBackgroundStyle.allCases) { preset in
                    Button {
                        model.style.backgroundStyle = preset
                    } label: {
                        HStack(spacing: Space.sm) {
                            Image(systemName: preset.symbol)
                                .foregroundStyle(model.style.backgroundStyle == preset ? Palette.accentText : .secondary)
                            Text(preset.title)
                                .foregroundStyle(.primary)
                            Spacer()
                            if model.style.backgroundStyle == preset {
                                Image(systemName: "checkmark")
                                    .font(.caption.weight(.bold))
                                    .foregroundStyle(Palette.accentText)
                            }
                        }
                    }
                    .buttonStyle(.plain)
                }
            }
        }
    }

    private func count(for page: SidebarItem) -> Int {
        switch page {
        case .all: items.count
        case .applied: items.filter(\.isApplied).count
        case .starred: items.filter(\.isStarred).count
        }
    }
}
