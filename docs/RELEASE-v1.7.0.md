# Modo v1.7.0 Release Notes

**Version:** 1.7.0  
**Date:** November 2026  
**Developer:** Atoma  

Modo v1.7.0 is a milestone release delivering full cross-platform parity between macOS and Windows, a native headless CLI automation engine, an advanced developer export suite, 3D parallax physics, and batch processing capabilities.

---

### Key Features & Improvements

1. **Complete Cross-Platform Parity**:
   - **macOS**: Native SwiftUI and SwiftData architecture with three-column `NavigationSplitView`, CoreGraphics shader rendering, and full macOS design system alignment.
   - **Windows**: High-performance pure C++17 and GDI+ native implementation without external dependencies (no Electron, no WebView2). Features Windows 11 Mica backdrop (`DWMWA_SYSTEMBACKDROP_TYPE`), hardware-accelerated rounded corners, sub-millisecond response time, and clean PE32+ binaries.

2. **Native Headless CLI Engine (`modo`)**:
   - Both platforms now feature a unified command-line interface directly within the main executable.
   - On Windows, `Modo.exe` detects console attachment via `AttachConsole` and provides immediate synchronous CLI output with `modo.cmd` wrapper support:
     - `modo style <app_path> [style]` — Apply adaptive styling with shapes, backgrounds, and scale.
     - `modo restore <app_path>` — Restore original system icons.
     - `modo folder <folder_path> [style]` — Customize folders with `desktop.ini` or Finder badges.
     - `modo batch <input_folder> [out_dir] [style]` — Batch convert folders of images to multi-size iconsets.
     - `modo showcase [out_png]` — Generate 1920x1080 presentation banners.
     - `modo export <app> <ico|png|svg|favicon|xcode|all>` — Export complete asset bundles.
     - `modo theme <system|light|dark>` — Switch interface themes.
     - `modo shell <refresh|clean-cache>` — Rebuild system icon cache.
     - `modo version` and `modo help` — Display build architecture and usage guides.

3. **Developer Export Suite**:
   - **Apple Xcode Asset Catalog**: Generates structured `AppIcon.appiconset` with valid `Contents.json` and all required resolutions (16px to 1024px).
   - **Web Favicon Kit**: Generates `favicon.ico`, `favicon-16x16.png`, `favicon-32x32.png`, `apple-touch-icon.png` (180px), `android-chrome-192x192.png`, `android-chrome-512x512.png`, and a W3C-compliant `site.webmanifest`.
   - **Pure Vector SVG**: Direct export of adaptive geometry, material gradients, and native XML drop shadows (`feDropShadow`).
   - Multi-format archive export bundling all sizes and formats into a clean ZIP archive (`Ctrl+E`).

4. **3D Parallax Tilt and Specular Glare Physics**:
   - Interactive preview canvas calculates dynamic 3D perspective tilt in real time based on cursor coordinates (`WM_MOUSEMOVE` on Windows, `GeometryReader` on macOS).
   - Smooth sweeping specular glare highlights reflecting off surface materials.
   - Kinematic spring interpolation smoothly returns the icon to neutral equilibrium when the cursor exits.

5. **Showcase Presentation Studio (1920x1080)**:
   - Built-in generator creates studio-grade 1920x1080 hero presentation graphics for App Store listings, portfolios, and GitHub READMEs.
   - Includes studio pedestal lighting, dual-tone ambient underglow, floating geometry previews, and studio typography.

6. **Batch Icon Creator**:
   - Interactive GUI dialog (`Ctrl+Shift+B` or Tools menu) for bulk processing image collections (`.png`, `.jpg`, `.jpeg`, `.bmp`, `.ico`).
   - Automatically generates multi-size `.ico` bundles and 1024px master `.png` files with live progress tracking.

7. **Dynamic Windows Accent Color Integration**:
   - Real-time synchronization with Windows personalization accent color via `DwmGetColorizationColor` with fallback to `GetSysColor(COLOR_HIGHLIGHT)`.
   - Dedicated keyboard accelerator (`Ctrl+Shift+A`) and a dynamic 9th swatch in the Style inspector.

8. **Desktop.ini Folder Customization & Drag-and-Drop**:
   - Native Windows folder styling using standard `desktop.ini` configuration, setting necessary attributes (`FILE_ATTRIBUTE_READONLY` / `SYSTEM`), and broadcasting `SHCNE_UPDATEITEM` / `SHCNE_ASSOCCHANGED`.
   - Full drag-and-drop support: drag folders or executables directly onto the studio window.

9. **Comprehensive Verification**:
   - Expanded automated test suite to 114 end-to-end checks in `tools/selftest.sh` covering CLI parsing, shader rendering, vector variant engines, export bundles, and security boundaries.
