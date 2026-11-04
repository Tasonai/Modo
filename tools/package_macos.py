#!/usr/bin/env python3
"""
Packages macOS build into standalone distribution formats:
  - `dist/Modo-macOS.dmg` (Disk Image with /Applications drag-and-drop link)
  - `dist/Modo-macOS-Installer.pkg` (Standard macOS Setup Wizard installer package)
  - `dist/Modo-macOS.zip` (Direct ZIP archive with signed app bundle)
"""

from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parent.parent
DIST_DIR = ROOT_DIR / "dist"
APP_SOURCE = Path.home() / "Applications" / "Modo.app"


def package_macos() -> None:
    DIST_DIR.mkdir(parents=True, exist_ok=True)

    # If ~/Applications/Modo.app does not exist, build it first via build.sh
    if not APP_SOURCE.exists():
        print("[*] ~/Applications/Modo.app не найден. Запуск ./build.sh...")
        subprocess.run([str(ROOT_DIR / "build.sh")], cwd=str(ROOT_DIR), check=True)

    assert APP_SOURCE.exists(), f"Не удалось найти {APP_SOURCE}"

    print("[1/3] Сборка стандартного установщика macOS (.pkg)...")
    pkg_path = DIST_DIR / "Modo-macOS-Installer.pkg"
    pkg_path.unlink(missing_ok=True)
    subprocess.run(
        [
            "pkgbuild",
            "--component",
            str(APP_SOURCE),
            "--install-location",
            "/Applications",
            "--identifier",
            "com.atoma.modo.pkg",
            "--version",
            "1.2.0",
            str(pkg_path),
        ],
        check=True,
    )

    print("[2/3] Сборка образа диска macOS (.dmg с ярлыком /Applications)...")
    dmg_staging = Path("/tmp/modo_dmg_staging")
    if dmg_staging.exists():
        shutil.rmtree(dmg_staging)
    dmg_staging.mkdir(parents=True, exist_ok=True)

    shutil.copytree(APP_SOURCE, dmg_staging / "Modo.app", symlinks=True)
    (dmg_staging / "Applications").symlink_to("/Applications")

    dmg_path = DIST_DIR / "Modo-macOS.dmg"
    dmg_path.unlink(missing_ok=True)
    subprocess.run(
        [
            "hdiutil",
            "create",
            "-volname",
            "Modo",
            "-srcfolder",
            str(dmg_staging),
            "-ov",
            "-format",
            "UDZO",
            str(dmg_path),
        ],
        check=True,
    )

    print("[3/3] Сборка ZIP-архива приложения (.zip)...")
    zip_path = DIST_DIR / "Modo-macOS.zip"
    zip_path.unlink(missing_ok=True)
    subprocess.run(
        ["ditto", "-c", "-k", "--keepParent", str(APP_SOURCE), str(zip_path)],
        check=True,
    )

    if dmg_staging.exists():
        shutil.rmtree(dmg_staging)

    print("\n========================================================")
    print("  ГОТОВЫ УСТАНОВЩИКИ MACOS")
    print("========================================================")
    for p in (pkg_path, dmg_path, zip_path):
        print(f"  • {p.name:34s} ({p.stat().st_size / 1024:.1f} KB)")


if __name__ == "__main__":
    package_macos()
