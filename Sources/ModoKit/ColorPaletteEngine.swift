import Foundation

/// Fast, deterministic K-Means color quantization engine to extract
/// a cohesive 5-color BrandPalette from raw image RGBA pixels.
public enum ColorPaletteEngine {
    public struct RGBA: Hashable, Sendable {
        public var r: UInt8
        public var g: UInt8
        public var b: UInt8
        public var a: UInt8

        public init(r: UInt8, g: UInt8, b: UInt8, a: UInt8 = 255) {
            self.r = r
            self.g = g
            self.b = b
            self.a = a
        }

        public var hex: UInt32 {
            (UInt32(r) << 16) | (UInt32(g) << 8) | UInt32(b)
        }

        public var luminance: Double {
            0.299 * Double(r) + 0.587 * Double(g) + 0.114 * Double(b)
        }

        public var saturation: Double {
            let rf = Double(r) / 255.0
            let gf = Double(g) / 255.0
            let bf = Double(b) / 255.0
            let mx = max(rf, gf, bf)
            let mn = min(rf, gf, bf)
            guard mx > 0.001 else { return 0.0 }
            return (mx - mn) / mx
        }

        public func distanceSquared(to other: RGBA) -> Double {
            let dr = Double(r) - Double(other.r)
            let dg = Double(g) - Double(other.g)
            let db = Double(b) - Double(other.b)
            return dr * dr + dg * dg + db * db
        }
    }

    /// Extracts a 5-color structured BrandPalette from an array of RGBA samples.
    public static func extractPalette(from samples: [RGBA], fallbackHex: UInt32 = 0x1A73E8) -> BrandPalette {
        let opaque = samples.filter { $0.a > 30 }
        guard !opaque.isEmpty else {
            return defaultPalette(fallbackHex: fallbackHex)
        }

        // Color quantization: filter extreme black / near-white for primary cluster
        let chromatic = opaque.filter {
            let lum = $0.luminance
            return lum > 15 && lum < 245 && $0.saturation > 0.12
        }

        let candidates = chromatic.isEmpty ? opaque : chromatic

        // Simple bucket-based dominant hue selection
        var buckets: [UInt32: (count: Int, r: Int, g: Int, b: Int)] = [:]
        for c in candidates {
            // Quantize to 4 bits per channel (16 levels)
            let qr = UInt32((c.r >> 4) << 4)
            let qg = UInt32((c.g >> 4) << 4)
            let qb = UInt32((c.b >> 4) << 4)
            let key = (qr << 16) | (qg << 8) | qb
            if var b = buckets[key] {
                b.count += 1
                b.r += Int(c.r)
                b.g += Int(c.g)
                b.b += Int(c.b)
                buckets[key] = b
            } else {
                buckets[key] = (1, Int(c.r), Int(c.g), Int(c.b))
            }
        }

        let sortedBuckets = buckets.values.sorted { $0.count > $1.count }
        guard !sortedBuckets.isEmpty else {
            return defaultPalette(fallbackHex: fallbackHex)
        }

        let primBucket = sortedBuckets[0]
        let pRGBA = RGBA(
            r: UInt8(clamping: primBucket.r / primBucket.count),
            g: UInt8(clamping: primBucket.g / primBucket.count),
            b: UInt8(clamping: primBucket.b / primBucket.count)
        )
        let primaryHex = pRGBA.hex

        // Secondary: bucket with distinct hue / largest color distance
        var secondaryHex = primaryHex
        var maxDist = 0.0
        for b in sortedBuckets.prefix(8) {
            let candRGBA = RGBA(
                r: UInt8(clamping: b.r / b.count),
                g: UInt8(clamping: b.g / b.count),
                b: UInt8(clamping: b.b / b.count)
            )
            let dist = pRGBA.distanceSquared(to: candRGBA)
            if dist > maxDist {
                maxDist = dist
                secondaryHex = candRGBA.hex
            }
        }
        if maxDist < 1200.0 {
            // Shift hue slightly if single color
            secondaryHex = adjustHue(primaryHex, degrees: 35)
        }

        // Background Tint: soft high-key pastel based on primary
        let tintHex = createPastelTint(primaryHex)

        // Deep Tone: dark obsidian shade based on primary
        let deepHex = createDeepTone(primaryHex)

        // Highlight: crisp high-luminance neutral
        let highlightHex: UInt32 = 0xFFFFFF

        return BrandPalette(
            primary: primaryHex,
            secondary: secondaryHex,
            backgroundTint: tintHex,
            deepTone: deepHex,
            highlight: highlightHex
        )
    }

    public static func defaultPalette(fallbackHex: UInt32) -> BrandPalette {
        let prim = fallbackHex & 0xFFFFFF
        return BrandPalette(
            primary: prim,
            secondary: adjustHue(prim, degrees: 35),
            backgroundTint: createPastelTint(prim),
            deepTone: createDeepTone(prim),
            highlight: 0xFFFFFF
        )
    }

    private static func createPastelTint(_ hex: UInt32) -> UInt32 {
        let r = Double((hex >> 16) & 0xFF) / 255.0
        let g = Double((hex >> 8) & 0xFF) / 255.0
        let b = Double(hex & 0xFF) / 255.0
        // Mix with 86% white
        let tr = UInt8(clamping: Int((r * 0.14 + 0.86) * 255.0))
        let tg = UInt8(clamping: Int((g * 0.14 + 0.86) * 255.0))
        let tb = UInt8(clamping: Int((b * 0.14 + 0.86) * 255.0))
        return (UInt32(tr) << 16) | (UInt32(tg) << 8) | UInt32(tb)
    }

    private static func createDeepTone(_ hex: UInt32) -> UInt32 {
        let r = Double((hex >> 16) & 0xFF) / 255.0
        let g = Double((hex >> 8) & 0xFF) / 255.0
        let b = Double(hex & 0xFF) / 255.0
        // Darken to 22%
        let dr = UInt8(clamping: Int((r * 0.22) * 255.0))
        let dg = UInt8(clamping: Int((g * 0.22) * 255.0))
        let db = UInt8(clamping: Int((b * 0.22) * 255.0))
        return (UInt32(dr) << 16) | (UInt32(dg) << 8) | UInt32(db)
    }

    private static func adjustHue(_ hex: UInt32, degrees: Double) -> UInt32 {
        let r = Double((hex >> 16) & 0xFF) / 255.0
        let g = Double((hex >> 8) & 0xFF) / 255.0
        let b = Double(hex & 0xFF) / 255.0

        let maxVal = max(r, g, b)
        let minVal = min(r, g, b)
        let delta = maxVal - minVal

        var h: Double = 0.0
        let s: Double = maxVal > 0.0 ? delta / maxVal : 0.0
        let v: Double = maxVal

        if delta > 0.001 {
            if maxVal == r {
                h = 60.0 * ((g - b) / delta).truncatingRemainder(dividingBy: 6.0)
            } else if maxVal == g {
                h = 60.0 * ((b - r) / delta + 2.0)
            } else {
                h = 60.0 * ((r - g) / delta + 4.0)
            }
        }
        if h < 0 { h += 360.0 }
        h = (h + degrees).truncatingRemainder(dividingBy: 360.0)

        // Convert HSV back to RGB
        let c = v * s
        let x = c * (1.0 - abs((h / 60.0).truncatingRemainder(dividingBy: 2.0) - 1.0))
        let m = v - c

        var (rf, gf, bf) = (0.0, 0.0, 0.0)
        switch Int(h / 60.0) % 6 {
        case 0: (rf, gf, bf) = (c, x, 0)
        case 1: (rf, gf, bf) = (x, c, 0)
        case 2: (rf, gf, bf) = (0, c, x)
        case 3: (rf, gf, bf) = (0, x, c)
        case 4: (rf, gf, bf) = (x, 0, c)
        default: (rf, gf, bf) = (c, 0, x)
        }

        let ur = UInt8(clamping: Int((rf + m) * 255.0))
        let ug = UInt8(clamping: Int((gf + m) * 255.0))
        let ub = UInt8(clamping: Int((bf + m) * 255.0))
        return (UInt32(ur) << 16) | (UInt32(ug) << 8) | UInt32(ub)
    }
}
