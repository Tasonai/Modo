#!/usr/bin/env python3
import json
import mimetypes
import os
import subprocess
import sys
import urllib.request
from pathlib import Path

def get_token():
    t = os.environ.get("GITHUB_TOKEN")
    if t:
        return t
    try:
        proc = subprocess.run(
            ["git", "credential", "fill"],
            input="protocol=https\nhost=github.com\n",
            capture_output=True,
            text=True,
            check=True
        )
        for line in proc.stdout.splitlines():
            if line.startswith("password="):
                return line.split("=", 1)[1].strip()
    except Exception:
        pass
    return ""

TOKEN = get_token()
REPO = "Tasonai/Modo"
TAG = "v1.7.0"
TITLE = "Modo 1.7.0 — Canonical HIG Menus, Curated Icon Packs, and Developer Studio"
BODY = """## Modo 1.7.0 — Official Release by Atoma

### What's New in v1.7.0:
- Canonical macOS HIG Menu Hierarchy: Standard File, Edit, View, Style, and Tools menus with system keyboard shortcuts (⌘1, ⌘2, ⌘E, ⌘Return, ⇧⌘A, ⇧⌘T, ⇧⌘C, ⇧⌘0).
- Windows Native Modo Menu & Accelerators: Integrated Modo dropdown menu next to window traffic lights and native Alt/Alt+M accelerator support.
- Curated Community Icon Packs: Added WhiteSur macOS, Tela Minimal, Lawnicons Themed, macOSicons Hub, Mac Icons Pro, macOS Folders, and Tabler & Lucide Vector.
- Developer Export Suite: One-click generation of Xcode Asset Catalogs (AppIcon.appiconset with complete Contents.json for macOS/iOS) and complete Web Favicon bundles (favicon.ico, apple-touch-icon.png, site.webmanifest, HTML snippet).
- Pure Scalable Vector Export (.svg): Pure SVG engine with XML filters for realistic material rendering (frosted glass, brushed titanium, neumorphic emboss, prism glow).
- Interactive 3D Parallax Tilt: Perspective physics and moving specular glare in the icon inspector.
- One-Click System Theming & Presets: Signature packs including VisionOS Glass, iOS 18 Tinted Dark, Obsidian Titanium, Prism Cyber, and Neumorphic Tactile.
- Windows Explorer Shell Integration: Right-click context menu to style icons on .exe/.lnk and folders registered under HKEY_CURRENT_USER.
- Extended Terminal CLI modo: Added export, theme, and shell integration commands.

### Artifacts & Checksums:
Verify integrity against checksums.txt.
"""

DIST_DIR = Path(__file__).resolve().parent.parent / "dist"

def main():
    print(f"Creating release {TAG} on {REPO}...")
    headers = {
        "Authorization": f"token {TOKEN}",
        "Accept": "application/vnd.github.v3+json",
        "User-Agent": "Modo-Release-Uploader"
    }

    # 1. Create Release
    payload = json.dumps({
        "tag_name": TAG,
        "name": TITLE,
        "body": BODY,
        "draft": False,
        "prerelease": False
    }).encode("utf-8")

    req = urllib.request.Request(
        f"https://api.github.com/repos/{REPO}/releases",
        data=payload,
        headers=headers,
        method="POST"
    )

    try:
        with urllib.request.urlopen(req) as resp:
            data = json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        err_msg = e.read().decode("utf-8")
        if "already_exists" in err_msg:
            print("Release already exists, fetching existing...")
            req_get = urllib.request.Request(
                f"https://api.github.com/repos/{REPO}/releases/tags/{TAG}",
                headers=headers
            )
            with urllib.request.urlopen(req_get) as resp_get:
                data = json.loads(resp_get.read().decode("utf-8"))
        else:
            print(f"Error creating release: {err_msg}")
            sys.exit(1)

    release_id = data["id"]
    upload_url_template = data["upload_url"] # e.g. https://uploads.github.com/.../assets{?name,label}
    upload_url_base = upload_url_template.split("{")[0]
    print(f"Release ID: {release_id}, Upload URL: {upload_url_base}")

    existing_assets = {a["name"]: a["id"] for a in data.get("assets", [])}

    # 2. Upload Assets
    for p in sorted(DIST_DIR.glob("*")):
        if not p.is_file():
            continue

        if p.name in existing_assets:
            del_id = existing_assets[p.name]
            del_req = urllib.request.Request(
                f"https://api.github.com/repos/{REPO}/releases/assets/{del_id}",
                headers=headers,
                method="DELETE"
            )
            try:
                with urllib.request.urlopen(del_req) as _:
                    pass
            except Exception as e:
                print(f"  Warning: couldn't delete existing asset {p.name}: {e}")

        print(f"Uploading {p.name} ({p.stat().st_size / 1024:.1f} KB)...")
        mime = mimetypes.guess_type(p.name)[0] or "application/octet-stream"
        file_data = p.read_bytes()

        upload_req = urllib.request.Request(
            f"{upload_url_base}?name={p.name}",
            data=file_data,
            headers={
                "Authorization": f"token {TOKEN}",
                "Content-Type": mime,
                "Content-Length": str(len(file_data)),
                "User-Agent": "Modo-Release-Uploader"
            },
            method="POST"
        )
        try:
            with urllib.request.urlopen(upload_req) as up_resp:
                print(f"  ✓ {p.name} uploaded successfully")
        except urllib.error.HTTPError as e:
            print(f"  ✗ Failed to upload {p.name}: {e.read().decode('utf-8')}")

    print("\nRelease v1.7.0 published successfully with all assets!")

if __name__ == "__main__":
    main()
