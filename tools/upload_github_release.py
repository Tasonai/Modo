#!/usr/bin/env python3
import json
import mimetypes
import os
import sys
import urllib.request
from pathlib import Path

TOKEN = os.environ.get("GITHUB_TOKEN") or ""
REPO = "Tasonai/Modo"
TAG = "v1.5.0"
TITLE = "Modo 1.5.0 — AI Glyph Studio & OTA Updates"
BODY = """## Modo 1.5.0 — Official Release by Atoma

### 🚀 What's New:
- **Over-The-Air (OTA) Updates**: In-app automated update engine with live download progress and cryptographic SHA-256 verification.
- **AI Glyph Studio**: Dual-engine generation featuring 9 procedural geometric vector styles (Monogram, Sparkle, Aperture, Orbit, Hexacore, Prism, Shield, Terminal, Waveform) and cloud neural logo generation.
- **1-Click System Theming Wizard**: Batch application, auto Simple Icons matching, snapshot backups, and 1-click rollback.
- **Adaptive Dark & Tinted Modes**: Seamless macOS Sequoia & iOS 18 style rendering with 3-variant PNG kit export.
- **Windows PE32+ Native Suite**: Ultra-compact (391 KB) zero-dependency native setup and portable editions without PE overlay.

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

    print("\nRelease v1.5.0 published successfully with all assets!")

if __name__ == "__main__":
    main()
