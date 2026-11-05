#!/usr/bin/env python3
"""
Master Installer Builder for Modo by Atoma:
  - Builds native macOS installers (.dmg, .pkg, .zip)
  - Builds clean native Windows PE32+ installers (.exe Setup, Portable .exe, .zip)
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parent
DIST_DIR = ROOT_DIR / "dist"


def main() -> None:
    print("========================================================")
    print("  СБОРКА УСТАНОВЩИКОВ MODO (MACOS + WINDOWS)")
    print("========================================================")

    # 1. macOS Build & Packaging
    if sys.platform == "darwin":
        print("\n>>> Сборка нативного приложения macOS через ./build.sh...")
        subprocess.run([str(ROOT_DIR / "build.sh")], cwd=str(ROOT_DIR), check=True)

        print("\n>>> Упаковка инсталляторов macOS...")
        subprocess.run(
            [sys.executable, str(ROOT_DIR / "tools" / "package_macos.py")],
            cwd=str(ROOT_DIR),
            check=True,
        )
    else:
        print("[!] macOS installers can only be built on macOS host.")

    # 2. Windows Native PE32+ Build & Packaging
    print("\n>>> Сборка чистых PE32+ установщиков Windows...")
    subprocess.run(
        [sys.executable, str(ROOT_DIR / "windows-native" / "build_windows_installers.py")],
        cwd=str(ROOT_DIR),
        check=True,
    )

    # 3. Generate OTA Update Manifest (dist/update.json)
    import hashlib
    import json
    from datetime import datetime

    assets = []
    for p in sorted(DIST_DIR.glob("*")):
        if not p.is_file() or p.name in ("checksums.txt", "update.json"):
            continue
        sha256 = hashlib.sha256(p.read_bytes()).hexdigest()
        size = p.stat().st_size
        name = p.name

        platform = "macos" if "macOS" in name or "mac" in name.lower() else ("windows" if "Windows" in name or "win" in name.lower() else "universal")
        fmt = "zip" if name.endswith(".zip") else ("dmg" if name.endswith(".dmg") else ("pkg" if name.endswith(".pkg") else ("exe" if name.endswith(".exe") else "zip")))
        is_primary = (name == "Modo-macOS.zip") or (name == "Modo-Windows-Setup.exe")

        assets.append({
            "name": name,
            "platform": platform,
            "format": fmt,
            "downloadURL": f"https://github.com/Tasonai/Modo/releases/download/v1.6.0/{name}",
            "sha256": sha256,
            "sizeBytes": size,
            "isPrimary": is_primary
        })

    manifest = {
        "version": "1.6.0",
        "build": datetime.now().strftime("%Y%m%d%H%M"),
        "releaseDate": datetime.now().strftime("%Y-%m-%d"),
        "title": "Modo 1.6.0 — Folder Styler, Batch Creator, MenuBar & CLI",
        "releaseNotes": "• Folder Icon Styler: 9 gradient palettes & 4 embossed badge styles for macOS Finder & Windows Explorer\n• Batch Iconset & Pack Creator: folder scan & multi-format export (.modopack, .icns, .ico, .png, .zip)\n• MenuBar Extra Companion: Quick Drag & Drop icon styling from macOS status bar\n• Terminal CLI 'modo': full headless command-line automation\n• App Store & Showcase Studio: high-resolution hero presentation banners\n• Raycast & Alfred launcher integrations",
        "minOSVersion": "14.0",
        "isMandatory": False,
        "assets": assets
    }
    (DIST_DIR / "update.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    # 4. Compute SHA256 Checksums
    checksum_lines = []
    for p in sorted(DIST_DIR.glob("*")):
        if p.is_file() and p.name != "checksums.txt":
            digest = hashlib.sha256(p.read_bytes()).hexdigest()
            checksum_lines.append(f"{digest}  {p.name}\n")
    (DIST_DIR / "checksums.txt").write_text("".join(checksum_lines), encoding="utf-8")

    print("\n========================================================")
    print("  ВСЕ УСТАНОВЩИКИ УСПЕШНО СОБРАНЫ В ПАПКЕ DIST/")
    print("========================================================")
    for p in sorted(DIST_DIR.glob("*")):
        if p.is_file():
            print(f"  • {p.name:36s} ({p.stat().st_size / 1024:.1f} KB)")


if __name__ == "__main__":
    main()
