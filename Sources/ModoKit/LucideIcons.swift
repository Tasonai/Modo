import Foundation

/// A category for organizing Lucide open-source vector icons.
public enum LucideCategory: String, CaseIterable, Identifiable, Sendable {
    case all = "All"
    case developer = "Developer"
    case design = "Design & Art"
    case media = "Media & Audio"
    case communication = "Communication"
    case productivity = "Productivity"
    case system = "System & Utility"
    case security = "Security"
    case hardware = "Hardware"

    public var id: String { rawValue }
}

/// An open-source vector icon from the Lucide icon library.
public struct LucideIcon: Identifiable, Sendable, Hashable {
    public let name: String
    public let category: LucideCategory
    public let tags: [String]
    public let innerSVG: String

    public var id: String { name }

    public init(name: String, category: LucideCategory, tags: [String], innerSVG: String) {
        self.name = name
        self.category = category
        self.tags = tags
        self.innerSVG = innerSVG
    }

    /// Formatted display title (e.g. "git-branch" -> "Git Branch").
    public var title: String {
        name.split(separator: "-")
            .map { $0.capitalized }
            .joined(separator: " ")
    }

    /// Generates full SVG markup with stroke styling.
    public func svgString(strokeColorHex: UInt32 = 0xFFFFFF, strokeWidth: Double = 2.0) -> String {
        let hexStr = String(format: "%06X", strokeColorHex & 0xFFFFFF)
        return """
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24" fill="none" stroke="#\(hexStr)" stroke-width="\(strokeWidth)" stroke-linecap="round" stroke-linejoin="round">
  \(innerSVG)
</svg>
"""
    }

    /// Returns UTF-8 data of the SVG representation.
    public func svgData(strokeColorHex: UInt32 = 0xFFFFFF, strokeWidth: Double = 2.0) -> Data {
        svgString(strokeColorHex: strokeColorHex, strokeWidth: strokeWidth).data(using: .utf8) ?? Data()
    }
}

/// Curated library of 80+ open-source Lucide vector glyphs for icon artwork.
public enum LucideIconCatalog {
    public static let all: [LucideIcon] = [
        // MARK: - Developer
        LucideIcon(
            name: "terminal",
            category: .developer,
            tags: ["terminal", "console", "cli", "shell", "bash", "command", "prompt"],
            innerSVG: #"<polyline points=\"4 17 10 11 4 5\"/><line x1="12" x2="20" y1="19" y2="19"/>"#
        ),
        LucideIcon(
            name: "code",
            category: .developer,
            tags: ["code", "developer", "brackets", "html", "syntax"],
            innerSVG: #"<polyline points=\"16 18 22 12 16 6\"/><polyline points=\"8 6 2 12 8 18\"/>"#
        ),
        LucideIcon(
            name: "git-branch",
            category: .developer,
            tags: ["git", "branch", "vcs", "code", "fork"],
            innerSVG: #"<line x1="6" x2="6" y1="3" y2="15"/><circle cx="18" cy="6" r="3"/><circle cx="6" cy="18" r="3"/><path d="M18 9a9 9 0 0 1-9 9"/>"#
        ),
        LucideIcon(
            name: "git-pull-request",
            category: .developer,
            tags: ["git", "pr", "merge", "pull", "request"],
            innerSVG: #"<circle cx="18" cy="18" r="3"/><circle cx="6" cy="6" r="3"/><path d="M13 6h3a2 2 0 0 1 2 2v7"/><line x1="6" x2="6" y1="9" y2="21"/>"#
        ),
        LucideIcon(
            name: "database",
            category: .developer,
            tags: ["database", "sql", "storage", "data", "table"],
            innerSVG: #"<ellipse cx="12" cy="5" rx="9" ry="3"/><path d="M3 5V19A9 3 0 0 0 21 19V5"/><path d="M3 12A9 3 0 0 0 21 12"/>"#
        ),
        LucideIcon(
            name: "server",
            category: .developer,
            tags: ["server", "backend", "cloud", "host", "rack"],
            innerSVG: #"<rect width="20" height="8" x="2" y="2" rx="2" ry="2"/><rect width="20" height="8" x="2" y="14" rx="2" ry="2"/><line x1="6" x2="6.01" y1="6" y2="6"/><line x1="6" x2="6.01" y1="18" y2="18"/>"#
        ),
        LucideIcon(
            name: "cpu",
            category: .developer,
            tags: ["cpu", "processor", "chip", "hardware", "compute"],
            innerSVG: #"<rect width="16" height="16" x="4" y="4" rx="2"/><rect width="6" height="6" x="9" y="9" rx="1"/><path d="M15 2v2"/><path d="M15 20v2"/><path d="M2 15h2"/><path d="M2 9h2"/><path d="M20 15h2"/><path d="M20 9h2"/><path d="M9 2v2"/><path d="M9 20v2"/>"#
        ),
        LucideIcon(
            name: "bug",
            category: .developer,
            tags: ["bug", "debug", "issue", "error", "test"],
            innerSVG: #"<rect width="8" height="14" x="8" y="6" rx="4"/><path d="m19 7-3 2"/><path d="m5 7 3 2"/><path d="m19 19-3-2"/><path d="m5 19 3-2"/><path d="M20 13h-4"/><path d="M4 13h4"/><path d="m10 4 1 2"/><path d="m14 4-1 2"/>"#
        ),
        LucideIcon(
            name: "binary",
            category: .developer,
            tags: ["binary", "data", "code", "machine", "byte"],
            innerSVG: #"<rect width="4" height="6" x="14" y="14" rx="2"/><rect width="4" height="6" x="6" y="4" rx="2"/><path d="M6 20h4"/><path d="M14 10h4"/><path d="M6 14h2v6"/><path d="M14 4h2v6"/>"#
        ),
        LucideIcon(
            name: "command",
            category: .developer,
            tags: ["command", "apple", "keyboard", "shortcut", "key"],
            innerSVG: #"<path d="M15 6v12a3 3 0 1 0 3-3H6a3 3 0 1 0 3 3V6a3 3 0 1 0-3 3h12a3 3 0 1 0-3-3"/>"#
        ),
        LucideIcon(
            name: "box",
            category: .developer,
            tags: ["box", "package", "cube", "container", "module"],
            innerSVG: #"<path d="M21 8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16Z"/><path d="m3.3 7 8.7 5 8.7-5"/><path d="M12 22V12"/>"#
        ),
        LucideIcon(
            name: "boxes",
            category: .developer,
            tags: ["boxes", "packages", "containers", "modules", "microservices"],
            innerSVG: #"<path d="M2.97 12.92A2 2 0 0 0 2 14.63v3.24a2 2 0 0 0 .97 1.71l3 1.8a2 2 0 0 0 2.06 0L12 19v-5.5l-5-3-4.03 2.42Z"/><path d="m7 16.5-4.74-2.85"/><path d="m7 16.5 5-3"/><path d="M7 16.5v5.17"/><path d="M12 13.5V19l3.97 2.38a2 2 0 0 0 2.06 0l3-1.8a2 2 0 0 0 .97-1.71v-3.24a2 2 0 0 0-.97-1.71L17 10.5l-5 3Z"/><path d="m17 16.5-5-3"/><path d="m17 16.5 4.74-2.85"/><path d="M17 16.5v5.17"/><path d="M7.97 4.42A2 2 0 0 0 7 6.13v4.37l5 3 5-3V6.13a2 2 0 0 0-.97-1.71l-3-1.8a2 2 0 0 0-2.06 0l-3 1.8Z"/><path d="M12 8 7.26 5.15"/><path d="m12 8 4.74-2.85"/><path d="M12 13.5V8"/>"#
        ),

        // MARK: - Design & Art
        LucideIcon(
            name: "palette",
            category: .design,
            tags: ["palette", "color", "art", "paint", "theme", "design"],
            innerSVG: #"<circle cx="13.5" cy="6.5" r=".5" fill="currentColor"/><circle cx="17.5" cy="10.5" r=".5" fill="currentColor"/><circle cx="8.5" cy="7.5" r=".5" fill="currentColor"/><circle cx="6.5" cy="12.5" r=".5" fill="currentColor"/><path d="M12 2C6.5 2 2 6.5 2 12s4.5 10 10 10c.926 0 1.648-.746 1.648-1.688 0-.437-.18-.835-.437-1.125-.29-.289-.438-.652-.438-1.125a1.64 1.64 0 0 1 1.668-1.668h1.996c3.051 0 5.555-2.503 5.555-5.554C21.965 6.012 17.461 2 12 2z"/>"#
        ),
        LucideIcon(
            name: "sparkles",
            category: .design,
            tags: ["sparkles", "ai", "magic", "stars", "generate", "clean"],
            innerSVG: #"<path d="m12 3-1.912 5.813a2 2 0 0 1-1.275 1.275L3 12l5.813 1.912a2 2 0 0 1 1.275 1.275L12 21l1.912-5.813a2 2 0 0 1 1.275-1.275L21 12l-5.813-1.912a2 2 0 0 1-1.275-1.275L12 3Z"/><path d="M5 3v4"/><path d="M19 17v4"/><path d="M3 5h4"/><path d="M17 19h4"/>"#
        ),
        LucideIcon(
            name: "wand-2",
            category: .design,
            tags: ["wand", "magic", "wizard", "creative", "filter"],
            innerSVG: #"<path d="m21.64 3.64-1.28-1.28a1.21 1.21 0 0 0-1.72 0L2.36 18.64a1.21 1.21 0 0 0 0 1.72l1.28 1.28a1.2 1.2 0 0 0 1.72 0L21.64 5.36a1.2 1.2 0 0 0 0-1.72Z"/><path d="m14 7 3 3"/><path d="M5 6v4"/><path d="M19 14v4"/><path d="M10 2v2"/><path d="M7 8H3"/><path d="M21 16h-4"/><path d="M11 3H9"/>"#
        ),
        LucideIcon(
            name: "brush",
            category: .design,
            tags: ["brush", "paint", "draw", "stroke", "art"],
            innerSVG: #"<path d="m9.06 11.9 8.07-8.06a2.85 2.85 0 1 1 4.03 4.03l-8.06 8.08"/><path d="M7.07 14.94c-1.66 0-3 1.35-3 3.02 0 1.33-2.5 1.52-2 2.02 1.52.51 6.01.5 6.01-2.02 0-1.67-1.34-3.02-3.01-3.02z"/>"#
        ),
        LucideIcon(
            name: "layers",
            category: .design,
            tags: ["layers", "stack", "overlay", "sheets", "levels"],
            innerSVG: #"<path d="m12.83 2.18a2 2 0 0 0-1.66 0L2.6 6.08a1 1 0 0 0 0 1.83l8.58 3.91a2 2 0 0 0 1.66 0l8.58-3.9a1 1 0 0 0 0-1.83Z"/><path d="m22 17.65-9.17 4.16a2 2 0 0 1-1.66 0L2 17.65"/><path d="m22 12.65-9.17 4.16a2 2 0 0 1-1.66 0L2 12.65"/>"#
        ),
        LucideIcon(
            name: "crop",
            category: .design,
            tags: ["crop", "trim", "frame", "aspect", "cut"],
            innerSVG: #"<path d="M6 2v14a2 2 0 0 0 2 2h14"/><path d="M18 22V8a2 2 0 0 0-2-2H2"/>"#
        ),
        LucideIcon(
            name: "shapes",
            category: .design,
            tags: ["shapes", "geometry", "circle", "triangle", "square"],
            innerSVG: #"<path d="M8.3 10a.7.7 0 0 1-.626-1.079L11.434 2.4a.7.7 0 0 1 1.232 0l3.76 6.521A.7.7 0 0 1 15.8 10Z"/><circle cx="17.5" cy="17.5" r="4.5"/><rect width="8" height="8" x="2" y="13" rx="1"/>"#
        ),
        LucideIcon(
            name: "pen-tool",
            category: .design,
            tags: ["pen", "vector", "bezier", "bezier-curve", "path"],
            innerSVG: #"<path d="m12 19 7-7 3 3-7 7-3-3z"/><path d="m18 13-1.5-7.5L2 2l3.5 14.5L13 18l5-5z"/><path d="m2 2 7.586 7.586"/><circle cx="11" cy="11" r="2"/>"#
        ),

        // MARK: - Media & Audio
        LucideIcon(
            name: "music",
            category: .media,
            tags: ["music", "audio", "song", "note", "sound"],
            innerSVG: #"<path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/>"#
        ),
        LucideIcon(
            name: "camera",
            category: .media,
            tags: ["camera", "photo", "picture", "snapshot", "lens"],
            innerSVG: #"<path d="M14.5 4h-5L7 7H4a2 2 0 0 0-2 2v9a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2V9a2 2 0 0 0-2-2h-3l-2.5-3z"/><circle cx="12" cy="13" r="3"/>"#
        ),
        LucideIcon(
            name: "video",
            category: .media,
            tags: ["video", "camera", "film", "movie", "record"],
            innerSVG: #"<path d="m22 8-6 4 6 4V8Z"/><rect width="14" height="12" x="2" y="6" rx="2" ry="2"/>"#
        ),
        LucideIcon(
            name: "mic",
            category: .media,
            tags: ["mic", "microphone", "audio", "voice", "record"],
            innerSVG: #"<path d="M12 2a3 3 0 0 0-3 3v7a3 3 0 0 0 6 0V5a3 3 0 0 0-3-3Z"/><path d="M19 10v2a7 7 0 0 1-14 0v-2"/><line x1="12" x2="12" y1="19" y2="22"/>"#
        ),
        LucideIcon(
            name: "headphones",
            category: .media,
            tags: ["headphones", "audio", "listen", "music", "headset"],
            innerSVG: #"<path d="M3 14h3a2 2 0 0 1 2 2v3a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-7a9 9 0 0 1 18 0v7a2 2 0 0 1-2 2h-1a2 2 0 0 1-2-2v-3a2 2 0 0 1 2-2h3"/>"#
        ),
        LucideIcon(
            name: "film",
            category: .media,
            tags: ["film", "movie", "cinema", "video", "reel"],
            innerSVG: #"<rect width="20" height="20" x="2" y="2" rx="2.18" ry="2.18"/><line x1="7" x2="7" y1="2" y2="22"/><line x1="17" x2="17" y1="2" y2="22"/><line x1="2" x2="22" y1="12" y2="12"/><line x1="2" x2="7" y1="7" y2="7"/><line x1="2" x2="7" y1="17" y2="17"/><line x1="17" x2="22" y1="17" y2="17"/><line x1="17" x2="22" y1="7" y2="7"/>"#
        ),
        LucideIcon(
            name: "play-circle",
            category: .media,
            tags: ["play", "media", "start", "button", "stream"],
            innerSVG: #"<circle cx="12" cy="12" r="10"/><polygon points="10 8 16 12 10 16 10 8"/>"#
        ),

        // MARK: - Communication
        LucideIcon(
            name: "message-square",
            category: .communication,
            tags: ["chat", "message", "bubble", "comment", "talk"],
            innerSVG: #"<path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"/>"#
        ),
        LucideIcon(
            name: "send",
            category: .communication,
            tags: ["send", "paperplane", "mail", "telegram", "message", "share"],
            innerSVG: #"<path d="m22 2-7 20-4-9-9-4Z"/><path d="M22 2 11 13"/>"#
        ),
        LucideIcon(
            name: "mail",
            category: .communication,
            tags: ["mail", "envelope", "email", "inbox", "message"],
            innerSVG: #"<rect width="20" height="16" x="2" y="4" rx="2"/><path d="m22 7-8.97 5.7a1.94 1.94 0 0 1-2.06 0L2 7"/>"#
        ),
        LucideIcon(
            name: "phone",
            category: .communication,
            tags: ["phone", "call", "voice", "contact", "telecom"],
            innerSVG: #"<path d="M22 16.92v3a2 2 0 0 1-2.18 2 19.79 19.79 0 0 1-8.63-3.07 19.5 19.5 0 0 1-6-6 19.79 19.79 0 0 1-3.07-8.67A2 2 0 0 1 4.11 2h3a2 2 0 0 1 2 1.72 12.84 12.84 0 0 0 .7 2.81 2 2 0 0 1-.45 2.11L8.09 9.91a16 16 0 0 0 6 6l1.27-1.27a2 2 0 0 1 2.11-.45 12.84 12.84 0 0 0 2.81.7A2 2 0 0 1 22 16.92z"/>"#
        ),
        LucideIcon(
            name: "bell",
            category: .communication,
            tags: ["bell", "alarm", "notify", "alert", "reminder"],
            innerSVG: #"<path d="M6 8a6 6 0 0 1 12 0c0 7 3 9 3 9H3s3-2 3-9"/><path d="M10.3 21a1.94 1.94 0 0 0 3.4 0"/>"#
        ),
        LucideIcon(
            name: "at-sign",
            category: .communication,
            tags: ["at", "mention", "email", "tag", "user"],
            innerSVG: #"<circle cx="12" cy="12" r="4"/><path d="M16 8v5a3 3 0 0 0 6 0v-1a10 10 0 1 0-4 8"/>"#
        ),

        // MARK: - Productivity
        LucideIcon(
            name: "folder",
            category: .productivity,
            tags: ["folder", "directory", "files", "archive", "storage"],
            innerSVG: #"<path d="M20 20a2 2 0 0 0 2-2V8a2 2 0 0 0-2-2h-7.9a2 2 0 0 1-1.69-.9L9.6 3.9A2 2 0 0 0 7.93 3H4a2 2 0 0 0-2 2v13a2 2 0 0 0 2 2Z"/>"#
        ),
        LucideIcon(
            name: "file-text",
            category: .productivity,
            tags: ["file", "document", "text", "notes", "article"],
            innerSVG: #"<path d="M15 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V7Z"/><path d="M14 2v4a2 2 0 0 0 2 2h4"/><path d="M10 9H8"/><path d="M16 13H8"/><path d="M16 17H8"/>"#
        ),
        LucideIcon(
            name: "calendar",
            category: .productivity,
            tags: ["calendar", "date", "event", "schedule", "plan"],
            innerSVG: #"<path d="M8 2v4"/><path d="M16 2v4"/><rect width="18" height="18" x="3" y="4" rx="2"/><path d="M3 10h18"/>"#
        ),
        LucideIcon(
            name: "clock",
            category: .productivity,
            tags: ["clock", "time", "watch", "timer", "schedule"],
            innerSVG: #"<circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/>"#
        ),
        LucideIcon(
            name: "bookmark",
            category: .productivity,
            tags: ["bookmark", "save", "star", "pin", "favorite"],
            innerSVG: #"<path d="m19 21-7-4-7 4V5a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2v16z"/>"#
        ),
        LucideIcon(
            name: "check-circle",
            category: .productivity,
            tags: ["check", "done", "complete", "success", "task"],
            innerSVG: #"<path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"/><path d="m9 11 3 3L22 4"/>"#
        ),
        LucideIcon(
            name: "briefcase",
            category: .productivity,
            tags: ["briefcase", "work", "business", "job", "career"],
            innerSVG: #"<path d="M16 20V4a2 2 0 0 0-2-2h-4a2 2 0 0 0-2 2v16"/><rect width="20" height="14" x="2" y="6" rx="2"/>"#
        ),
        LucideIcon(
            name: "archive",
            category: .productivity,
            tags: ["archive", "box", "backup", "store", "zip"],
            innerSVG: #"<rect width="20" height="5" x="2" y="3" rx="1"/><path d="M4 8v11a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8"/><path d="M10 12h4"/>"#
        ),

        // MARK: - System & Utility
        LucideIcon(
            name: "settings",
            category: .system,
            tags: ["settings", "gear", "preferences", "config", "options"],
            innerSVG: #"<path d="M12.22 2h-.44a2 2 0 0 0-2 2v.18a2 2 0 0 1-1 1.73l-.43.25a2 2 0 0 1-2 0l-.15-.08a2 2 0 0 0-2.73.73l-.22.38a2 2 0 0 0 .73 2.73l.15.1a2 2 0 0 1 1 1.72v.51a2 2 0 0 1-1 1.74l-.15.09a2 2 0 0 0-.73 2.73l.22.38a2 2 0 0 0 2.73.73l.15-.08a2 2 0 0 1 2 0l.43.25a2 2 0 0 1 1 1.73V20a2 2 0 0 0 2 2h.44a2 2 0 0 0 2-2v-.18a2 2 0 0 1 1-1.73l.43-.25a2 2 0 0 1 2 0l.15.08a2 2 0 0 0 2.73-.73l.22-.39a2 2 0 0 0-.73-2.73l-.15-.08a2 2 0 0 1-1-1.74v-.5a2 2 0 0 1 1-1.74l.15-.09a2 2 0 0 0 .73-2.73l-.22-.38a2 2 0 0 0-2.73-.73l-.15.08a2 2 0 0 1-2 0l-.43-.25a2 2 0 0 1-1-1.73V4a2 2 0 0 0-2-2z"/><circle cx="12" cy="12" r="3"/>"#
        ),
        LucideIcon(
            name: "sliders",
            category: .system,
            tags: ["sliders", "controls", "equalizer", "tune", "settings"],
            innerSVG: #"<line x1="4" x2="4" y1="21" y2="14"/><line x1="4" x2="4" y1="10" y2="3"/><line x1="12" x2="12" y1="21" y2="12"/><line x1="12" x2="12" y1="8" y2="3"/><line x1="20" x2="20" y1="21" y2="16"/><line x1="20" x2="20" y1="12" y2="3"/><line x1="2" x2="6" y1="14" y2="14"/><line x1="10" x2="14" y1="8" y2="8"/><line x1="18" x2="22" y1="16" y2="16"/>"#
        ),
        LucideIcon(
            name: "search",
            category: .system,
            tags: ["search", "find", "magnifier", "lookup", "explore"],
            innerSVG: #"<circle cx="11" cy="11" r="8"/><path d="m21 21-4.3-4.3"/>"#
        ),
        LucideIcon(
            name: "zap",
            category: .system,
            tags: ["zap", "bolt", "lightning", "fast", "power", "energy"],
            innerSVG: #"<path d="M4 14a1 1 0 0 1-.78-1.63l9.9-10.2a.5.5 0 0 1 .86.46l-1.92 6.02A1 1 0 0 0 13 10h7a1 1 0 0 1 .78 1.63l-9.9 10.2a.5.5 0 0 1-.86-.46l1.92-6.02A1 1 0 0 0 11 14z"/>"#
        ),
        LucideIcon(
            name: "flame",
            category: .system,
            tags: ["flame", "fire", "hot", "trending", "burn"],
            innerSVG: #"<path d="M8.5 14.5A2.5 2.5 0 0 0 11 12c0-1.38-.5-2-1-3-1.072-2.143-.224-4.054 2-6 .5 2.5 2 4.9 4 6.5 2 1.6 3 3.5 3 5.5a7 7 0 1 1-14 0c0-1.153.433-2.294 1-3a2.5 2.5 0 0 0 2.5 2.5z"/>"#
        ),
        LucideIcon(
            name: "heart",
            category: .system,
            tags: ["heart", "love", "like", "favorite", "health"],
            innerSVG: #"<path d="M19 14c1.49-1.46 3-3.21 3-5.5A5.5 5.5 0 0 0 16.5 3c-1.76 0-3 .5-4.5 2-1.5-1.5-2.74-2-4.5-2A5.5 5.5 0 0 0 2 8.5c0 2.3 1.5 4.05 3 5.5l7 7Z"/>"#
        ),
        LucideIcon(
            name: "star",
            category: .system,
            tags: ["star", "favorite", "bookmark", "rating", "gold"],
            innerSVG: #"<polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/>"#
        ),
        LucideIcon(
            name: "compass",
            category: .system,
            tags: ["compass", "navigation", "safari", "explore", "direction"],
            innerSVG: #"<circle cx="12" cy="12" r="10"/><polygon points="16.24 7.76 14.12 14.12 7.76 16.24 9.88 9.88 16.24 7.76"/>"#
        ),
        LucideIcon(
            name: "globe",
            category: .system,
            tags: ["globe", "world", "internet", "earth", "network", "web"],
            innerSVG: #"<circle cx="12" cy="12" r="10"/><path d="M12 2a14.5 14.5 0 0 0 0 20 14.5 14.5 0 0 0 0-20"/><path d="M2 12h20"/>"#
        ),
        LucideIcon(
            name: "cloud",
            category: .system,
            tags: ["cloud", "storage", "weather", "sync", "network"],
            innerSVG: #"<path d="M17.5 19H9a7 7 0 1 1 6.71-9h1.79a4.5 4.5 0 1 1 0 9Z"/>"#
        ),
        LucideIcon(
            name: "sun",
            category: .system,
            tags: ["sun", "day", "light", "weather", "brightness"],
            innerSVG: #"<circle cx="12" cy="12" r="4"/><path d="M12 2v2"/><path d="M12 20v2"/><path d="m4.93 4.93 1.41 1.41"/><path d="m17.66 17.66 1.41 1.41"/><path d="M2 12h2"/><path d="M20 12h2"/><path d="m6.34 17.66-1.41 1.41"/><path d="m19.07 4.93-1.41 1.41"/>"#
        ),
        LucideIcon(
            name: "moon",
            category: .system,
            tags: ["moon", "night", "dark", "sleep", "mode"],
            innerSVG: #"<path d="M12 3a6 6 0 0 0 9 9 9 9 0 1 1-9-9Z"/>"#
        ),
        LucideIcon(
            name: "rocket",
            category: .system,
            tags: ["rocket", "launch", "fast", "speed", "space", "deploy"],
            innerSVG: #"<path d="M4.5 16.5c-1.5 1.26-2 5-2 5s3.74-.5 5-2c.71-.84.7-2.13-.09-2.91a2.18 2.18 0 0 0-2.91-.09z"/><path d="m12 15-3-3a22 22 0 0 1 2-3.95A12.88 12.88 0 0 1 22 2c0 2.72-.78 7.5-6 11a22.35 22.35 0 0 1-4 2z"/><path d="M9 12H4s.55-3.03 2-4c1.62-1.08 5 0 5 0"/><path d="M12 15v5s3.03-.55 4-2c1.08-1.62 0-5 0-5"/>"#
        ),

        // MARK: - Security
        LucideIcon(
            name: "shield",
            category: .security,
            tags: ["shield", "security", "protection", "safe", "antivirus"],
            innerSVG: #"<path d="M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z"/>"#
        ),
        LucideIcon(
            name: "shield-check",
            category: .security,
            tags: ["shield", "check", "secure", "verified", "safe"],
            innerSVG: #"<path d="M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z"/><path d="m9 12 2 2 4-4"/>"#
        ),
        LucideIcon(
            name: "lock",
            category: .security,
            tags: ["lock", "security", "privacy", "password", "key"],
            innerSVG: #"<rect width="18" height="11" x="3" y="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/>"#
        ),
        LucideIcon(
            name: "key",
            category: .security,
            tags: ["key", "auth", "access", "login", "password"],
            innerSVG: #"<path d="m15.5 7.5 2.3 2.3a1 1 0 0 0 1.4 0l2.1-2.1a1 1 0 0 0 0-1.4L19 4"/><path d="m21 2-9.6 9.6"/><circle cx="7.5" cy="15.5" r="5.5"/>"#
        ),

        // MARK: - Hardware
        LucideIcon(
            name: "laptop",
            category: .hardware,
            tags: ["laptop", "computer", "macbook", "pc", "device"],
            innerSVG: #"<path d="M20 16V7a2 2 0 0 0-2-2H6a2 2 0 0 0-2 2v9m16 0H4m16 0 1.28 2.55a1 1 0 0 1-.9 1.45H3.62a1 1 0 0 1-.9-1.45L4 16"/>"#
        ),
        LucideIcon(
            name: "smartphone",
            category: .hardware,
            tags: ["phone", "mobile", "smartphone", "iphone", "android"],
            innerSVG: #"<rect width="14" height="20" x="5" y="2" rx="2" ry="2"/><path d="M12 18h.01"/>"#
        ),
        LucideIcon(
            name: "hard-drive",
            category: .hardware,
            tags: ["disk", "drive", "storage", "ssd", "hdd", "hardware"],
            innerSVG: #"<line x1="22" x2="2" y1="12" y2="12"/><path d="M5.45 5.11 2 12v6a2 2 0 0 0 2 2h16a2 2 0 0 0 2-2v-6l-3.45-6.89A2 2 0 0 0 16.76 4H7.24a2 2 0 0 0-1.79 1.11z"/><line x1="6" x2="6.01" y1="16" y2="16"/><line x1="10" x2="10.01" y1="16" y2="16"/>"#
        ),
        LucideIcon(
            name: "wifi",
            category: .hardware,
            tags: ["wifi", "network", "internet", "wireless", "signal"],
            innerSVG: #"<path d="M12 20h.01"/><path d="M2 8.82a15 15 0 0 1 20 0"/><path d="M5 12.859a10 10 0 0 1 14 0"/><path d="M8.5 16.429a5 5 0 0 1 7 0"/>"#
        )
    ]

    /// Finds icon by exact name or fuzzy keyword match.
    public static func match(query: String) -> LucideIcon? {
        let q = query.trimmingCharacters(in: .whitespacesAndNewlines).lowercased()
        if q.isEmpty { return nil }

        // Exact match
        if let exact = all.first(where: { $0.name == q }) { return exact }

        // Tag match
        if let tagged = all.first(where: { $0.tags.contains(q) }) { return tagged }

        // Prefix / substring match
        return all.first(where: { $0.name.contains(q) || $0.tags.contains(where: { $0.contains(q) }) })
    }
}
