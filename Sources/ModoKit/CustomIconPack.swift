import Foundation

/// A user-saved icon pack: a name plus a snapshot of the full style.
public struct CustomIconPack: Codable, Hashable, Identifiable, Sendable {
    public let id: String
    public var name: String
    public var style: RoundIconStyle

    public init(id: String = "custom_" + UUID().uuidString, name: String, style: RoundIconStyle) {
        self.id = id
        self.name = name.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
            ? "My Pack"
            : name.trimmingCharacters(in: .whitespacesAndNewlines)
        self.style = style
    }
}
