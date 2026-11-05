import SwiftData
import SwiftUI

struct AppListView: View {
    @Environment(AppModel.self) private var model
    @Environment(\.modelContext) private var context
    let items: [Item]

    var body: some View {
        @Bindable var model = model
        VStack(spacing: 0) {
            // Header Bar: Quick Filter Pills + View Mode Picker
            HStack(spacing: Space.xs) {
                ForEach(AppCategoryFilter.allCases) { filter in
                    Button {
                        withAnimation(.snappy(duration: 0.2)) {
                            model.categoryFilter = filter
                        }
                    } label: {
                        Text(filter.rawValue)
                            .font(.caption2.weight(model.categoryFilter == filter ? .semibold : .regular))
                            .padding(.horizontal, Space.sm)
                            .padding(.vertical, 3)
                            .background(
                                Capsule()
                                    .fill(model.categoryFilter == filter ? Palette.accent.opacity(0.18) : Color.clear)
                            )
                            .overlay(
                                Capsule()
                                    .strokeBorder(model.categoryFilter == filter ? Palette.accent : Color.secondary.opacity(0.2), lineWidth: 1)
                            )
                            .foregroundStyle(model.categoryFilter == filter ? Palette.accentText : Color.secondary)
                    }
                    .buttonStyle(.plain)
                }

                Spacer(minLength: Space.xs)

                // View Mode Switch: List vs Grid
                Picker("View Mode", selection: $model.viewMode) {
                    Image(systemName: "list.bullet").tag(AppViewMode.list)
                    Image(systemName: "square.grid.2x2").tag(AppViewMode.grid)
                }
                .pickerStyle(.segmented)
                .frame(width: 68)
                .controlSize(.small)
                .help("Switch between Compact List and Icon Gallery Grid")
            }
            .padding(.horizontal, Space.md)
            .padding(.vertical, Space.xs)
            .background(.background.secondary.opacity(0.4))

            Divider()

            // Main Content: List or Grid
            Group {
                if model.viewMode == .list {
                    listView
                } else {
                    gridView
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)

            Divider()

            // Bottom Footer: Item Counts & Grid Scale Slider
            footerBar
        }
        .overlay {
            if items.isEmpty {
                VStack(spacing: Space.sm) {
                    Image(systemName: "magnifyingglass")
                        .font(.title2)
                        .foregroundStyle(.tertiary)
                    Text("No Matching Apps")
                        .font(.callout.weight(.medium))
                        .foregroundStyle(.secondary)
                    if !model.search.isEmpty {
                        Button("Clear Search") {
                            model.search = ""
                        }
                        .controlSize(.small)
                    }
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
                .padding()
            }
        }
    }

    // MARK: - List Mode

    private var listView: some View {
        @Bindable var model = model
        return List(items, selection: $model.selection) { item in
            AppRowView(item: item)
                .tag(item.id)
                .contextMenu { contextMenuItems(for: item) }
        }
    }

    // MARK: - Grid / Gallery Mode

    private var gridView: some View {
        @Bindable var model = model
        let minColW = max(model.gridIconSize + 24, 76)
        let maxColW = max(model.gridIconSize + 48, 180)
        let columns = [GridItem(.adaptive(minimum: minColW, maximum: maxColW), spacing: Space.md)]

        return ScrollView {
            LazyVGrid(columns: columns, spacing: Space.md) {
                ForEach(items) { item in
                    AppGridCellView(
                        item: item,
                        size: model.gridIconSize,
                        isSelected: model.selection == item.id
                    )
                    .contentShape(Rectangle())
                    .onTapGesture {
                        model.selection = item.id
                    }
                    .contextMenu { contextMenuItems(for: item) }
                }
            }
            .padding(Space.md)
        }
    }

    // MARK: - Footer Bar

    private var footerBar: some View {
        @Bindable var model = model
        let styledCount = items.filter { $0.isApplied }.count

        return HStack(spacing: Space.sm) {
            Text("\(items.count) apps")
                .font(.caption2)
                .foregroundStyle(.secondary)
            if styledCount > 0 {
                Text("· \(styledCount) styled")
                    .font(.caption2)
                    .foregroundStyle(Palette.accentText)
            }

            Spacer()

            if model.viewMode == .grid {
                HStack(spacing: 4) {
                    Image(systemName: "circle")
                        .font(.system(size: 7))
                        .foregroundStyle(.secondary)
                    Slider(value: $model.gridIconSize, in: 36...140)
                        .frame(width: 84)
                        .controlSize(.mini)
                    Image(systemName: "circle.fill")
                        .font(.system(size: 10))
                        .foregroundStyle(.secondary)
                    Text("\(Int(model.gridIconSize))px")
                        .font(.caption2.monospacedDigit())
                        .foregroundStyle(.secondary)
                        .frame(width: 32, alignment: .trailing)
                }
                .help("Adjust icon size in Gallery view (36–140px)")
            }
        }
        .padding(.horizontal, Space.md)
        .padding(.vertical, 4)
        .background(.background.secondary.opacity(0.35))
    }

    // MARK: - Context Menu

    @ViewBuilder
    private func contextMenuItems(for item: Item) -> some View {
        Button(item.isApplied ? "Update Round Icon" : "Apply Round Icon") {
            model.applyRoundIcon(to: item)
        }
        if item.isApplied {
            Button("Restore Original Icon") {
                model.restoreOriginalIcon(for: item)
            }
        }
        Divider()
        Button("Choose Custom Icon / SVG…") {
            model.promptCustomArtwork(for: item)
            try? context.save()
        }
        Button("Paste Custom Icon") {
            if model.pasteCustomArtwork(for: item) {
                try? context.save()
            }
        }
        .disabled(!CustomArtworkHelper.canPasteArtwork())

        if item.customImageData != nil {
            Button("Reset to Default Icon") {
                model.clearCustomArtwork(for: item)
                try? context.save()
            }
        }
        Divider()
        Button(item.isStarred ? "Remove from Favorites" : "Add to Favorites") {
            item.isStarred.toggle()
            try? context.save()
        }
        Divider()
        Button("Delete from Library", role: .destructive) {
            if model.selection == item.id { model.selection = nil }
            context.delete(item)
            try? context.save()
        }
    }
}

// MARK: - Row View

struct AppRowView: View {
    @Environment(AppModel.self) private var model
    @Environment(\.modelContext) private var context
    let item: Item
    @State private var isTargeted = false

    var body: some View {
        HStack(spacing: Space.md) {
            Image(nsImage: model.roundIcon(for: item))
                .resizable()
                .interpolation(.high)
                .frame(width: 40, height: 40)

            VStack(alignment: .leading, spacing: 2) {
                HStack(spacing: Space.xs) {
                    Text(item.title)
                        .font(.body.weight(.medium))
                        .lineLimit(1)
                    if item.isStarred {
                        Image(systemName: "star.fill")
                            .font(.caption2)
                            .foregroundStyle(.yellow)
                    }
                    if item.customImageData != nil {
                        Image(systemName: "sparkles")
                            .font(.caption2)
                            .foregroundStyle(Palette.accentText)
                            .help("Custom SVG / image assigned")
                    }
                    if item.customScale != nil {
                        Image(systemName: "slider.horizontal.2")
                            .font(.caption2)
                            .foregroundStyle(Palette.accentText)
                            .help("Custom scale override applied")
                    }
                }
                Text(item.categoryName)
                    .font(.caption)
                    .foregroundStyle(.secondary)
                    .lineLimit(1)
            }

            Spacer(minLength: Space.sm)

            if item.isApplied {
                Text("Styled")
                    .font(.caption2.weight(.semibold))
                    .foregroundStyle(Palette.accentText)
                    .padding(.horizontal, Space.sm)
                    .padding(.vertical, 2)
                    .background(Palette.accent.opacity(0.14), in: Capsule())
            }
        }
        .padding(.vertical, Space.xs)
        .padding(.horizontal, isTargeted ? Space.xs : 0)
        .background(isTargeted ? Palette.accent.opacity(0.15) : Color.clear, in: RoundedRectangle(cornerRadius: 6))
        .dropDestination(for: URL.self) { urls, _ in
            guard let url = urls.first, let data = CustomArtworkHelper.loadArtworkData(from: url) else { return false }
            model.setCustomArtwork(data: data, for: item)
            try? context.save()
            return true
        } isTargeted: { targeted in
            isTargeted = targeted
        }
    }
}

// MARK: - Grid Cell View

struct AppGridCellView: View {
    @Environment(AppModel.self) private var model
    @Environment(\.modelContext) private var context
    let item: Item
    let size: Double
    let isSelected: Bool
    @State private var isTargeted = false

    var body: some View {
        let cardShape = RoundedRectangle(cornerRadius: Radius.sm, style: .continuous)
        VStack(spacing: 6) {
            ZStack(alignment: .topTrailing) {
                Image(nsImage: model.roundIcon(for: item))
                    .resizable()
                    .interpolation(.high)
                    .frame(width: size, height: size)
                    .padding(4)

                if item.isStarred {
                    Image(systemName: "star.fill")
                        .font(.system(size: max(size * 0.18, 10)))
                        .foregroundStyle(.yellow)
                        .shadow(color: .black.opacity(0.4), radius: 2, x: 0, y: 1)
                        .offset(x: 2, y: -2)
                }
            }

            Text(item.title)
                .font(.caption.weight(isSelected ? .semibold : .regular))
                .lineLimit(2)
                .multilineTextAlignment(.center)
                .foregroundStyle(isSelected ? .primary : .secondary)
                .frame(maxWidth: .infinity)

            HStack(spacing: 3) {
                if item.customImageData != nil {
                    let isSVG = CustomArtworkHelper.isSVGData(item.customImageData!)
                    Text(isSVG ? "SVG" : "Custom")
                        .font(.system(size: 8, weight: .bold))
                        .foregroundStyle(Palette.accentText)
                        .padding(.horizontal, 4)
                        .padding(.vertical, 1)
                        .background(Palette.accent.opacity(0.14), in: Capsule())
                }

                if item.isApplied {
                    Text("Styled")
                        .font(.system(size: 9, weight: .bold))
                        .foregroundStyle(Palette.accentText)
                        .padding(.horizontal, 5)
                        .padding(.vertical, 1)
                        .background(Palette.accent.opacity(0.16), in: Capsule())
                }
            }
        }
        .padding(Space.xs)
        .background(
            cardShape
                .fill(isTargeted ? Palette.accent.opacity(0.20) : (isSelected ? Palette.accent.opacity(0.12) : Color.clear))
        )
        .overlay(
            cardShape
                .strokeBorder(isTargeted ? Palette.accent : (isSelected ? Palette.accent : Color.clear), lineWidth: isTargeted ? 2.0 : 1.5)
        )
        .dropDestination(for: URL.self) { urls, _ in
            guard let url = urls.first, let data = CustomArtworkHelper.loadArtworkData(from: url) else { return false }
            model.setCustomArtwork(data: data, for: item)
            try? context.save()
            return true
        } isTargeted: { targeted in
            isTargeted = targeted
        }
    }
}
