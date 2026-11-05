import CryptoKit
import Foundation
import Testing
@testable import ModoKit

@Test("Semantic version parsing and equality")
func semVerParsing() {
    let v1 = SemanticVersion("1.5.0")
    #expect(v1 != nil)
    #expect(v1?.major == 1)
    #expect(v1?.minor == 5)
    #expect(v1?.patch == 0)
    #expect(v1?.prerelease == nil)

    let vWithV = SemanticVersion("v2.1.4")
    #expect(vWithV?.major == 2)
    #expect(vWithV?.minor == 1)
    #expect(vWithV?.patch == 4)

    let pre = SemanticVersion("1.6.0-beta.1")
    #expect(pre?.major == 1)
    #expect(pre?.minor == 6)
    #expect(pre?.patch == 0)
    #expect(pre?.prerelease == "beta.1")

    let build = SemanticVersion("1.6.0+20261105")
    #expect(build?.major == 1)
    #expect(build?.minor == 6)
    #expect(build?.patch == 0)
    #expect(build?.build == "20261105")

    let preAndBuild = SemanticVersion("1.6.0-rc.2+build.99")
    #expect(preAndBuild?.prerelease == "rc.2")
    #expect(preAndBuild?.build == "build.99")

    #expect(SemanticVersion("") == nil)
    #expect(SemanticVersion("invalid.not.number") == nil)
}

@Test("Semantic version comparisons and ordering")
func semVerComparisons() {
    let v1_5_0 = SemanticVersion("1.5.0")!
    let v1_5_1 = SemanticVersion("1.5.1")!
    let v1_6_0 = SemanticVersion("1.6.0")!
    let v2_0_0 = SemanticVersion("2.0.0")!

    #expect(v1_5_1 > v1_5_0)
    #expect(v1_6_0 > v1_5_1)
    #expect(v2_0_0 > v1_6_0)
    #expect(v1_5_0 == SemanticVersion("1.5.0")!)
    #expect(v1_5_0 < v1_5_1)

    // Two-digit version parsing
    let v1_5 = SemanticVersion("1.5")!
    #expect(v1_5.major == 1 && v1_5.minor == 5 && v1_5.patch == 0)
    #expect(v1_5 == v1_5_0)

    // Prerelease comparison: final release is newer than beta
    let v1_6_beta = SemanticVersion("1.6.0-beta.1")!
    #expect(v1_6_0 > v1_6_beta)
}

@Test("Update manifest decoding and version check")
func manifestDecoding() throws {
    let json = """
    {
      "version": "1.6.0",
      "build": "202611051800",
      "releaseDate": "2026-11-05",
      "title": "Modo 1.6.0 — OTA Updates",
      "releaseNotes": "Added over-the-air auto updates.",
      "minOSVersion": "14.0",
      "isMandatory": false,
      "assets": [
        {
          "name": "Modo-macOS.zip",
          "platform": "macos",
          "format": "zip",
          "downloadURL": "https://example.com/Modo-macOS.zip",
          "sha256": "abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890",
          "sizeBytes": 2900000,
          "isPrimary": true
        },
        {
          "name": "Modo-Windows-Setup.exe",
          "platform": "windows",
          "format": "exe",
          "downloadURL": "https://example.com/Modo-Windows-Setup.exe",
          "sha256": "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef",
          "sizeBytes": 400000,
          "isPrimary": true
        }
      ]
    }
    """

    let data = try #require(json.data(using: .utf8))
    let manifest = try JSONDecoder().decode(UpdateManifest.self, from: data)

    #expect(manifest.version == "1.6.0")
    #expect(manifest.assets.count == 2)

    #expect(OTAUpdateEngine.isUpdateAvailable(manifest: manifest, currentVersion: "1.5.0") == true)
    #expect(OTAUpdateEngine.isUpdateAvailable(manifest: manifest, currentVersion: "1.6.0") == false)
    #expect(OTAUpdateEngine.isUpdateAvailable(manifest: manifest, currentVersion: "2.0.0") == false)

    let macAsset = OTAUpdateEngine.findBestAsset(in: manifest, for: .macos)
    #expect(macAsset?.name == "Modo-macOS.zip")
    #expect(macAsset?.format == .zip)

    let winAsset = OTAUpdateEngine.findBestAsset(in: manifest, for: .windows)
    #expect(winAsset?.name == "Modo-Windows-Setup.exe")
    #expect(winAsset?.format == .exe)
}

@Test("SHA-256 calculation and verification")
func checksumVerification() {
    let testData = "Hello Atoma Modo OTA Updates!".data(using: .utf8)!
    let hash = OTAUpdateEngine.sha256(data: testData)
    #expect(hash.count == 64)

    #expect(OTAUpdateEngine.verifyChecksum(data: testData, expectedSHA256: hash) == true)
    #expect(OTAUpdateEngine.verifyChecksum(data: testData, expectedSHA256: hash.uppercased()) == true)

    let corruptedData = "Tampered data!".data(using: .utf8)!
    #expect(OTAUpdateEngine.verifyChecksum(data: corruptedData, expectedSHA256: hash) == false)
}
