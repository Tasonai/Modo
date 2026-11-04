import CoreGraphics
import Foundation

/// Geometry for Modo icon surfaces.
public enum IconShape: String, CaseIterable, Codable, Sendable, Identifiable {
    /// Classic circular disc (Google Pixel, Android adaptive circle).
    case circle = "circle"
    /// Superellipse (n = 5): continuous curvature, close to the macOS app-icon outline.
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
        case .squircle: "Smooth superellipse (macOS-like)"
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

extension IconShape {
    /// Outline of this shape inside `rect`.
    public func path(in rect: CGRect) -> CGPath {
        switch self {
        case .circle:
            return CGPath(ellipseIn: rect, transform: nil)
        case .squircle:
            return Self.superellipsePath(in: rect, exponent: 5.0)
        case .roundedRect:
            let r = rect.width * 0.16
            return CGPath(roundedRect: rect, cornerWidth: r, cornerHeight: r, transform: nil)
        case .pebble:
            let r = rect.width * 0.38
            return CGPath(roundedRect: rect, cornerWidth: r, cornerHeight: r, transform: nil)
        case .hexagon:
            let path = CGMutablePath()
            let radius = min(rect.width, rect.height) * 0.5
            for i in 0..<6 {
                let angle = CGFloat(i) * CGFloat.pi / 3.0 - CGFloat.pi / 6.0
                let pt = CGPoint(x: rect.midX + radius * cos(angle), y: rect.midY + radius * sin(angle))
                if i == 0 { path.move(to: pt) } else { path.addLine(to: pt) }
            }
            path.closeSubpath()
            return path
        }
    }

    /// Smooth superellipse |x|^n + |y|^n = 1 (n = 5 is close to the macOS app-icon outline).
    /// Curvature changes continuously, so there is no visible seam where a corner meets an edge.
    public static func superellipsePath(in rect: CGRect, exponent n: Double) -> CGPath {
        let path = CGMutablePath()
        let steps = 256
        let power = 2.0 / n
        for i in 0..<steps {
            let t = 2.0 * Double.pi * Double(i) / Double(steps)
            let c = cos(t), s = sin(t)
            let x = (c < 0 ? -1.0 : 1.0) * pow(abs(c), power)
            let y = (s < 0 ? -1.0 : 1.0) * pow(abs(s), power)
            let pt = CGPoint(x: rect.midX + CGFloat(x) * rect.width * 0.5,
                             y: rect.midY + CGFloat(y) * rect.height * 0.5)
            if i == 0 { path.move(to: pt) } else { path.addLine(to: pt) }
        }
        path.closeSubpath()
        return path
    }
}
