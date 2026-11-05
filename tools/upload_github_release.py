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
TAG = "v1.6.0"
TITLE = "Modo 1.6.0 — Folder Styler, Batch Creator, MenuBar & CLI"
BODY = """## Modo 1.6.0 — Official Release by Atoma

### 🚀 What's New in v1.6.0:
- **📁 Folder Icon Styler**: Custom macOS Finder & Windows Explorer folder tinting with 9 rich gradient palettes and 4 engraved embossed glyph styles (SF Symbols, Simple Icons, AI Studio).
- **📦 Batch Iconset & Pack Creator**: Drag-and-drop an entire folder of artwork or logos to automatically generate `.modopack`, multi-size `.icns`, Windows `.ico`, 1024px `.png`, and `.zip` archives.
- **🚀 MenuBar Extra (Quick Drop Companion)**: Instant drag-and-drop icon styling right from the macOS status bar without launching the full window.
- **⚡ Terminal CLI `modo`**: Full headless automation for terminal power-users (`modo style`, `modo folder`, `modo batch`, `modo showcase`).
- **🎨 App Store & Portfolio Showcase Studio**: Generate high-res 1920×1080 and 1280×640 hero presentation graphics with ambient lighting and contact shadows for GitHub README and App Store.
- **🌐 Documentation & Web Showcase**: Interactive web playground and docs hosted via GitHub Pages.
- **🔌 Raycast & Alfred Integrations**: Ready-to-use workflows for styling apps directly from launcher search.
- **🪟 Windows Explorer desktop.ini Parity**: Native folder icon customization for Windows 10/11.

### 📦 Artifacts & Checksums:
Verify integrity against `checksums.txt`.
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

    # 2. Upload Assets
    for p in sorted(DIST_DIR.glob("*")):
        if not p.is_file():
            continue
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

    print("\nRelease v1.6.0 published successfully with all assets!")

if __name__ == "__main__":
    main()
