import Foundation
import SwiftData

/// Sample data for snapshots: APP_SEED=1 with APP_STORE in /tmp fills an empty
/// store with realistic macOS applications (both applied and unapplied, starred and not).
@MainActor
enum SeedData {
    static func insertIfRequested(into model: AppModel) {
        guard ProcessInfo.processInfo.environment["APP_SEED"] == "1" else { return }
        guard Store.isOverridden else {
            print("APP_SEED ignored: APP_STORE must be a file in /tmp")
            return
        }
        let context = model.container.mainContext
        guard ((try? context.fetchCount(FetchDescriptor<Item>())) ?? 0) == 0 else { return }

        struct SampleApp {
            let title: String
            let bundleID: String
            let category: String
            let symbol: String
            let hex: Int
            let starred: Bool
            let applied: Bool
        }

        let samples: [SampleApp] = [
            .init(title: "Google Chrome", bundleID: "com.google.Chrome", category: "Web Browser",
                  symbol: "globe", hex: 0x1A73E8, starred: true, applied: true),
            .init(title: "Visual Studio Code", bundleID: "com.microsoft.VSCode", category: "Developer Tools",
                  symbol: "chevron.left.forwardslash.chevron.right", hex: 0x007ACC, starred: true, applied: true),
            .init(title: "Spotify", bundleID: "com.spotify.client", category: "Music & Audio",
                  symbol: "waveform.circle.fill", hex: 0x1ED760, starred: true, applied: true),
            .init(title: "Figma", bundleID: "com.figma.Desktop", category: "Design & Prototyping",
                  symbol: "paintbrush.pointed.fill", hex: 0xF24E1E, starred: false, applied: true),
            .init(title: "Telegram", bundleID: "ru.keepcoder.Telegram", category: "Messaging",
                  symbol: "paperplane.fill", hex: 0x2AABEE, starred: true, applied: false),
            .init(title: "Slack", bundleID: "com.tinyspeck.slackmacgap", category: "Team Collaboration",
                  symbol: "number.square.fill", hex: 0x7B5CFF, starred: false, applied: false),
            .init(title: "Notion", bundleID: "notion.id", category: "Productivity & Docs",
                  symbol: "doc.text.fill", hex: 0x2D3138, starred: false, applied: false),
            .init(title: "Arc Browser", bundleID: "company.thebrowser.Browser", category: "Web Browser",
                  symbol: "sparkles", hex: 0xE84A93, starred: false, applied: true),
        ]

        var firstID: UUID?
        for (index, sample) in samples.enumerated() {
            let item = Item(
                title: sample.title,
                bundlePath: "",
                bundleIdentifier: sample.bundleID,
                categoryName: sample.category,
                symbolName: sample.symbol,
                brandHex: sample.hex,
                isStarred: sample.starred,
                isApplied: sample.applied
            )
            item.createdAt = Date.now.addingTimeInterval(Double(-index) * 3_600)
            context.insert(item)
            if firstID == nil { firstID = item.id }
        }
        try? context.save()
        model.selection = firstID
    }
}
