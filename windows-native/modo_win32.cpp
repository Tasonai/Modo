/*
 * Modo Studio for Windows — macOS-Canonical Native GDI+ Studio UI Engine
 * Designed by Atoma to strictly match Apple macOS Human Interface Guidelines.
 *
 * Fully independent of Windows OS theme:
 * - Dedicated macOS-style Monochrome Dark & Light themes with on-the-fly switching
 * - macOS window chrome with authentic Traffic Lights (Close, Minimize, Zoom)
 * - Unified macOS top toolbar with integrated theme segment, search & filters
 * - Clean card-based inspector with continuous rounded squircle corners
 * - 100% custom double-buffered GDI+ vector rendering (zero emojis)
 */

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "types.h"
#include "icon_engine.h"
#include "shell_integration.h"
#include "installer.h"
#include "updater.h"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <commoncontrols.h>
#include <objbase.h>
#include <gdiplus.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <algorithm>
#include <string>
#include <vector>

using namespace Gdiplus;
using namespace IconEngine;
using namespace ShellIntegration;

static const int WIN_W = 1140;
static const int WIN_H = 840;

enum WidgetID {
    W_NONE = 0,
    // macOS Window controls
    W_TRAFFIC_CLOSE,
    W_TRAFFIC_MIN,
    W_TRAFFIC_ZOOM,
    W_BTN_MODO_MENU,
    // Top bar theme switcher & actions
    W_BTN_THEME_DARK,
    W_BTN_THEME_LIGHT,
    W_BTN_ADMIN,
    W_BTN_SHORTCUTS,
    W_BTN_RESCAN,
    W_BTN_ADD_FILE,
    // Sidebar filter tabs
    W_TAB_ALL,
    W_TAB_APPLIED,
    W_TAB_SYSTEM,
    W_TAB_THIRDPARTY,
    W_SEARCH_BOX,
    W_SEARCH_CLEAR,
    W_APP_LIST_AREA,
    // Preview stage
    W_STAGE_ROUND_DRAG,
    // Preset pills
    W_PRESET_0,
    W_PRESET_1,
    W_PRESET_2,
    W_PRESET_3,
    W_PRESET_4,
    // Color & Eyedropper
    W_BTN_EYEDROPPER,
    W_BTN_PALETTE,
    W_SWATCH_0,
    W_SWATCH_1,
    W_SWATCH_2,
    W_SWATCH_3,
    W_SWATCH_4,
    W_SWATCH_5,
    W_SWATCH_6,
    W_SWATCH_7,
    // Sliders & step buttons
    W_SLIDER_SCALE,
    W_BTN_SC_68,
    W_BTN_SC_100,
    W_BTN_SC_135,
    W_BTN_SC_175,
    W_SLIDER_OFFX,
    W_BTN_LEFT,
    W_BTN_RIGHT,
    W_SLIDER_OFFY,
    W_BTN_DOWN,
    W_BTN_UP,
    W_BTN_CENTER,
    W_SLIDER_SHADOW,
    // Toggle switches
    W_TOGGLE_AUTOTRIM,
    W_TOGGLE_INNERSHAD,
    W_TOGGLE_FULLBLEED,
    W_TOGGLE_MONO,
    // Action dock buttons
    W_BTN_APPLY_ONE,
    W_BTN_APPLY_ALL,
    W_BTN_RESTORE_ONE,
    W_BTN_RESTORE_ALL,
    W_BTN_EXPORT,
    // Shape pills
    W_SHAPE_0, W_SHAPE_1, W_SHAPE_2, W_SHAPE_3, W_SHAPE_4,
    // Icon pack pills
    W_PACK_0, W_PACK_1, W_PACK_2, W_PACK_3, W_PACK_4, W_PACK_5, W_PACK_6, W_PACK_7
};

struct HitZone {
    WidgetID id;
    RectF rect;
};

// ============================================================================
// macOS MONOCHROME PALETTES (DARK & LIGHT)
// ============================================================================

struct AppThemeColors {
    bool isDark;
    Color canvas;
    Color topBar;
    Color sidebar;
    Color cardBg;
    Color cardBorder;
    Color wellBg;
    Color wellBorder;
    Color textPrimary;
    Color textSecondary;
    Color textMuted;
    Color divider;
    Color controlBg;
    Color controlBorder;
    Color controlHover;
    Color controlPressed;
    Color primaryBtnBg;
    Color primaryBtnBorder;
    Color primaryBtnText;
    Color selRowBg;
    Color selRowBorder;
    Color hovRowBg;
    Color toggleTrackOn;
    Color toggleTrackOff;
    Color toggleKnob;
    Color sliderGroove;
    Color sliderFill;
    Color sliderThumb;
    Color badgeBg;
    Color badgeBorder;
    Color badgeText;
    Color statusBar;
};

static AppThemeColors GetThemeColors(bool isDark) {
    AppThemeColors t;
    t.isDark = isDark;
    if (isDark) {
        // macOS Monochrome Deep Graphite (Dark Mode)
        t.canvas           = Color(255, 24, 24, 26);      // #18181A
        t.topBar           = Color(255, 32, 32, 35);      // #202023
        t.sidebar          = Color(255, 28, 28, 30);      // #1C1C1E
        t.cardBg           = Color(255, 36, 36, 40);      // #242428
        t.cardBorder       = Color(255, 52, 52, 58);      // #34343A
        t.wellBg           = Color(255, 20, 20, 22);      // #141416
        t.wellBorder       = Color(255, 44, 44, 48);      // #2C2C30
        t.textPrimary      = Color(255, 255, 255, 255);  // #FFFFFF
        t.textSecondary    = Color(255, 172, 172, 178);  // #ACACB2
        t.textMuted        = Color(255, 130, 130, 136);  // #828288
        t.divider          = Color(255, 46, 46, 50);      // #2E2E32
        t.controlBg        = Color(255, 44, 44, 48);      // #2C2C30
        t.controlBorder    = Color(255, 62, 62, 68);      // #3E3E44
        t.controlHover     = Color(255, 56, 56, 62);      // #38383E
        t.controlPressed   = Color(255, 36, 36, 40);      // #242428
        t.primaryBtnBg     = Color(255, 255, 255, 255);  // #FFFFFF
        t.primaryBtnBorder = Color(255, 255, 255, 255);
        t.primaryBtnText   = Color(255, 18, 18, 20);     // #121214
        t.selRowBg         = Color(255, 50, 50, 56);      // #323238
        t.selRowBorder     = Color(255, 82, 82, 92);
        t.hovRowBg         = Color(255, 38, 38, 42);      // #26262A
        t.toggleTrackOn    = Color(255, 245, 245, 248);
        t.toggleTrackOff   = Color(255, 50, 50, 54);
        t.toggleKnob       = Color(255, 20, 20, 22);
        t.sliderGroove     = Color(255, 48, 48, 52);
        t.sliderFill       = Color(255, 225, 225, 230);
        t.sliderThumb      = Color(255, 255, 255, 255);
        t.badgeBg          = Color(255, 38, 48, 40);
        t.badgeBorder      = Color(255, 60, 95, 65);
        t.badgeText        = Color(255, 130, 220, 140);
        t.statusBar        = Color(255, 26, 26, 28);
    } else {
        // macOS Monochrome Silver (Light Mode)
        t.canvas           = Color(255, 242, 242, 247);  // #F2F2F7
        t.topBar           = Color(255, 236, 236, 241);  // #ECECF1
        t.sidebar          = Color(255, 230, 230, 235);  // #E6E6EB
        t.cardBg           = Color(255, 255, 255, 255);  // #FFFFFF
        t.cardBorder       = Color(255, 220, 220, 225);  // #DCDCE1
        t.wellBg           = Color(255, 244, 244, 247);  // #F4F4F7
        t.wellBorder       = Color(255, 218, 218, 223);  // #DADAE0
        t.textPrimary      = Color(255, 24, 24, 28);     // #18181C
        t.textSecondary    = Color(255, 90, 90, 96);     // #5A5A60
        t.textMuted        = Color(255, 140, 140, 146);  // #8C8C92
        t.divider          = Color(255, 216, 216, 222);  // #D8D8DE
        t.controlBg        = Color(255, 244, 244, 247);  // #F4F4F7
        t.controlBorder    = Color(255, 210, 210, 216);  // #D2D2D8
        t.controlHover     = Color(255, 232, 232, 238);  // #E8E8EE
        t.controlPressed   = Color(255, 222, 222, 228);  // #DEDED4
        t.primaryBtnBg     = Color(255, 24, 24, 28);     // #18181C
        t.primaryBtnBorder = Color(255, 24, 24, 28);
        t.primaryBtnText   = Color(255, 255, 255, 255);  // #FFFFFF
        t.selRowBg         = Color(255, 218, 220, 228);  // #DADCDE
        t.selRowBorder     = Color(255, 180, 182, 192);
        t.hovRowBg         = Color(255, 236, 236, 242);  // #ECECF2
        t.toggleTrackOn    = Color(255, 24, 24, 28);
        t.toggleTrackOff   = Color(255, 214, 214, 220);
        t.toggleKnob       = Color(255, 255, 255, 255);
        t.sliderGroove     = Color(255, 216, 216, 222);
        t.sliderFill       = Color(255, 45, 45, 50);
        t.sliderThumb      = Color(255, 255, 255, 255);
        t.badgeBg          = Color(255, 232, 246, 236);
        t.badgeBorder      = Color(255, 180, 220, 190);
        t.badgeText        = Color(255, 30, 120, 50);
        t.statusBar        = Color(255, 236, 236, 241);
    }
    return t;
}

// App State
static ULONG_PTR g_gdiplusToken = 0;
static CLSID g_pngClsid = {0};
static HWND g_hMainWnd = nullptr;
static RoundStyle g_style;
static bool g_isDarkMode = true; // Independent theme switch
static std::vector<AppEntry> g_allApps;
static std::vector<int> g_filteredIndices;
static int g_selectedAppIndex = -1;
static int g_sidebarFilter = 0; // 0 = All, 1 = Applied only, 2 = System, 3 = Third-party
static int g_listScrollY = 0;
static int g_hoveredListRow = -1;

static bool g_searchFocused = false;
static std::wstring g_searchText;

static std::vector<HitZone> g_hitZones;
static WidgetID g_hoveredWidget = W_NONE;
static WidgetID g_activeWidget = W_NONE;

static bool g_isDraggingStage = false;
static POINT g_dragStartPt = {0, 0};
static double g_dragStartOffX = 0.0;
static double g_dragStartOffY = 0.0;

static bool g_isEyedropperActive = false;
static POINT g_cursorClientPt = {0, 0};
static COLORREF g_eyedropperLiveColor = RGB(26, 115, 232);

static std::wstring g_statusText = L"Готово: выберите приложение слева или настройте параметры иконки";
static double g_progressRatio = 0.0;
static uint32_t g_iconRevCounter = 1;

static int g_inspectorScrollY = 0;
static int g_maxInspectorScroll = 0;
static const RectF* g_currentClipRect = nullptr;

static int g_activePack = -1;

// Forward declarations
static void InvalidateAllThumbs();
static void RebuildFilteredList();

static void UpdateWindowImmersiveDarkMode(HWND hwnd, bool dark) {
    HMODULE hDwm = LoadLibraryW(L"dwmapi.dll");
    if (hDwm) {
        typedef HRESULT(WINAPI* PFN_DwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);
        PFN_DwmSetWindowAttribute fn = (PFN_DwmSetWindowAttribute)GetProcAddress(hDwm, "DwmSetWindowAttribute");
        if (fn) {
            BOOL bDark = dark ? TRUE : FALSE;
            fn(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &bDark, sizeof(bDark));
            fn(hwnd, 19 /* older Win10 build */, &bDark, sizeof(bDark));

            DWORD cornerPref = 2; // DWMWCP_ROUND
            fn(hwnd, 33, &cornerPref, sizeof(cornerPref));
        }
        FreeLibrary(hDwm);
    }
}

static bool GetPngEncoderClsid(CLSID* pClsid) {
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return false;
    ImageCodecInfo* pInfo = (ImageCodecInfo*)malloc(size);
    if (!pInfo) return false;
    GetImageEncoders(num, size, pInfo);
    bool found = false;
    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(pInfo[i].MimeType, L"image/png") == 0) {
            *pClsid = pInfo[i].Clsid;
            found = true;
            break;
        }
    }
    free(pInfo);
    return found;
}

static void InvalidateAllThumbs() {
    for (auto& app : g_allApps) {
        delete app.cachedThumb;
        app.cachedThumb = nullptr;
    }
}

static Bitmap* GetAppThumb(AppEntry& app) {
    if (!app.cachedThumb && app.origBitmap) {
        app.cachedThumb = RenderGoogleRoundIcon(app.origBitmap, g_style, 64);
    }
    return app.cachedThumb;
}

static void SaveStyleConfig() {
    std::wstring cfgPath = GetAppDataRoot() + L"\\style.cfg";
    wchar_t buf[64];
    WritePrivateProfileStringW(L"Style", L"Theme", g_isDarkMode ? L"dark" : L"light", cfgPath.c_str());
    wsprintfW(buf, L"%d", (int)g_style.preset);
    WritePrivateProfileStringW(L"Style", L"Preset", buf, cfgPath.c_str());
    wsprintfW(buf, L"%d", (int)g_style.shape);
    WritePrivateProfileStringW(L"Style", L"Shape", buf, cfgPath.c_str());
    wsprintfW(buf, L"%d", (int)std::round(g_style.scale * 1000.0));
    WritePrivateProfileStringW(L"Style", L"ScaleMilli", buf, cfgPath.c_str());
    wsprintfW(buf, L"%d", (int)std::round(g_style.offsetX * 1000.0));
    WritePrivateProfileStringW(L"Style", L"OffsetXMilli", buf, cfgPath.c_str());
    wsprintfW(buf, L"%d", (int)std::round(g_style.offsetY * 1000.0));
    WritePrivateProfileStringW(L"Style", L"OffsetYMilli", buf, cfgPath.c_str());
    wsprintfW(buf, L"%d", (int)std::round(g_style.shadow * 1000.0));
    WritePrivateProfileStringW(L"Style", L"ShadowMilli", buf, cfgPath.c_str());
    WritePrivateProfileStringW(L"Style", L"AutoTrim", g_style.autoTrim ? L"1" : L"0", cfgPath.c_str());
    WritePrivateProfileStringW(L"Style", L"InnerShadow", g_style.innerShadow ? L"1" : L"0", cfgPath.c_str());
    WritePrivateProfileStringW(L"Style", L"FullBleed", g_style.fullBleed ? L"1" : L"0", cfgPath.c_str());
    WritePrivateProfileStringW(L"Style", L"Monochrome", g_style.monochrome ? L"1" : L"0", cfgPath.c_str());
    wsprintfW(buf, L"%u", (unsigned)g_style.customColor);
    WritePrivateProfileStringW(L"Style", L"CustomColor", buf, cfgPath.c_str());
}

static void LoadStyleConfig() {
    std::wstring cfgPath = GetAppDataRoot() + L"\\style.cfg";
    if (GetFileAttributesW(cfgPath.c_str()) == INVALID_FILE_ATTRIBUTES) return;

    wchar_t themeBuf[32] = {0};
    GetPrivateProfileStringW(L"Style", L"Theme", L"dark", themeBuf, 31, cfgPath.c_str());
    g_isDarkMode = (_wcsicmp(themeBuf, L"light") != 0);

    g_style.preset = (BgPreset)std::clamp((int)GetPrivateProfileIntW(L"Style", L"Preset", 0, cfgPath.c_str()), 0, 4);
    g_style.shape = (IconShape)std::clamp((int)GetPrivateProfileIntW(L"Style", L"Shape", 0, cfgPath.c_str()), 0, 4);
    int sc = (int)GetPrivateProfileIntW(L"Style", L"ScaleMilli", 680, cfgPath.c_str());
    g_style.scale = std::clamp(sc / 1000.0, 0.30, 2.50);
    int ox = (int)GetPrivateProfileIntW(L"Style", L"OffsetXMilli", 0, cfgPath.c_str());
    g_style.offsetX = std::clamp(ox / 1000.0, -1.0, 1.0);
    int oy = (int)GetPrivateProfileIntW(L"Style", L"OffsetYMilli", 0, cfgPath.c_str());
    g_style.offsetY = std::clamp(oy / 1000.0, -1.0, 1.0);
    int sh = (int)GetPrivateProfileIntW(L"Style", L"ShadowMilli", 450, cfgPath.c_str());
    g_style.shadow = std::clamp(sh / 1000.0, 0.0, 1.0);
    g_style.autoTrim = GetPrivateProfileIntW(L"Style", L"AutoTrim", 1, cfgPath.c_str()) != 0;
    g_style.innerShadow = GetPrivateProfileIntW(L"Style", L"InnerShadow", 1, cfgPath.c_str()) != 0;
    g_style.fullBleed = GetPrivateProfileIntW(L"Style", L"FullBleed", 0, cfgPath.c_str()) != 0;
    g_style.monochrome = GetPrivateProfileIntW(L"Style", L"Monochrome", 0, cfgPath.c_str()) != 0;
    g_style.customColor = (COLORREF)GetPrivateProfileIntW(L"Style", L"CustomColor", (INT)RGB(26, 115, 232), cfgPath.c_str());
}

static void RebuildFilteredList() {
    std::wstring q = g_searchText;
    for (auto& c : q) c = towlower(c);

    int prevSelectedIdx = g_selectedAppIndex;
    g_filteredIndices.clear();

    for (size_t i = 0; i < g_allApps.size(); ++i) {
        const auto& app = g_allApps[i];
        if (g_sidebarFilter == 1 && !app.isApplied) continue;
        if (g_sidebarFilter == 2 && !(app.isFileExplorer || app.isRecycleBin || !app.systemClsid.empty() || app.category.find(L"Windows") != std::wstring::npos)) continue;
        if (g_sidebarFilter == 3 && (app.isFileExplorer || app.isRecycleBin || !app.systemClsid.empty() || app.category.find(L"Windows") != std::wstring::npos)) continue;

        if (!q.empty()) {
            std::wstring hay = app.name + L" " + app.searchKeywords + L" " + app.targetPath;
            for (auto& c : hay) c = towlower(c);
            if (hay.find(q) == std::wstring::npos) continue;
        }
        g_filteredIndices.push_back((int)i);
    }

    if (g_filteredIndices.empty()) {
        g_selectedAppIndex = -1;
    } else {
        bool kept = false;
        for (int idx : g_filteredIndices) {
            if (idx == prevSelectedIdx) { kept = true; break; }
        }
        if (!kept) g_selectedAppIndex = g_filteredIndices[0];
    }
    g_listScrollY = 0;
}

static void ScanInstalledWindowsApps() {
    for (auto& app : g_allApps) {
        delete app.origBitmap;
        delete app.cachedThumb;
    }
    g_allApps.clear();

    ShellIntegration::AddBuiltInSystemEntries(g_allApps, &g_pngClsid);

    wchar_t appData[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData)) && appData[0]) {
        std::wstring taskbarDir = std::wstring(appData) + L"\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar";
        if (GetFileAttributesW(taskbarDir.c_str()) != INVALID_FILE_ATTRIBUTES) {
            ShellIntegration::ScanDirectoryForShortcuts(taskbarDir, L"Панель задач", g_allApps, 0, true);
        }
    }

    const int folders[] = {
        CSIDL_DESKTOPDIRECTORY,
        CSIDL_COMMON_DESKTOPDIRECTORY,
        CSIDL_PROGRAMS,
        CSIDL_COMMON_PROGRAMS
    };
    const wchar_t* labels[] = {
        L"Рабочий стол",
        L"Общий Рабочий стол",
        L"Меню «Пуск»",
        L"Программы Windows"
    };
    const bool isDesk[] = { true, true, false, false };

    for (int i = 0; i < 4; ++i) {
        wchar_t path[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(NULL, folders[i], NULL, 0, path)) && path[0]) {
            ShellIntegration::ScanDirectoryForShortcuts(path, labels[i], g_allApps, 0, isDesk[i]);
        }
    }

    std::sort(g_allApps.begin(), g_allApps.end(), [](const AppEntry& a, const AppEntry& b) {
        if (a.isSystemPinnedTop != b.isSystemPinnedTop) return a.isSystemPinnedTop;
        if (a.isFileExplorer != b.isFileExplorer) return a.isFileExplorer;
        return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0;
    });

    RebuildFilteredList();
}

static bool ApplyRoundIcon(AppEntry& app, bool* outNeedsAdmin = nullptr) {
    return ShellIntegration::ApplyRoundIconToEntry(app, g_style, &g_pngClsid, g_iconRevCounter, outNeedsAdmin);
}

static void AddCustomAppOrImage(HWND hwnd) {
    wchar_t filePath[MAX_PATH] = {0};
    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"Приложения и иконки (*.exe;*.lnk;*.png;*.ico)\0*.exe;*.lnk;*.png;*.ico\0Все файлы (*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Выберите приложение (.exe/.lnk) или изображение (.png/.ico)";

    if (GetOpenFileNameW(&ofn) && filePath[0]) {
        std::wstring path = filePath;
        std::wstring fname = PathFindFileNameW(filePath);
        size_t dot = fname.find_last_of(L'.');
        std::wstring title = (dot != std::wstring::npos) ? fname.substr(0, dot) : fname;

        AppEntry entry;
        entry.name = title;
        if (_wcsicmp(PathFindExtensionW(filePath), L".lnk") == 0) {
            entry.lnkPath = path;
            entry.allLnkPaths.push_back(path);
            std::wstring customIcon;
            ResolveShortcut(path, entry.targetPath, customIcon);
            if (entry.targetPath.empty()) entry.targetPath = path;
        } else {
            entry.targetPath = path;
        }
        entry.category = L"Добавлено вручную";
        entry.origBitmap = LoadHighResIcon(entry.targetPath, entry.lnkPath);
        g_allApps.insert(g_allApps.begin(), entry);
        RebuildFilteredList();
        g_selectedAppIndex = 0;
        g_statusText = L"Добавлено: " + entry.name;
        InvalidateRect(hwnd, NULL, FALSE);
    }
}

static void ExportCurrentOrAllIcons(HWND hwnd) {
    BROWSEINFOW bi = {0};
    bi.hwndOwner = hwnd;
    bi.lpszTitle = L"Выберите папку для экспорта иконок (.ICO и 1024x1024 .PNG):";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return;

    wchar_t outDir[MAX_PATH] = {0};
    if (SHGetPathFromIDListW(pidl, outDir) && outDir[0]) {
        std::wstring dir = outDir;
        if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
            AppEntry& app = g_allApps[g_selectedAppIndex];
            std::wstring safe = SanitizeFileName(app.name);
            Bitmap* master1024 = RenderGoogleRoundIcon(app.origBitmap, g_style, 1024);
            std::wstring icoFile = dir + L"\\" + safe + L".ico";
            std::wstring pngFile = dir + L"\\" + safe + L".png";
            SaveMultiSizeIco(app.origBitmap, g_style, icoFile);
            master1024->Save(pngFile.c_str(), &g_pngClsid, NULL);
            delete master1024;
        }
        g_statusText = L"Экспортированы .ICO + 1024px .PNG в " + dir;
        InvalidateRect(hwnd, NULL, FALSE);
        MessageBoxW(hwnd, (L"Иконка (.ICO + 1024px .PNG) сохранена в:\n" + dir).c_str(),
                    L"Экспорт завершён", MB_ICONINFORMATION);
    }
    CoTaskMemFree(pidl);
}

// ============================================================================
// macOS HIG GDI+ CANVAS RENDERER (ZERO EMOJIS, PURE DESIGN CANONS)
// ============================================================================

static void RegisterHit(WidgetID id, RectF r) {
    if (g_currentClipRect) {
        if (r.GetBottom() < g_currentClipRect->Y || r.Y > g_currentClipRect->GetBottom()) {
            return; // Out of view in scrolled container
        }
    }
    g_hitZones.push_back({ id, r });
}

static void DrawModoBrandMark(Graphics& g, RectF r, const AppThemeColors& theme) {
    FillRoundRect(g, r, 7.0f, theme.isDark ? Color(255, 48, 48, 52) : Color(255, 230, 230, 235));
    DrawRoundRect(g, r, 7.0f, theme.isDark ? Color(255, 80, 80, 86) : Color(255, 200, 200, 208), 1.0f);
    Pen mPen(theme.isDark ? Color(255, 255, 255, 255) : Color(255, 20, 20, 22), 1.8f);
    g.DrawLine(&mPen, r.X + 7.0f, r.GetBottom() - 8.0f, r.X + 7.0f, r.Y + 8.0f);
    g.DrawLine(&mPen, r.X + 7.0f, r.Y + 8.0f, r.X + 14.0f, r.GetBottom() - 11.0f);
    g.DrawLine(&mPen, r.X + 14.0f, r.GetBottom() - 11.0f, r.GetRight() - 7.0f, r.Y + 8.0f);
    g.DrawLine(&mPen, r.GetRight() - 7.0f, r.Y + 8.0f, r.GetRight() - 7.0f, r.GetBottom() - 8.0f);
}

static void DrawSegmentedThemeSwitch(
    Graphics& g,
    RectF r,
    bool isDark,
    const AppThemeColors& theme,
    Font& font
) {
    FillRoundRect(g, r, 7.0f, theme.controlBg);
    DrawRoundRect(g, r, 7.0f, theme.controlBorder, 1.0f);

    REAL segW = (r.Width - 4.0f) * 0.5f;
    RectF darkR(r.X + 2.0f, r.Y + 2.0f, segW, r.Height - 4.0f);
    RectF lightR(r.X + 2.0f + segW, r.Y + 2.0f, segW, r.Height - 4.0f);

    RegisterHit(W_BTN_THEME_DARK, darkR);
    RegisterHit(W_BTN_THEME_LIGHT, lightR);

    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);

    if (isDark) {
        FillRoundRect(g, darkR, 6.0f, Color(255, 68, 68, 72));
        DrawRoundRect(g, darkR, 6.0f, Color(255, 120, 120, 125), 1.0f);
    } else {
        FillRoundRect(g, lightR, 6.0f, Color(255, 255, 255, 255));
        DrawRoundRect(g, lightR, 6.0f, Color(255, 200, 200, 205), 1.0f);
    }

    SolidBrush textDarkBr(isDark ? theme.textPrimary : theme.textSecondary);
    SolidBrush textLightBr(!isDark ? theme.textPrimary : theme.textSecondary);

    g.DrawString(L"Темная", -1, &font, darkR, &sf, &textDarkBr);
    g.DrawString(L"Светлая", -1, &font, lightR, &sf, &textLightBr);
}

static void DrawSegmentedFilterBar(
    Graphics& g,
    RectF r,
    int activeFilter,
    int totalApps,
    int appliedApps,
    const AppThemeColors& theme,
    Font& font
) {
    FillRoundRect(g, r, 7.0f, theme.controlBg);
    DrawRoundRect(g, r, 7.0f, theme.controlBorder, 1.0f);

    REAL segW = (r.Width - 4.0f) / 4.0f;
    WidgetID segIDs[4] = { W_TAB_ALL, W_TAB_APPLIED, W_TAB_SYSTEM, W_TAB_THIRDPARTY };

    wchar_t tabAllLbl[32], tabAppLbl[32];
    wsprintfW(tabAllLbl, L"Все (%d)", totalApps);
    wsprintfW(tabAppLbl, L"Круг (%d)", appliedApps);
    const wchar_t* titles[4] = { tabAllLbl, tabAppLbl, L"Система", L"Сторонние" };

    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);

    for (int i = 0; i < 4; ++i) {
        RectF sr(r.X + 2.0f + i * segW, r.Y + 2.0f, segW, r.Height - 4.0f);
        RegisterHit(segIDs[i], sr);

        bool isSel = (activeFilter == i);
        if (isSel) {
            FillRoundRect(g, sr, 5.0f, theme.isDark ? Color(255, 68, 68, 72) : Color(255, 255, 255, 255));
            DrawRoundRect(g, sr, 5.0f, theme.isDark ? Color(255, 110, 110, 115) : Color(255, 200, 200, 205), 1.0f);
        }
        SolidBrush tb(isSel ? theme.textPrimary : theme.textSecondary);
        g.DrawString(titles[i], -1, &font, sr, &sf, &tb);
    }
}

enum IconGlyphType {
    GLYPH_NONE = 0,
    GLYPH_ADMIN,
    GLYPH_SHORTCUTS,
    GLYPH_REFRESH,
    GLYPH_ADD_FILE,
    GLYPH_EYEDROPPER,
    GLYPH_PALETTE,
    GLYPH_CHECKMARK,
    GLYPH_BATCH,
    GLYPH_REVERT,
    GLYPH_TRASH,
    GLYPH_EXPORT,
    GLYPH_SHAPE_CIRCLE,
    GLYPH_SHAPE_SQUIRCLE,
    GLYPH_SHAPE_ROUNDRECT,
    GLYPH_SHAPE_PEBBLE,
    GLYPH_SHAPE_HEXAGON
};

static void DrawGlyphIcon(Graphics& g, IconGlyphType glyph, RectF r, Color clr) {
    Pen p(clr, 1.35f);
    p.SetStartCap(LineCapRound);
    p.SetEndCap(LineCapRound);
    SolidBrush b(clr);

    switch (glyph) {
    case GLYPH_ADMIN: {
        PointF pts[5] = {
            PointF(r.X + r.Width * 0.5f, r.Y + 1.0f),
            PointF(r.GetRight() - 1.0f, r.Y + 3.5f),
            PointF(r.GetRight() - 1.0f, r.Y + r.Height * 0.6f),
            PointF(r.X + r.Width * 0.5f, r.GetBottom()),
            PointF(r.X + 1.0f, r.Y + r.Height * 0.6f)
        };
        g.DrawPolygon(&p, pts, 5);
        break;
    }
    case GLYPH_SHORTCUTS: {
        g.DrawRectangle(&p, r.X + 1.0f, r.Y + 2.0f, r.Width - 2.0f, r.Height - 6.0f);
        g.DrawLine(&p, r.X + r.Width * 0.5f, r.GetBottom() - 4.0f, r.X + r.Width * 0.5f, r.GetBottom() - 1.0f);
        g.DrawLine(&p, r.X + 3.0f, r.GetBottom() - 1.0f, r.GetRight() - 3.0f, r.GetBottom() - 1.0f);
        break;
    }
    case GLYPH_REFRESH: {
        g.DrawArc(&p, r.X + 1.5f, r.Y + 1.5f, r.Width - 3.0f, r.Height - 3.0f, 40.0f, 280.0f);
        PointF pts[3] = {
            PointF(r.GetRight() - 1.0f, r.Y + r.Height * 0.4f),
            PointF(r.GetRight() - 5.0f, r.Y + 1.0f),
            PointF(r.GetRight() - 1.0f, r.Y + 1.0f)
        };
        g.FillPolygon(&b, pts, 3);
        break;
    }
    case GLYPH_ADD_FILE: {
        g.DrawLine(&p, r.X + r.Width * 0.5f, r.Y + 2.0f, r.X + r.Width * 0.5f, r.GetBottom() - 2.0f);
        g.DrawLine(&p, r.X + 2.0f, r.Y + r.Height * 0.5f, r.GetRight() - 2.0f, r.Y + r.Height * 0.5f);
        break;
    }
    case GLYPH_EYEDROPPER: {
        Pen ep(clr, 1.5f);
        ep.SetStartCap(LineCapRound);
        ep.SetEndCap(LineCapRound);
        g.DrawLine(&ep, r.X + 2.0f, r.GetBottom() - 2.0f, r.GetRight() - 3.0f, r.Y + 3.0f);
        g.DrawLine(&ep, r.GetRight() - 6.0f, r.Y + 2.0f, r.GetRight() - 2.0f, r.Y + 6.0f);
        break;
    }
    case GLYPH_PALETTE: {
        g.DrawEllipse(&p, r.X + 1.0f, r.Y + 1.0f, r.Width - 2.0f, r.Height - 2.0f);
        g.FillEllipse(&b, r.X + 4.0f, r.Y + 4.0f, 2.5f, 2.5f);
        g.FillEllipse(&b, r.GetRight() - 6.5f, r.Y + 4.0f, 2.5f, 2.5f);
        g.FillEllipse(&b, r.X + r.Width * 0.5f - 1.25f, r.GetBottom() - 6.0f, 2.5f, 2.5f);
        break;
    }
    case GLYPH_CHECKMARK: {
        Pen cp(clr, 1.8f);
        cp.SetStartCap(LineCapRound);
        cp.SetEndCap(LineCapRound);
        g.DrawLine(&cp, r.X + 2.0f, r.Y + r.Height * 0.55f, r.X + r.Width * 0.42f, r.GetBottom() - 3.0f);
        g.DrawLine(&cp, r.X + r.Width * 0.42f, r.GetBottom() - 3.0f, r.GetRight() - 2.0f, r.Y + 3.0f);
        break;
    }
    case GLYPH_BATCH: {
        Pen bp(clr, 1.3f);
        g.DrawRectangle(&bp, r.X + 4.0f, r.Y + 1.0f, r.Width - 5.0f, r.Height - 5.0f);
        g.DrawRectangle(&bp, r.X + 1.0f, r.Y + 4.0f, r.Width - 5.0f, r.Height - 5.0f);
        break;
    }
    case GLYPH_REVERT: {
        g.DrawArc(&p, r.X + 1.5f, r.Y + 1.5f, r.Width - 3.0f, r.Height - 3.0f, 130.0f, 280.0f);
        PointF pts[3] = {
            PointF(r.X + 1.0f, r.Y + r.Height * 0.4f),
            PointF(r.X + 5.0f, r.Y + 1.0f),
            PointF(r.X + 1.0f, r.Y + 1.0f)
        };
        g.FillPolygon(&b, pts, 3);
        break;
    }
    case GLYPH_TRASH: {
        g.DrawLine(&p, r.X + 3.0f, r.Y + 3.0f, r.GetRight() - 3.0f, r.GetBottom() - 3.0f);
        g.DrawLine(&p, r.GetRight() - 3.0f, r.Y + 3.0f, r.X + 3.0f, r.GetBottom() - 3.0f);
        break;
    }
    case GLYPH_EXPORT: {
        g.DrawLine(&p, r.X + 2.0f, r.GetBottom() - 2.0f, r.GetRight() - 2.0f, r.GetBottom() - 2.0f);
        g.DrawLine(&p, r.X + 2.0f, r.Y + r.Height * 0.5f, r.X + 2.0f, r.GetBottom() - 2.0f);
        g.DrawLine(&p, r.GetRight() - 2.0f, r.Y + r.Height * 0.5f, r.GetRight() - 2.0f, r.GetBottom() - 2.0f);
        REAL midX = r.X + r.Width * 0.5f;
        g.DrawLine(&p, midX, r.Y + 1.0f, midX, r.GetBottom() - 5.0f);
        g.DrawLine(&p, midX - 3.0f, r.Y + 4.0f, midX, r.Y + 1.0f);
        g.DrawLine(&p, midX + 3.0f, r.Y + 4.0f, midX, r.Y + 1.0f);
        break;
    }
    case GLYPH_SHAPE_CIRCLE: {
        g.DrawEllipse(&p, r.X + 2.0f, r.Y + 2.0f, r.Width - 4.0f, r.Height - 4.0f);
        break;
    }
    case GLYPH_SHAPE_SQUIRCLE: {
        DrawRoundRect(g, RectF(r.X + 2.0f, r.Y + 2.0f, r.Width - 4.0f, r.Height - 4.0f), 5.5f, clr, 1.35f);
        break;
    }
    case GLYPH_SHAPE_ROUNDRECT: {
        DrawRoundRect(g, RectF(r.X + 2.0f, r.Y + 2.0f, r.Width - 4.0f, r.Height - 4.0f), 3.0f, clr, 1.35f);
        break;
    }
    case GLYPH_SHAPE_PEBBLE: {
        g.DrawEllipse(&p, r.X + 1.5f, r.Y + 3.0f, r.Width - 3.0f, r.Height - 6.0f);
        break;
    }
    case GLYPH_SHAPE_HEXAGON: {
        PointF hpts[6];
        REAL cx = r.X + r.Width * 0.5f;
        REAL cy = r.Y + r.Height * 0.5f;
        REAL rad = r.Width * 0.42f;
        for (int i = 0; i < 6; ++i) {
            double ang = i * 3.14159265 / 3.0;
            hpts[i] = PointF(cx + (REAL)(rad * cos(ang)), cy + (REAL)(rad * sin(ang)));
        }
        g.DrawPolygon(&p, hpts, 6);
        break;
    }
    default:
        break;
    }
}

static void DrawStudioPillButton(
    Graphics& g,
    WidgetID id,
    RectF r,
    const wchar_t* label,
    Font& font,
    const AppThemeColors& theme,
    bool isPrimary = false,
    bool isSelected = false,
    bool isDanger = false,
    IconGlyphType glyph = GLYPH_NONE,
    Color dotColor = Color(0, 0, 0, 0)
) {
    RegisterHit(id, r);
    bool hovered = (g_hoveredWidget == id);
    bool pressed = (g_activeWidget == id);

    Color fill = theme.controlBg;
    Color border = theme.controlBorder;
    Color textClr = theme.textPrimary;

    if (isPrimary) {
        fill = pressed ? (theme.isDark ? Color(255, 210, 210, 215) : Color(255, 45, 45, 48))
             : (hovered ? (theme.isDark ? Color(255, 255, 255, 255) : Color(255, 18, 18, 20))
                        : theme.primaryBtnBg);
        border = theme.primaryBtnBorder;
        textClr = theme.primaryBtnText;
    } else if (isSelected) {
        fill = theme.isDark ? Color(255, 68, 68, 74) : Color(255, 216, 216, 224);
        border = theme.isDark ? Color(255, 130, 130, 138) : Color(255, 160, 160, 172);
        textClr = theme.textPrimary;
    } else if (isDanger) {
        fill = hovered ? (theme.isDark ? Color(255, 65, 42, 45) : Color(255, 255, 230, 232))
                       : (theme.isDark ? Color(255, 50, 36, 38) : Color(255, 253, 240, 242));
        border = theme.isDark ? Color(255, 100, 60, 65) : Color(255, 235, 180, 185);
        textClr = theme.isDark ? Color(255, 240, 160, 165) : Color(255, 185, 45, 55);
    } else if (pressed) {
        fill = theme.controlPressed;
        border = theme.controlBorder;
        textClr = theme.textPrimary;
    } else if (hovered) {
        fill = theme.controlHover;
        border = theme.isDark ? Color(255, 95, 95, 102) : Color(255, 180, 180, 188);
        textClr = theme.textPrimary;
    }

    REAL rad = std::min(7.0f, r.Height * 0.5f);
    FillRoundRect(g, r, rad, fill);
    DrawRoundRect(g, r, rad, border, 1.0f);

    REAL leftShift = 0.0f;
    if (glyph != GLYPH_NONE) {
        RectF gr(r.X + 8.0f, r.Y + (r.Height - 14.0f) * 0.5f, 14.0f, 14.0f);
        DrawGlyphIcon(g, glyph, gr, textClr);
        leftShift = 12.0f;
    } else if (dotColor.GetAlpha() > 0) {
        SolidBrush db(dotColor);
        g.FillEllipse(&db, r.X + 8.0f, r.Y + (r.Height - 8.0f) * 0.5f, 8.0f, 8.0f);
        leftShift = 10.0f;
    }

    RectF textR(r.X + leftShift, r.Y, r.Width - leftShift, r.Height);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);
    SolidBrush tb(textClr);
    g.DrawString(label, -1, &font, textR, &sf, &tb);
}

static void DrawStudioSlider(
    Graphics& g,
    WidgetID id,
    RectF trackRect,
    double normalizedValue,
    const AppThemeColors& theme
) {
    double v = std::clamp(normalizedValue, 0.0, 1.0);
    RectF hitRect(trackRect.X - 6.0f, trackRect.Y - 8.0f, trackRect.Width + 12.0f, trackRect.Height + 16.0f);
    RegisterHit(id, hitRect);

    bool hovered = (g_hoveredWidget == id || g_activeWidget == id);

    RectF groove(trackRect.X, trackRect.Y + trackRect.Height * 0.5f - 2.5f, trackRect.Width, 5.0f);
    FillRoundRect(g, groove, 2.5f, theme.sliderGroove);
    DrawRoundRect(g, groove, 2.5f, theme.controlBorder, 1.0f);

    REAL fillW = std::max(5.0f, (REAL)(groove.Width * v));
    RectF activeBar(groove.X, groove.Y, fillW, groove.Height);
    FillRoundRect(g, activeBar, 2.5f, theme.sliderFill);

    REAL thumbX = groove.X + (REAL)(groove.Width * v);
    REAL thumbY = groove.Y + groove.Height * 0.5f;
    REAL thumbR = hovered ? 8.0f : 7.0f;

    SolidBrush shadowBr(Color(70, 0, 0, 0));
    g.FillEllipse(&shadowBr, thumbX - thumbR, thumbY - thumbR + 1.5f, thumbR * 2.0f, thumbR * 2.0f);

    SolidBrush thumbBr(theme.sliderThumb);
    g.FillEllipse(&thumbBr, thumbX - thumbR, thumbY - thumbR, thumbR * 2.0f, thumbR * 2.0f);
    Pen ringPen(theme.isDark ? Color(255, 90, 90, 95) : Color(255, 190, 190, 195), 1.5f);
    g.DrawEllipse(&ringPen, thumbX - thumbR, thumbY - thumbR, thumbR * 2.0f, thumbR * 2.0f);
}

static void DrawStudioToggle(
    Graphics& g,
    WidgetID id,
    RectF rowRect,
    const wchar_t* label,
    bool isOn,
    Font& font,
    const AppThemeColors& theme
) {
    RegisterHit(id, rowRect);
    bool hovered = (g_hoveredWidget == id);

    RectF pill(rowRect.X, rowRect.Y + (rowRect.Height - 22.0f) * 0.5f, 38.0f, 22.0f);
    Color pillFill = isOn ? theme.toggleTrackOn : (hovered ? theme.controlHover : theme.toggleTrackOff);
    FillRoundRect(g, pill, 11.0f, pillFill);
    DrawRoundRect(g, pill, 11.0f, isOn ? theme.primaryBtnBorder : theme.controlBorder, 1.0f);

    REAL knobX = isOn ? (pill.GetRight() - 19.0f) : (pill.X + 3.0f);
    Color knobColor = theme.toggleKnob;
    SolidBrush knobBr(knobColor);
    g.FillEllipse(&knobBr, knobX, pill.Y + 3.0f, 16.0f, 16.0f);

    RectF textRect(pill.GetRight() + 12.0f, rowRect.Y, rowRect.Width - 50.0f, rowRect.Height);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentNear);
    sf.SetLineAlignment(StringAlignmentCenter);
    SolidBrush tb(hovered ? theme.textPrimary : theme.textSecondary);
    g.DrawString(label, -1, &font, textRect, &sf, &tb);
}

static void RenderStudioFrame(HDC hdc, int clientW, int clientH) {
    if (clientW <= 0 || clientH <= 0) return;

    Bitmap backbuffer(clientW, clientH, PixelFormat32bppARGB);
    Graphics g(&backbuffer);
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    g_hitZones.clear();
    g_currentClipRect = nullptr;
    AppThemeColors theme = GetThemeColors(g_isDarkMode);

    FontFamily ff(L"Segoe UI");
    Font fHero(&ff, 16.0f, FontStyleBold, UnitPixel);
    Font fLogo(&ff, 14.0f, FontStyleBold, UnitPixel);
    Font fCardHeader(&ff, 12.5f, FontStyleBold, UnitPixel);
    Font fBodyBold(&ff, 12.0f, FontStyleBold, UnitPixel);
    Font fBody(&ff, 11.5f, FontStyleRegular, UnitPixel);
    Font fSmallBold(&ff, 10.5f, FontStyleBold, UnitPixel);
    Font fSmall(&ff, 10.5f, FontStyleRegular, UnitPixel);

    // 1. Full Canvas
    g.Clear(theme.canvas);

    // 2. Windows Top Header Bar (56px)
    SolidBrush topBarBr(theme.topBar);
    g.FillRectangle(&topBarBr, 0, 0, clientW, 56);
    Pen divPen(theme.divider, 1.0f);
    g.DrawLine(&divPen, 0, 56, clientW, 56);

    // Modo Brand Mark + Title + Dropdown Menu
    RectF brandR(16.0f, 14.0f, 28.0f, 28.0f);
    DrawModoBrandMark(g, brandR, theme);

    SolidBrush primBr(theme.textPrimary);
    SolidBrush secBr(theme.textSecondary);
    SolidBrush mutBr(theme.textMuted);

    g.DrawString(L"Modo Studio", -1, &fLogo, PointF(52.0f, 13.0f), NULL, &primBr);
    g.DrawString(L"Студия адаптивных иконок", -1, &fSmall, PointF(52.0f, 32.0f), NULL, &secBr);

    RectF menuR(226.0f, 14.0f, 76.0f, 28.0f);
    DrawStudioPillButton(g, W_BTN_MODO_MENU, menuR, L"Меню %BE", fSmallBold, theme, false, false, false);

    // Top-right Toolbar Controls
    // Theme Switch (Темная | Светлая)
    RectF themeR((REAL)(clientW - 576), 14.0f, 132.0f, 28.0f);
    DrawSegmentedThemeSwitch(g, themeR, g_isDarkMode, theme, fSmallBold);

    // Administrator
    RectF adminR((REAL)(clientW - 436), 14.0f, 124.0f, 28.0f);
    DrawStudioPillButton(g, W_BTN_ADMIN, adminR, L"Администратор", fSmallBold, theme, false, false, false, GLYPH_ADMIN);

    // Desktop Shortcuts
    RectF scR((REAL)(clientW - 304), 14.0f, 96.0f, 28.0f);
    DrawStudioPillButton(g, W_BTN_SHORTCUTS, scR, L"Ярлыки", fSmallBold, theme, false, false, false, GLYPH_SHORTCUTS);

    // Rescan / Refresh
    RectF rescanR((REAL)(clientW - 200), 14.0f, 94.0f, 28.0f);
    DrawStudioPillButton(g, W_BTN_RESCAN, rescanR, L"Обновить", fSmallBold, theme, false, false, false, GLYPH_REFRESH);

    // Add File
    RectF addR((REAL)(clientW - 98), 14.0f, 84.0f, 28.0f);
    DrawStudioPillButton(g, W_BTN_ADD_FILE, addR, L"+ Файл", fSmallBold, theme, false, false, false, GLYPH_ADD_FILE);

    // 3. Left Sidebar — Application Browser (x: 0..340)
    int sideW = 340;
    SolidBrush sideBr(theme.sidebar);
    g.FillRectangle(&sideBr, 0, 57, sideW, clientH - 57 - 32);
    g.DrawLine(&divPen, sideW, 57, sideW, clientH - 32);

    int appliedTotal = 0;
    for (const auto& a : g_allApps) if (a.isApplied) appliedTotal++;

    // Segmented Filter Bar
    RectF filterR(14.0f, 66.0f, 312.0f, 28.0f);
    DrawSegmentedFilterBar(g, filterR, g_sidebarFilter, (int)g_allApps.size(), appliedTotal, theme, fSmallBold);

    // Search Field
    RectF searchRect(14.0f, 102.0f, 312.0f, 32.0f);
    RegisterHit(W_SEARCH_BOX, searchRect);
    FillRoundRect(g, searchRect, 7.0f, theme.wellBg);
    DrawRoundRect(g, searchRect, 7.0f, g_searchFocused ? theme.textPrimary : theme.controlBorder, 1.0f);

    // Draw Vector Search Loupe
    RectF searchLoupeR(searchRect.X + 8.0f, searchRect.Y + 9.0f, 14.0f, 14.0f);
    Pen loupePen(theme.textMuted, 1.35f);
    loupePen.SetStartCap(LineCapRound); loupePen.SetEndCap(LineCapRound);
    g.DrawEllipse(&loupePen, searchLoupeR.X, searchLoupeR.Y, 9.0f, 9.0f);
    g.DrawLine(&loupePen, searchLoupeR.X + 7.0f, searchLoupeR.Y + 7.0f, searchLoupeR.GetRight(), searchLoupeR.GetBottom());

    StringFormat sfVCenter;
    sfVCenter.SetLineAlignment(StringAlignmentCenter);
    sfVCenter.SetFormatFlags(StringFormatFlagsNoWrap);

    if (g_searchText.empty()) {
        std::wstring ph = g_searchFocused ? L"Введите название...| " : L"Поиск приложений...";
        g.DrawString(ph.c_str(), -1, &fBody, RectF(searchRect.X + 28.0f, searchRect.Y, searchRect.Width - 36.0f, searchRect.Height), &sfVCenter, &mutBr);
    } else {
        std::wstring disp = g_searchText + (g_searchFocused ? L"|" : L"");
        g.DrawString(disp.c_str(), -1, &fBody, RectF(searchRect.X + 28.0f, searchRect.Y, searchRect.Width - 58.0f, searchRect.Height), &sfVCenter, &primBr);
        RectF clearRect(searchRect.GetRight() - 26.0f, searchRect.Y + 4.0f, 24.0f, 24.0f);
        RegisterHit(W_SEARCH_CLEAR, clearRect);
        Pen xPen(theme.textMuted, 1.4f);
        g.DrawLine(&xPen, clearRect.X + 7.0f, clearRect.Y + 7.0f, clearRect.GetRight() - 7.0f, clearRect.GetBottom() - 7.0f);
        g.DrawLine(&xPen, clearRect.GetRight() - 7.0f, clearRect.Y + 7.0f, clearRect.X + 7.0f, clearRect.GetBottom() - 7.0f);
    }

    // Scrollable Application List
    RectF listArea(8.0f, 142.0f, (REAL)(sideW - 16), (REAL)(clientH - 142 - 38));
    RegisterHit(W_APP_LIST_AREA, listArea);

    GraphicsState listState = g.Save();
    g.SetClip(listArea);

    const int rowH = 52;
    int totalRowsH = (int)g_filteredIndices.size() * rowH;
    int maxScroll = std::max(0, totalRowsH - (int)listArea.Height);
    g_listScrollY = std::clamp(g_listScrollY, 0, maxScroll);

    for (size_t i = 0; i < g_filteredIndices.size(); ++i) {
        int appIdx = g_filteredIndices[i];
        REAL ry = listArea.Y + (REAL)(i * rowH - g_listScrollY);
        if (ry + rowH < listArea.Y || ry > listArea.GetBottom()) continue;

        RectF rowRect(listArea.X + 4.0f, ry + 2.0f, listArea.Width - 14.0f, (REAL)(rowH - 4));
        bool isSel = (appIdx == g_selectedAppIndex);
        bool isHov = ((int)i == g_hoveredListRow);

        if (isSel) {
            FillRoundRect(g, rowRect, 8.0f, theme.selRowBg);
            DrawRoundRect(g, rowRect, 8.0f, theme.selRowBorder, 1.0f);
            // Left vertical accent bar
            RectF barR(rowRect.X + 2.0f, rowRect.Y + 10.0f, 3.0f, rowRect.Height - 20.0f);
            FillRoundRect(g, barR, 1.5f, theme.isDark ? Color(255, 255, 255, 255) : Color(255, 0, 122, 255));
        } else if (isHov) {
            FillRoundRect(g, rowRect, 8.0f, theme.hovRowBg);
        }

        AppEntry& app = g_allApps[appIdx];
        if (Bitmap* thumb = GetAppThumb(app)) {
            RectF thumbR(rowRect.X + 10.0f, rowRect.Y + 6.0f, 36.0f, 36.0f);
            g.DrawImage(thumb, thumbR);
        }

        REAL textMaxW = app.isApplied ? (rowRect.Width - 124.0f) : (rowRect.Width - 56.0f);
        RectF titleR(rowRect.X + 54.0f, rowRect.Y + 7.0f, textMaxW, 18.0f);
        RectF subR(rowRect.X + 54.0f, rowRect.Y + 25.0f, textMaxW, 16.0f);
        StringFormat sfTrim;
        sfTrim.SetTrimming(StringTrimmingEllipsisCharacter);
        sfTrim.SetFormatFlags(StringFormatFlagsNoWrap);

        g.DrawString(app.name.c_str(), -1, &fBodyBold, titleR, &sfTrim, &primBr);

        wchar_t subBuf[160];
        wsprintfW(subBuf, L"%s · %u ярл.", app.category.c_str(), (unsigned)std::max<size_t>(1, app.allLnkPaths.size()));
        g.DrawString(subBuf, -1, &fSmall, subR, &sfTrim, &mutBr);

        if (app.isApplied) {
            RectF badgeR(rowRect.GetRight() - 62.0f, rowRect.Y + 15.0f, 54.0f, 20.0f);
            FillRoundRect(g, badgeR, 10.0f, theme.badgeBg);
            DrawRoundRect(g, badgeR, 10.0f, theme.badgeBorder, 1.0f);
            SolidBrush dotBr(theme.isDark ? Color(255, 52, 199, 89) : Color(255, 40, 160, 60));
            g.FillEllipse(&dotBr, badgeR.X + 6.0f, badgeR.Y + 6.0f, 7.0f, 7.0f);
            StringFormat sfB; sfB.SetAlignment(StringAlignmentCenter); sfB.SetLineAlignment(StringAlignmentCenter);
            SolidBrush bText(theme.badgeText);
            RectF badgeTextR(badgeR.X + 6.0f, badgeR.Y, badgeR.Width - 6.0f, badgeR.Height);
            g.DrawString(L"Modo", -1, &fSmallBold, badgeTextR, &sfB, &bText);
        }
    }

    if (maxScroll > 0) {
        REAL trackH = listArea.Height;
        REAL thumbH = std::max(28.0f, trackH * (trackH / (REAL)totalRowsH));
        REAL thumbY = listArea.Y + (trackH - thumbH) * ((REAL)g_listScrollY / (REAL)maxScroll);
        FillRoundRect(g, RectF(listArea.GetRight() - 5.0f, thumbY, 4.0f, thumbH), 2.0f, theme.isDark ? Color(140, 110, 118, 135) : Color(140, 160, 165, 175));
    }

    g.Restore(listState);

    // 4. Right Inspector Area
    REAL rx = (REAL)(sideW + 18);
    REAL rw = (REAL)(clientW - rx - 18);

    // Pinned Bottom Action Dock (Sticky above status bar)
    REAL dockH = 48.0f;
    REAL dockY = (REAL)(clientH - 32 - dockH);
    RectF dockArea(rx, dockY, rw, dockH);

    // Inspector Scrollable Viewport
    REAL inspTop = 62.0f;
    REAL inspH = dockY - inspTop - 6.0f;
    RectF inspClip(rx, inspTop, rw, inspH);

    const REAL kTotalInspectorContentH = 680.0f;
    g_maxInspectorScroll = std::max(0, (int)(kTotalInspectorContentH - inspH));
    g_inspectorScrollY = std::clamp(g_inspectorScrollY, 0, g_maxInspectorScroll);

    GraphicsState inspState = g.Save();
    g.SetClip(inspClip);
    g_currentClipRect = &inspClip;

    REAL baseY = inspTop - (REAL)g_inspectorScrollY;

    // --- CARD 1: Luxury Interactive Preview Stage (h: 216) ---
    RectF stageCard(rx, baseY, rw, 216.0f);
    FillRoundRect(g, stageCard, 10.0f, theme.cardBg);
    DrawRoundRect(g, stageCard, 10.0f, theme.cardBorder, 1.0f);

    std::wstring selTitle = (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size())
        ? g_allApps[g_selectedAppIndex].name
        : L"Приложение не выбрано";
    g.DrawString(selTitle.c_str(), -1, &fHero, PointF(rx + 18.0f, baseY + 12.0f), NULL, &primBr);

    wchar_t teleBuf[128];
    wsprintfW(teleBuf, L"Масштаб: %d%%  ·  X: %+d%%  ·  Y: %+d%%",
              (int)std::round(g_style.scale * 100.0),
              (int)std::round(g_style.offsetX * 100.0),
              (int)std::round(g_style.offsetY * 100.0));
    StringFormat sfRight; sfRight.SetAlignment(StringAlignmentFar);
    g.DrawString(teleBuf, -1, &fSmallBold, RectF(rx + 200.0f, baseY + 14.0f, rw - 218.0f, 18.0f), &sfRight, &secBr);

    // Ambient Luxury Stage Podiums
    REAL wellSize = 114.0f;
    REAL leftWellX = rx + 80.0f;
    REAL rightWellX = rx + rw - 80.0f - wellSize;
    REAL wellY = baseY + 44.0f;

    auto drawLuxuryPodium = [&](REAL px, REAL py, REAL pw, REAL ph) {
        RectF pr(px, py, pw, ph);
        RectF shR(px - 2.0f, py + 3.0f, pw + 4.0f, ph + 4.0f);
        FillRoundRect(g, shR, 14.0f, theme.isDark ? Color(50, 0, 0, 0) : Color(25, 0, 0, 0));
        FillRoundRect(g, pr, 12.0f, theme.wellBg);
        DrawRoundRect(g, pr, 12.0f, theme.wellBorder, 1.0f);
    };

    drawLuxuryPodium(leftWellX, wellY, wellSize, wellSize);
    drawLuxuryPodium(rightWellX, wellY, wellSize, wellSize);

    RectF roundStageRect(rightWellX, wellY, wellSize, wellSize);
    RegisterHit(W_STAGE_ROUND_DRAG, roundStageRect);

    if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
        AppEntry& app = g_allApps[g_selectedAppIndex];
        if (app.origBitmap) {
            g.DrawImage(app.origBitmap, RectF(leftWellX + 14.0f, wellY + 14.0f, wellSize - 28.0f, wellSize - 28.0f));
            Bitmap* hiRound = RenderGoogleRoundIcon(app.origBitmap, g_style, 256);
            if (hiRound) {
                FillRoundRect(g, RectF(rightWellX + 8.0f, wellY + 10.0f, wellSize - 16.0f, wellSize - 16.0f), 18.0f, Color(60, 0, 0, 0));
                g.DrawImage(hiRound, RectF(rightWellX + 7.0f, wellY + 7.0f, wellSize - 14.0f, wellSize - 14.0f));
                delete hiRound;
            }
        }
    }

    StringFormat sfCenter; sfCenter.SetAlignment(StringAlignmentCenter);
    g.DrawString(L"Оригинал", -1, &fSmallBold, PointF(leftWellX + wellSize * 0.5f, wellY + wellSize + 8.0f), &sfCenter, &mutBr);
    g.DrawString(L"Адаптивный результат (тяните для смещения)", -1, &fSmallBold,
                 PointF(rightWellX + wellSize * 0.5f, wellY + wellSize + 8.0f), &sfCenter, &secBr);

    // Center node connector arrow
    REAL midStageX = rx + rw * 0.5f;
    FillRoundRect(g, RectF(midStageX - 38.0f, wellY + 40.0f, 76.0f, 32.0f), 16.0f, theme.controlBg);
    DrawRoundRect(g, RectF(midStageX - 38.0f, wellY + 40.0f, 76.0f, 32.0f), 16.0f, theme.controlBorder, 1.0f);
    StringFormat sfMid; sfMid.SetAlignment(StringAlignmentCenter); sfMid.SetLineAlignment(StringAlignmentCenter);
    g.DrawString(L"!92 MODO", -1, &fSmallBold, RectF(midStageX - 38.0f, wellY + 40.0f, 76.0f, 32.0f), &sfMid, &primBr);

    // --- CARD 2: Surface Presets & Eyedropper Color Bar (h: 104) ---
    REAL card2Y = baseY + 226.0f;
    RectF colorCard(rx, card2Y, rw, 104.0f);
    FillRoundRect(g, colorCard, 10.0f, theme.cardBg);
    DrawRoundRect(g, colorCard, 10.0f, theme.cardBorder, 1.0f);

    RectF palGlyphR(rx + 16.0f, card2Y + 11.0f, 13.0f, 13.0f);
    DrawGlyphIcon(g, GLYPH_PALETTE, palGlyphR, theme.textPrimary);
    g.DrawString(L"Подложка и цвет", -1, &fCardHeader, PointF(rx + 34.0f, card2Y + 9.0f), NULL, &primBr);

    const wchar_t* presetNames[5] = {
        L"Google White",
        L"Material Pastel",
        L"Dark Material",
        L"Adaptive Edge",
        L"Custom Color"
    };
    REAL presetPillW = (rw - 32.0f - 4.0f * 6.0f) / 5.0f;
    for (int p = 0; p < 5; ++p) {
        RectF pr(rx + 16.0f + p * (presetPillW + 6.0f), card2Y + 28.0f, presetPillW, 28.0f);
        DrawStudioPillButton(g, (WidgetID)(W_PRESET_0 + p), pr, presetNames[p], fBody, theme, false, (int)g_style.preset == p);
    }

    // Eyedropper + Palette + Live Hex + 8 Swatches
    DrawStudioPillButton(
        g, W_BTN_EYEDROPPER,
        RectF(rx + 16.0f, card2Y + 64.0f, 128.0f, 28.0f),
        g_isEyedropperActive ? L"Выберите точку" : L"Пипетка",
        fBodyBold,
        theme,
        g_isEyedropperActive,
        g_isEyedropperActive,
        false,
        GLYPH_EYEDROPPER
    );
    DrawStudioPillButton(g, W_BTN_PALETTE, RectF(rx + 152.0f, card2Y + 64.0f, 104.0f, 28.0f), L"Палитра...", fBody, theme, false, false, false, GLYPH_PALETTE);

    // Current Color Pill + HEX
    Color curC(255, GetRValue(g_style.customColor), GetGValue(g_style.customColor), GetBValue(g_style.customColor));
    RectF hexBox(rx + 264.0f, card2Y + 64.0f, 106.0f, 28.0f);
    FillRoundRect(g, hexBox, 6.0f, theme.controlBg);
    DrawRoundRect(g, hexBox, 6.0f, theme.controlBorder, 1.0f);
    SolidBrush curSwBr(curC);
    g.FillEllipse(&curSwBr, hexBox.X + 8.0f, hexBox.Y + 5.0f, 18.0f, 18.0f);
    Pen swRing(Color(160, 255, 255, 255), 1.0f);
    g.DrawEllipse(&swRing, hexBox.X + 8.0f, hexBox.Y + 5.0f, 18.0f, 18.0f);

    wchar_t hexStr[32];
    wsprintfW(hexStr, L"#%02X%02X%02X", GetRValue(g_style.customColor), GetGValue(g_style.customColor), GetBValue(g_style.customColor));
    g.DrawString(hexStr, -1, &fBodyBold, RectF(hexBox.X + 32.0f, card2Y + 64.0f, 70.0f, 28.0f), &sfVCenter, &primBr);

    REAL swStartX = rx + 384.0f;
    for (int s = 0; s < 8; ++s) {
        RectF sr(swStartX + s * 28.0f, card2Y + 67.0f, 22.0f, 22.0f);
        RegisterHit((WidgetID)(W_SWATCH_0 + s), sr);
        Color sc(255, GetRValue(kSwatchPalette[s]), GetGValue(kSwatchPalette[s]), GetBValue(kSwatchPalette[s]));
        SolidBrush sbr(sc);
        g.FillEllipse(&sbr, sr);
        bool match = (g_style.customColor == kSwatchPalette[s] && g_style.preset == PRESET_CUSTOM_COLOR);
        Pen sp(match ? (theme.isDark ? Color(255, 255, 255, 255) : Color(255, 0, 0, 0)) : Color(120, 180, 180, 185), match ? 2.5f : 1.0f);
        g.DrawEllipse(&sp, sr);
    }

    // --- CARD 3: Shape geometry & curated icon packs (h: 96) ---
    REAL card3Y = card2Y + 114.0f;
    RectF shapeCard(rx, card3Y, rw, 96.0f);
    FillRoundRect(g, shapeCard, 10.0f, theme.cardBg);
    DrawRoundRect(g, shapeCard, 10.0f, theme.cardBorder, 1.0f);

    RectF shGlyphR(rx + 16.0f, card3Y + 9.0f, 13.0f, 13.0f);
    DrawGlyphIcon(g, GLYPH_SHAPE_SQUIRCLE, shGlyphR, theme.textPrimary);
    g.DrawString(L"Форма и коллекции иконок", -1, &fCardHeader, PointF(rx + 34.0f, card3Y + 7.0f), NULL, &primBr);

    {
        const wchar_t* shapeNames[5] = { L"Круг", L"Сквиркл", L"Скругл. квадрат", L"Галька", L"Шестиугольник" };
        IconGlyphType shapeGlyphs[5] = { GLYPH_SHAPE_CIRCLE, GLYPH_SHAPE_SQUIRCLE, GLYPH_SHAPE_ROUNDRECT, GLYPH_SHAPE_PEBBLE, GLYPH_SHAPE_HEXAGON };
        REAL shapeW = (rw - 32.0f - 4.0f * 6.0f) / 5.0f;
        for (int p = 0; p < 5; ++p) {
            RectF sr(rx + 16.0f + p * (shapeW + 6.0f), card3Y + 28.0f, shapeW, 26.0f);
            DrawStudioPillButton(g, (WidgetID)(W_SHAPE_0 + p), sr, shapeNames[p], fBody, theme, false, (int)g_style.shape == p, false, shapeGlyphs[p]);
        }

        Color packDots[8] = {
            Color(255, 10, 132, 255),  // WhiteSur Apple Blue
            Color(255, 66, 133, 244),  // Pixel Blue
            Color(255, 136, 192, 208), // Nordic Frost
            Color(255, 245, 194, 231), // Catppuccin
            Color(255, 0, 255, 157),   // Neon Cyber
            Color(255, 142, 142, 147), // Obsidian
            Color(255, 191, 90, 242),  // Tela
            Color(255, 255, 159, 10)   // Lawnicons
        };

        REAL packW = (rw - 32.0f - 7.0f * 6.0f) / 8.0f;
        for (int p = 0; p < 8; ++p) {
            RectF pr(rx + 16.0f + p * (packW + 6.0f), card3Y + 60.0f, packW, 26.0f);
            DrawStudioPillButton(g, (WidgetID)(W_PACK_0 + p), pr, kIconPacks[p].name, fSmallBold, theme, false, g_activePack == p, false, GLYPH_NONE, packDots[p]);
        }
    }

    // --- CARD 4: Geometry & Sliders (h: 154) ---
    REAL card4Y = card3Y + 106.0f;
    RectF geoCard(rx, card4Y, rw, 154.0f);
    FillRoundRect(g, geoCard, 10.0f, theme.cardBg);
    DrawRoundRect(g, geoCard, 10.0f, theme.cardBorder, 1.0f);

    RectF slGlyphR(rx + 16.0f, card4Y + 10.0f, 13.0f, 13.0f);
    DrawGlyphIcon(g, GLYPH_BATCH, slGlyphR, theme.textPrimary);
    g.DrawString(L"Геометрия и масштаб", -1, &fCardHeader, PointF(rx + 34.0f, card4Y + 8.0f), NULL, &primBr);

    // Row 1: Scale (30% .. 250%)
    wchar_t lblBuf[128];
    wsprintfW(lblBuf, L"Масштаб значка: %d%%", (int)std::round(g_style.scale * 100.0));
    g.DrawString(lblBuf, -1, &fBodyBold, PointF(rx + 16.0f, card4Y + 34.0f), NULL, &primBr);
    DrawStudioSlider(g, W_SLIDER_SCALE, RectF(rx + 220.0f, card4Y + 32.0f, rw - 310.0f, 22.0f), (g_style.scale - 0.30) / (2.50 - 0.30), theme);
    DrawStudioPillButton(g, W_BTN_SC_100, RectF(rx + rw - 76.0f, card4Y + 31.0f, 60.0f, 24.0f), L"100%", fSmallBold, theme, false, std::abs(g_style.scale - 1.00) < 0.01);

    // Row 2: Horizontal X (-100% .. +100%)
    wsprintfW(lblBuf, L"Смещение по X: %+d%%", (int)std::round(g_style.offsetX * 100.0));
    g.DrawString(lblBuf, -1, &fBodyBold, PointF(rx + 16.0f, card4Y + 64.0f), NULL, &primBr);
    DrawStudioSlider(g, W_SLIDER_OFFX, RectF(rx + 220.0f, card4Y + 62.0f, rw - 310.0f, 22.0f), (g_style.offsetX + 1.0) * 0.5, theme);
    DrawStudioPillButton(g, W_BTN_CENTER, RectF(rx + rw - 76.0f, card4Y + 61.0f, 60.0f, 24.0f), L"Центр", fSmallBold, theme);

    // Row 3: Vertical Y (-100% .. +100%)
    wsprintfW(lblBuf, L"Смещение по Y: %+d%%", (int)std::round(g_style.offsetY * 100.0));
    g.DrawString(lblBuf, -1, &fBodyBold, PointF(rx + 16.0f, card4Y + 94.0f), NULL, &primBr);
    DrawStudioSlider(g, W_SLIDER_OFFY, RectF(rx + 220.0f, card4Y + 92.0f, rw - 310.0f, 22.0f), (g_style.offsetY + 1.0) * 0.5, theme);

    // Row 4: Outer Drop Shadow (0% .. 100%)
    wsprintfW(lblBuf, L"Внешняя тень: %d%%", (int)std::round(g_style.shadow * 100.0));
    g.DrawString(lblBuf, -1, &fBodyBold, PointF(rx + 16.0f, card4Y + 124.0f), NULL, &primBr);
    DrawStudioSlider(g, W_SLIDER_SHADOW, RectF(rx + 220.0f, card4Y + 122.0f, rw - 236.0f, 22.0f), g_style.shadow, theme);

    // --- CARD 5: Options Toggles (h: 68) ---
    REAL card5Y = card4Y + 164.0f;
    RectF optCard(rx, card5Y, rw, 68.0f);
    FillRoundRect(g, optCard, 10.0f, theme.cardBg);
    DrawRoundRect(g, optCard, 10.0f, theme.cardBorder, 1.0f);

    REAL halfW = (rw - 36.0f) * 0.5f;
    DrawStudioToggle(g, W_TOGGLE_AUTOTRIM,  RectF(rx + 16.0f,         card5Y + 8.0f,  halfW, 24.0f), L"Авто-обрезка прозрачных полей", g_style.autoTrim, fBody, theme);
    DrawStudioToggle(g, W_TOGGLE_INNERSHAD, RectF(rx + 24.0f + halfW, card5Y + 8.0f,  halfW, 24.0f), L"Внутренняя тень иконки",        g_style.innerShadow, fBody, theme);
    DrawStudioToggle(g, W_TOGGLE_FULLBLEED, RectF(rx + 16.0f,         card5Y + 36.0f, halfW, 24.0f), L"Квадратные иконки на всю форму", g_style.fullBleed, fBody, theme);
    DrawStudioToggle(g, W_TOGGLE_MONO,      RectF(rx + 24.0f + halfW, card5Y + 36.0f, halfW, 24.0f), L"Монохромный тон Material You",  g_style.monochrome, fBody, theme);

    // Inspector Scrollbar Indicator
    if (g_maxInspectorScroll > 0) {
        REAL trackH = inspH;
        REAL thumbH = std::max(28.0f, trackH * (trackH / kTotalInspectorContentH));
        REAL thumbY = inspTop + (trackH - thumbH) * ((REAL)g_inspectorScrollY / (REAL)g_maxInspectorScroll);
        FillRoundRect(g, RectF(rx + rw - 4.0f, thumbY, 3.0f, thumbH), 1.5f, theme.isDark ? Color(140, 110, 118, 135) : Color(140, 160, 165, 175));
    }

    g.Restore(inspState);
    g_currentClipRect = nullptr;

    // --- 5. PERMANENT STICKY ACTION DOCK (NEVER CLIPPED) ---
    FillRoundRect(g, dockArea, 10.0f, theme.cardBg);
    DrawRoundRect(g, dockArea, 10.0f, theme.cardBorder, 1.0f);

    REAL btnY = dockY + 5.0f;
    REAL btnH = 38.0f;
    REAL gap = 8.0f;
    REAL btn1W = 196.0f;
    REAL btn2W = 164.0f;
    REAL btn3W = 110.0f;
    REAL btn4W = 120.0f;
    REAL btn5W = rw - (btn1W + btn2W + btn3W + btn4W + gap * 6.0f);
    if (btn5W < 95.0f) btn5W = 95.0f;

    REAL curX = rx + 8.0f;
    DrawStudioPillButton(g, W_BTN_APPLY_ONE,   RectF(curX, btnY, btn1W, btnH), L"Применить к выбранному", fBodyBold, theme, true, false, false, GLYPH_CHECKMARK);
    curX += btn1W + gap;
    DrawStudioPillButton(g, W_BTN_APPLY_ALL,   RectF(curX, btnY, btn2W, btnH), L"Применить ко всем",      fBodyBold, theme, false, true, false, GLYPH_BATCH);
    curX += btn2W + gap;
    DrawStudioPillButton(g, W_BTN_RESTORE_ONE, RectF(curX, btnY, btn3W, btnH), L"Оригинал",               fBodyBold, theme, false, false, false, GLYPH_REVERT);
    curX += btn3W + gap;
    DrawStudioPillButton(g, W_BTN_RESTORE_ALL, RectF(curX, btnY, btn4W, btnH), L"Сбросить всё",           fBodyBold, theme, false, false, true,  GLYPH_TRASH);
    curX += btn4W + gap;
    DrawStudioPillButton(g, W_BTN_EXPORT,      RectF(curX, btnY, btn5W, btnH), L"Экспорт...",             fBodyBold, theme, false, false, false, GLYPH_EXPORT);

    // 6. Bottom Status Bar (y: clientH - 32 .. clientH)
    SolidBrush footBr(theme.statusBar);
    g.FillRectangle(&footBr, 0, clientH - 32, clientW, 32);
    g.DrawLine(&divPen, 0, clientH - 32, clientW, clientH - 32);

    StringFormat sfTrimEllipsis;
    sfTrimEllipsis.SetTrimming(StringTrimmingEllipsisCharacter);
    sfTrimEllipsis.SetLineAlignment(StringAlignmentCenter);
    sfTrimEllipsis.SetFormatFlags(StringFormatFlagsNoWrap);
    g.DrawString(g_statusText.c_str(), -1, &fSmall, RectF(16.0f, (REAL)(clientH - 32), (REAL)(clientW - 260), 32.0f), &sfTrimEllipsis, &mutBr);

    if (g_progressRatio > 0.001) {
        RectF progBg((REAL)(clientW - 220), (REAL)(clientH - 20), 200.0f, 8.0f);
        FillRoundRect(g, progBg, 4.0f, theme.controlBg);
        RectF progFg(progBg.X, progBg.Y, (REAL)(progBg.Width * std::clamp(g_progressRatio, 0.0, 1.0)), 8.0f);
        FillRoundRect(g, progFg, 4.0f, theme.primaryBtnBg);
    }

    // 7. Floating Eyedropper Magnifier Loupe
    if (g_isEyedropperActive) {
        REAL lx = (REAL)std::clamp((int)g_cursorClientPt.x + 22, 20, clientW - 150);
        REAL ly = (REAL)std::clamp((int)g_cursorClientPt.y - 54, 20, clientH - 64);
        RectF loupe(lx, ly, 128.0f, 42.0f);
        FillRoundRect(g, loupe, 10.0f, theme.cardBg);
        DrawRoundRect(g, loupe, 10.0f, theme.cardBorder, 1.5f);

        Color lc(255, GetRValue(g_eyedropperLiveColor), GetGValue(g_eyedropperLiveColor), GetBValue(g_eyedropperLiveColor));
        SolidBrush lbr(lc);
        g.FillEllipse(&lbr, lx + 8.0f, ly + 8.0f, 26.0f, 26.0f);
        Pen swRing(Color(160, 255, 255, 255), 1.0f);
        g.DrawEllipse(&swRing, lx + 8.0f, ly + 8.0f, 26.0f, 26.0f);

        wchar_t lhex[32];
        wsprintfW(lhex, L"#%02X%02X%02X", GetRValue(g_eyedropperLiveColor), GetGValue(g_eyedropperLiveColor), GetBValue(g_eyedropperLiveColor));
        g.DrawString(lhex, -1, &fBodyBold, RectF(lx + 42.0f, ly, 80.0f, 42.0f), &sfVCenter, &primBr);
    }

    Graphics screenG(hdc);
    screenG.DrawImage(&backbuffer, 0, 0);
}

static WidgetID HitTestWidget(POINT pt) {
    PointF pf((REAL)pt.x, (REAL)pt.y);
    for (int i = (int)g_hitZones.size() - 1; i >= 0; --i) {
        if (g_hitZones[i].rect.Contains(pf)) {
            return g_hitZones[i].id;
        }
    }
    return W_NONE;
}

static bool GetWidgetRect(WidgetID id, RectF& outRect) {
    for (const auto& z : g_hitZones) {
        if (z.id == id) { outRect = z.rect; return true; }
    }
    return false;
}

static void UpdateSliderFromMouse(WidgetID id, int mouseX, bool isFinalRelease = false) {
    RectF r;
    if (!GetWidgetRect(id, r)) return;
    double norm = std::clamp((double)(mouseX - (r.X + 6.0f)) / std::max(1.0, (double)(r.Width - 12.0f)), 0.0, 1.0);
    if (id == W_SLIDER_SCALE) {
        g_style.scale = 0.30 + norm * (2.50 - 0.30);
    } else if (id == W_SLIDER_OFFX) {
        g_style.offsetX = -1.0 + norm * 2.0;
    } else if (id == W_SLIDER_OFFY) {
        g_style.offsetY = -1.0 + norm * 2.0;
    } else if (id == W_SLIDER_SHADOW) {
        g_style.shadow = norm;
    }
    if (isFinalRelease) {
        InvalidateAllThumbs();
    }
    InvalidateRect(g_hMainWnd, NULL, FALSE);
}

enum MenuCommandID {
    IDM_APP_ABOUT = 10001,
    IDM_APP_EXIT,
    IDM_APP_CHECK_UPDATES,

    // File
    IDM_FILE_ADD_CUSTOM,
    IDM_FILE_ADD_FOLDER,
    IDM_FILE_EXPORT,
    IDM_FILE_RESCAN,

    // Edit
    IDM_EDIT_FIND,
    IDM_EDIT_CLEAR_SEARCH,

    // View
    IDM_VIEW_DARK_THEME,
    IDM_VIEW_LIGHT_THEME,
    IDM_VIEW_FILTER_ALL,
    IDM_VIEW_FILTER_APPLIED,
    IDM_VIEW_FILTER_SYSTEM,
    IDM_VIEW_FILTER_THIRDPARTY,

    // Style
    IDM_STYLE_APPLY_ONE,
    IDM_STYLE_APPLY_ALL,
    IDM_STYLE_RESTORE_ONE,
    IDM_STYLE_SHAPE_CIRCLE,
    IDM_STYLE_SHAPE_SQUIRCLE,
    IDM_STYLE_SHAPE_PEBBLE,
    IDM_STYLE_SHAPE_HEXAGON,
    IDM_STYLE_SHAPE_ROUNDEDRECT,
    IDM_STYLE_EYEDROPPER,
    IDM_STYLE_RESET_DEFAULT,
    IDM_STYLE_PACK_BASE = 10100,

    // Tools
    IDM_TOOLS_ADMIN,
    IDM_TOOLS_SHORTCUTS,
    IDM_TOOLS_RESTART_EXPLORER,
    IDM_TOOLS_SHORTCUTS_INFO
};

static void HandleWidgetClick(HWND hwnd, WidgetID id, POINT pt);

static void ShowModoAppMenu(HWND hwnd, POINT screenPt) {
    HMENU hMainMenu = CreatePopupMenu();

    // 1. File Menu
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_ADD_CUSTOM, L"Добавить приложение или файл…\tCtrl+N");
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_ADD_FOLDER, L"Добавить папку для стилизации…\tCtrl+O");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_EXPORT, L"Экспортировать иконку (.ICO, .PNG)…\tCtrl+E");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_RESCAN, L"Обновить список приложений\tF5");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFileMenu, MF_STRING, IDM_APP_EXIT, L"Закрыть Modo\tAlt+F4");
    AppendMenuW(hMainMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"Файл");

    // 2. Edit Menu
    HMENU hEditMenu = CreatePopupMenu();
    AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_FIND, L"Найти приложение…\tCtrl+F");
    AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_CLEAR_SEARCH, L"Сбросить поиск\tEsc");
    AppendMenuW(hMainMenu, MF_POPUP, (UINT_PTR)hEditMenu, L"Правка");

    // 3. View Menu
    HMENU hViewMenu = CreatePopupMenu();
    AppendMenuW(hViewMenu, (g_isDarkMode ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_VIEW_DARK_THEME, L"Тёмная тема (Монохром)");
    AppendMenuW(hViewMenu, (!g_isDarkMode ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_VIEW_LIGHT_THEME, L"Светлая тема (Монохром)");
    AppendMenuW(hViewMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hViewMenu, (g_sidebarFilter == 0 ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_VIEW_FILTER_ALL, L"Все приложения");
    AppendMenuW(hViewMenu, (g_sidebarFilter == 1 ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_VIEW_FILTER_APPLIED, L"Стилизованные (Круг)");
    AppendMenuW(hViewMenu, (g_sidebarFilter == 2 ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_VIEW_FILTER_SYSTEM, L"Системные приложения");
    AppendMenuW(hViewMenu, (g_sidebarFilter == 3 ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_VIEW_FILTER_THIRDPARTY, L"Сторонние приложения");
    AppendMenuW(hMainMenu, MF_POPUP, (UINT_PTR)hViewMenu, L"Вид");

    // 4. Style Menu
    HMENU hStyleMenu = CreatePopupMenu();
    AppendMenuW(hStyleMenu, MF_STRING, IDM_STYLE_APPLY_ONE, L"Применить к выбранному\tEnter");
    AppendMenuW(hStyleMenu, MF_STRING, IDM_STYLE_APPLY_ALL, L"Применить ко всем\tShift+Enter");
    AppendMenuW(hStyleMenu, MF_STRING, IDM_STYLE_RESTORE_ONE, L"Восстановить оригинал\tDelete");
    AppendMenuW(hStyleMenu, MF_SEPARATOR, 0, NULL);

    HMENU hShapeSub = CreatePopupMenu();
    AppendMenuW(hShapeSub, (g_style.shape == SHAPE_CIRCLE ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_SHAPE_CIRCLE, L"Круг (Classic Circle)");
    AppendMenuW(hShapeSub, (g_style.shape == SHAPE_SQUIRCLE ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_SHAPE_SQUIRCLE, L"Сквиркл (Squircle)");
    AppendMenuW(hShapeSub, (g_style.shape == SHAPE_PEBBLE ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_SHAPE_PEBBLE, L"Pebble (Галька)");
    AppendMenuW(hShapeSub, (g_style.shape == SHAPE_HEXAGON ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_SHAPE_HEXAGON, L"Шестиугольник (Hexagon)");
    AppendMenuW(hShapeSub, (g_style.shape == SHAPE_ROUNDED_RECT ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_SHAPE_ROUNDEDRECT, L"Скругленный прямоугольник");
    AppendMenuW(hStyleMenu, MF_POPUP, (UINT_PTR)hShapeSub, L"Форма подложки (Surface)");

    HMENU hPackSub = CreatePopupMenu();
    int totalPacks = (int)(sizeof(kIconPacks) / sizeof(kIconPacks[0]));
    for (int i = 0; i < totalPacks; ++i) {
        UINT flags = (g_activePack == i ? MF_CHECKED : MF_UNCHECKED) | MF_STRING;
        AppendMenuW(hPackSub, flags, IDM_STYLE_PACK_BASE + i, kIconPacks[i].name);
    }
    AppendMenuW(hStyleMenu, MF_POPUP, (UINT_PTR)hPackSub, L"Пакеты стилей (Icon Packs)");

    AppendMenuW(hStyleMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hStyleMenu, MF_STRING, IDM_STYLE_EYEDROPPER, L"Пипетка цвета фона…");
    AppendMenuW(hStyleMenu, MF_STRING, IDM_STYLE_RESET_DEFAULT, L"Сбросить стиль по умолчанию");
    AppendMenuW(hMainMenu, MF_POPUP, (UINT_PTR)hStyleMenu, L"Стиль");

    // 5. Tools Menu
    HMENU hToolsMenu = CreatePopupMenu();
    AppendMenuW(hToolsMenu, MF_STRING, IDM_TOOLS_ADMIN, L"Интеграция в контекстное меню Windows…");
    AppendMenuW(hToolsMenu, MF_STRING, IDM_TOOLS_SHORTCUTS, L"Создать ярлыки на Рабочем столе…");
    AppendMenuW(hToolsMenu, MF_STRING, IDM_TOOLS_RESTART_EXPLORER, L"Перезапустить Проводник Windows");
    AppendMenuW(hMainMenu, MF_POPUP, (UINT_PTR)hToolsMenu, L"Инструменты");

    // 6. Help Menu
    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenuW(hHelpMenu, MF_STRING, IDM_APP_CHECK_UPDATES, L"Проверить обновления…");
    AppendMenuW(hHelpMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hHelpMenu, MF_STRING, IDM_TOOLS_SHORTCUTS_INFO, L"Горячие клавиши…\tF1");
    AppendMenuW(hHelpMenu, MF_STRING, IDM_APP_ABOUT, L"О программе Modo Studio…");
    AppendMenuW(hMainMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"Справка");

    int cmd = TrackPopupMenu(
        hMainMenu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON,
        screenPt.x, screenPt.y,
        0, hwnd, NULL
    );
    DestroyMenu(hMainMenu);

    if (cmd == 0) return;

    POINT dummyPt = {0, 0};
    switch (cmd) {
        case IDM_FILE_ADD_CUSTOM:
        case IDM_FILE_ADD_FOLDER:
            AddCustomAppOrImage(hwnd);
            break;
        case IDM_FILE_EXPORT:
            HandleWidgetClick(hwnd, W_BTN_EXPORT, dummyPt);
            break;
        case IDM_FILE_RESCAN:
            HandleWidgetClick(hwnd, W_BTN_RESCAN, dummyPt);
            break;
        case IDM_APP_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        case IDM_EDIT_FIND:
            g_searchFocused = true;
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case IDM_EDIT_CLEAR_SEARCH:
            g_searchText.clear();
            RebuildFilteredList();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case IDM_VIEW_DARK_THEME:
            HandleWidgetClick(hwnd, W_BTN_THEME_DARK, dummyPt);
            break;
        case IDM_VIEW_LIGHT_THEME:
            HandleWidgetClick(hwnd, W_BTN_THEME_LIGHT, dummyPt);
            break;
        case IDM_VIEW_FILTER_ALL:
            HandleWidgetClick(hwnd, W_TAB_ALL, dummyPt);
            break;
        case IDM_VIEW_FILTER_APPLIED:
            HandleWidgetClick(hwnd, W_TAB_APPLIED, dummyPt);
            break;
        case IDM_VIEW_FILTER_SYSTEM:
            HandleWidgetClick(hwnd, W_TAB_SYSTEM, dummyPt);
            break;
        case IDM_VIEW_FILTER_THIRDPARTY:
            HandleWidgetClick(hwnd, W_TAB_THIRDPARTY, dummyPt);
            break;
        case IDM_STYLE_APPLY_ONE:
            HandleWidgetClick(hwnd, W_BTN_APPLY_ONE, dummyPt);
            break;
        case IDM_STYLE_APPLY_ALL:
            HandleWidgetClick(hwnd, W_BTN_APPLY_ALL, dummyPt);
            break;
        case IDM_STYLE_RESTORE_ONE:
            HandleWidgetClick(hwnd, W_BTN_RESTORE_ONE, dummyPt);
            break;
        case IDM_STYLE_SHAPE_CIRCLE:
            HandleWidgetClick(hwnd, W_SHAPE_0, dummyPt);
            break;
        case IDM_STYLE_SHAPE_SQUIRCLE:
            HandleWidgetClick(hwnd, W_SHAPE_1, dummyPt);
            break;
        case IDM_STYLE_SHAPE_PEBBLE:
            HandleWidgetClick(hwnd, W_SHAPE_2, dummyPt);
            break;
        case IDM_STYLE_SHAPE_HEXAGON:
            HandleWidgetClick(hwnd, W_SHAPE_3, dummyPt);
            break;
        case IDM_STYLE_SHAPE_ROUNDEDRECT:
            HandleWidgetClick(hwnd, W_SHAPE_4, dummyPt);
            break;
        case IDM_STYLE_EYEDROPPER:
            HandleWidgetClick(hwnd, W_BTN_EYEDROPPER, dummyPt);
            break;
        case IDM_STYLE_RESET_DEFAULT:
            HandleWidgetClick(hwnd, W_PACK_0, dummyPt);
            break;
        case IDM_TOOLS_ADMIN:
            HandleWidgetClick(hwnd, W_BTN_ADMIN, dummyPt);
            break;
        case IDM_TOOLS_SHORTCUTS:
            HandleWidgetClick(hwnd, W_BTN_SHORTCUTS, dummyPt);
            break;
        case IDM_TOOLS_RESTART_EXPLORER:
            RefreshWindowsExplorerIconCache(true);
            g_statusText = L"Кэш и Проводник Windows перезапущены";
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case IDM_TOOLS_SHORTCUTS_INFO:
            MessageBoxW(
                hwnd,
                L"Горячие клавиши Modo Studio (Windows):\n\n"
                L"  • Ctrl+N / Ctrl+O: Добавить приложение или папку\n"
                L"  • Ctrl+E: Экспорт иконки (.ICO + 1024px .PNG)\n"
                L"  • Ctrl+F: Поиск приложения в списке\n"
                L"  • Enter: Применить адаптивную иконку к выбранному\n"
                L"  • Shift+Enter: Применить ко всем отфильтрованным\n"
                L"  • Delete: Восстановить оригинальную иконку\n"
                L"  • F5: Обновить список приложений и рескан\n"
                L"  • Alt / Alt+M: Открыть главное меню Modo\n"
                L"  • Esc: Сброс поиска / отмена пипетки",
                L"Горячие клавиши Modo Studio",
                MB_OK | MB_ICONINFORMATION
            );
            break;
        case IDM_APP_CHECK_UPDATES:
            ModoUpdater::CheckForUpdates(hwnd, false);
            break;
        case IDM_APP_ABOUT:
            MessageBoxW(
                hwnd,
                L"Modo Studio 1.7.0 для Windows\n"
                L"Нативная студия адаптивных иконок по канонам Apple macOS HIG.\n\n"
                L"Разработчик: Atoma\n"
                L"Движок: Нативный Win32 / GDI+ (PE32+ чистая сборка)\n"
                L"Поддержка тем: Тёмная и Светлая монохромные палитры\n\n"
                L"Соответствует стандартам оформления macOS и Windows.",
                L"О программе Modo Studio",
                MB_OK | MB_ICONINFORMATION
            );
            break;
        default: {
            int totalPacks = (int)(sizeof(kIconPacks) / sizeof(kIconPacks[0]));
            if (cmd >= IDM_STYLE_PACK_BASE && cmd < IDM_STYLE_PACK_BASE + totalPacks) {
                int packIdx = cmd - IDM_STYLE_PACK_BASE;
                const IconPackDef& pk = kIconPacks[packIdx];
                g_style.preset = pk.preset;
                g_style.shape = pk.shape;
                g_style.scale = pk.scale;
                g_style.shadow = pk.shadow;
                g_style.innerShadow = pk.innerShadow;
                g_style.monochrome = pk.monochrome;
                g_style.customColor = pk.customColor;
                g_style.offsetX = 0.0;
                g_style.offsetY = 0.0;
                g_activePack = packIdx;
                SaveStyleConfig();
                InvalidateAllThumbs();
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }
    }
}

static void HandleWidgetClick(HWND hwnd, WidgetID id, POINT pt) {
    switch (id) {
        case W_TRAFFIC_CLOSE:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        case W_TRAFFIC_MIN:
            ShowWindow(hwnd, SW_MINIMIZE);
            break;
        case W_TRAFFIC_ZOOM: {
            WINDOWPLACEMENT wp = { sizeof(wp) };
            GetWindowPlacement(hwnd, &wp);
            ShowWindow(hwnd, wp.showCmd == SW_SHOWMAXIMIZED ? SW_RESTORE : SW_MAXIMIZE);
            break;
        }
        case W_BTN_MODO_MENU: {
            RectF menuR;
            if (GetWidgetRect(W_BTN_MODO_MENU, menuR)) {
                POINT p = { (LONG)menuR.X, (LONG)menuR.GetBottom() + 2 };
                ClientToScreen(hwnd, &p);
                ShowModoAppMenu(hwnd, p);
            }
            break;
        }

        case W_BTN_THEME_DARK:
            if (!g_isDarkMode) {
                g_isDarkMode = true;
                UpdateWindowImmersiveDarkMode(hwnd, true);
                SaveStyleConfig();
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        case W_BTN_THEME_LIGHT:
            if (g_isDarkMode) {
                g_isDarkMode = false;
                UpdateWindowImmersiveDarkMode(hwnd, false);
                SaveStyleConfig();
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;

        case W_SEARCH_BOX:
            g_searchFocused = true;
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case W_SEARCH_CLEAR:
            g_searchText.clear();
            RebuildFilteredList();
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case W_TAB_ALL:
            g_sidebarFilter = 0;
            RebuildFilteredList();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case W_TAB_APPLIED:
            g_sidebarFilter = 1;
            RebuildFilteredList();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case W_TAB_SYSTEM:
            g_sidebarFilter = 2;
            RebuildFilteredList();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case W_TAB_THIRDPARTY:
            g_sidebarFilter = 3;
            RebuildFilteredList();
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case W_APP_LIST_AREA: {
            RectF listArea;
            if (GetWidgetRect(W_APP_LIST_AREA, listArea)) {
                int relY = pt.y - (int)listArea.Y + g_listScrollY;
                int row = relY / 50;
                if (row >= 0 && row < (int)g_filteredIndices.size()) {
                    g_selectedAppIndex = g_filteredIndices[row];
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
            break;
        }

        case W_BTN_ADMIN: {
            SaveStyleConfig();
            wchar_t selfPath[MAX_PATH * 2] = {0};
            if (GetModuleFileNameW(NULL, selfPath, MAX_PATH * 2 - 1)) {
                if ((INT_PTR)ShellExecuteW(hwnd, L"runas", selfPath, NULL, NULL, SW_SHOWNORMAL) > 32) {
                    PostQuitMessage(0);
                }
            }
            break;
        }
        case W_BTN_SHORTCUTS:
            InstallSelfAndShortcuts(hwnd, true);
            break;
        case W_BTN_RESCAN:
            g_statusText = L"Сканирование установленных приложений...";
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
            ScanInstalledWindowsApps();
            g_statusText = L"Список приложений обновлён";
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case W_BTN_ADD_FILE:
            AddCustomAppOrImage(hwnd);
            break;

        case W_SHAPE_0: case W_SHAPE_1: case W_SHAPE_2: case W_SHAPE_3: case W_SHAPE_4:
            g_style.shape = (IconShape)(id - W_SHAPE_0);
            g_activePack = -1;
            SaveStyleConfig();
            InvalidateAllThumbs();
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case W_PACK_0: case W_PACK_1: case W_PACK_2: case W_PACK_3:
        case W_PACK_4: case W_PACK_5: case W_PACK_6: case W_PACK_7: {
            int idx = id - W_PACK_0;
            const IconPackDef& pk = kIconPacks[idx];
            g_style.preset = pk.preset;
            g_style.shape = pk.shape;
            g_style.scale = pk.scale;
            g_style.shadow = pk.shadow;
            g_style.innerShadow = pk.innerShadow;
            g_style.monochrome = pk.monochrome;
            g_style.customColor = pk.customColor;
            g_style.offsetX = 0.0;
            g_style.offsetY = 0.0;
            g_activePack = idx;
            SaveStyleConfig();
            InvalidateAllThumbs();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case W_PRESET_0:
        case W_PRESET_1:
        case W_PRESET_2:
        case W_PRESET_3:
        case W_PRESET_4:
            g_style.preset = (BgPreset)(id - W_PRESET_0);
            g_activePack = -1;
            SaveStyleConfig();
            InvalidateAllThumbs();
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case W_BTN_EYEDROPPER:
            g_isEyedropperActive = true;
            g_eyedropperLiveColor = g_style.customColor;
            SetCapture(hwnd);
            SetCursor(LoadCursor(NULL, IDC_CROSS));
            g_statusText = L"Пипетка активна: кликните точку на экране (ПКМ для отмены)";
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case W_BTN_PALETTE: {
            static COLORREF cust[16] = {0};
            CHOOSECOLORW cc = {0};
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = hwnd;
            cc.lpCustColors = cust;
            cc.rgbResult = g_style.customColor;
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColorW(&cc)) {
                g_style.customColor = cc.rgbResult;
                g_style.preset = PRESET_CUSTOM_COLOR;
                SaveStyleConfig();
                InvalidateAllThumbs();
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }

        case W_SWATCH_0:
        case W_SWATCH_1:
        case W_SWATCH_2:
        case W_SWATCH_3:
        case W_SWATCH_4:
        case W_SWATCH_5:
        case W_SWATCH_6:
        case W_SWATCH_7:
            g_style.customColor = kSwatchPalette[id - W_SWATCH_0];
            g_style.preset = PRESET_CUSTOM_COLOR;
            SaveStyleConfig();
            InvalidateAllThumbs();
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case W_BTN_SC_68:  g_style.scale = 0.68; SaveStyleConfig(); InvalidateAllThumbs(); InvalidateRect(hwnd, NULL, FALSE); break;
        case W_BTN_SC_100: g_style.scale = 1.00; SaveStyleConfig(); InvalidateAllThumbs(); InvalidateRect(hwnd, NULL, FALSE); break;
        case W_BTN_SC_135: g_style.scale = 1.35; SaveStyleConfig(); InvalidateAllThumbs(); InvalidateRect(hwnd, NULL, FALSE); break;
        case W_BTN_SC_175: g_style.scale = 1.75; SaveStyleConfig(); InvalidateAllThumbs(); InvalidateRect(hwnd, NULL, FALSE); break;

        case W_BTN_LEFT:
            g_style.offsetX = std::clamp(g_style.offsetX - 0.05, -1.0, 1.0);
            SaveStyleConfig(); InvalidateAllThumbs(); InvalidateRect(hwnd, NULL, FALSE); break;
        case W_BTN_RIGHT:
            g_style.offsetX = std::clamp(g_style.offsetX + 0.05, -1.0, 1.0);
            SaveStyleConfig(); InvalidateAllThumbs(); InvalidateRect(hwnd, NULL, FALSE); break;
        case W_BTN_DOWN:
            g_style.offsetY = std::clamp(g_style.offsetY - 0.05, -1.0, 1.0);
            SaveStyleConfig(); InvalidateAllThumbs(); InvalidateRect(hwnd, NULL, FALSE); break;
        case W_BTN_UP:
            g_style.offsetY = std::clamp(g_style.offsetY + 0.05, -1.0, 1.0);
            SaveStyleConfig(); InvalidateAllThumbs(); InvalidateRect(hwnd, NULL, FALSE); break;
        case W_BTN_CENTER:
            g_style.offsetX = 0.0; g_style.offsetY = 0.0;
            SaveStyleConfig(); InvalidateAllThumbs(); InvalidateRect(hwnd, NULL, FALSE); break;

        case W_TOGGLE_AUTOTRIM:
            g_style.autoTrim = !g_style.autoTrim;
            SaveStyleConfig(); InvalidateAllThumbs(); InvalidateRect(hwnd, NULL, FALSE); break;
        case W_TOGGLE_INNERSHAD:
            g_style.innerShadow = !g_style.innerShadow;
            SaveStyleConfig(); InvalidateAllThumbs(); InvalidateRect(hwnd, NULL, FALSE); break;
        case W_TOGGLE_FULLBLEED:
            g_style.fullBleed = !g_style.fullBleed;
            SaveStyleConfig(); InvalidateAllThumbs(); InvalidateRect(hwnd, NULL, FALSE); break;
        case W_TOGGLE_MONO:
            g_style.monochrome = !g_style.monochrome;
            SaveStyleConfig(); InvalidateAllThumbs(); InvalidateRect(hwnd, NULL, FALSE); break;

        case W_BTN_APPLY_ONE: {
            if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
                SaveStyleConfig();
                AppEntry& app = g_allApps[g_selectedAppIndex];
                bool needsAdmin = false;
                ApplyRoundIcon(app, &needsAdmin);
                RefreshWindowsExplorerIconCache(app.isFileExplorer);
                g_statusText = L"Готово: иконка применена к " + app.name;
                InvalidateRect(hwnd, NULL, FALSE);
                if (needsAdmin) {
                    int ans = MessageBoxW(
                        hwnd,
                        L"Этот ярлык находится в общей системной папке (Public Desktop / ProgramData).\n\n"
                        L"Разрешить обновление с правами Администратора, чтобы иконка обновилась на Рабочем столе?",
                        L"Modo Studio — Системный ярлык",
                        MB_YESNO | MB_ICONQUESTION
                    );
                    if (ans == IDYES) {
                        wchar_t selfPath[MAX_PATH * 2] = {0};
                        if (GetModuleFileNameW(NULL, selfPath, MAX_PATH * 2 - 1)) {
                            if ((INT_PTR)ShellExecuteW(hwnd, L"runas", selfPath, L"--apply-all-elevated", NULL, SW_SHOWNORMAL) > 32) {
                                PostQuitMessage(0);
                            }
                        }
                    }
                }
            }
            break;
        }

        case W_BTN_APPLY_ALL: {
            if (g_filteredIndices.empty()) break;
            SaveStyleConfig();
            int count = 0;
            bool anyNeedsAdmin = false;
            for (size_t i = 0; i < g_filteredIndices.size(); ++i) {
                AppEntry& app = g_allApps[g_filteredIndices[i]];
                bool na = false;
                if (ApplyRoundIcon(app, &na)) count++;
                if (na) anyNeedsAdmin = true;
                g_progressRatio = (double)(i + 1) / (double)g_filteredIndices.size();
                g_statusText = L"Применение иконок: " + app.name;
                InvalidateRect(hwnd, NULL, FALSE);
                UpdateWindow(hwnd);
            }
            RefreshWindowsExplorerIconCache(true);
            g_progressRatio = 0.0;

            if (anyNeedsAdmin) {
                int ans = MessageBoxW(
                    hwnd,
                    L"Часть ярлыков на Рабочем столе находится в общей папке C:\\Users\\Public\\Desktop (Chrome, Steam, Edge и др.).\n\n"
                    L"Нажмите «Да», чтобы применить с правами Администратора для всех ярлыков.",
                    L"Modo Studio — Применить ко всем ярлыкам",
                    MB_YESNO | MB_ICONQUESTION
                );
                if (ans == IDYES) {
                    wchar_t selfPath[MAX_PATH * 2] = {0};
                    if (GetModuleFileNameW(NULL, selfPath, MAX_PATH * 2 - 1)) {
                        if ((INT_PTR)ShellExecuteW(hwnd, L"runas", selfPath, L"--apply-all-elevated", NULL, SW_SHOWNORMAL) > 32) {
                            PostQuitMessage(0);
                            break;
                        }
                    }
                }
            }

            wchar_t msg[384];
            wsprintfW(msg, L"Готово: стиль применён к %d приложениям.", count);
            g_statusText = msg;
            InvalidateRect(hwnd, NULL, FALSE);
            MessageBoxW(hwnd, msg, L"Modo Studio", MB_ICONINFORMATION);
            break;
        }

        case W_BTN_RESTORE_ONE: {
            if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
                AppEntry& app = g_allApps[g_selectedAppIndex];
                RestoreOriginalEntry(app);
                RefreshWindowsExplorerIconCache(app.isFileExplorer);
                g_statusText = L"Восстановлена оригинальная иконка для " + app.name;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }

        case W_BTN_RESTORE_ALL: {
            for (auto& app : g_allApps) {
                RestoreOriginalEntry(app);
            }
            RefreshWindowsExplorerIconCache(true);
            g_statusText = L"Оригинальные иконки восстановлены для всех приложений";
            InvalidateRect(hwnd, NULL, FALSE);
            MessageBoxW(hwnd, L"Оригинальные иконки восстановлены для всех приложений.", L"Modo Studio", MB_ICONINFORMATION);
            break;
        }

        case W_BTN_EXPORT:
            ExportCurrentOrAllIcons(hwnd);
            break;

        default:
            break;
    }
}

static LRESULT CALLBACK StudioWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            RenderStudioFrame(hdc, rc.right - rc.left, rc.bottom - rc.top);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_SETCURSOR: {
            if (g_isEyedropperActive) {
                SetCursor(LoadCursor(NULL, IDC_CROSS));
                return TRUE;
            }
            if (g_hoveredWidget != W_NONE && g_hoveredWidget != W_APP_LIST_AREA) {
                SetCursor(LoadCursor(NULL, IDC_HAND));
                return TRUE;
            }
            break;
        }

        case WM_LBUTTONDOWN: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            g_cursorClientPt = pt;

            if (g_isEyedropperActive) {
                POINT screenPt = pt;
                ClientToScreen(hwnd, &screenPt);
                HDC screenDC = GetDC(NULL);
                if (screenDC) {
                    COLORREF sampled = GetPixel(screenDC, screenPt.x, screenPt.y);
                    ReleaseDC(NULL, screenDC);
                    if (sampled != CLR_INVALID) {
                        g_style.customColor = sampled;
                        g_style.preset = PRESET_CUSTOM_COLOR;
                        InvalidateAllThumbs();
                    }
                }
                g_isEyedropperActive = false;
                ReleaseCapture();
                wchar_t msgBuf[128];
                wsprintfW(msgBuf, L"Пипетка: выбран цвет #%02X%02X%02X",
                          GetRValue(g_style.customColor), GetGValue(g_style.customColor), GetBValue(g_style.customColor));
                g_statusText = msgBuf;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            WidgetID hit = HitTestWidget(pt);
            g_activeWidget = hit;
            if (hit != W_SEARCH_BOX) g_searchFocused = false;

            if (hit == W_STAGE_ROUND_DRAG) {
                g_isDraggingStage = true;
                g_dragStartPt = pt;
                g_dragStartOffX = g_style.offsetX;
                g_dragStartOffY = g_style.offsetY;
                SetCapture(hwnd);
                return 0;
            }

            if (hit == W_SLIDER_SCALE || hit == W_SLIDER_OFFX || hit == W_SLIDER_OFFY || hit == W_SLIDER_SHADOW) {
                SetCapture(hwnd);
                UpdateSliderFromMouse(hit, pt.x);
                return 0;
            }

            if (hit == W_NONE && pt.y <= 56) {
                // Drag window by macOS top toolbar
                ReleaseCapture();
                SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
                return 0;
            }

            HandleWidgetClick(hwnd, hit, pt);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        case WM_LBUTTONDBLCLK: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            WidgetID hit = HitTestWidget(pt);
            if (hit == W_STAGE_ROUND_DRAG) {
                g_style.offsetX = 0.0;
                g_style.offsetY = 0.0;
                InvalidateAllThumbs();
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if (hit == W_NONE && pt.y <= 56) {
                WINDOWPLACEMENT wp = { sizeof(wp) };
                GetWindowPlacement(hwnd, &wp);
                ShowWindow(hwnd, wp.showCmd == SW_SHOWMAXIMIZED ? SW_RESTORE : SW_MAXIMIZE);
                return 0;
            }
            break;
        }

        case WM_RBUTTONDOWN: {
            if (g_isEyedropperActive) {
                g_isEyedropperActive = false;
                ReleaseCapture();
                g_statusText = L"Пипетка отменена";
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            break;
        }

        case WM_MOUSEMOVE: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            g_cursorClientPt = pt;

            if (g_isEyedropperActive) {
                POINT screenPt = pt;
                ClientToScreen(hwnd, &screenPt);
                HDC screenDC = GetDC(NULL);
                if (screenDC) {
                    COLORREF sampled = GetPixel(screenDC, screenPt.x, screenPt.y);
                    ReleaseDC(NULL, screenDC);
                    if (sampled != CLR_INVALID) {
                        g_eyedropperLiveColor = sampled;
                        g_style.customColor = sampled;
                        g_style.preset = PRESET_CUSTOM_COLOR;
                    }
                }
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            if (g_isDraggingStage) {
                double dx = (double)(pt.x - g_dragStartPt.x) / 56.0;
                double dy = -(double)(pt.y - g_dragStartPt.y) / 56.0;
                g_style.offsetX = std::clamp(g_dragStartOffX + dx, -1.0, 1.0);
                g_style.offsetY = std::clamp(g_dragStartOffY + dy, -1.0, 1.0);
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            if (g_activeWidget == W_SLIDER_SCALE || g_activeWidget == W_SLIDER_OFFX ||
                g_activeWidget == W_SLIDER_OFFY  || g_activeWidget == W_SLIDER_SHADOW) {
                UpdateSliderFromMouse(g_activeWidget, pt.x);
                return 0;
            }

            WidgetID prevHov = g_hoveredWidget;
            int prevRow = g_hoveredListRow;
            g_hoveredWidget = HitTestWidget(pt);
            g_hoveredListRow = -1;

            if (g_hoveredWidget == W_APP_LIST_AREA) {
                RectF listArea;
                if (GetWidgetRect(W_APP_LIST_AREA, listArea)) {
                    int row = (pt.y - (int)listArea.Y + g_listScrollY) / 50;
                    if (row >= 0 && row < (int)g_filteredIndices.size()) {
                        g_hoveredListRow = row;
                    }
                }
            }

            if (prevHov != g_hoveredWidget || prevRow != g_hoveredListRow) {
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            if (g_isDraggingStage) {
                g_isDraggingStage = false;
                ReleaseCapture();
                SaveStyleConfig();
                InvalidateAllThumbs();
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if (g_activeWidget != W_NONE) {
                if (g_activeWidget == W_SLIDER_SCALE || g_activeWidget == W_SLIDER_OFFX ||
                    g_activeWidget == W_SLIDER_OFFY  || g_activeWidget == W_SLIDER_SHADOW) {
                    UpdateSliderFromMouse(g_activeWidget, GET_X_LPARAM(lParam), true);
                    ReleaseCapture();
                    SaveStyleConfig();
                }
                g_activeWidget = W_NONE;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_GETMINMAXINFO: {
            LPMINMAXINFO mmi = (LPMINMAXINFO)lParam;
            mmi->ptMinTrackSize.x = 980;
            mmi->ptMinTrackSize.y = 620;
            return 0;
        }

        case WM_SIZE: {
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        case WM_MOUSEWHEEL: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);

            if (pt.x < 340) {
                g_listScrollY -= (delta > 0) ? 45 : -45;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            } else if (HitTestWidget(pt) == W_STAGE_ROUND_DRAG) {
                double step = (delta > 0) ? 0.05 : -0.05;
                g_style.scale = std::clamp(g_style.scale + step, 0.30, 2.50);
                SaveStyleConfig();
                InvalidateAllThumbs();
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            } else {
                g_inspectorScrollY = std::clamp(g_inspectorScrollY - ((delta > 0) ? 45 : -45), 0, g_maxInspectorScroll);
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
        }

        case WM_CHAR: {
            if (g_searchFocused) {
                wchar_t ch = (wchar_t)wParam;
                if (ch == L'\b') {
                    if (!g_searchText.empty()) g_searchText.pop_back();
                } else if (ch == 27) { // ESC
                    g_searchText.clear();
                    g_searchFocused = false;
                } else if (ch >= 32) {
                    g_searchText.push_back(ch);
                }
                RebuildFilteredList();
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            break;
        }

        case WM_KEYDOWN: {
            if ((GetKeyState(VK_CONTROL) & 0x8000) && (wParam == 'F' || wParam == 'f')) {
                g_searchFocused = true;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if ((GetKeyState(VK_CONTROL) & 0x8000) && (wParam == 'N' || wParam == 'n' || wParam == 'O' || wParam == 'o')) {
                AddCustomAppOrImage(hwnd);
                return 0;
            }
            if ((GetKeyState(VK_CONTROL) & 0x8000) && (wParam == 'E' || wParam == 'e')) {
                POINT dummyPt = {0, 0};
                HandleWidgetClick(hwnd, W_BTN_EXPORT, dummyPt);
                return 0;
            }
            if (wParam == VK_F5) {
                POINT dummyPt = {0, 0};
                HandleWidgetClick(hwnd, W_BTN_RESCAN, dummyPt);
                return 0;
            }
            if (wParam == VK_F1) {
                RectF menuR;
                POINT pt = { 80, 44 };
                if (GetWidgetRect(W_BTN_MODO_MENU, menuR)) {
                    pt = { (LONG)menuR.X, (LONG)menuR.GetBottom() + 2 };
                    ClientToScreen(hwnd, &pt);
                }
                ShowModoAppMenu(hwnd, pt);
                return 0;
            }

            if (!g_searchFocused && !g_filteredIndices.empty()) {
                if (wParam == VK_DOWN || wParam == VK_UP) {
                    int pos = 0;
                    for (size_t i = 0; i < g_filteredIndices.size(); ++i) {
                        if (g_filteredIndices[i] == g_selectedAppIndex) { pos = (int)i; break; }
                    }
                    pos = std::clamp(pos + (wParam == VK_DOWN ? 1 : -1), 0, (int)g_filteredIndices.size() - 1);
                    g_selectedAppIndex = g_filteredIndices[pos];
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
                if (wParam == VK_RETURN) {
                    POINT dummyPt = {0, 0};
                    if (GetKeyState(VK_SHIFT) & 0x8000) {
                        HandleWidgetClick(hwnd, W_BTN_APPLY_ALL, dummyPt);
                    } else {
                        HandleWidgetClick(hwnd, W_BTN_APPLY_ONE, dummyPt);
                    }
                    return 0;
                }
                if (wParam == VK_DELETE) {
                    POINT dummyPt = {0, 0};
                    HandleWidgetClick(hwnd, W_BTN_RESTORE_ONE, dummyPt);
                    return 0;
                }
            }
            break;
        }

        case WM_SYSKEYDOWN: {
            if (wParam == VK_MENU || wParam == 'M' || wParam == 'm') {
                RectF menuR;
                POINT pt = { 80, 44 };
                if (GetWidgetRect(W_BTN_MODO_MENU, menuR)) {
                    pt = { (LONG)menuR.X, (LONG)menuR.GetBottom() + 2 };
                    ClientToScreen(hwnd, &pt);
                }
                ShowModoAppMenu(hwnd, pt);
                return 0;
            }
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmdLine, int nCmdShow) {
    (void)hPrev;

    SetProcessDPIAware();
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    GdiplusStartupInput gsi;
    GdiplusStartup(&g_gdiplusToken, &gsi, NULL);
    GetPngEncoderClsid(&g_pngClsid);
    LoadStyleConfig();

    bool isSetupMode = false;
    bool isUpdateMode = false;
    bool isUninstallMode = false;
    bool isSilent = false;
    bool isPortableOverride = false;

    if (lpCmdLine) {
        if (wcsstr(lpCmdLine, L"--uninstall") != nullptr) isUninstallMode = true;
        if (wcsstr(lpCmdLine, L"--update") != nullptr) isUpdateMode = true;
        if (wcsstr(lpCmdLine, L"--setup") != nullptr || wcsstr(lpCmdLine, L"--install") != nullptr) isSetupMode = true;
        if (wcsstr(lpCmdLine, L"--portable") != nullptr) isPortableOverride = true;
        if (wcsstr(lpCmdLine, L"--silent") != nullptr || wcsstr(lpCmdLine, L"/S") != nullptr) isSilent = true;
    }

    wchar_t exeSelfPath[MAX_PATH * 2] = {0};
    GetModuleFileNameW(NULL, exeSelfPath, MAX_PATH * 2 - 1);
    std::wstring exeFileName = PathFindFileNameW(exeSelfPath);
    for (auto& c : exeFileName) c = towlower(c);

    if (!isPortableOverride && exeFileName.find(L"setup") != std::wstring::npos) {
        isSetupMode = true;
    }

    if (isUninstallMode) {
        ModoInstaller::PerformUninstall(NULL, isSilent);
        GdiplusShutdown(g_gdiplusToken);
        CoUninitialize();
        return 0;
    }

    if (isSetupMode || isUpdateMode) {
        int res = ModoInstaller::RunSetupWizard(hInst, nCmdShow, isUpdateMode);
        GdiplusShutdown(g_gdiplusToken);
        CoUninitialize();
        return res;
    }

    if (lpCmdLine && (wcsstr(lpCmdLine, L"shell --enable") != nullptr || wcsstr(lpCmdLine, L"--enable-shell") != nullptr)) {
        wchar_t exePath[MAX_PATH * 2] = {0};
        GetModuleFileNameW(NULL, exePath, MAX_PATH * 2 - 1);
        ShellIntegration::RegisterShellContextMenu(exePath, true);
        MessageBoxW(NULL, L"Интеграция с контекстным меню Проводника успешно включена!", L"Modo Studio", MB_ICONINFORMATION);
        GdiplusShutdown(g_gdiplusToken);
        CoUninitialize();
        return 0;
    }
    if (lpCmdLine && (wcsstr(lpCmdLine, L"shell --disable") != nullptr || wcsstr(lpCmdLine, L"--disable-shell") != nullptr)) {
        ShellIntegration::RegisterShellContextMenu(L"", false);
        MessageBoxW(NULL, L"Интеграция с контекстным меню Проводника отключена.", L"Modo Studio", MB_ICONINFORMATION);
        GdiplusShutdown(g_gdiplusToken);
        CoUninitialize();
        return 0;
    }

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = StudioWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(101));
    wc.hIconSm = LoadIconW(hInst, MAKEINTRESOURCEW(101));
    if (!wc.hIcon) wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = NULL;
    wc.lpszClassName = L"ModoStudioCanvasClass";
    RegisterClassExW(&wc);

    int winW = 1200;
    int winH = 800;
    RECT adj = { 0, 0, winW, winH };
    DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&adj, style, FALSE);
    int outerW = adj.right - adj.left;
    int outerH = adj.bottom - adj.top;

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    g_hMainWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"Modo Studio — Adaptive Icon Studio by Atoma",
        style,
        std::max(0, (screenW - outerW) / 2),
        std::max(0, (screenH - outerH) / 2),
        outerW,
        outerH,
        NULL,
        NULL,
        hInst,
        NULL
    );

    UpdateWindowImmersiveDarkMode(g_hMainWnd, g_isDarkMode);
    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    ScanInstalledWindowsApps();
    InvalidateRect(g_hMainWnd, NULL, FALSE);

    // Asynchronously check for updates in background (silent check)
    ModoUpdater::CheckForUpdates(g_hMainWnd, true);

    if (lpCmdLine && wcsstr(lpCmdLine, L"--apply-all-elevated") != nullptr) {
        int count = 0;
        for (size_t i = 0; i < g_allApps.size(); ++i) {
            if (ApplyRoundIcon(g_allApps[i], nullptr)) count++;
            g_progressRatio = (double)(i + 1) / (double)std::max<size_t>(1, g_allApps.size());
            g_statusText = L"Применение иконок (Администратор): " + g_allApps[i].name;
            InvalidateRect(g_hMainWnd, NULL, FALSE);
            UpdateWindow(g_hMainWnd);
        }
        RefreshWindowsExplorerIconCache(true);
        g_progressRatio = 0.0;
        wchar_t doneMsg[256];
        wsprintfW(doneMsg, L"Все ярлыки (%d шт.) обновлены!", count);
        g_statusText = doneMsg;
        InvalidateRect(g_hMainWnd, NULL, FALSE);
        MessageBoxW(g_hMainWnd, doneMsg, L"Modo Studio (Администратор)", MB_ICONINFORMATION);
    }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    for (auto& app : g_allApps) {
        delete app.origBitmap;
        delete app.cachedThumb;
    }
    GdiplusShutdown(g_gdiplusToken);
    CoUninitialize();
    return (int)msg.wParam;
}
