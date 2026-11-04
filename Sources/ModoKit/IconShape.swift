import Foundation

/// Geometry for Modo icon surfaces.
public enum IconShape: String, CaseIterable, Codable, Sendable, Identifiable {
    /// Classic circular disc (Google Pixel, Android adaptive circle).
    case circle = "circle"
    /// macOS-style rounded app-icon plate (22.5% corner radius; not a true continuous-curvature squircle).
    case squircle = "squircle"
    /// Smooth rounded rectangle with customizable optical radius.
    case roundedRect = "rounded_rect"
    /// Organic capsule / pebble shape.
    case pebble = "pebble"
    /// Strict geometric hexagon plate.
    case hexagon = "hexagon"

    public var id: String { rawValue }

    public var title: String {
        switch self {
        case .circle: "Circle"
        case .squircle: "Squircle"
        case .roundedRect: "Rounded Rect"
        case .pebble: "Pebble"
        case .hexagon: "Hexagon"
        }
    }

    public var subtitle: String {
        switch self {
        case .circle: "Classic circular disc (Google Pixel)"
        case .squircle: "macOS-style rounded plate"
        case .roundedRect: "Modern rounded rectangle"
        case .pebble: "Organic smooth capsule"
        case .hexagon: "Sharp geometric tech plate"
        }
    }

    public var symbol: String {
        switch self {
        case .circle: "circle.fill"
        case .squircle: "app.fill"
        case .roundedRect: "rectangle.roundedtop"
        case .pebble: "capsule.portrait.fill"
        case .hexagon: "hexagon.fill"
        }
    }
}
