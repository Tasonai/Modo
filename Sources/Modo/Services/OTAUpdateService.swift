import AppKit
import Foundation
import ModoKit
import Observation

/// Over-the-air update service for Modo.
/// Manages manifest discovery, SHA-256 & signature verification, off-main-thread download, and atomic rollback-safe installation.
@Observable
@MainActor
public final class OTAUpdateService {
    public static let shared = OTAUpdateService()

    private static let maxDownloadBytes: Int64 = 250 * 1024 * 1024

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
            if Store.isOverridden,
               let env = ProcessInfo.processInfo.environment["MODO_UPDATE_FEED_URL"],
               !env.isEmpty {
                return env
            }
            if let custom = UserDefaults.standard.string(forKey: Self.feedURLKey),
               let parsed = URL(string: custom),
               Self.isAllowedFeedOrAssetURL(parsed) {
                return custom
            }
            return Self.defaultFeedURLString
        }
        set {
            let trimmed = newValue.trimmingCharacters(in: .whitespacesAndNewlines)
            if let parsed = URL(string: trimmed), Self.isAllowedFeedOrAssetURL(parsed) {
                UserDefaults.standard.set(trimmed, forKey: Self.feedURLKey)
            } else if trimmed.isEmpty {
                UserDefaults.standard.removeObject(forKey: Self.feedURLKey)
            }
        }
    }

    private static func isAllowedFeedOrAssetURL(_ url: URL) -> Bool {
        guard let scheme = url.scheme?.lowercased() else { return false }
        if scheme == "https" {
            return url.host != nil
        }
        if scheme == "file" {
            return Store.isOverridden
        }
        return false
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
        Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String ?? "1.7.0"
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

    public func checkForUpdates(manual: Bool = false) async {
        guard status != .checking && !isBusyDownloading else { return }
        status = .checking
        errorMessage = nil

        let urlString = feedURLString
        guard let url = URL(string: urlString), Self.isAllowedFeedOrAssetURL(url) else {
            status = .failed("Insecure or invalid update feed URL.")
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
                guard data.count <= 2 * 1024 * 1024 else {
                    status = .failed("Update manifest exceeds size limit.")
                    return
                }
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

            let available = OTAUpdateEngine.isUpdateAvailable(manifest: manifest, currentVersion: currentVersion)
            if !available {
                status = .upToDate(version: currentVersion, checkedAt: now)
                return
            }

            if !manual, let skipped = skippedVersion, skipped == manifest.version, !manifest.isMandatory {
                status = .idle
                return
            }

            guard let asset = OTAUpdateEngine.findBestAsset(in: manifest, for: .macos) else {
                status = .failed("Update \(manifest.version) is available, but no compatible macOS package was found.")
                return
            }

            guard Self.isAllowedFeedOrAssetURL(asset.downloadURL),
                  asset.sha256.trimmingCharacters(in: .whitespacesAndNewlines).count == 64 else {
                status = .failed("Update package metadata failed security validation (HTTPS and SHA-256 required).")
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

    // MARK: - Off-Main-Thread Download & SHA-256 Verification

    public func startDownload() {
        guard case let .available(manifest, asset) = status else { return }

        downloadTask?.cancel()
        downloadTask = Task { [weak self] in
            guard let self else { return }
            await self.downloadAndVerify(manifest: manifest, asset: asset)
        }
    }

    private func downloadAndVerify(manifest: UpdateManifest, asset: UpdateAsset) async {
        guard Self.isAllowedFeedOrAssetURL(asset.downloadURL) else {
            status = .failed("Blocked insecure update download URL.")
            return
        }

        status = .downloading(progress: 0.0, bytesReceived: 0, totalBytes: asset.sizeBytes)

        let tempDir = URL(fileURLWithPath: NSTemporaryDirectory()).appending(path: "ModoOTA_\(UUID().uuidString)", directoryHint: .isDirectory)
        let safeExt: String = switch asset.format {
        case .zip: "zip"
        case .dmg: "dmg"
        case .pkg: "pkg"
        case .exe: "exe"
        }
        let destinationURL = tempDir.appending(path: "Modo-Update.\(safeExt)")

        do {
            try FileManager.default.createDirectory(at: tempDir, withIntermediateDirectories: true)

            if asset.downloadURL.isFileURL {
                try FileManager.default.copyItem(at: asset.downloadURL, to: destinationURL)
                let size = Int64((try? destinationURL.resourceValues(forKeys: [.fileSizeKey]).fileSize) ?? 0)
                guard size > 0, size <= Self.maxDownloadBytes else {
                    try? FileManager.default.removeItem(at: tempDir)
                    status = .failed("Downloaded file size is invalid.")
                    return
                }
                status = .downloading(progress: 1.0, bytesReceived: size, totalBytes: size)
            } else {
                var request = URLRequest(url: asset.downloadURL)
                request.timeoutInterval = 120.0
                request.setValue("Modo/\(currentVersion) (macOS; Atoma OTA)", forHTTPHeaderField: "User-Agent")

                try await performStreamingFileDownload(
                    request: request,
                    destinationURL: destinationURL,
                    fallbackExpectedBytes: asset.sizeBytes
                )
            }

            status = .verifying
            let expectedHash = asset.sha256
            let isValid = await Task.detached(priority: .userInitiated) {
                OTAUpdateEngine.verifyChecksum(fileURL: destinationURL, expectedSHA256: expectedHash)
            }.value

            guard isValid else {
                try? FileManager.default.removeItem(at: tempDir)
                status = .failed("Security verification failed: SHA-256 checksum mismatch.")
                return
            }

            status = .readyToInstall(localURL: destinationURL, manifest: manifest, asset: asset)
        } catch {
            try? FileManager.default.removeItem(at: tempDir)
            if Task.isCancelled || (error as? URLError)?.code == .cancelled {
                status = .idle
            } else {
                status = .failed("Download failed: \(error.localizedDescription)")
            }
        }
    }

    private func performStreamingFileDownload(
        request: URLRequest,
        destinationURL: URL,
        fallbackExpectedBytes: Int64
    ) async throws {
        let delegate = DownloadTaskBridge(
            destinationURL: destinationURL,
            maxBytes: Self.maxDownloadBytes,
            fallbackExpectedBytes: fallbackExpectedBytes
        ) { [weak self] progress, written, expected in
            Task { @MainActor [weak self] in
                guard let self, case .downloading = self.status else { return }
                self.status = .downloading(progress: progress, bytesReceived: written, totalBytes: expected)
            }
        }

        let session = URLSession(configuration: .ephemeral, delegate: delegate, delegateQueue: nil)
        defer { session.finishTasksAndInvalidate() }

        try await withTaskCancellationHandler {
            try await delegate.start(with: session, request: request)
        } onCancel: {
            delegate.cancel()
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
        UserDefaults.standard.removeObject(forKey: Self.feedURLKey)
    }

    // MARK: - Verified In-Place Install & Rollback-Safe Swap

    public func installAndRelaunch() {
        guard case let .readyToInstall(localURL, _, asset) = status else { return }
        status = .installing

        switch asset.format {
        case .zip:
            Task {
                await performVerifiedZipUpdate(zipURL: localURL)
            }
        case .dmg, .pkg:
            NSWorkspace.shared.open(localURL)
            DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
                NSApp.terminate(nil)
            }
        case .exe:
            status = .failed("Windows executable cannot be installed on macOS.")
        }
    }

    private nonisolated static func stageAndVerifyZip(
        zipURL: URL,
        stagingDir: URL,
        expectedBundleID: String?
    ) throws -> URL {
        let fm = FileManager.default
        try? fm.removeItem(at: stagingDir)
        try fm.createDirectory(at: stagingDir, withIntermediateDirectories: true)

        let ditto = Process()
        ditto.executableURL = URL(fileURLWithPath: "/usr/bin/ditto")
        ditto.arguments = ["-x", "-k", zipURL.path, stagingDir.path]
        try ditto.run()
        ditto.waitUntilExit()
        guard ditto.terminationStatus == 0 else {
            throw NSError(domain: "ModoOTA", code: 1, userInfo: [NSLocalizedDescriptionKey: "Archive extraction failed."])
        }

        // Ensure no path escapes stagingDir and find top-level .app bundle
        let basePrefix = stagingDir.standardizedFileURL.path + "/"
        var candidateApp: URL?
        if let enumerator = fm.enumerator(at: stagingDir, includingPropertiesForKeys: [.isSymbolicLinkKey, .isDirectoryKey]) {
            for case let itemURL as URL in enumerator {
                let resolved = itemURL.standardizedFileURL.path
                guard resolved.hasPrefix(basePrefix) else {
                    throw NSError(domain: "ModoOTA", code: 2, userInfo: [NSLocalizedDescriptionKey: "Unsafe archive path detected."])
                }
                if itemURL.pathExtension == "app", candidateApp == nil {
                    candidateApp = itemURL
                }
            }
        }

        guard let newAppURL = candidateApp else {
            throw NSError(domain: "ModoOTA", code: 3, userInfo: [NSLocalizedDescriptionKey: "No .app bundle found inside update archive."])
        }

        if let expectedID = expectedBundleID,
           let newBundleID = Bundle(url: newAppURL)?.bundleIdentifier,
           newBundleID != expectedID {
            throw NSError(domain: "ModoOTA", code: 4, userInfo: [NSLocalizedDescriptionKey: "Bundle identifier mismatch (\(newBundleID))."])
        }

        // Verify macOS code signature before touching quarantine attributes
        let codesign = Process()
        codesign.executableURL = URL(fileURLWithPath: "/usr/bin/codesign")
        codesign.arguments = ["--verify", "--deep", "--strict", newAppURL.path]
        codesign.standardError = FileHandle.nullDevice
        try codesign.run()
        codesign.waitUntilExit()
        guard codesign.terminationStatus == 0 else {
            throw NSError(domain: "ModoOTA", code: 5, userInfo: [NSLocalizedDescriptionKey: "Update bundle failed macOS code signature verification."])
        }

        // Clear quarantine only after SHA-256 and codesign verification succeed
        let xattr = Process()
        xattr.executableURL = URL(fileURLWithPath: "/usr/bin/xattr")
        xattr.arguments = ["-cr", newAppURL.path]
        xattr.standardError = FileHandle.nullDevice
        try? xattr.run()
        xattr.waitUntilExit()

        return newAppURL
    }

    private func performVerifiedZipUpdate(zipURL: URL) async {
        let currentAppURL = Bundle.main.bundleURL.standardizedFileURL
        let expectedBundleID = Bundle.main.bundleIdentifier
        let pid = ProcessInfo.processInfo.processIdentifier
        let workDir = zipURL.deletingLastPathComponent().standardizedFileURL
        let stagingDir = workDir.appending(path: "extracted", directoryHint: .isDirectory)

        do {
            // 1. Extract and verify bundle signature off the main thread BEFORE terminating the running app.
            let verifiedNewAppURL = try await Task.detached(priority: .userInitiated) { () throws -> URL in
                try Self.stageAndVerifyZip(zipURL: zipURL, stagingDir: stagingDir, expectedBundleID: expectedBundleID)
            }.value

            // 2. Check write access to destination parent directory before quitting
            let parentDir = currentAppURL.deletingLastPathComponent().path
            if !FileManager.default.isWritableFile(atPath: parentDir) {
                status = .failed("Cannot write to \(parentDir). Move Modo.app to ~/Applications or install via the .pkg installer.")
                return
            }

            // 3. Launch rollback-safe swapper passing all paths strictly via environment variables (zero shell interpolation)
            let swapScript = """
            while kill -0 "$MODO_PID" 2>/dev/null; do
                sleep 0.1
            done
            BACKUP_APP="${MODO_TARGET_APP}.ota-backup"
            rm -rf "$BACKUP_APP"
            if [ -e "$MODO_TARGET_APP" ]; then
                mv "$MODO_TARGET_APP" "$BACKUP_APP" || exit 1
            fi
            if mv "$MODO_NEW_APP" "$MODO_TARGET_APP"; then
                rm -rf "$BACKUP_APP" "$MODO_WORK_DIR"
                open "$MODO_TARGET_APP"
            else
                if [ -e "$BACKUP_APP" ]; then
                    mv "$BACKUP_APP" "$MODO_TARGET_APP"
                fi
                exit 1
            fi
            """

            let process = Process()
            process.executableURL = URL(fileURLWithPath: "/bin/sh")
            process.arguments = ["-c", swapScript]
            var env = ProcessInfo.processInfo.environment
            env["MODO_PID"] = String(pid)
            env["MODO_NEW_APP"] = verifiedNewAppURL.path
            env["MODO_TARGET_APP"] = currentAppURL.path
            env["MODO_WORK_DIR"] = workDir.path
            process.environment = env
            try process.run()

            NSApp.terminate(nil)
        } catch {
            try? FileManager.default.removeItem(at: workDir)
            status = .failed("Failed to verify or install update: \(error.localizedDescription)")
        }
    }
}

private nonisolated final class DownloadTaskBridge: NSObject, URLSessionDownloadDelegate, @unchecked Sendable {
    private let destinationURL: URL
    private let maxBytes: Int64
    private let fallbackExpectedBytes: Int64
    private let onProgress: @Sendable (Double, Int64, Int64) -> Void

    private let lock = NSLock()
    private var continuation: CheckedContinuation<Void, Error>?
    private var task: URLSessionDownloadTask?
    private var lastReportedUptime: TimeInterval = 0
    private var finished = false

    init(
        destinationURL: URL,
        maxBytes: Int64,
        fallbackExpectedBytes: Int64,
        onProgress: @escaping @Sendable (Double, Int64, Int64) -> Void
    ) {
        self.destinationURL = destinationURL
        self.maxBytes = maxBytes
        self.fallbackExpectedBytes = fallbackExpectedBytes
        self.onProgress = onProgress
    }

    func start(with session: URLSession, request: URLRequest) async throws {
        try await withCheckedThrowingContinuation { (cont: CheckedContinuation<Void, Error>) in
            lock.lock()
            self.continuation = cont
            let dlTask = session.downloadTask(with: request)
            self.task = dlTask
            lock.unlock()
            dlTask.resume()
        }
    }

    func cancel() {
        lock.lock()
        let dlTask = task
        lock.unlock()
        dlTask?.cancel()
    }

    private func complete(with result: Result<Void, Error>) {
        lock.lock()
        guard !finished, let cont = continuation else {
            lock.unlock()
            return
        }
        finished = true
        continuation = nil
        lock.unlock()
        cont.resume(with: result)
    }

    func urlSession(
        _ session: URLSession,
        downloadTask: URLSessionDownloadTask,
        didWriteData bytesWritten: Int64,
        totalBytesWritten: Int64,
        totalBytesExpectedToWrite: Int64
    ) {
        if totalBytesWritten > maxBytes {
            downloadTask.cancel()
            complete(with: .failure(NSError(domain: "ModoOTA", code: 10, userInfo: [NSLocalizedDescriptionKey: "Update package exceeds maximum allowed size."])))
            return
        }
        let now = ProcessInfo.processInfo.systemUptime
        lock.lock()
        let shouldNotify = (now - lastReportedUptime) >= 0.08
        if shouldNotify { lastReportedUptime = now }
        lock.unlock()

        if shouldNotify {
            let expected = totalBytesExpectedToWrite > 0 ? totalBytesExpectedToWrite : fallbackExpectedBytes
            let progress = expected > 0 ? min(1.0, Double(totalBytesWritten) / Double(expected)) : 0.5
            onProgress(progress, totalBytesWritten, expected)
        }
    }

    func urlSession(
        _ session: URLSession,
        downloadTask: URLSessionDownloadTask,
        didFinishDownloadingTo location: URL
    ) {
        if let http = downloadTask.response as? HTTPURLResponse, !(200...299).contains(http.statusCode) {
            complete(with: .failure(NSError(domain: "ModoOTA", code: http.statusCode, userInfo: [NSLocalizedDescriptionKey: "HTTP status \(http.statusCode)"])))
            return
        }
        let size = Int64((try? location.resourceValues(forKeys: [.fileSizeKey]).fileSize) ?? 0)
        if size <= 0 || size > maxBytes {
            complete(with: .failure(NSError(domain: "ModoOTA", code: 11, userInfo: [NSLocalizedDescriptionKey: "Invalid downloaded package size."])))
            return
        }
        do {
            try? FileManager.default.removeItem(at: destinationURL)
            try FileManager.default.moveItem(at: location, to: destinationURL)
            onProgress(1.0, size, size)
            complete(with: .success(()))
        } catch {
            complete(with: .failure(error))
        }
    }

    func urlSession(_ session: URLSession, task: URLSessionTask, didCompleteWithError error: Error?) {
        if let error {
            complete(with: .failure(error))
        }
    }
}
