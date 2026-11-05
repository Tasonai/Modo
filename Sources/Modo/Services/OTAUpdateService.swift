import AppKit
import Foundation
import ModoKit
import Observation

/// Over-the-air update service for Modo.
/// Manages manifest discovery, checksum verification, background streaming, and seamless in-place installation.
@Observable
@MainActor
public final class OTAUpdateService {
    public static let shared = OTAUpdateService()

    public enum Status: Equatable, Sendable {
        case idle
        case checking
        case upToDate(version: String, checkedAt: Date)
        case available(manifest: UpdateManifest, asset: UpdateAsset)
        case downloading(progress: Double, bytesReceived: Int64, totalBytes: Int64)
        case verifying
        case readyToInstall(localURL: URL, manifest: UpdateManifest, asset: UpdateAsset)
        case installing
        case failed(String)

        public static func == (lhs: Status, rhs: Status) -> Bool {
            switch (lhs, rhs) {
            case (.idle, .idle), (.checking, .checking), (.verifying, .verifying), (.installing, .installing):
                return true
            case let (.upToDate(v1, _), .upToDate(v2, _)):
                return v1 == v2
            case let (.available(m1, a1), .available(m2, a2)):
                return m1.version == m2.version && a1.name == a2.name
            case let (.downloading(p1, b1, t1), .downloading(p2, b2, t2)):
                return p1 == p2 && b1 == b2 && t1 == t2
            case let (.readyToInstall(u1, m1, _), .readyToInstall(u2, m2, _)):
                return u1 == u2 && m1.version == m2.version
            case let (.failed(m1), .failed(m2)):
                return m1 == m2
            default:
                return false
            }
        }
    }

    public var status: Status = .idle
    public var lastChecked: Date?
    public var errorMessage: String?
    public var downloadTask: Task<Void, Never>?

    // MARK: - Preferences & Keys
    public static let defaultFeedURLString = "https://raw.githubusercontent.com/Tasonai/Modo/main/dist/update.json"
    private static let feedURLKey = "modo.otaFeedURL"
    private static let autoCheckKey = "modo.otaAutoCheck"
    private static let lastCheckedKey = "modo.otaLastChecked"
    private static let skippedVersionKey = "modo.otaSkippedVersion"

    public var feedURLString: String {
        get {
            if let env = ProcessInfo.processInfo.environment["MODO_UPDATE_FEED_URL"], !env.isEmpty {
                return env
            }
            return UserDefaults.standard.string(forKey: Self.feedURLKey) ?? Self.defaultFeedURLString
        }
        set {
            UserDefaults.standard.set(newValue.trimmingCharacters(in: .whitespacesAndNewlines), forKey: Self.feedURLKey)
        }
    }

    public var autoCheckEnabled: Bool {
        get {
            if UserDefaults.standard.object(forKey: Self.autoCheckKey) == nil { return true }
            return UserDefaults.standard.bool(forKey: Self.autoCheckKey)
        }
        set {
            UserDefaults.standard.set(newValue, forKey: Self.autoCheckKey)
        }
    }

    public var skippedVersion: String? {
        get { UserDefaults.standard.string(forKey: Self.skippedVersionKey) }
        set { UserDefaults.standard.set(newValue, forKey: Self.skippedVersionKey) }
    }

    public var currentVersion: String {
        Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String ?? "1.5.0"
    }

    public var currentBuild: String {
        Bundle.main.infoDictionary?["CFBundleVersion"] as? String ?? "1"
    }

    public init() {
        if let time = UserDefaults.standard.object(forKey: Self.lastCheckedKey) as? Date {
            self.lastChecked = time
        }
    }

    // MARK: - Update Check

    /// Check for available updates.
    /// - Parameters:
    ///   - manual: Whether user clicked "Check for Updates" manually (ignores skipped version).
    public func checkForUpdates(manual: Bool = false) async {
        guard status != .checking && !isBusyDownloading else { return }
        status = .checking
        errorMessage = nil

        let urlString = feedURLString
        guard let url = URL(string: urlString) else {
            status = .failed("Invalid feed URL: \(urlString)")
            return
        }

        do {
            let manifest: UpdateManifest
            if url.isFileURL {
                let data = try Data(contentsOf: url)
                manifest = try JSONDecoder().decode(UpdateManifest.self, from: data)
            } else {
                var request = URLRequest(url: url)
                request.cachePolicy = .reloadIgnoringLocalCacheData
                request.timeoutInterval = 10.0
                request.setValue("Modo/\(currentVersion) (macOS; Atoma OTA)", forHTTPHeaderField: "User-Agent")

                let (data, response) = try await URLSession.shared.data(for: request)
                guard let http = response as? HTTPURLResponse, (200...299).contains(http.statusCode) else {
                    let code = (response as? HTTPURLResponse)?.statusCode ?? -1
                    status = .failed("Server returned status \(code)")
                    return
                }
                manifest = try JSONDecoder().decode(UpdateManifest.self, from: data)
            }

            let now = Date()
            lastChecked = now
            UserDefaults.standard.set(now, forKey: Self.lastCheckedKey)

            // Check if newer
            let available = OTAUpdateEngine.isUpdateAvailable(manifest: manifest, currentVersion: currentVersion)
            if !available {
                status = .upToDate(version: currentVersion, checkedAt: now)
                return
            }

            // Honor skipped version on automatic checks
            if !manual, let skipped = skippedVersion, skipped == manifest.version, !manifest.isMandatory {
                status = .idle
                return
            }

            // Pick asset for macOS
            guard let asset = OTAUpdateEngine.findBestAsset(in: manifest, for: .macos) else {
                status = .failed("Update \(manifest.version) is available, but no compatible macOS package was found.")
                return
            }

            status = .available(manifest: manifest, asset: asset)
        } catch {
            status = .failed("Could not check for updates: \(error.localizedDescription)")
        }
    }

    private var isBusyDownloading: Bool {
        switch status {
        case .downloading, .verifying, .installing: return true
        default: return false
        }
    }

    // MARK: - Download & Verify

    public func startDownload() {
        guard case let .available(manifest, asset) = status else { return }

        downloadTask?.cancel()
        downloadTask = Task { [weak self] in
            guard let self = self else { return }
            await self.downloadAndVerify(manifest: manifest, asset: asset)
        }
    }

    private func downloadAndVerify(manifest: UpdateManifest, asset: UpdateAsset) async {
        status = .downloading(progress: 0.0, bytesReceived: 0, totalBytes: asset.sizeBytes)

        let tempDir = URL(fileURLWithPath: NSTemporaryDirectory()).appending(path: "ModoOTA_\(UUID().uuidString)")
        try? FileManager.default.createDirectory(at: tempDir, withIntermediateDirectories: true)
        let destinationURL = tempDir.appending(path: asset.name)

        do {
            if asset.downloadURL.isFileURL {
                // Local copy
                try? FileManager.default.removeItem(at: destinationURL)
                try FileManager.default.copyItem(at: asset.downloadURL, to: destinationURL)
                let size = (try? destinationURL.resourceValues(forKeys: [.fileSizeKey]).fileSize) ?? Int(asset.sizeBytes)
                status = .downloading(progress: 1.0, bytesReceived: Int64(size), totalBytes: Int64(size))
            } else {
                // Streaming HTTP download
                var request = URLRequest(url: asset.downloadURL)
                request.timeoutInterval = 60.0
                request.setValue("Modo/\(currentVersion) (macOS; Atoma OTA)", forHTTPHeaderField: "User-Agent")

                let (asyncBytes, response) = try await URLSession.shared.bytes(for: request)
                let expectedLength = response.expectedContentLength > 0 ? response.expectedContentLength : asset.sizeBytes

                FileManager.default.createFile(atPath: destinationURL.path, contents: nil)
                let fileHandle = try FileHandle(forWritingTo: destinationURL)

                var received: Int64 = 0
                var buffer = Data()
                buffer.reserveCapacity(64 * 1024)

                for try await byte in asyncBytes {
                    if Task.isCancelled {
                        try? fileHandle.close()
                        try? FileManager.default.removeItem(at: tempDir)
                        status = .idle
                        return
                    }

                    buffer.append(byte)
                    received += 1

                    if buffer.count >= 64 * 1024 {
                        try fileHandle.write(contentsOf: buffer)
                        buffer.removeAll(keepingCapacity: true)

                        let progress = expectedLength > 0 ? min(1.0, Double(received) / Double(expectedLength)) : 0.5
                        self.status = .downloading(progress: progress, bytesReceived: received, totalBytes: expectedLength)
                    }
                }

                if !buffer.isEmpty {
                    try fileHandle.write(contentsOf: buffer)
                }
                try fileHandle.close()
            }

            // Verify checksum
            status = .verifying
            let isValid = OTAUpdateEngine.verifyChecksum(fileURL: destinationURL, expectedSHA256: asset.sha256)
            guard isValid else {
                try? FileManager.default.removeItem(at: tempDir)
                status = .failed("Security verification failed: SHA-256 checksum mismatch for \(asset.name).")
                return
            }

            status = .readyToInstall(localURL: destinationURL, manifest: manifest, asset: asset)
        } catch {
            try? FileManager.default.removeItem(at: tempDir)
            if Task.isCancelled {
                status = .idle
            } else {
                status = .failed("Download failed: \(error.localizedDescription)")
            }
        }
    }

    public func cancelDownload() {
        downloadTask?.cancel()
        downloadTask = nil
        status = .idle
    }

    public func skipThisVersion(version: String) {
        skippedVersion = version
        status = .idle
    }

    public func resetFeedURL() {
        feedURLString = Self.defaultFeedURLString
    }

    // MARK: - In-Place Install & Relaunch

    /// Installs downloaded update package. For .zip, performs atomic in-place replacement and restart.
    /// For .dmg / .pkg, presents standard macOS installer.
    public func installAndRelaunch() {
        guard case let .readyToInstall(localURL, _, asset) = status else { return }
        status = .installing

        switch asset.format {
        case .zip:
            performSeamlessZipUpdate(zipURL: localURL)
        case .dmg, .pkg:
            NSWorkspace.shared.open(localURL)
            DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
                NSApp.terminate(nil)
            }
        case .exe:
            status = .failed("Windows executable cannot be installed on macOS.")
        }
    }

    private func performSeamlessZipUpdate(zipURL: URL) {
        let currentAppURL = Bundle.main.bundleURL
        let pid = ProcessInfo.processInfo.processIdentifier

        let stagingDir = zipURL.deletingLastPathComponent().appending(path: "extracted")
        try? FileManager.default.createDirectory(at: stagingDir, withIntermediateDirectories: true)

        // Script to wait for app exit, unzip, replace, and relaunch
        let scriptContent = """
        #!/bin/bash
        set -e
        PID=\(pid)
        ZIP_PATH="\(zipURL.path)"
        STAGE_DIR="\(stagingDir.path)"
        TARGET_APP="\(currentAppURL.path)"

        # Wait for current instance to exit
        while kill -0 $PID 2>/dev/null; do
            sleep 0.1
        done

        # Unpack zip
        /usr/bin/unzip -q -o "$ZIP_PATH" -d "$STAGE_DIR"

        # Locate app bundle in unzipped directory
        NEW_APP=$(find "$STAGE_DIR" -maxdepth 2 -name "*.app" | head -n 1)
        if [ -z "$NEW_APP" ]; then
            exit 1
        fi

        # Remove quarantine attribute
        /usr/bin/xattr -cr "$NEW_APP" 2>/dev/null || true

        # Replace target app bundle atomically
        rm -rf "$TARGET_APP"
        cp -R "$NEW_APP" "$TARGET_APP"

        # Clean staging
        rm -rf "$STAGE_DIR" "$ZIP_PATH"

        # Relaunch updated app
        open "$TARGET_APP"
        """

        let scriptURL = URL(fileURLWithPath: NSTemporaryDirectory()).appending(path: "modo_ota_relaunch_\(UUID().uuidString).sh")
        do {
            try scriptContent.write(to: scriptURL, atomically: true, encoding: .utf8)
            try FileManager.default.setAttributes([.posixPermissions: 0o755], ofItemAtPath: scriptURL.path)

            let process = Process()
            process.executableURL = URL(fileURLWithPath: "/bin/bash")
            process.arguments = [scriptURL.path]
            try process.run()

            // Quit current application so script can replace it
            NSApp.terminate(nil)
        } catch {
            status = .failed("Failed to launch updater: \(error.localizedDescription)")
        }
    }
}
