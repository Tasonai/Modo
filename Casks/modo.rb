cask "modo" do
  version "1.7.0"
  sha256 "eaa8ccad41b18272d479945a30e493b03c891de5b13502349a27c68a7959e444"

  url "https://github.com/Tasonai/Modo/releases/download/v#{version}/Modo-macOS.dmg"
  name "Modo"
  desc "Native Adaptive Icon Studio for macOS & Windows by Atoma"
  homepage "https://github.com/Tasonai/Modo"

  livecheck do
    url "https://github.com/Tasonai/Modo/releases/latest"
    strategy :github_latest
  end

  auto_updates true
  depends_on macos: :sonoma

  app "Modo.app"
  binary "#{appdir}/Modo.app/Contents/MacOS/Modo", target: "modo"

  zap trash: [
    "~/Library/Application Support/Modo",
    "~/Library/Preferences/com.atoma.modo.plist",
    "~/Library/Saved Application State/com.atoma.modo.savedState",
  ]
end
