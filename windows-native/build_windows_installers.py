#!/usr/bin/env python3
"""
Builds 100% Native, Zero-Dependency Windows x64 Executables & Setup Wizard:
  1. dist/IconMaker-Windows-Setup.exe       — Native Win32 GUI Setup Wizard (installs IconMaker.exe, Desktop & Start Menu shortcuts)
  2. dist/GoogleRoundIconizer-Setup.exe     — Alias of the Setup Wizard
  3. dist/IconMaker-Windows-Portable.exe    — Standalone single-file native Win32/GDI+ executable (run directly, no install needed)
"""

from __future__ import annotations

import shutil
import struct
import subprocess
import zlib
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parent.parent
WIN_DIR = ROOT_DIR / "windows-native"
LEGACY_DIR = ROOT_DIR / "legacy-windows"
DIST_DIR = ROOT_DIR / "dist"
ZIG_BIN = Path("/tmp/zig-compiler/zig")

MAGIC_FOOTER = 0x47524931  # "GRI1"


def main() -> None:
    DIST_DIR.mkdir(parents=True, exist_ok=True)
    build_tmp = Path("/tmp/iconmaker-win-build")
    if build_tmp.exists():
        shutil.rmtree(build_tmp)
    build_tmp.mkdir(parents=True, exist_ok=True)

    portable_exe = DIST_DIR / "IconMaker-Windows-Portable.exe"

    print("[1/4] Компиляция нативного Win32/GDI+ приложения (IconMaker-Windows-Portable.exe)...")
    subprocess.run(
        [
            str(ZIG_BIN),
            "c++",
            "-target",
            "x86_64-windows-gnu",
            "-std=c++17",
            "-O2",
            "-municode",
            str(WIN_DIR / "iconmaker_win32.cpp"),
            "-o",
            str(portable_exe),
            "-Wl,--subsystem,windows",
            "-lgdiplus",
            "-lcomctl32",
            "-lcomdlg32",
            "-lshell32",
            "-lole32",
            "-lshlwapi",
            "-luuid",
            "-luser32",
            "-lgdi32",
        ],
        check=True,
    )

    print("[2/4] Компиляция нативного Win32 Setup-инсталлятора через Zig...")
    decomp_obj = build_tmp / "decomp.obj"
    setup_stub_exe = build_tmp / "setup_stub.exe"

    subprocess.run(
        [
            str(ZIG_BIN),
            "build-obj",
            str(LEGACY_DIR / "installer" / "decomp.zig"),
            "-target",
            "x86_64-windows-gnu",
            "-O",
            "ReleaseFast",
            f"-femit-bin={decomp_obj}",
        ],
        check=True,
    )

    subprocess.run(
        [
            str(ZIG_BIN),
            "cc",
            "-target",
            "x86_64-windows-gnu",
            "-municode",
            "-O2",
            str(LEGACY_DIR / "installer" / "win_installer.c"),
            str(decomp_obj),
            "-o",
            str(setup_stub_exe),
            "-Wl,--subsystem,windows",
            "-luser32",
            "-lgdi32",
            "-lshell32",
            "-lole32",
            "-luuid",
            "-lcomctl32",
        ],
        check=True,
    )

    print("[3/4] Формирование пакета файлов для установщика Windows...")
    bundle_root = build_tmp / "bundle"
    (bundle_root / "assets").mkdir(parents=True, exist_ok=True)
    # Place the native standalone executable under both names so shortcuts and direct launches work seamlessly
    shutil.copy2(portable_exe, bundle_root / "GoogleRoundIconizer.exe")
    shutil.copy2(portable_exe, bundle_root / "IconMaker.exe")

    ico_src = LEGACY_DIR / "assets" / "app_icon.ico"
    if ico_src.exists():
        shutil.copy2(ico_src, bundle_root / "assets" / "app_icon.ico")

    print("[4/4] Упаковка автономного инсталлятора dist/IconMaker-Windows-Setup.exe...")
    setup_exe = DIST_DIR / "IconMaker-Windows-Setup.exe"
    alias_setup_exe = DIST_DIR / "GoogleRoundIconizer-Setup.exe"

    all_files = sorted(f for f in bundle_root.rglob("*") if f.is_file())
    stub_bytes = setup_stub_exe.read_bytes()
    payload_offset = len(stub_bytes)

    with open(setup_exe, "wb") as f:
        f.write(stub_bytes)
        for fpath in all_files:
            rel_utf8 = fpath.relative_to(bundle_root).as_posix().encode("utf-8")
            raw_data = fpath.read_bytes()
            comp_data = zlib.compress(raw_data, level=9) if raw_data else b""
            f.write(struct.pack("<I", len(rel_utf8)))
            f.write(rel_utf8)
            f.write(struct.pack("<II", len(raw_data), len(comp_data)))
            if comp_data:
                f.write(comp_data)

        footer = struct.pack("<QII", payload_offset, len(all_files), MAGIC_FOOTER)
        f.write(footer)

    shutil.copy2(setup_exe, alias_setup_exe)

    print("\n========================================================")
    print("  НАТИВНЫЕ EXE ДЛЯ WINDOWS 10/11 УСПЕШНО СОБРАНЫ")
    print("========================================================")
    print(f"  1. Установщик (Setup Wizard): {setup_exe} ({setup_exe.stat().st_size / 1024:.0f} KB)")
    print(f"  2. Копия установщика:         {alias_setup_exe} ({alias_setup_exe.stat().st_size / 1024:.0f} KB)")
    print(f"  3. Portable (без установки):  {portable_exe} ({portable_exe.stat().st_size / 1024:.0f} KB)")


if __name__ == "__main__":
    main()
