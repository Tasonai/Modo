cask "modo" do
  version "1.6.0"
  sha256 "106e5fb8bb0083f45cb9aae468237ee800f132ec43d19858f1f9afc5d35b47c6"

  url "https://github.com/Tasonai/Modo/releases/download/v#{version}/Modo-macOS.dmg"
  name "Modo"
  desc "Native Adaptive Icon Studio for macOS & Windows by Atoma"
  homepage "https://github.com/Tasonai/Modo"

  livecheck do
    url "https://github.com/Tasonai/Modo/releases/latest"
    strategy :github_latest
  end

  auto_updates true
  depends_on macos: ">= :sonoma"

  app "Modo.app"
  binary "#{appdir}/Modo.app/Contents/MacOS/Modo", target: "modo"

  zap trash: [
    "~/Library/Application Support/Modo",
    "~/Library/Preferences/com.atoma.modo.plist",
    "~/Library/Saved Application State/com.atoma.modo.savedState",
  ]
end
