import ModoKit
import SwiftUI

/// Sheet modal presented when an update is found or manually checked.
struct OTAUpdateSheetView: View {
    @Environment(\.dismiss) private var dismiss
    @State private var ota = OTAUpdateService.shared

    var body: some View {
        VStack(spacing: 0) {
            header
                .padding(.horizontal, 24)
                .padding(.top, 24)
                .padding(.bottom, 16)

            Divider()

            contentBody
                .padding(24)

            Divider()

            actionsFooter
                .padding(.horizontal, 24)
                .padding(.vertical, 16)
                .background(Color(nsColor: .windowBackgroundColor))
        }
        .frame(width: 480)
        .background(Color(nsColor: .controlBackgroundColor))
    }

    // MARK: - Subviews

    private var header: some View {
        HStack(spacing: 16) {
            ZStack {
                RoundedRectangle(cornerRadius: 12, style: .continuous)
                    .fill(
                        LinearGradient(
                            colors: [Palette.accent, Palette.accent.opacity(0.7)],
                            startPoint: .topLeading,
                            endPoint: .bottomTrailing
                        )
                    )
                    .frame(width: 52, height: 52)
                    .shadow(color: Palette.accent.opacity(0.3), radius: 6, x: 0, y: 3)

                Image(systemName: "arrow.triangle.2.circlepath")
                    .font(.system(size: 24, weight: .semibold))
                    .foregroundStyle(.white)
            }

            VStack(alignment: .leading, spacing: 4) {
                Text("Modo Software Update")
                    .font(.headline)

                Text("Over-the-Air Auto-Update Engine")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            Spacer()

            statusBadge
        }
    }

    @ViewBuilder
    private var statusBadge: some View {
        switch ota.status {
        case .checking:
            HStack(spacing: 6) {
                ProgressView().controlSize(.small)
                Text("Checking…").font(.caption).foregroundStyle(.secondary)
            }
        case .upToDate:
            Label("Up to Date", systemImage: "checkmark.circle.fill")
                .font(.caption.weight(.medium))
                .foregroundStyle(.green)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(Color.green.opacity(0.12), in: Capsule())
        case .available:
            Label("New Version", systemImage: "sparkles")
                .font(.caption.weight(.medium))
                .foregroundStyle(Palette.accent)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(Palette.accent.opacity(0.12), in: Capsule())
        case .downloading:
            Label("Downloading", systemImage: "arrow.down.circle")
                .font(.caption.weight(.medium))
                .foregroundStyle(Palette.accent)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(Palette.accent.opacity(0.12), in: Capsule())
        case .verifying:
            Label("Verifying SHA-256", systemImage: "lock.shield")
                .font(.caption.weight(.medium))
                .foregroundStyle(.orange)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(Color.orange.opacity(0.12), in: Capsule())
        case .readyToInstall:
            Label("Ready to Install", systemImage: "checkmark.seal.fill")
                .font(.caption.weight(.medium))
                .foregroundStyle(.green)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(Color.green.opacity(0.12), in: Capsule())
        case .installing:
            Label("Installing…", systemImage: "arrow.clockwise")
                .font(.caption.weight(.medium))
                .foregroundStyle(Palette.accent)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(Palette.accent.opacity(0.12), in: Capsule())
        case .failed:
            Label("Error", systemImage: "exclamationmark.triangle.fill")
                .font(.caption.weight(.medium))
                .foregroundStyle(.red)
                .padding(.horizontal, 8)
                .padding(.vertical, 4)
                .background(Color.red.opacity(0.12), in: Capsule())
        case .idle:
            EmptyView()
        }
    }

    @ViewBuilder
    private var contentBody: some View {
        switch ota.status {
        case .idle, .checking:
            VStack(spacing: 16) {
                ProgressView()
                    .controlSize(.large)
                    .padding()
                Text("Connecting to update channel…")
                    .font(.callout)
                    .foregroundStyle(.secondary)
            }
            .frame(maxWidth: .infinity, minHeight: 180)

        case let .upToDate(version, checkedAt):
            VStack(spacing: 14) {
                Image(systemName: "checkmark.circle.fill")
                    .font(.system(size: 48))
                    .foregroundStyle(.green)

                Text("Modo is Up to Date")
                    .font(.title3.weight(.semibold))

                Text("Version \(version) is currently the newest version available.")
                    .font(.callout)
                    .foregroundStyle(.secondary)
                    .multilineTextAlignment(.center)

                Text("Last checked: \(checkedAt.formatted(date: .abbreviated, time: .shortened))")
                    .font(.caption2)
                    .foregroundStyle(.tertiary)
            }
            .frame(maxWidth: .infinity, minHeight: 180)

        case let .available(manifest, asset):
            VStack(alignment: .leading, spacing: 14) {
                HStack(alignment: .firstTextBaseline) {
                    VStack(alignment: .leading, spacing: 2) {
                        Text(manifest.title)
                            .font(.title3.weight(.bold))
                        Text("Modo \(manifest.version) is ready to download.")
                            .font(.subheadline)
                            .foregroundStyle(.secondary)
                    }
                    Spacer()
                    VStack(alignment: .trailing, spacing: 2) {
                        Text(OTAUpdateEngine.formatBytes(asset.sizeBytes))
                            .font(.caption.weight(.medium))
                        Text(manifest.releaseDate)
                            .font(.caption2)
                            .foregroundStyle(.tertiary)
                    }
                }

                // Version jump pill
                HStack(spacing: 8) {
                    Text("Current: v\(ota.currentVersion)")
                        .font(.caption.monospaced())
                        .padding(.horizontal, 8)
                        .padding(.vertical, 3)
                        .background(Color(nsColor: .controlBackgroundColor), in: RoundedRectangle(cornerRadius: 6))

                    Image(systemName: "arrow.right")
                        .font(.caption2)
                        .foregroundStyle(.secondary)

                    Text("New: v\(manifest.version)")
                        .font(.caption.bold().monospaced())
                        .foregroundStyle(Palette.accent)
                        .padding(.horizontal, 8)
                        .padding(.vertical, 3)
                        .background(Palette.accent.opacity(0.12), in: RoundedRectangle(cornerRadius: 6))
                }

                Text("Release Notes:")
                    .font(.caption.weight(.medium))
                    .foregroundStyle(.secondary)

                ScrollView {
                    Text(manifest.releaseNotes)
                        .font(.system(size: 12, design: .default))
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .padding(10)
                }
                .frame(height: 110)
                .background(Color(nsColor: .controlBackgroundColor))
                .clipShape(RoundedRectangle(cornerRadius: 8))
                .overlay(
                    RoundedRectangle(cornerRadius: 8)
                        .strokeBorder(Color(nsColor: .separatorColor), lineWidth: 1)
                )
            }

        case let .downloading(progress, received, total):
            VStack(spacing: 16) {
                Text("Downloading Update…")
                    .font(.headline)

                ProgressView(value: progress, total: 1.0)
                    .progressViewStyle(.linear)

                HStack {
                    Text("\(Int(progress * 100))%")
                        .font(.caption.bold())
                    Spacer()
                    Text("\(OTAUpdateEngine.formatBytes(received)) of \(OTAUpdateEngine.formatBytes(total))")
                        .font(.caption.monospacedDigit())
                        .foregroundStyle(.secondary)
                }
            }
            .frame(maxWidth: .infinity, minHeight: 180)

        case .verifying:
            VStack(spacing: 16) {
                ProgressView()
                    .controlSize(.regular)
                Text("Verifying cryptographic signature & SHA-256…")
                    .font(.callout)
                    .foregroundStyle(.secondary)
            }
            .frame(maxWidth: .infinity, minHeight: 180)

        case let .readyToInstall(_, manifest, _):
            VStack(spacing: 14) {
                Image(systemName: "checkmark.seal.fill")
                    .font(.system(size: 48))
                    .foregroundStyle(.green)

                Text("Update Ready to Install")
                    .font(.title3.weight(.bold))

                Text("Modo \(manifest.version) has been downloaded and verified.")
                    .font(.callout)
                    .foregroundStyle(.secondary)

                Text("The app will automatically restart into the new version.")
                    .font(.caption)
                    .foregroundStyle(.tertiary)
            }
            .frame(maxWidth: .infinity, minHeight: 180)

        case .installing:
            VStack(spacing: 16) {
                ProgressView()
                    .controlSize(.large)
                Text("Applying update and relaunching Modo…")
                    .font(.headline)
            }
            .frame(maxWidth: .infinity, minHeight: 180)

        case let .failed(msg):
            VStack(spacing: 14) {
                Image(systemName: "exclamationmark.triangle.fill")
                    .font(.system(size: 48))
                    .foregroundStyle(.red)

                Text("Update Error")
                    .font(.title3.weight(.bold))

                Text(msg)
                    .font(.callout)
                    .foregroundStyle(.secondary)
                    .multilineTextAlignment(.center)
            }
            .frame(maxWidth: .infinity, minHeight: 180)
        }
    }

    @ViewBuilder
    private var actionsFooter: some View {
        HStack {
            switch ota.status {
            case .idle, .checking:
                Button("Cancel") { dismiss() }
                Spacer()

            case .upToDate:
                Spacer()
                Button("OK") { dismiss() }
                    .keyboardShortcut(.defaultAction)

            case let .available(manifest, _):
                Button("Skip This Version") {
                    ota.skipThisVersion(version: manifest.version)
                    dismiss()
                }
                .foregroundStyle(.secondary)

                Spacer()

                Button("Remind Me Later") { dismiss() }

                Button("Download & Install") {
                    ota.startDownload()
                }
                .buttonStyle(.borderedProminent)
                .keyboardShortcut(.defaultAction)

            case .downloading:
                Button("Cancel") {
                    ota.cancelDownload()
                    dismiss()
                }
                Spacer()

            case .verifying, .installing:
                Spacer()

            case .readyToInstall:
                Button("Later") { dismiss() }
                Spacer()
                Button("Install & Relaunch") {
                    ota.installAndRelaunch()
                }
                .buttonStyle(.borderedProminent)
                .keyboardShortcut(.defaultAction)

            case .failed:
                Button("Close") { dismiss() }
                Spacer()
                Button("Try Again") {
                    Task { await ota.checkForUpdates(manual: true) }
                }
                .buttonStyle(.borderedProminent)
            }
        }
    }
}

/// Settings pane for Over-the-Air Auto-Updates.
struct UpdatesSettingsView: View {
    @State private var ota = OTAUpdateService.shared
    @State private var showFeedURLField = false

    var body: some View {
        Section("Installed Version") {
            HStack {
                VStack(alignment: .leading, spacing: 3) {
                    Text("Modo \(ota.currentVersion)")
                        .font(.headline)
                    Text("Build \(ota.currentBuild) • Native macOS App by Atoma")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
                Spacer()

                Button("Check for Updates Now") {
                    Task { await ota.checkForUpdates(manual: true) }
                }
                .disabled(ota.status == .checking)
            }

            if case let .upToDate(version, checkedAt) = ota.status {
                HStack(spacing: 8) {
                    Image(systemName: "checkmark.circle.fill").foregroundStyle(.green)
                    Text("Modo \(version) is the latest version. Checked \(checkedAt.formatted(date: .abbreviated, time: .shortened)).")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                }
            } else if case let .failed(msg) = ota.status {
                HStack(spacing: 8) {
                    Image(systemName: "exclamationmark.triangle.fill").foregroundStyle(.red)
                    Text(msg)
                        .font(.caption)
                        .foregroundStyle(.red)
                }
            }
        }

        Section("Automatic Updates") {
            Toggle("Check for updates automatically in background", isOn: Binding(
                get: { ota.autoCheckEnabled },
                set: { ota.autoCheckEnabled = $0 }
            ))

            if let skipped = ota.skippedVersion {
                HStack {
                    Text("Skipped version: v\(skipped)")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                    Spacer()
                    Button("Clear") { ota.skippedVersion = nil }
                        .controlSize(.small)
                }
            }
        }

        Section("Update Channel & Feed") {
            VStack(alignment: .leading, spacing: 8) {
                Text("OTA Manifest URL:")
                    .font(.caption)
                    .foregroundStyle(.secondary)

                HStack {
                    TextField("OTA Update Feed URL", text: Binding(
                        get: { ota.feedURLString },
                        set: { ota.feedURLString = $0 }
                    ))
                    .textFieldStyle(.roundedBorder)
                    .font(.caption.monospaced())

                    Button("Reset") {
                        ota.resetFeedURL()
                    }
                    .controlSize(.small)
                }

                Text("Default: Official Atoma GitHub Releases manifest channel.")
                    .font(.caption2)
                    .foregroundStyle(.tertiary)
            }
        }
    }
}
