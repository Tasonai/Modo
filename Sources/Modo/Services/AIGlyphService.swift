import AppKit
import Foundation
import ImageIO
import ModoKit

/// Service for generating AI glyphs and downloading neural images.
enum AIGlyphService {
    private static let maxImageBytes = 10 * 1024 * 1024

    /// Fetches a neural AI generated image based on a prompt and style.
    static func generateNeuralImage(
        prompt: String,
        style: AIImageStylePreset,
        seed: Int = Int.random(in: 1000...999999)
    ) async throws -> Data {
        guard let url = AIImagePromptBuilder.buildURL(prompt: prompt, style: style, seed: seed),
              url.scheme?.lowercased() == "https" else {
            throw URLError(.badURL)
        }

        var request = URLRequest(url: url)
        request.timeoutInterval = 15.0
        let (data, response) = try await URLSession.shared.data(for: request)

        guard let http = response as? HTTPURLResponse,
              http.statusCode == 200,
              !data.isEmpty,
              data.count <= maxImageBytes else {
            throw URLError(.badServerResponse)
        }

        guard let source = CGImageSourceCreateWithData(data as CFData, nil),
              let uti = CGImageSourceGetType(source) as String?,
              ["public.png", "public.jpeg", "org.webmproject.webp"].contains(uti),
              let props = CGImageSourceCopyPropertiesAtIndex(source, 0, nil) as? [CFString: Any],
              let w = props[kCGImagePropertyPixelWidth] as? Int,
              let h = props[kCGImagePropertyPixelHeight] as? Int,
              (16...4096).contains(w),
              (16...4096).contains(h),
              NSImage(data: data) != nil else {
            throw URLError(.cannotDecodeContentData)
        }

        return data
    }
}
