# IconMaker v1.0.0 Release Notes

**Version:** 1.0.0  
**Release Date:** 2026-11-04  
**Developer:** Atoma  

---

## 🚀 Overview

**IconMaker v1.0.0** is the first official release of the unified cross-platform icon design studio. The application is completely native on both macOS and Windows, requiring zero external interpreters (no Python runtime, no electron, no bloatware).

---

## 🌟 Key Features

### 🍏 Native macOS Application
- **SwiftUI & SwiftData**: Lightweight, responsive UI utilizing Apple's 3-column `NavigationSplitView`.
- **Interactive Drag-to-Pan Canvas**: Fluidly adjust glyph placement with mouse/trackpad drag gestures and auto-centering on double-click.
- **Precision Controls**: Fine-tune glyph scaling (30% to 250%), elevation shadow intensity, inner shadows, and auto-trim margins.
- **Format Support**: Export master `.icns`, Windows multi-size `.ico`, and 1024×1024 `.png` icon packs.

### 🪟 Native Windows Application
- **100% C++17 & GDI+**: Self-contained single PE32+ executable.
- **Windows Defender Friendly**: Clean compilation without DWARF debug sections or PE overlay droppers.
- **Windows Shell Integration**:
  - Full support for **File Explorer (`explorer.exe`)** across Taskbar and CLSID registry.
  - Desktop Shell Icons: **This PC**, **Recycle Bin** (full/empty), **User Files**, **Control Panel**, **Network**.
  - Shortcut detection and icon patching across Taskbar (`User Pinned\TaskBar`), Desktop, and Start Menu.
  - Safe 1-click restore with automated backups in `%LOCALAPPDATA%\IconMaker\backup`.

### 🎨 Achromatic & Neutral Styling
- Clean, minimal layout with zero distracting rainbow or saturated accent colors (`можно без акцентного цвета`).
- Strict WCAG AAA contrast in both Light and Dark modes.

---

## 📦 Distribution Packages

All packages are available in [`dist/`](file:///Users/tasonai/Developer/IconMaker/dist/):

| Platform | Package | Size | Description |
| :--- | :--- | :---: | :--- |
| **macOS** | `IconMaker-macOS.dmg` | 2.4 MB | Drag-and-drop disk image |
| **macOS** | `IconMaker-macOS-Installer.pkg` | 2.1 MB | Standard macOS Setup Wizard |
| **macOS** | `IconMaker-macOS.zip` | 2.1 MB | Signed standalone application bundle |
| **Windows** | `IconMaker-Windows-Setup.exe` | 375.5 KB | Native installer with Start & Desktop shortcuts |
| **Windows** | `IconMaker-Windows-Portable.exe` | 375.5 KB | Zero-install standalone executable |
| **Windows** | `IconMaker-Windows-x64.zip` | 187.5 KB | Compact ZIP archive |
| **Checksums** | `checksums.txt` | 0.6 KB | Official SHA-256 verification hashes |

---

## 🛠️ Build from Source

```bash
# Build all installers (macOS + Windows)
python3 build_installers.py

# Build macOS app only
./build.sh --open

# Build Windows binaries only
python3 windows-native/build_windows_installers.py
```
