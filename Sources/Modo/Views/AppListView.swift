import SwiftData
import SwiftUI

struct AppListView: View {
    @Environment(AppModel.self) private var model
    @Environment(\.modelContext) private var context
    let items: [Item]

    var body: some View {
        @Bindable var model = model
        VStack(spacing: 0) {
            // Quick Filter Pills (All, Customized, System, Third-Party)
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
                Spacer(minLength: 0)
            }
            .padding(.horizontal, Space.md)
            .padding(.vertical, Space.xs)
            .background(.background.secondary.opacity(0.4))

            Divider()

            List(items, selection: $model.selection) { item in
                AppRowView(item: item)
                    .tag(item.id)
                    .contextMenu {
                        Button(item.isApplied ? "Update Round Icon" : "Apply Round Icon") {
                            model.applyRoundIcon(to: item)
                        }
                        if item.isApplied {
                            Button("Restore Original Icon") {
                                model.restoreOriginalIcon(for: item)
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
}

struct AppRowView: View {
    @Environment(AppModel.self) private var model
    let item: Item

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
                }
                Text(item.categoryName)
                    .font(.caption)
                    .foregroundStyle(.secondary)
                    .lineLimit(1)
            }

            Spacer(minLength: Space.sm)

            if item.isApplied {
                Text("Round")
                    .font(.caption2.weight(.semibold))
                    .foregroundStyle(Palette.accentText)
                    .padding(.horizontal, Space.sm)
                    .padding(.vertical, 2)
                    .background(Palette.accent.opacity(0.14), in: Capsule())
            }
        }
        .padding(.vertical, Space.xs)
    }
}
