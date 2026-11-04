#!/usr/bin/env python3
"""
Master Installer Builder for IconMaker:
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
    print("  СБОРКА УСТАНОВЩИКОВ ICONMAKER (MACOS + WINDOWS)")
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

    # 3. Compute SHA256 Checksums
    import hashlib

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
