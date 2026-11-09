cask "modo" do
  version "1.7.0"
  sha256 "588eff16e4d618e6f3fff9f997f8f331d28d6d4412de7dc1c740d345c1545786"

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
