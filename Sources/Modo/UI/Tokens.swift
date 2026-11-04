import AppKit
import SwiftUI

/// Corner radius scale for IconMaker (BAMA design-system).
enum Radius {
    static let xs: CGFloat = 6
    static let sm: CGFloat = 8
    static let row: CGFloat = 10
    static let md: CGFloat = 12
    static let lg: CGFloat = 16
}

/// Curated Google Pixel & Material You surface swatches for Custom Color mode.
struct ColorSwatchToken: Identifiable, Hashable {
    let id: String
    let name: String
    let hex: UInt32

    var color: Color { Color(nsColor: NSColor(hex: hex)) }

    static let presets: [ColorSwatchToken] = [
        .init(id: "ice", name: "Google Ice", hex: 0xE8F0FE),
        .init(id: "mint", name: "Pixel Mint", hex: 0xE6F4EA),
        .init(id: "amber", name: "Warm Amber", hex: 0xFEF7E0),
        .init(id: "rose", name: "Blush Rose", hex: 0xFCE8E6),
        .init(id: "lavender", name: "Material Lavender", hex: 0xF3E8FD),
        .init(id: "slate", name: "Graphite Slate", hex: 0x202124),
        .init(id: "cobalt", name: "Pixel Cobalt", hex: 0x1A73E8),
        .init(id: "emerald", name: "Material Emerald", hex: 0x1E8E3E),
    ]
}

/// Bridges NSColor <-> hex UInt32 and launches the native macOS magnifying glass Eyedropper (NSColorSampler).
@MainActor
enum EyedropperPicker {
    /// Opens the native macOS screen magnifying-glass eyedropper (`NSColorSampler`) and returns the sampled 24-bit hex color.
    static func sampleScreenColor(completion: @escaping @MainActor (UInt32) -> Void) {
        let sampler = NSColorSampler()
        sampler.show { pickedColor in
            guard let pickedColor,
                  let srgb = pickedColor.usingColorSpace(.sRGB) else { return }
            let r = UInt32(clamping: Int((srgb.redComponent * 255.0).rounded()))
            let g = UInt32(clamping: Int((srgb.greenComponent * 255.0).rounded()))
            let b = UInt32(clamping: Int((srgb.blueComponent * 255.0).rounded()))
            let hex = (r << 16) | (g << 8) | b
            Task { @MainActor in
                completion(hex)
            }
        }
    }

    static func hex(from color: Color) -> UInt32 {
        guard let srgb = NSColor(color).usingColorSpace(.sRGB) else { return 0xE8F0FE }
        let r = UInt32(clamping: Int((srgb.redComponent * 255.0).rounded()))
        let g = UInt32(clamping: Int((srgb.greenComponent * 255.0).rounded()))
        let b = UInt32(clamping: Int((srgb.blueComponent * 255.0).rounded()))
        return (r << 16) | (g << 8) | b
    }

    static func color(from hex: UInt32) -> Color {
        Color(nsColor: NSColor(hex: hex))
    }
}
