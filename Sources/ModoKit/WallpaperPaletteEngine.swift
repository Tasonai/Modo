import AppKit
import CoreGraphics
import Foundation

// MARK: - Wallpaper Dynamic Palette Model

public struct WallpaperPalette: Codable, Equatable, Sendable {
    public var primaryHex: UInt32
    public var pastelHex: UInt32
    public var darkHex: UInt32
    public var accentHex: UInt32

    public init(
        primaryHex: UInt32 = 0x2A82E4,
        pastelHex: UInt32 = 0xDCEDFE,
        darkHex: UInt32 = 0x131A26,
        accentHex: UInt32 = 0x60A5FA
    ) {
        self.primaryHex = primaryHex
        self.pastelHex = pastelHex
        self.darkHex = darkHex
        self.accentHex = accentHex
    }
}

// MARK: - Wallpaper Palette Engine

public enum WallpaperPaletteEngine {
    /// Gets the current macOS desktop wallpaper image file URL.
    public static func currentWallpaperURL() -> URL? {
        #if os(macOS)
        let screen = NSScreen.main ?? NSScreen.screens.first
        guard let s = screen else { return nil }
        return NSWorkspace.shared.desktopImageURL(for: s)
        #else
        return nil
        #endif
    }

    /// Loads the current wallpaper and extracts its dynamic palette.
    public static func currentWallpaperPalette() -> WallpaperPalette {
        if let url = currentWallpaperURL(),
           let img = NSImage(contentsOf: url) {
            return extractPalette(from: img)
        }
        return defaultPalette()
    }

    /// Extracts dominant and harmonic pastel/dark colors from any image.
    public static func extractPalette(from image: NSImage) -> WallpaperPalette {
        // Downsample to a small 64x64 grid for instant color clustering
        let targetSize = 64
        guard let tiff = image.tiffRepresentation,
              let rep = NSBitmapImageRep(data: tiff),
              let cgImage = rep.cgImage else {
            return defaultPalette()
        }

        let width = targetSize
        let height = targetSize
        let colorSpace = CGColorSpaceCreateDeviceRGB()
        let bytesPerPixel = 4
        let bytesPerRow = bytesPerPixel * width
        var rawData = [UInt8](repeating: 0, count: bytesPerRow * height)

        guard let ctx = CGContext(
            data: &rawData,
            width: width,
            height: height,
            bitsPerComponent: 8,
            bytesPerRow: bytesPerRow,
            space: colorSpace,
            bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue | CGBitmapInfo.byteOrder32Big.rawValue
        ) else {
            return defaultPalette()
        }

        ctx.draw(cgImage, in: CGRect(x: 0, y: 0, width: width, height: height))

        var totalR: Double = 0
        var totalG: Double = 0
        var totalB: Double = 0
        var totalSamples: Double = 0

        // Calculate average and find most saturated prominent hue
        var maxSat: Double = -1.0
        var bestColor: (r: Double, g: Double, b: Double) = (42, 130, 228)

        for y in 0..<height {
            for x in 0..<width {
                let offset = (y * bytesPerRow) + (x * bytesPerPixel)
                let r = Double(rawData[offset])
                let g = Double(rawData[offset + 1])
                let b = Double(rawData[offset + 2])
                let a = Double(rawData[offset + 3])

                if a < 128 { continue }

                totalR += r
                totalG += g
                totalB += b
                totalSamples += 1.0

                // Saturation = (max - min) / max
                let maxC = max(r, max(g, b))
                let minC = min(r, min(g, b))
                let sat = maxC > 0 ? (maxC - minC) / maxC : 0.0

                // Filter out near-black or blown-out white
                if maxC > 40 && minC < 235 && sat > maxSat {
                    maxSat = sat
                    bestColor = (r, g, b)
                }
            }
        }

        guard totalSamples > 0 else { return defaultPalette() }

        // Blend dominant vibrant with average tone
        let avgR = totalR / totalSamples
        let avgG = totalG / totalSamples
        let avgB = totalB / totalSamples

        let primR = UInt32((bestColor.r * 0.7) + (avgR * 0.3))
        let primG = UInt32((bestColor.g * 0.7) + (avgG * 0.3))
        let primB = UInt32((bestColor.b * 0.7) + (avgB * 0.3))
        let primaryHex = (primR << 16) | (primG << 8) | primB

        // Generate Material You Pastel tint: mix 82% white with 18% primary
        let pastelR = UInt32(Double(primR) * 0.20 + 255.0 * 0.80)
        let pastelG = UInt32(Double(primG) * 0.20 + 255.0 * 0.80)
        let pastelB = UInt32(Double(primB) * 0.20 + 255.0 * 0.80)
        let pastelHex = (pastelR << 16) | (pastelG << 8) | pastelB

        // Generate Dark tone: mix 12% primary with 88% dark obsidian
        let darkR = UInt32(Double(primR) * 0.12 + 15.0 * 0.88)
        let darkG = UInt32(Double(primG) * 0.12 + 18.0 * 0.88)
        let darkB = UInt32(Double(primB) * 0.12 + 25.0 * 0.88)
        let darkHex = (darkR << 16) | (darkG << 8) | darkB

        // Accent tone: high contrast vibrant
        let accR = min(UInt32(Double(primR) * 1.25), 255)
        let accG = min(UInt32(Double(primG) * 1.25), 255)
        let accB = min(UInt32(Double(primB) * 1.25), 255)
        let accentHex = (accR << 16) | (accG << 8) | accB

        return WallpaperPalette(
            primaryHex: primaryHex,
            pastelHex: pastelHex,
            darkHex: darkHex,
            accentHex: accentHex
        )
    }

    public static func defaultPalette() -> WallpaperPalette {
        WallpaperPalette(
            primaryHex: 0x2A82E4,
            pastelHex: 0xDCEDFE,
            darkHex: 0x131A26,
            accentHex: 0x60A5FA
        )
    }
}
