#include "modo_cli.h"
#include "icon_engine.h"
#include "shell_integration.h"

#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <gdiplus.h>

#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace Gdiplus;

namespace ModoCLI {

static bool g_consoleAttached = false;

static void EnsureConsoleOutput() {
    if (!g_consoleAttached) {
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            FILE* fp = nullptr;
            freopen_s(&fp, "CONOUT$", "w", stdout);
            freopen_s(&fp, "CONOUT$", "w", stderr);
            freopen_s(&fp, "CONIN$", "r", stdin);
            g_consoleAttached = true;
        }
    }
}

static void ReleaseConsoleOutput() {
    if (g_consoleAttached) {
        fflush(stdout);
        fflush(stderr);
        FreeConsole();
        g_consoleAttached = false;
    }
}

static std::wstring ToLower(const std::wstring& s) {
    std::wstring r = s;
    for (auto& c : r) c = (wchar_t)towlower(c);
    return r;
}

static void PrintHelp() {
    wprintf(
        L"Modo \x2014 Native Windows Adaptive Icon Studio (v1.7.0)\n"
        L"Command Line Interface (CLI)\n\n"
        L"USAGE:\n"
        L"  modo <command> [options]\n\n"
        L"COMMANDS:\n"
        L"  style <app_path>       Apply adaptive styling to Windows shortcut (.lnk) or executable (.exe)\n"
        L"                           --shape <squircle|circle|roundedRect|pebble|hexagon>\n"
        L"                           --bg <pastel|dark|white|clear|custom>\n"
        L"                           --scale <0.4..1.8>\n"
        L"                           --material <smooth|glass|titanium|neumorphic|prism>\n"
        L"  restore <path>         Restore original system icon for an app or folder\n"
        L"  folder <folder_path>   Style a folder using desktop.ini & custom palette\n"
        L"                           --color <appleBlue|graphite|purple|teal|sunsetOrange|emerald|coralRed|amberGold|obsidian>\n"
        L"  batch <input_folder>   Batch convert all images in folder into multi-size icons\n"
        L"                           --out <output_dir>\n"
        L"                           --shape <squircle|circle|roundedRect|pebble|hexagon>\n"
        L"  showcase <app_or_icon> Generate 1920\x00D7" L"1080 App Store & README presentation graphic\n"
        L"                           --out <banner.png>\n"
        L"                           --theme <mesh|studio|white|sunset|cyber>\n"
        L"                           --layout <hero|triple|card>\n"
        L"  export <app_path>      Developer Export: Xcode Asset Catalog, Web Favicons, or SVG\n"
        L"                           --type <xcode|web|svg|ico>\n"
        L"                           --out <output_dir>\n"
        L"  theme <theme_name>     One-Click System Theming (ios18, visionos, titanium, nord, catppuccin, prism)\n"
        L"                           --apply (apply to all installed applications)\n"
        L"  shell <options>        Integrate into Windows Explorer right-click context menu\n"
        L"                           --enable | --disable\n"
        L"  version                Print Modo CLI version\n"
        L"  help                   Show this help message\n\n"
        L"EXAMPLES:\n"
        L"  modo style \"C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe\" --shape squircle --bg pastel\n"
        L"  modo folder \"C:\\Projects\\Client\" --color sunsetOrange\n"
        L"  modo showcase \"C:\\Path\\icon.png\" --out ./banner.png --theme mesh\n"
        L"  modo export \"C:\\Path\\icon.png\" --type web --out ./favicons\n"
        L"  modo theme visionos --apply\n"
    );
}

static void PrintVersion() {
    wprintf(L"Modo 1.7.0 (Windows x64) \x2014 Native Adaptive Icon Studio by Atoma\n");
}

static COLORREF ParseColorPreset(const std::wstring& name) {
    std::wstring lower = ToLower(name);
    if (lower.find(L"blue") != std::wstring::npos) return RGB(0, 122, 255);
    if (lower.find(L"graphite") != std::wstring::npos) return RGB(50, 50, 55);
    if (lower.find(L"purple") != std::wstring::npos) return RGB(175, 82, 222);
    if (lower.find(L"teal") != std::wstring::npos) return RGB(48, 176, 199);
    if (lower.find(L"orange") != std::wstring::npos || lower.find(L"sunset") != std::wstring::npos) return RGB(255, 149, 0);
    if (lower.find(L"emerald") != std::wstring::npos || lower.find(L"green") != std::wstring::npos) return RGB(52, 199, 89);
    if (lower.find(L"coral") != std::wstring::npos || lower.find(L"red") != std::wstring::npos) return RGB(255, 59, 48);
    if (lower.find(L"gold") != std::wstring::npos || lower.find(L"amber") != std::wstring::npos) return RGB(255, 204, 0);
    if (lower.find(L"obsidian") != std::wstring::npos || lower.find(L"black") != std::wstring::npos) return RGB(24, 24, 27);
    if (!name.empty() && name[0] == L'#') {
        unsigned int hexVal = 0;
        if (swscanf_s(name.c_str() + 1, L"%x", &hexVal) == 1) {
            BYTE r = (BYTE)((hexVal >> 16) & 0xFF);
            BYTE g = (BYTE)((hexVal >> 8) & 0xFF);
            BYTE b = (BYTE)(hexVal & 0xFF);
            return RGB(r, g, b);
        }
    }
    return RGB(0, 122, 255);
}

static int HandleStyle(int argc, wchar_t* argv[], const CLSID* pngClsid) {
    if (argc < 1 || argv[0][0] == L'-') {
        wprintf(L"Error: Missing target app path. Usage: modo style <app_path> [options]\n");
        return 1;
    }
    std::wstring targetPath = argv[0];
    DWORD attr = GetFileAttributesW(targetPath.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        wprintf(L"Error: Target path does not exist: %ls\n", targetPath.c_str());
        return 1;
    }

    RoundStyle style;
    style.shape = SHAPE_SQUIRCLE;
    style.preset = PRESET_MATERIAL_PASTEL;
    style.scale = 0.68;
    style.material = MATERIAL_SMOOTH;

    for (int i = 1; i < argc; ++i) {
        std::wstring opt = ToLower(argv[i]);
        if (opt == L"--shape" && i + 1 < argc) {
            std::wstring val = ToLower(argv[++i]);
            if (val == L"circle") style.shape = SHAPE_CIRCLE;
            else if (val == L"squircle") style.shape = SHAPE_SQUIRCLE;
            else if (val == L"roundedrect" || val == L"rect") style.shape = SHAPE_ROUNDED_RECT;
            else if (val == L"pebble") style.shape = SHAPE_PEBBLE;
            else if (val == L"hexagon") style.shape = SHAPE_HEXAGON;
        } else if (opt == L"--bg" && i + 1 < argc) {
            std::wstring val = ToLower(argv[++i]);
            if (val == L"white") style.preset = PRESET_GOOGLE_WHITE;
            else if (val == L"pastel") style.preset = PRESET_MATERIAL_PASTEL;
            else if (val == L"dark") style.preset = PRESET_DARK_MATERIAL;
            else if (val == L"clear" || val == L"adaptive") style.preset = PRESET_ADAPTIVE_EDGE;
            else if (val == L"custom") style.preset = PRESET_CUSTOM_COLOR;
        } else if (opt == L"--scale" && i + 1 < argc) {
            double sc = _wtof(argv[++i]);
            if (sc >= 0.3 && sc <= 2.5) style.scale = sc;
        } else if (opt == L"--material" && i + 1 < argc) {
            std::wstring val = ToLower(argv[++i]);
            if (val == L"smooth") style.material = MATERIAL_SMOOTH;
            else if (val == L"glass" || val == L"frosted") style.material = MATERIAL_FROSTED_GLASS;
            else if (val == L"titanium" || val == L"metal") style.material = MATERIAL_BRUSHED_TITANIUM;
            else if (val == L"neumorphic") style.material = MATERIAL_NEUMORPHIC;
            else if (val == L"prism" || val == L"cyber") style.material = MATERIAL_PRISM_GLOW;
        } else if (opt == L"--color" && i + 1 < argc) {
            style.customColor = ParseColorPreset(argv[++i]);
            style.preset = PRESET_CUSTOM_COLOR;
        }
    }

    AppEntry app;
    std::wstring fname = PathFindFileNameW(targetPath.c_str());
    size_t dot = fname.find_last_of(L'.');
    app.name = (dot != std::wstring::npos) ? fname.substr(0, dot) : fname;

    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        app.isFolder = true;
        app.targetPath = targetPath;
        app.category = L"Папки";
        app.origBitmap = IconEngine::LoadHighResIcon(targetPath, L"", L"", pngClsid);
    } else {
        std::wstring ext = PathFindExtensionW(targetPath.c_str());
        if (_wcsicmp(ext.c_str(), L".lnk") == 0) {
            app.lnkPath = targetPath;
            app.allLnkPaths.push_back(targetPath);
            std::wstring customIcon;
            ShellIntegration::ResolveShortcut(targetPath, app.targetPath, customIcon);
            if (app.targetPath.empty()) app.targetPath = targetPath;
        } else {
            app.targetPath = targetPath;
        }
        app.category = L"Приложения";
        app.origBitmap = IconEngine::LoadHighResIcon(app.targetPath, app.lnkPath, L"", pngClsid);
    }

    uint32_t rev = (uint32_t)(GetTickCount() & 0xFFFF);
    bool needsAdmin = false;
    if (ShellIntegration::ApplyRoundIconToEntry(app, style, pngClsid, rev, &needsAdmin)) {
        ShellIntegration::RefreshWindowsExplorerIconCache(true);
        wprintf(L"[+] Successfully styled %ls\n", app.name.c_str());
        wprintf(L"    Shape: %ls, Scale: %d%%\n",
                (style.shape == SHAPE_CIRCLE ? L"Circle" : style.shape == SHAPE_SQUIRCLE ? L"Squircle" :
                 style.shape == SHAPE_ROUNDED_RECT ? L"Rounded Rect" : style.shape == SHAPE_PEBBLE ? L"Pebble" : L"Hexagon"),
                (int)std::round(style.scale * 100.0));
        return 0;
    } else {
        wprintf(L"[-] Failed to apply icon to %ls. %ls\n", targetPath.c_str(),
                needsAdmin ? L"Administrator elevation required for this shortcut." : L"Ensure you have write permissions.");
        return 1;
    }
}

static int HandleRestore(int argc, wchar_t* argv[]) {
    if (argc < 1 || argv[0][0] == L'-') {
        wprintf(L"Error: Missing target path. Usage: modo restore <path>\n");
        return 1;
    }
    std::wstring path = argv[0];
    DWORD attr = GetFileAttributesW(path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        wprintf(L"Error: Path does not exist: %ls\n", path.c_str());
        return 1;
    }

    bool ok = false;
    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        ok = ShellIntegration::RestoreFolderIcon(path);
    } else {
        AppEntry app;
        app.targetPath = path;
        app.lnkPath = path;
        app.name = PathFindFileNameW(path.c_str());
        ok = ShellIntegration::RestoreOriginalEntry(app);
    }

    if (ok) {
        ShellIntegration::RefreshWindowsExplorerIconCache(true);
        wprintf(L"[+] Restored original icon for %ls\n", path.c_str());
        return 0;
    } else {
        wprintf(L"[-] Failed to restore icon for %ls\n", path.c_str());
        return 1;
    }
}

static int HandleFolder(int argc, wchar_t* argv[], const CLSID* pngClsid) {
    if (argc < 1 || argv[0][0] == L'-') {
        wprintf(L"Error: Missing folder path. Usage: modo folder <folder_path> [--color <preset>]\n");
        return 1;
    }
    std::wstring folderPath = argv[0];
    DWORD attr = GetFileAttributesW(folderPath.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        wprintf(L"Error: Folder path does not exist: %ls\n", folderPath.c_str());
        return 1;
    }

    COLORREF color = RGB(0, 122, 255);
    for (int i = 1; i < argc; ++i) {
        if (ToLower(argv[i]) == L"--color" && i + 1 < argc) {
            color = ParseColorPreset(argv[++i]);
        }
    }

    RoundStyle style;
    style.preset = PRESET_CUSTOM_COLOR;
    style.customColor = color;
    style.shape = SHAPE_SQUIRCLE;
    style.scale = 0.68;

    AppEntry app;
    app.name = PathFindFileNameW(folderPath.c_str());
    app.targetPath = folderPath;
    app.isFolder = true;
    app.origBitmap = IconEngine::LoadHighResIcon(folderPath, L"", L"", pngClsid);

    uint32_t rev = (uint32_t)(GetTickCount() & 0xFFFF);
    if (ShellIntegration::ApplyRoundIconToEntry(app, style, pngClsid, rev)) {
        ShellIntegration::RefreshWindowsExplorerIconCache(false);
        wprintf(L"[+] Styled folder %ls with custom desktop.ini palette\n", folderPath.c_str());
        return 0;
    } else {
        wprintf(L"[-] Failed to apply custom folder icon to %ls\n", folderPath.c_str());
        return 1;
    }
}

static int HandleBatch(int argc, wchar_t* argv[], const CLSID* pngClsid) {
    if (argc < 1 || argv[0][0] == L'-') {
        wprintf(L"Error: Missing input folder. Usage: modo batch <input_folder> [--out <output_dir>]\n");
        return 1;
    }
    std::wstring inputDir = argv[0];
    DWORD attr = GetFileAttributesW(inputDir.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        wprintf(L"Error: Input directory does not exist: %ls\n", inputDir.c_str());
        return 1;
    }

    std::wstring outDir = inputDir + L"\\Modo-Icons";
    RoundStyle style;
    style.shape = SHAPE_SQUIRCLE;
    style.preset = PRESET_MATERIAL_PASTEL;
    style.scale = 0.68;

    for (int i = 1; i < argc; ++i) {
        std::wstring opt = ToLower(argv[i]);
        if ((opt == L"--out" || opt == L"-o") && i + 1 < argc) {
            outDir = argv[++i];
        } else if (opt == L"--shape" && i + 1 < argc) {
            std::wstring val = ToLower(argv[++i]);
            if (val == L"circle") style.shape = SHAPE_CIRCLE;
            else if (val == L"squircle") style.shape = SHAPE_SQUIRCLE;
            else if (val == L"roundedrect" || val == L"rect") style.shape = SHAPE_ROUNDED_RECT;
            else if (val == L"pebble") style.shape = SHAPE_PEBBLE;
            else if (val == L"hexagon") style.shape = SHAPE_HEXAGON;
        }
    }

    CreateDirectoryW(outDir.c_str(), NULL);

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW((inputDir + L"\\*.*").c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        wprintf(L"[-] No files found in %ls\n", inputDir.c_str());
        return 1;
    }

    int processed = 0;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::wstring fname = fd.cFileName;
        std::wstring ext = PathFindExtensionW(fname.c_str());
        for (auto& c : ext) c = (wchar_t)towlower(c);

        if (ext == L".png" || ext == L".ico" || ext == L".jpg" || ext == L".jpeg" || ext == L".bmp") {
            std::wstring srcPath = inputDir + L"\\" + fname;
            Bitmap* bmp = Bitmap::FromFile(srcPath.c_str());
            if (bmp && bmp->GetLastStatus() == Ok) {
                size_t dot = fname.find_last_of(L'.');
                std::wstring baseName = (dot != std::wstring::npos) ? fname.substr(0, dot) : fname;
                std::wstring icoPath = outDir + L"\\" + baseName + L".ico";
                std::wstring pngPath = outDir + L"\\" + baseName + L".png";

                IconEngine::SaveMultiSizeIco(bmp, style, icoPath, pngClsid);
                Bitmap* master1024 = IconEngine::RenderGoogleRoundIcon(bmp, style, 1024);
                if (master1024) {
                    master1024->Save(pngPath.c_str(), pngClsid, NULL);
                    delete master1024;
                }
                delete bmp;
                processed++;
                wprintf(L"  [%d] %ls -> .ico, .png\n", processed, baseName.c_str());
            }
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);

    wprintf(L"[+] Successfully exported %d icons to %ls\n", processed, outDir.c_str());
    return 0;
}

static int HandleShowcase(int argc, wchar_t* argv[], const CLSID* pngClsid) {
    if (argc < 1 || argv[0][0] == L'-') {
        wprintf(L"Error: Missing input icon/app. Usage: modo showcase <app_path> --out <banner.png>\n");
        return 1;
    }
    std::wstring inputPath = argv[0];
    if (GetFileAttributesW(inputPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        wprintf(L"Error: File not found: %ls\n", inputPath.c_str());
        return 1;
    }

    std::wstring outPath = L"./showcase.png";
    ShowcaseTheme theme = SHOWCASE_THEME_AMBIENT_MESH;
    ShowcaseLayout layout = SHOWCASE_LAYOUT_HERO;

    for (int i = 1; i < argc; ++i) {
        std::wstring opt = ToLower(argv[i]);
        if ((opt == L"--out" || opt == L"-o") && i + 1 < argc) {
            outPath = argv[++i];
        } else if (opt == L"--theme" && i + 1 < argc) {
            std::wstring val = ToLower(argv[++i]);
            if (val.find(L"mesh") != std::wstring::npos) theme = SHOWCASE_THEME_AMBIENT_MESH;
            else if (val.find(L"studio") != std::wstring::npos || val.find(L"dark") != std::wstring::npos) theme = SHOWCASE_THEME_STUDIO_DARK;
            else if (val.find(L"white") != std::wstring::npos || val.find(L"clean") != std::wstring::npos) theme = SHOWCASE_THEME_CLEAN_WHITE;
            else if (val.find(L"sunset") != std::wstring::npos) theme = SHOWCASE_THEME_SUNSET_GLOW;
            else if (val.find(L"cyber") != std::wstring::npos || val.find(L"neon") != std::wstring::npos) theme = SHOWCASE_THEME_CYBER_NEON;
        } else if (opt == L"--layout" && i + 1 < argc) {
            std::wstring val = ToLower(argv[++i]);
            if (val.find(L"hero") != std::wstring::npos) layout = SHOWCASE_LAYOUT_HERO;
            else if (val.find(L"triple") != std::wstring::npos) layout = SHOWCASE_LAYOUT_TRIPLE;
            else if (val.find(L"card") != std::wstring::npos || val.find(L"window") != std::wstring::npos) layout = SHOWCASE_LAYOUT_WINDOW_CARD;
        }
    }

    Bitmap* bmp = IconEngine::LoadHighResIcon(inputPath, inputPath, L"", pngClsid);
    if (!bmp) {
        wprintf(L"[-] Could not load icon artwork from %ls\n", inputPath.c_str());
        return 1;
    }

    std::wstring fname = PathFindFileNameW(inputPath.c_str());
    size_t dot = fname.find_last_of(L'.');
    std::wstring title = (dot != std::wstring::npos) ? fname.substr(0, dot) : fname;

    RoundStyle style;
    style.shape = SHAPE_SQUIRCLE;
    style.preset = PRESET_MATERIAL_PASTEL;
    style.scale = 0.68;

    bool ok = IconEngine::ExportShowcaseBanner(bmp, style, title, theme, layout, outPath, pngClsid, 1920, 1080);
    delete bmp;

    if (ok) {
        wprintf(L"[+] Saved presentation banner to %ls\n", outPath.c_str());
        return 0;
    } else {
        wprintf(L"[-] Failed to generate presentation banner\n");
        return 1;
    }
}

static int HandleExport(int argc, wchar_t* argv[], const CLSID* pngClsid) {
    if (argc < 1 || argv[0][0] == L'-') {
        wprintf(L"Usage: modo export <app_or_image_path> [--type <xcode|web|svg|ico>] [--out <output_dir>]\n");
        return 1;
    }
    std::wstring inputPath = argv[0];
    if (GetFileAttributesW(inputPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        wprintf(L"Error: Input path not found: %ls\n", inputPath.c_str());
        return 1;
    }

    std::wstring exportType = L"xcode";
    std::wstring outDir = L".";

    for (int i = 1; i < argc; ++i) {
        std::wstring opt = ToLower(argv[i]);
        if (opt == L"--type" && i + 1 < argc) {
            exportType = ToLower(argv[++i]);
        } else if ((opt == L"--out" || opt == L"-o") && i + 1 < argc) {
            outDir = argv[++i];
        }
    }

    Bitmap* bmp = IconEngine::LoadHighResIcon(inputPath, inputPath, L"", pngClsid);
    if (!bmp) {
        wprintf(L"[-] Could not load icon artwork from %ls\n", inputPath.c_str());
        return 1;
    }

    std::wstring fname = PathFindFileNameW(inputPath.c_str());
    size_t dot = fname.find_last_of(L'.');
    std::wstring title = (dot != std::wstring::npos) ? fname.substr(0, dot) : fname;

    RoundStyle style;
    style.shape = SHAPE_SQUIRCLE;
    style.preset = PRESET_MATERIAL_PASTEL;
    style.scale = 0.68;

    bool ok = false;
    if (exportType == L"xcode" || exportType == L"appiconset") {
        ok = IconEngine::ExportXcodeAssetCatalog(bmp, style, title, outDir, pngClsid);
        if (ok) wprintf(L"[+] Exported Xcode Asset Catalog (.appiconset) to %ls\n", outDir.c_str());
    } else if (exportType == L"web" || exportType == L"favicons") {
        ok = IconEngine::ExportWebFaviconKit(bmp, style, title, outDir, pngClsid);
        if (ok) wprintf(L"[+] Exported Web Favicon Kit & site.webmanifest to %ls\n", outDir.c_str());
    } else if (exportType == L"svg") {
        ok = IconEngine::ExportSvgVector(bmp, style, title, outDir, pngClsid);
        if (ok) wprintf(L"[+] Exported pure scalable SVG vector to %ls\n", outDir.c_str());
    } else if (exportType == L"ico") {
        std::wstring icoPath = outDir + L"\\" + title + L".ico";
        ok = IconEngine::SaveMultiSizeIco(bmp, style, icoPath, pngClsid);
        if (ok) wprintf(L"[+] Exported multi-size .ico to %ls\n", icoPath.c_str());
    } else {
        wprintf(L"Error: Unknown export type '%ls'. Use 'xcode', 'web', 'svg', or 'ico'.\n", exportType.c_str());
        delete bmp;
        return 1;
    }

    delete bmp;
    return ok ? 0 : 1;
}

static int HandleTheme(int argc, wchar_t* argv[], const CLSID* pngClsid) {
    if (argc < 1 || argv[0][0] == L'-') {
        wprintf(L"Usage: modo theme <ios18|visionos|titanium|nord|catppuccin|prism> [--apply]\n");
        return 1;
    }

    std::wstring themeId = ToLower(argv[0]);
    bool shouldApply = false;
    for (int i = 1; i < argc; ++i) {
        if (ToLower(argv[i]) == L"--apply") shouldApply = true;
    }

    int packIdx = 4; // WhiteSur default
    if (themeId == L"visionos" || themeId == L"glass") packIdx = 14;
    else if (themeId == L"titanium" || themeId == L"obsidian") packIdx = 18;
    else if (themeId == L"nord" || themeId == L"arctic") packIdx = 14;
    else if (themeId == L"catppuccin" || themeId == L"mocha") packIdx = 13;
    else if (themeId == L"prism" || themeId == L"cyber") packIdx = 12;
    else if (themeId == L"pixel" || themeId == L"pure") packIdx = 0;
    else if (themeId == L"material") packIdx = 1;
    else if (themeId == L"noir") packIdx = 2;
    else if (themeId == L"mono") packIdx = 3;
    else if (themeId == L"whitesur") packIdx = 4;
    else if (themeId == L"tela") packIdx = 5;
    else if (themeId == L"lawnicons") packIdx = 6;
    else if (themeId == L"macosicons") packIdx = 7;
    else if (themeId == L"tabler") packIdx = 8;
    else if (themeId == L"simpleicons" || themeId == L"simple") packIdx = 9;

    const IconPackDef& pk = kIconPacks[packIdx];
    wprintf(L"Selected Theme: %ls\n", pk.name);
    wprintf(L"  \x2022 Shape: %ls\n",
            (pk.shape == SHAPE_CIRCLE ? L"Circle" : pk.shape == SHAPE_SQUIRCLE ? L"Squircle" :
             pk.shape == SHAPE_ROUNDED_RECT ? L"Rounded Rect" : pk.shape == SHAPE_PEBBLE ? L"Pebble" : L"Hexagon"));
    wprintf(L"  \x2022 Background: %ls\n",
            (pk.preset == PRESET_GOOGLE_WHITE ? L"Pure White" :
             pk.preset == PRESET_MATERIAL_PASTEL ? L"Material Pastel" :
             pk.preset == PRESET_DARK_MATERIAL ? L"Dark Material" : L"Custom Palette"));
    wprintf(L"  \x2022 Surface Material: %ls\n",
            (pk.material == MATERIAL_FROSTED_GLASS ? L"Frosted Glass" :
             pk.material == MATERIAL_BRUSHED_TITANIUM ? L"Brushed Titanium" :
             pk.material == MATERIAL_NEUMORPHIC ? L"Neumorphic" :
             pk.material == MATERIAL_PRISM_GLOW ? L"Prism Glow" : L"Smooth Flat"));

    if (shouldApply) {
        std::vector<AppEntry> apps;
        wchar_t buf[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, buf))) {
            ShellIntegration::ScanDirectoryForShortcuts(buf, L"Рабочий стол", apps, 0, true);
        }
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, 0, buf))) {
            ShellIntegration::ScanDirectoryForShortcuts(buf, L"Общий стол", apps, 0, true);
        }
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, buf))) {
            ShellIntegration::ScanDirectoryForShortcuts(buf, L"Меню Пуск", apps, 0, false);
        }
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, buf))) {
            ShellIntegration::ScanDirectoryForShortcuts(buf, L"Общие программы", apps, 0, false);
        }

        wprintf(L"Applying %ls to %zu discovered applications...\n", pk.name, apps.size());

        RoundStyle style;
        style.preset = pk.preset;
        style.shape = pk.shape;
        style.scale = pk.scale;
        style.shadow = pk.shadow;
        style.innerShadow = pk.innerShadow;
        style.monochrome = pk.monochrome;
        style.customColor = pk.customColor;
        style.material = pk.material;

        int count = 0;
        uint32_t rev = (uint32_t)(GetTickCount() & 0xFFFF);
        for (size_t i = 0; i < apps.size(); ++i) {
            bool needsAdmin = false;
            if (ShellIntegration::ApplyRoundIconToEntry(apps[i], style, pngClsid, rev, &needsAdmin)) {
                count++;
            }
        }
        ShellIntegration::RefreshWindowsExplorerIconCache(true);
        wprintf(L"[+] Successfully styled %d/%zu applications with %ls!\n", count, apps.size(), pk.name);
    } else {
        wprintf(L"Run with '--apply' to apply this theme to installed applications in Start Menu & Desktop.\n");
    }

    return 0;
}

bool TryDispatchCLI(int argc, wchar_t* argv[], int& outExitCode, const CLSID* pngClsid) {
    if (argc <= 1) return false;

    std::wstring cmd = ToLower(argv[1]);

    // Check GUI setup / update switches: let GUI handle them
    if (cmd == L"--setup" || cmd == L"--install" || cmd == L"--uninstall" ||
        cmd == L"--update" || cmd == L"--portable" || cmd == L"--silent" || cmd == L"/s") {
        return false;
    }

    EnsureConsoleOutput();

    if (cmd == L"--help" || cmd == L"-h" || cmd == L"help" || cmd == L"/?") {
        PrintHelp();
        outExitCode = 0;
        ReleaseConsoleOutput();
        return true;
    }

    if (cmd == L"--version" || cmd == L"-v" || cmd == L"version") {
        PrintVersion();
        outExitCode = 0;
        ReleaseConsoleOutput();
        return true;
    }

    if (cmd == L"style") {
        outExitCode = HandleStyle(argc - 2, argv + 2, pngClsid);
        ReleaseConsoleOutput();
        return true;
    }

    if (cmd == L"restore") {
        outExitCode = HandleRestore(argc - 2, argv + 2);
        ReleaseConsoleOutput();
        return true;
    }

    if (cmd == L"folder") {
        outExitCode = HandleFolder(argc - 2, argv + 2, pngClsid);
        ReleaseConsoleOutput();
        return true;
    }

    if (cmd == L"batch") {
        outExitCode = HandleBatch(argc - 2, argv + 2, pngClsid);
        ReleaseConsoleOutput();
        return true;
    }

    if (cmd == L"showcase") {
        outExitCode = HandleShowcase(argc - 2, argv + 2, pngClsid);
        ReleaseConsoleOutput();
        return true;
    }

    if (cmd == L"export") {
        outExitCode = HandleExport(argc - 2, argv + 2, pngClsid);
        ReleaseConsoleOutput();
        return true;
    }

    if (cmd == L"theme") {
        outExitCode = HandleTheme(argc - 2, argv + 2, pngClsid);
        ReleaseConsoleOutput();
        return true;
    }

    if (cmd == L"shell") {
        if (argc >= 3 && std::wstring(argv[2]) == L"--enable") {
            wchar_t exePath[MAX_PATH * 2] = {0};
            GetModuleFileNameW(NULL, exePath, MAX_PATH * 2 - 1);
            ShellIntegration::RegisterShellContextMenu(exePath, true);
            wprintf(L"[+] Windows Explorer context menu enabled successfully.\n");
            outExitCode = 0;
        } else if (argc >= 3 && std::wstring(argv[2]) == L"--disable") {
            ShellIntegration::RegisterShellContextMenu(L"", false);
            wprintf(L"[+] Windows Explorer context menu disabled.\n");
            outExitCode = 0;
        } else {
            wprintf(L"[-] Unknown shell option. Use 'modo shell --enable' or 'modo shell --disable'.\n");
            outExitCode = 1;
        }
        ReleaseConsoleOutput();
        return true;
    }

    wprintf(L"Unknown command: '%ls'. Run 'modo --help' for usage.\n", argv[1]);
    outExitCode = 1;
    ReleaseConsoleOutput();
    return true;
}

} // namespace ModoCLI
