import Foundation

/// The `.modopack` file format: a JSON list of custom icon packs, so packs can be shared.
public enum PackFile {
    public static let formatName = "modo-icon-packs"
    public static let currentVersion = 1
    public static let fileExtension = "modopack"

    public enum PackFileError: Error, Equatable {
        case notAModoPackFile
        case unsupportedVersion(Int)
        case empty
    }

    private struct Envelope: Codable {
        var format: String
        var version: Int
        var packs: [CustomIconPack]
    }

    public static func encode(_ packs: [CustomIconPack]) throws -> Data {
        let encoder = JSONEncoder()
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        return try encoder.encode(Envelope(format: formatName, version: currentVersion, packs: packs))
    }

    /// Decodes and sanitizes a pack file. Imported packs get fresh ids (no clashes with existing
    /// packs), names are trimmed and capped, and every style value is clamped to its valid range.
    public static func decode(_ data: Data) throws -> [CustomIconPack] {
        guard let envelope = try? JSONDecoder().decode(Envelope.self, from: data),
              envelope.format == formatName else { throw PackFileError.notAModoPackFile }
        guard envelope.version <= currentVersion else { throw PackFileError.unsupportedVersion(envelope.version) }
        guard !envelope.packs.isEmpty else { throw PackFileError.empty }
        return envelope.packs.map { pack in
            CustomIconPack(name: String(pack.name.prefix(60)), style: sanitized(pack.style))
        }
    }

    /// Re-runs the clamping initializer: Codable decoding bypasses it.
    static func sanitized(_ style: RoundIconStyle) -> RoundIconStyle {
        RoundIconStyle(
            backgroundStyle: style.backgroundStyle,
            shape: style.shape,
            iconScale: style.iconScale.isFinite ? style.iconScale : 0.68,
            offsetX: style.offsetX.isFinite ? style.offsetX : 0,
            offsetY: style.offsetY.isFinite ? style.offsetY : 0,
            shadowIntensity: style.shadowIntensity.isFinite ? style.shadowIntensity : 0.45,
            innerShadow: style.innerShadow,
            fullBleedSquares: style.fullBleedSquares,
            monochromeTint: style.monochromeTint,
            autoTrim: style.autoTrim,
            customHex: style.customHex
        )
    }
}
