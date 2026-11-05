import AppKit
import Foundation
import ModoKit

/// Service for generating AI glyphs and downloading neural images.
enum AIGlyphService {

    /// Fetches a neural AI generated image based on a prompt and style.
    static func generateNeuralImage(
        prompt: String,
        style: AIImageStylePreset,
        seed: Int = Int.random(in: 1000...999999)
    ) async throws -> Data {
        guard let url = AIImagePromptBuilder.buildURL(prompt: prompt, style: style, seed: seed) else {
            throw URLError(.badURL)
        }

        var request = URLRequest(url: url)
        request.timeoutInterval = 12.0
        let (data, response) = try await URLSession.shared.data(for: request)

        guard let http = response as? HTTPURLResponse, http.statusCode == 200, !data.isEmpty else {
            throw URLError(.badServerResponse)
        }

        guard NSImage(data: data) != nil else {
            throw URLError(.cannotDecodeContentData)
        }

        return data
    }
}
