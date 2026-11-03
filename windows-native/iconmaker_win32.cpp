/*
 * IconMaker Studio for Windows — Custom Self-Rendered GDI+ Dark Studio UI Engine
 * Inspired by DaVinci Resolve & macOS Pro Apps (Zero native Win32 child controls!)
 *
 * Every button, slider, toggle switch, segmented pill, color swatch, search box,
 * scrollable application list with live circular icon thumbnails, and interactive
 * preview stage is rendered directly onto a double-buffered GDI+ canvas.
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
static const int WIN_H = 760;

enum WidgetID {
    W_NONE = 0,
    // Top bar
    W_SEARCH_BOX,
    W_SEARCH_CLEAR,
    W_BTN_ADMIN,
    W_BTN_SHORTCUTS,
    W_BTN_RESCAN,
    W_BTN_ADD_FILE,
    // Sidebar filter tabs
    W_TAB_ALL,
    W_TAB_APPLIED,
    W_TAB_SYSTEM,
    W_TAB_THIRDPARTY,
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
    W_BTN_EXPORT
};

struct HitZone {
    WidgetID id;
    RectF rect;
};

static ULONG_PTR g_gdiplusToken = 0;
static CLSID g_pngClsid = {0};
static HWND g_hMainWnd = nullptr;
static RoundStyle g_style;
static std::vector<AppEntry> g_allApps;
static std::vector<int> g_filteredIndices;
static int g_selectedAppIndex = -1;
static int g_sidebarFilter = 0; // 0 = All, 1 = Applied only
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

static std::wstring g_statusText = L"Готово — выберите приложение слева или настройте круглую иконку в студии";
static double g_progressRatio = 0.0; // 0.0 .. 1.0
static uint32_t g_iconRevCounter = 1;

// Forward declarations
static void InvalidateAllThumbs();
static void RebuildFilteredList();

static void EnableMicaAndImmersiveDarkMode(HWND hwnd) {
    HMODULE hDwm = LoadLibraryW(L"dwmapi.dll");
    if (hDwm) {
        typedef HRESULT(WINAPI* PFN_DwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);
        PFN_DwmSetWindowAttribute fn = (PFN_DwmSetWindowAttribute)GetProcAddress(hDwm, "DwmSetWindowAttribute");
        if (fn) {
            // Windows 11 Fluent dark mode
            BOOL dark = TRUE;
            fn(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dark, sizeof(dark));
            fn(hwnd, 19 /* older Win10 build */, &dark, sizeof(dark));

            // Windows 11 Mica backdrop material (DWMWA_SYSTEMBACKDROP_TYPE = 38, DWMSBT_MAINWINDOW = 2)
            DWORD backdropType = 2; // DWMSBT_MAINWINDOW (Mica)
            fn(hwnd, 38, &backdropType, sizeof(backdropType));

            // Windows 11 rounded window corners (DWMWA_WINDOW_CORNER_PREFERENCE = 33, DWMWCP_ROUND = 2)
            DWORD cornerPref = 2;
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
    wsprintfW(buf, L"%d", (int)g_style.preset);
    WritePrivateProfileStringW(L"Style", L"Preset", buf, cfgPath.c_str());
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
    g_style.preset = (BgPreset)std::clamp((int)GetPrivateProfileIntW(L"Style", L"Preset", 0, cfgPath.c_str()), 0, 4);
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

    // 1. Built-in Windows system components
    ShellIntegration::AddBuiltInSystemEntries(g_allApps, &g_pngClsid);

    // 2. Scan Taskbar, Desktop, and Start Menu shortcuts
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
    bi.lpszTitle = L"Выберите папку для экспорта круглых иконок (.ICO и 1024x1024 .PNG):";
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
        MessageBoxW(hwnd, (L"Круглая иконка (.ICO + 1024px .PNG) сохранена в:\n" + dir).c_str(),
                    L"Экспорт завершён", MB_ICONINFORMATION);
    }
    CoTaskMemFree(pidl);
}

// ============================================================================
// CUSTOM DAVINCI RESOLVE / MACOS DARK STUDIO RENDERER (100% GDI+ CANVAS)
// ============================================================================

static void RegisterHit(WidgetID id, RectF r) {
    g_hitZones.push_back({ id, r });
}

static void DrawStudioPillButton(
    Graphics& g,
    WidgetID id,
    RectF r,
    const wchar_t* label,
    Font& font,
    bool isPrimary = false,
    bool isSelected = false,
    bool isDanger = false
) {
    RegisterHit(id, r);
    bool hovered = (g_hoveredWidget == id);
    bool pressed = (g_activeWidget == id);

    // Monochromatic neutral dark palette (No harsh accent color)
    Color fill(255, 45, 45, 45);
    Color border(255, 68, 68, 68);
    Color textClr(255, 230, 230, 230);

    if (isPrimary) {
        // High-contrast clean neutral white/light-gray fill for primary button
        fill = pressed ? Color(255, 205, 205, 205)
             : (hovered ? Color(255, 255, 255, 255) : Color(255, 235, 235, 235));
        border = Color(255, 255, 255, 255);
        textClr = Color(255, 20, 20, 20); // Dark text on light primary
    } else if (isSelected) {
        // Subtle neutral dark-gray selection
        fill = Color(255, 65, 65, 65);
        border = Color(255, 120, 120, 120);
        textClr = Color(255, 255, 255, 255);
    } else if (isDanger) {
        fill = hovered ? Color(255, 60, 48, 48) : Color(255, 48, 42, 42);
        border = Color(255, 100, 75, 75);
        textClr = Color(255, 220, 180, 180);
    } else if (pressed) {
        fill = Color(255, 38, 38, 38);
        border = Color(255, 75, 75, 75);
        textClr = Color(255, 255, 255, 255);
    } else if (hovered) {
        fill = Color(255, 58, 58, 58);
        border = Color(255, 95, 95, 95);
        textClr = Color(255, 255, 255, 255);
    }

    REAL rad = std::min(6.0f, r.Height * 0.5f);
    FillRoundRect(g, r, rad, fill);
    DrawRoundRect(g, r, rad, border, 1.0f);

    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);
    SolidBrush tb(textClr);
    g.DrawString(label, -1, &font, r, &sf, &tb);
}

static void DrawStudioSlider(
    Graphics& g,
    WidgetID id,
    RectF trackRect,
    double normalizedValue // 0.0 .. 1.0
) {
    double v = std::clamp(normalizedValue, 0.0, 1.0);
    RectF hitRect(trackRect.X - 6.0f, trackRect.Y - 8.0f, trackRect.Width + 12.0f, trackRect.Height + 16.0f);
    RegisterHit(id, hitRect);

    bool hovered = (g_hoveredWidget == id || g_activeWidget == id);

    // Track groove (Clean Neutral Dark)
    RectF groove(trackRect.X, trackRect.Y + trackRect.Height * 0.5f - 2.5f, trackRect.Width, 5.0f);
    FillRoundRect(g, groove, 2.5f, Color(255, 50, 50, 50));
    DrawRoundRect(g, groove, 2.5f, Color(255, 70, 70, 70), 1.0f);

    // Active fill in neutral light gray
    REAL fillW = std::max(5.0f, (REAL)(groove.Width * v));
    RectF activeBar(groove.X, groove.Y, fillW, groove.Height);
    FillRoundRect(g, activeBar, 2.5f, Color(255, 180, 180, 180));

    // Thumb knob
    REAL thumbX = groove.X + (REAL)(groove.Width * v);
    REAL thumbY = groove.Y + groove.Height * 0.5f;
    REAL thumbR = hovered ? 8.0f : 7.0f;

    SolidBrush shadowBr(Color(90, 0, 0, 0));
    g.FillEllipse(&shadowBr, thumbX - thumbR, thumbY - thumbR + 1.5f, thumbR * 2.0f, thumbR * 2.0f);

    SolidBrush thumbBr(Color(255, 255, 255, 255));
    g.FillEllipse(&thumbBr, thumbX - thumbR, thumbY - thumbR, thumbR * 2.0f, thumbR * 2.0f);
    Pen ringPen(Color(255, 80, 80, 80), 1.5f);
    g.DrawEllipse(&ringPen, thumbX - thumbR, thumbY - thumbR, thumbR * 2.0f, thumbR * 2.0f);
}

static void DrawStudioToggle(
    Graphics& g,
    WidgetID id,
    RectF rowRect,
    const wchar_t* label,
    bool isOn,
    Font& font
) {
    RegisterHit(id, rowRect);
    bool hovered = (g_hoveredWidget == id);

    // Neutral monochrome ToggleSwitch (white/gray pill when ON)
    RectF pill(rowRect.X, rowRect.Y + (rowRect.Height - 20.0f) * 0.5f, 40.0f, 20.0f);
    Color pillFill = isOn
        ? (hovered ? Color(255, 240, 240, 240) : Color(255, 220, 220, 220))
        : (hovered ? Color(255, 58, 58, 58) : Color(255, 45, 45, 45));
    FillRoundRect(g, pill, 10.0f, pillFill);
    DrawRoundRect(g, pill, 10.0f, isOn ? Color(255, 255, 255, 255) : Color(255, 80, 80, 80), 1.0f);

    REAL knobX = isOn ? (pill.GetRight() - 17.0f) : (pill.X + 3.0f);
    Color knobColor = isOn ? Color(255, 20, 20, 20) : Color(255, 240, 240, 240);
    SolidBrush knobBr(knobColor);
    g.FillEllipse(&knobBr, knobX, pill.Y + 3.0f, 14.0f, 14.0f);

    RectF textRect(pill.GetRight() + 12.0f, rowRect.Y, rowRect.Width - 52.0f, rowRect.Height);
    StringFormat sf;
    sf.SetAlignment(StringAlignmentNear);
    sf.SetLineAlignment(StringAlignmentCenter);
    SolidBrush tb(hovered ? Color(255, 255, 255, 255) : Color(255, 225, 225, 225));
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

    FontFamily ff(L"Segoe UI");
    Font fLogo(&ff, 15.0f, FontStyleBold, UnitPixel);
    Font fSection(&ff, 13.0f, FontStyleBold, UnitPixel);
    Font fBodyBold(&ff, 13.0f, FontStyleBold, UnitPixel);
    Font fBody(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fSmall(&ff, 11.0f, FontStyleRegular, UnitPixel);
    Font fSmallBold(&ff, 11.0f, FontStyleBold, UnitPixel);

    // 1. Windows 11 Fluent Dark Mica Canvas (#202020 / RGB 32, 32, 32)
    g.Clear(Color(255, 32, 32, 32));

    // 2. Windows 11 CommandBar / Title Row (height: 56px, #2A2A2A with Fluent bottom divider)
    SolidBrush topBarBr(Color(255, 40, 40, 40));
    g.FillRectangle(&topBarBr, 0, 0, clientW, 56);
    Pen divPen(Color(255, 54, 54, 54), 1.0f);
    g.DrawLine(&divPen, 0, 56, clientW, 56);

    // Brand Logo Circle + Title
    SolidBrush brandCircle(Color(255, 60, 60, 60)); // Neutral graphite emblem
    g.FillEllipse(&brandCircle, 18.0f, 14.0f, 28.0f, 28.0f);
    SolidBrush innerWhite(Color(255, 230, 230, 230));
    g.FillEllipse(&innerWhite, 25.0f, 21.0f, 14.0f, 14.0f);

    SolidBrush whiteBr(Color(255, 255, 255, 255));
    SolidBrush mutedBr(Color(255, 160, 160, 160));
    SolidBrush accentBr(Color(255, 220, 220, 220)); // Clean neutral light monochrome

    g.DrawString(L"IconMaker Studio", -1, &fLogo, PointF(54.0f, 12.0f), NULL, &whiteBr);
    g.DrawString(L"GOOGLE PIXEL & MATERIAL YOU ENGINE", -1, &fSmallBold, PointF(55.0f, 31.0f), NULL, &mutedBr);

    // Top-right Windows 11 CommandBar buttons
    DrawStudioPillButton(g, W_BTN_ADMIN,     RectF((REAL)(clientW - 552), 13.0f, 132.0f, 30.0f), L"🛡 Права админа", fBody);
    DrawStudioPillButton(g, W_BTN_SHORTCUTS, RectF((REAL)(clientW - 412), 13.0f, 136.0f, 30.0f), L"📌 Значки на стол", fBody);
    DrawStudioPillButton(g, W_BTN_RESCAN,    RectF((REAL)(clientW - 268), 13.0f, 118.0f, 30.0f), L"↻ Обновить", fBody);
    DrawStudioPillButton(g, W_BTN_ADD_FILE,  RectF((REAL)(clientW - 142), 13.0f, 126.0f, 30.0f), L"+ Добавить .exe…", fBody);

    // 3. Left Sidebar — Application Browser (x: 0..356)
    int sideW = 356;
    SolidBrush sideBr(Color(255, 36, 36, 36));
    g.FillRectangle(&sideBr, 0, 57, sideW, clientH - 57 - 32);
    g.DrawLine(&divPen, sideW, 57, sideW, clientH - 32);

    // 4 Category Pills (All, Applied, System, Third-Party)
    int appliedTotal = 0;
    for (const auto& a : g_allApps) if (a.isApplied) appliedTotal++;
    wchar_t tabAllLbl[32], tabAppLbl[32];
    wsprintfW(tabAllLbl, L"Все (%d)", (int)g_allApps.size());
    wsprintfW(tabAppLbl, L"Круглые (%d)", appliedTotal);

    REAL pillW = (REAL)(sideW - 32 - 12) / 4.0f;
    DrawStudioPillButton(g, W_TAB_ALL,        RectF(16.0f + 0 * (pillW + 4.0f), 66.0f, pillW, 26.0f), tabAllLbl,        fSmallBold, false, g_sidebarFilter == 0);
    DrawStudioPillButton(g, W_TAB_APPLIED,    RectF(16.0f + 1 * (pillW + 4.0f), 66.0f, pillW, 26.0f), tabAppLbl,        fSmallBold, false, g_sidebarFilter == 1);
    DrawStudioPillButton(g, W_TAB_SYSTEM,     RectF(16.0f + 2 * (pillW + 4.0f), 66.0f, pillW, 26.0f), L"Системные",    fSmallBold, false, g_sidebarFilter == 2);
    DrawStudioPillButton(g, W_TAB_THIRDPARTY, RectF(16.0f + 3 * (pillW + 4.0f), 66.0f, pillW, 26.0f), L"Сторонние",   fSmallBold, false, g_sidebarFilter == 3);

    // Custom Search Box
    RectF searchRect(14.0f, 102.0f, 328.0f, 32.0f);
    RegisterHit(W_SEARCH_BOX, searchRect);
    FillRoundRect(g, searchRect, 6.0f, Color(255, 24, 24, 24));
    DrawRoundRect(g, searchRect, 6.0f, g_searchFocused ? Color(255, 140, 140, 140) : Color(255, 60, 60, 60), 1.2f);

    StringFormat sfVCenter;
    sfVCenter.SetLineAlignment(StringAlignmentCenter);
    sfVCenter.SetFormatFlags(StringFormatFlagsNoWrap);
    if (g_searchText.empty()) {
        std::wstring ph = g_searchFocused ? L"🔍 Введите название программы| " : L"🔍 Поиск установленных программ…";
        g.DrawString(ph.c_str(), -1, &fBody, RectF(24.0f, 102.0f, 290.0f, 32.0f), &sfVCenter, &mutedBr);
    } else {
        std::wstring disp = L"🔍 " + g_searchText + (g_searchFocused ? L"|" : L"");
        g.DrawString(disp.c_str(), -1, &fBody, RectF(24.0f, 102.0f, 282.0f, 32.0f), &sfVCenter, &whiteBr);
        RectF clearRect(312.0f, 106.0f, 24.0f, 24.0f);
        RegisterHit(W_SEARCH_CLEAR, clearRect);
        StringFormat sfC; sfC.SetAlignment(StringAlignmentCenter); sfC.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(L"✕", -1, &fSmallBold, clearRect, &sfC, &mutedBr);
    }

    // Scrollable Application List (y: 142 .. clientH - 38)
    RectF listArea(8.0f, 142.0f, (REAL)(sideW - 16), (REAL)(clientH - 142 - 38));
    RegisterHit(W_APP_LIST_AREA, listArea);

    GraphicsState listState = g.Save();
    g.SetClip(listArea);

    const int rowH = 50;
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
            // Neutral dark-gray selection with clean white/gray border
            FillRoundRect(g, rowRect, 6.0f, Color(255, 60, 60, 60));
            DrawRoundRect(g, rowRect, 6.0f, Color(255, 120, 120, 120), 1.0f);
        } else if (isHov) {
            FillRoundRect(g, rowRect, 6.0f, Color(255, 46, 46, 46));
        }

        AppEntry& app = g_allApps[appIdx];
        if (Bitmap* thumb = GetAppThumb(app)) {
            g.DrawImage(thumb, RectF(rowRect.X + 8.0f, rowRect.Y + 5.0f, 36.0f, 36.0f));
        }

        REAL textMaxW = app.isApplied ? (rowRect.Width - 122.0f) : (rowRect.Width - 58.0f);
        RectF titleR(rowRect.X + 52.0f, rowRect.Y + 6.0f, textMaxW, 18.0f);
        RectF subR(rowRect.X + 52.0f, rowRect.Y + 24.0f, textMaxW, 16.0f);
        StringFormat sfTrim;
        sfTrim.SetTrimming(StringTrimmingEllipsisCharacter);
        sfTrim.SetFormatFlags(StringFormatFlagsNoWrap);

        g.DrawString(app.name.c_str(), -1, &fBodyBold, titleR, &sfTrim, &whiteBr);

        wchar_t subBuf[160];
        wsprintfW(subBuf, L"%s · %u ярл.", app.category.c_str(), (unsigned)std::max<size_t>(1, app.allLnkPaths.size()));
        g.DrawString(subBuf, -1, &fSmall, subR, &sfTrim, &mutedBr);

        if (app.isApplied) {
            RectF badgeR(rowRect.GetRight() - 64.0f, rowRect.Y + 13.0f, 56.0f, 20.0f);
            FillRoundRect(g, badgeR, 10.0f, Color(255, 45, 45, 45));
            DrawRoundRect(g, badgeR, 10.0f, Color(255, 85, 85, 85), 1.0f);
            StringFormat sfB; sfB.SetAlignment(StringAlignmentCenter); sfB.SetLineAlignment(StringAlignmentCenter);
            SolidBrush neutralBadgeText(Color(255, 220, 220, 220));
            g.DrawString(L"ROUND", -1, &fSmallBold, badgeR, &sfB, &neutralBadgeText);
        }
    }

    // Custom scrollbar indicator
    if (maxScroll > 0) {
        REAL trackH = listArea.Height;
        REAL thumbH = std::max(28.0f, trackH * (trackH / (REAL)totalRowsH));
        REAL thumbY = listArea.Y + (trackH - thumbH) * ((REAL)g_listScrollY / (REAL)maxScroll);
        FillRoundRect(g, RectF(listArea.GetRight() - 5.0f, thumbY, 4.0f, thumbH), 2.0f, Color(140, 110, 118, 135));
    }

    g.Restore(listState);

    // 4. Right Windows 11 Card Inspector (x: 374 .. clientW - 18)
    REAL rx = 374.0f;
    REAL rw = (REAL)(clientW - 374 - 18);

    // --- CARD 1: Interactive Fluent Node Preview Stage (y: 66, h: 204) ---
    RectF stageCard(rx, 66.0f, rw, 204.0f);
    FillRoundRect(g, stageCard, 8.0f, Color(255, 43, 43, 43)); // Win11 Card Background
    DrawRoundRect(g, stageCard, 8.0f, Color(255, 60, 60, 60), 1.0f); // 1px Card Stroke

    // Header inside Stage Card
    std::wstring selTitle = (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size())
        ? g_allApps[g_selectedAppIndex].name
        : L"Приложение не выбрано";
    g.DrawString(selTitle.c_str(), -1, &fLogo, PointF(rx + 18.0f, 76.0f), NULL, &whiteBr);

    wchar_t teleBuf[128];
    wsprintfW(teleBuf, L"ZOOM %d%%   |   X %+d%%   |   Y %+d%%",
              (int)std::round(g_style.scale * 100.0),
              (int)std::round(g_style.offsetX * 100.0),
              (int)std::round(g_style.offsetY * 100.0));
    StringFormat sfRight; sfRight.SetAlignment(StringAlignmentFar);
    g.DrawString(teleBuf, -1, &fSmallBold, RectF(rx + 200.0f, 80.0f, rw - 218.0f, 18.0f), &sfRight, &accentBr);

    auto drawDarkCheckerWell = [&](REAL wx, REAL wy, REAL ww, REAL wh) {
        RectF wr(wx, wy, ww, wh);
        FillRoundRect(g, wr, 8.0f, Color(255, 28, 28, 28));
        GraphicsPath cp;
        AddRoundedRect(cp, wr, 8.0f);
        GraphicsState st = g.Save();
        g.SetClip(&cp);
        SolidBrush c2(Color(255, 36, 36, 36));
        int step = 12;
        for (int y = 0; y < (int)wh; y += step) {
            for (int x = 0; x < (int)ww; x += step) {
                if (((x / step) + (y / step)) & 1) {
                    g.FillRectangle(&c2, (INT)wx + x, (INT)wy + y, step, step);
                }
            }
        }
        g.Restore(st);
        DrawRoundRect(g, wr, 8.0f, Color(255, 54, 54, 54), 1.0f);
    };

    REAL wellSize = 124.0f;
    REAL leftWellX = rx + 110.0f;
    REAL rightWellX = rx + rw - 110.0f - wellSize;
    REAL wellY = 108.0f;

    drawDarkCheckerWell(leftWellX, wellY, wellSize, wellSize);
    drawDarkCheckerWell(rightWellX, wellY, wellSize, wellSize);

    RectF roundStageRect(rightWellX, wellY, wellSize, wellSize);
    RegisterHit(W_STAGE_ROUND_DRAG, roundStageRect);

    if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
        AppEntry& app = g_allApps[g_selectedAppIndex];
        if (app.origBitmap) {
            g.DrawImage(app.origBitmap, RectF(leftWellX + 12.0f, wellY + 12.0f, wellSize - 24.0f, wellSize - 24.0f));
            Bitmap* hiRound = RenderGoogleRoundIcon(app.origBitmap, g_style, 256);
            g.DrawImage(hiRound, RectF(rightWellX + 4.0f, wellY + 4.0f, wellSize - 8.0f, wellSize - 8.0f));
            delete hiRound;
        }
    }

    StringFormat sfCenter; sfCenter.SetAlignment(StringAlignmentCenter);
    g.DrawString(L"ОРИГИНАЛ", -1, &fSmallBold, PointF(leftWellX + wellSize * 0.5f, wellY + wellSize + 6.0f), &sfCenter, &mutedBr);
    g.DrawString(L"GOOGLE PIXEL (ТЯНИТЕ МЫШЬЮ / КОЛЕСО = ЗУМ)", -1, &fSmallBold,
                 PointF(rightWellX + wellSize * 0.5f, wellY + wellSize + 6.0f), &sfCenter, &accentBr);

    // Center node arrow
    REAL midStageX = rx + rw * 0.5f;
    FillRoundRect(g, RectF(midStageX - 44.0f, wellY + 42.0f, 88.0f, 36.0f), 18.0f, Color(255, 36, 36, 36));
    DrawRoundRect(g, RectF(midStageX - 44.0f, wellY + 42.0f, 88.0f, 36.0f), 18.0f, Color(255, 60, 60, 60), 1.0f);
    StringFormat sfMid; sfMid.SetAlignment(StringAlignmentCenter); sfMid.SetLineAlignment(StringAlignmentCenter);
    g.DrawString(L"➔  PIXEL", -1, &fSmallBold, RectF(midStageX - 44.0f, wellY + 42.0f, 88.0f, 36.0f), &sfMid, &accentBr);

    // --- CARD 2: Surface Presets & Eyedropper Color Bar (y: 280, h: 108) ---
    RectF colorCard(rx, 280.0f, rw, 108.0f);
    FillRoundRect(g, colorCard, 8.0f, Color(255, 43, 43, 43));
    DrawRoundRect(g, colorCard, 8.0f, Color(255, 60, 60, 60), 1.0f);

    g.DrawString(L"ПОДЛОЖКА И ЦВЕТ (GOOGLE PIXEL / MATERIAL YOU)", -1, &fSmallBold, PointF(rx + 16.0f, 290.0f), NULL, &mutedBr);

    const wchar_t* presetNames[5] = {
        L"Google White",
        L"Material Pastel",
        L"Dark Material",
        L"Adaptive Edge",
        L"Custom Color"
    };
    REAL presetPillW = (rw - 32.0f - 4.0f * 6.0f) / 5.0f;
    for (int p = 0; p < 5; ++p) {
        RectF pr(rx + 16.0f + p * (presetPillW + 6.0f), 310.0f, presetPillW, 28.0f);
        DrawStudioPillButton(g, (WidgetID)(W_PRESET_0 + p), pr, presetNames[p], fBody, false, (int)g_style.preset == p);
    }

    // Eyedropper + Palette + Live Hex + 8 Swatches row (y: 348)
    DrawStudioPillButton(
        g, W_BTN_EYEDROPPER,
        RectF(rx + 16.0f, 348.0f, 134.0f, 28.0f),
        g_isEyedropperActive ? L"🎯 Выберите точку…" : L"🔍 Пипетка с экрана",
        fBodyBold,
        g_isEyedropperActive,
        g_isEyedropperActive
    );
    DrawStudioPillButton(g, W_BTN_PALETTE, RectF(rx + 158.0f, 348.0f, 106.0f, 28.0f), L"🎨 Палитра…", fBody);

    // Current Color Pill + HEX
    Color curC(255, GetRValue(g_style.customColor), GetGValue(g_style.customColor), GetBValue(g_style.customColor));
    RectF hexBox(rx + 272.0f, 348.0f, 106.0f, 28.0f);
    FillRoundRect(g, hexBox, 6.0f, Color(255, 30, 30, 30));
    DrawRoundRect(g, hexBox, 6.0f, Color(255, 60, 60, 60), 1.0f);
    SolidBrush curSwBr(curC);
    g.FillEllipse(&curSwBr, hexBox.X + 8.0f, hexBox.Y + 5.0f, 18.0f, 18.0f);
    Pen swRing(Color(160, 255, 255, 255), 1.0f);
    g.DrawEllipse(&swRing, hexBox.X + 8.0f, hexBox.Y + 5.0f, 18.0f, 18.0f);

    wchar_t hexStr[32];
    wsprintfW(hexStr, L"#%02X%02X%02X", GetRValue(g_style.customColor), GetGValue(g_style.customColor), GetBValue(g_style.customColor));
    g.DrawString(hexStr, -1, &fBodyBold, RectF(hexBox.X + 32.0f, hexBox.Y, 70.0f, 28.0f), &sfVCenter, &whiteBr);

    // 8 Quick Material Swatch Circles
    REAL swStartX = rx + 392.0f;
    for (int s = 0; s < 8; ++s) {
        RectF sr(swStartX + s * 28.0f, 351.0f, 22.0f, 22.0f);
        RegisterHit((WidgetID)(W_SWATCH_0 + s), sr);
        Color sc(255, GetRValue(kSwatchPalette[s]), GetGValue(kSwatchPalette[s]), GetBValue(kSwatchPalette[s]));
        SolidBrush sbr(sc);
        g.FillEllipse(&sbr, sr);
        bool match = (g_style.customColor == kSwatchPalette[s] && g_style.preset == PRESET_CUSTOM_COLOR);
        Pen sp(match ? Color(255, 255, 255, 255) : Color(120, 255, 255, 255), match ? 2.5f : 1.0f);
        g.DrawEllipse(&sp, sr);
    }

    // --- CARD 3: Geometry, Zoom (30%..250%) & X/Y Pan Sliders (y: 398, h: 184) ---
    RectF geoCard(rx, 398.0f, rw, 184.0f);
    FillRoundRect(g, geoCard, 8.0f, Color(255, 43, 43, 43));
    DrawRoundRect(g, geoCard, 8.0f, Color(255, 60, 60, 60), 1.0f);

    // Row 1: Scale (30% .. 250%)
    wchar_t lblBuf[128];
    wsprintfW(lblBuf, L"Масштаб внутри круга: %d%% (до 250%%)", (int)std::round(g_style.scale * 100.0));
    g.DrawString(lblBuf, -1, &fBodyBold, PointF(rx + 16.0f, 412.0f), NULL, &whiteBr);
    DrawStudioSlider(g, W_SLIDER_SCALE, RectF(rx + 260.0f, 410.0f, rw - 486.0f, 22.0f), (g_style.scale - 0.30) / (2.50 - 0.30));
    DrawStudioPillButton(g, W_BTN_SC_68,  RectF(rx + rw - 214.0f, 409.0f, 46.0f, 24.0f), L"68%",  fSmallBold, false, std::abs(g_style.scale - 0.68) < 0.01);
    DrawStudioPillButton(g, W_BTN_SC_100, RectF(rx + rw - 164.0f, 409.0f, 48.0f, 24.0f), L"100%", fSmallBold, false, std::abs(g_style.scale - 1.00) < 0.01);
    DrawStudioPillButton(g, W_BTN_SC_135, RectF(rx + rw - 112.0f, 409.0f, 48.0f, 24.0f), L"135%", fSmallBold, false, std::abs(g_style.scale - 1.35) < 0.01);
    DrawStudioPillButton(g, W_BTN_SC_175, RectF(rx + rw - 60.0f,  409.0f, 46.0f, 24.0f), L"175%", fSmallBold, false, std::abs(g_style.scale - 1.75) < 0.01);

    // Row 2: Horizontal X (-100% .. +100%)
    wsprintfW(lblBuf, L"Смещение Влево / Вправо X: %+d%%", (int)std::round(g_style.offsetX * 100.0));
    g.DrawString(lblBuf, -1, &fBodyBold, PointF(rx + 16.0f, 452.0f), NULL, &whiteBr);
    DrawStudioSlider(g, W_SLIDER_OFFX, RectF(rx + 260.0f, 450.0f, rw - 486.0f, 22.0f), (g_style.offsetX + 1.0) * 0.5);
    DrawStudioPillButton(g, W_BTN_LEFT,  RectF(rx + rw - 214.0f, 449.0f, 96.0f, 24.0f), L"← Влево",  fSmallBold);
    DrawStudioPillButton(g, W_BTN_RIGHT, RectF(rx + rw - 112.0f, 449.0f, 98.0f, 24.0f), L"Вправо →", fSmallBold);

    // Row 3: Vertical Y (-100% .. +100%)
    wsprintfW(lblBuf, L"Смещение Вверх / Вниз Y: %+d%%", (int)std::round(g_style.offsetY * 100.0));
    g.DrawString(lblBuf, -1, &fBodyBold, PointF(rx + 16.0f, 492.0f), NULL, &whiteBr);
    DrawStudioSlider(g, W_SLIDER_OFFY, RectF(rx + 260.0f, 490.0f, rw - 486.0f, 22.0f), (g_style.offsetY + 1.0) * 0.5);
    DrawStudioPillButton(g, W_BTN_DOWN,   RectF(rx + rw - 214.0f, 489.0f, 62.0f, 24.0f), L"↓ Вниз",  fSmallBold);
    DrawStudioPillButton(g, W_BTN_UP,     RectF(rx + rw - 148.0f, 489.0f, 62.0f, 24.0f), L"↑ Вверх", fSmallBold);
    DrawStudioPillButton(g, W_BTN_CENTER, RectF(rx + rw - 82.0f,  489.0f, 68.0f, 24.0f), L"Центр",   fSmallBold);

    // Row 4: Outer Drop Shadow (0% .. 100%)
    wsprintfW(lblBuf, L"Внешняя тень Google Pixel: %d%%", (int)std::round(g_style.shadow * 100.0));
    g.DrawString(lblBuf, -1, &fBodyBold, PointF(rx + 16.0f, 532.0f), NULL, &whiteBr);
    DrawStudioSlider(g, W_SLIDER_SHADOW, RectF(rx + 260.0f, 530.0f, rw - 280.0f, 22.0f), g_style.shadow);

    // --- CARD 4: Fluent Toggle Switches (y: 592, h: 68) ---
    RectF optCard(rx, 592.0f, rw, 68.0f);
    FillRoundRect(g, optCard, 8.0f, Color(255, 43, 43, 43));
    DrawRoundRect(g, optCard, 8.0f, Color(255, 60, 60, 60), 1.0f);

    REAL halfW = (rw - 36.0f) * 0.5f;
    DrawStudioToggle(g, W_TOGGLE_AUTOTRIM,  RectF(rx + 16.0f,         600.0f, halfW, 24.0f), L"Авто-обрезка прозрачных полей", g_style.autoTrim, fBody);
    DrawStudioToggle(g, W_TOGGLE_INNERSHAD, RectF(rx + 24.0f + halfW, 600.0f, halfW, 24.0f), L"Внутренняя тень иконки",        g_style.innerShadow, fBody);
    DrawStudioToggle(g, W_TOGGLE_FULLBLEED, RectF(rx + 16.0f,         628.0f, halfW, 24.0f), L"Квадратные иконки на весь круг", g_style.fullBleed, fBody);
    DrawStudioToggle(g, W_TOGGLE_MONO,      RectF(rx + 24.0f + halfW, 628.0f, halfW, 24.0f), L"Монохромный тон Material You",  g_style.monochrome, fBody);

    // --- ACTION DOCK (y: 672, h: 42) ---
    DrawStudioPillButton(g, W_BTN_APPLY_ONE,   RectF(rx,         672.0f, 200.0f, 40.0f), L"✓ Применить к выбранному", fSection, true);
    DrawStudioPillButton(g, W_BTN_APPLY_ALL,   RectF(rx + 208.0f, 672.0f, 178.0f, 40.0f), L"⚡ Применить ко всем",      fSection, false, true);
    DrawStudioPillButton(g, W_BTN_RESTORE_ONE, RectF(rx + 394.0f, 672.0f, 118.0f, 40.0f), L"↩ Оригинал",               fBodyBold);
    DrawStudioPillButton(g, W_BTN_RESTORE_ALL, RectF(rx + 520.0f, 672.0f, 112.0f, 40.0f), L"↺ Сброс всех",             fBodyBold, false, false, true);
    DrawStudioPillButton(g, W_BTN_EXPORT,      RectF(rx + 640.0f, 672.0f, rw - 640.0f, 40.0f), L"⬇ Экспорт",            fBodyBold);

    // 5. Bottom Status Bar (y: clientH - 32 .. clientH)
    SolidBrush footBr(Color(255, 28, 28, 28));
    g.FillRectangle(&footBr, 0, clientH - 32, clientW, 32);
    g.DrawLine(&divPen, 0, clientH - 32, clientW, clientH - 32);

    StringFormat sfTrimEllipsis;
    sfTrimEllipsis.SetTrimming(StringTrimmingEllipsisCharacter);
    sfTrimEllipsis.SetLineAlignment(StringAlignmentCenter);
    sfTrimEllipsis.SetFormatFlags(StringFormatFlagsNoWrap);
    g.DrawString(g_statusText.c_str(), -1, &fSmall, RectF(16.0f, (REAL)(clientH - 32), (REAL)(clientW - 260), 32.0f), &sfTrimEllipsis, &mutedBr);

    if (g_progressRatio > 0.001) {
        RectF progBg((REAL)(clientW - 220), (REAL)(clientH - 20), 200.0f, 8.0f);
        FillRoundRect(g, progBg, 4.0f, Color(255, 32, 34, 42));
        RectF progFg(progBg.X, progBg.Y, (REAL)(progBg.Width * std::clamp(g_progressRatio, 0.0, 1.0)), 8.0f);
        FillRoundRect(g, progFg, 4.0f, Color(255, 52, 168, 83));
    }

    // 6. Floating Eyedropper Magnifier Loupe when active
    if (g_isEyedropperActive) {
        REAL lx = (REAL)std::clamp((int)g_cursorClientPt.x + 22, 20, clientW - 150);
        REAL ly = (REAL)std::clamp((int)g_cursorClientPt.y - 54, 20, clientH - 64);
        RectF loupe(lx, ly, 128.0f, 42.0f);
        FillRoundRect(g, loupe, 10.0f, Color(240, 20, 22, 28));
        DrawRoundRect(g, loupe, 10.0f, Color(255, 82, 158, 255), 1.5f);

        Color lc(255, GetRValue(g_eyedropperLiveColor), GetGValue(g_eyedropperLiveColor), GetBValue(g_eyedropperLiveColor));
        SolidBrush lbr(lc);
        g.FillEllipse(&lbr, lx + 8.0f, ly + 8.0f, 26.0f, 26.0f);
        g.DrawEllipse(&swRing, lx + 8.0f, ly + 8.0f, 26.0f, 26.0f);

        wchar_t lhex[32];
        wsprintfW(lhex, L"#%02X%02X%02X", GetRValue(g_eyedropperLiveColor), GetGValue(g_eyedropperLiveColor), GetBValue(g_eyedropperLiveColor));
        g.DrawString(lhex, -1, &fBodyBold, RectF(lx + 42.0f, ly, 80.0f, 42.0f), &sfVCenter, &whiteBr);
    }

    // Blit offscreen buffer to screen in one atomic operation
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

static void HandleWidgetClick(HWND hwnd, WidgetID id, POINT pt) {
    switch (id) {
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
            g_statusText = L"Сканирование установленных приложений Windows…";
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
            ScanInstalledWindowsApps();
            g_statusText = L"Список приложений обновлён";
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case W_BTN_ADD_FILE:
            AddCustomAppOrImage(hwnd);
            break;

        case W_PRESET_0:
        case W_PRESET_1:
        case W_PRESET_2:
        case W_PRESET_3:
        case W_PRESET_4:
            g_style.preset = (BgPreset)(id - W_PRESET_0);
            SaveStyleConfig();
            InvalidateAllThumbs();
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case W_BTN_EYEDROPPER:
            g_isEyedropperActive = true;
            g_eyedropperLiveColor = g_style.customColor;
            SetCapture(hwnd);
            SetCursor(LoadCursor(NULL, IDC_CROSS));
            g_statusText = L"🔍 Пипетка активна: наведите курсор на любую точку иконки или экрана и кликните ЛКМ (ПКМ — отмена)";
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
                g_statusText = L"✓ Единая круглая иконка Google Pixel применена к " + app.name;
                InvalidateRect(hwnd, NULL, FALSE);
                if (needsAdmin) {
                    int ans = MessageBoxW(
                        hwnd,
                        L"Этот ярлык находится в общей системной папке (Public Desktop / ProgramData).\n\n"
                        L"Разрешить обновление с правами Администратора, чтобы иконка на Рабочем столе обновилась до единого круга?",
                        L"IconMaker Studio — Системный ярлык",
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
                g_statusText = L"Применение одинаковых круглых иконок: " + app.name;
                InvalidateRect(hwnd, NULL, FALSE);
                UpdateWindow(hwnd);
            }
            RefreshWindowsExplorerIconCache(true);
            g_progressRatio = 0.0;

            if (anyNeedsAdmin) {
                int ans = MessageBoxW(
                    hwnd,
                    L"Часть ярлыков на Рабочем столе находится в общей папке C:\\Users\\Public\\Desktop (Chrome, Steam, Edge и др.).\n\n"
                    L"Нажмите «Да», чтобы автоматически применить с правами Администратора и сделать АБСОЛЮТНО ВСЕ иконки одинаковыми кругами!",
                    L"IconMaker Studio — Применить ко всем ярлыкам",
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
            wsprintfW(msg, L"Готово! Одинаковые круглые иконки Google Pixel (240px / 93.75%%) применены к %d приложениям (включая File Explorer и значки Рабочего стола).", count);
            g_statusText = msg;
            InvalidateRect(hwnd, NULL, FALSE);
            MessageBoxW(hwnd, msg, L"IconMaker Studio", MB_ICONINFORMATION);
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
            MessageBoxW(hwnd, L"Оригинальные иконки восстановлены для всех приложений!", L"IconMaker Studio", MB_ICONINFORMATION);
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
            return 1; // Prevent any GDI background flicker; WM_PAINT blits the full double-buffer

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

            HandleWidgetClick(hwnd, hit, pt);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        case WM_LBUTTONDBLCLK: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            if (HitTestWidget(pt) == W_STAGE_ROUND_DRAG) {
                g_style.offsetX = 0.0;
                g_style.offsetY = 0.0;
                InvalidateAllThumbs();
                InvalidateRect(hwnd, NULL, FALSE);
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

        case WM_MOUSEWHEEL: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);

            if (pt.x < 356) {
                g_listScrollY -= (delta > 0) ? 45 : -45;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            } else {
                double step = (delta > 0) ? 0.05 : -0.05;
                g_style.scale = std::clamp(g_style.scale + step, 0.30, 2.50);
                SaveStyleConfig();
                InvalidateAllThumbs();
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
            // Global Ctrl+F shortcut to immediately focus and activate Search Box
            if ((GetKeyState(VK_CONTROL) & 0x8000) && (wParam == 'F' || wParam == 'f')) {
                g_searchFocused = true;
                InvalidateRect(hwnd, NULL, FALSE);
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

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = StudioWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(101));
    wc.hIconSm = LoadIconW(hInst, MAKEINTRESOURCEW(101));
    if (!wc.hIcon) wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = NULL; // Custom GDI+ full-window canvas
    wc.lpszClassName = L"IconMakerStudioCanvasClass";
    RegisterClassExW(&wc);

    RECT adj = { 0, 0, WIN_W, WIN_H };
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&adj, style, FALSE);
    int outerW = adj.right - adj.left;
    int outerH = adj.bottom - adj.top;

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    g_hMainWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"IconMaker Studio — Google Pixel & Material You Circular Icons",
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

    EnableMicaAndImmersiveDarkMode(g_hMainWnd);
    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    ScanInstalledWindowsApps();
    InvalidateRect(g_hMainWnd, NULL, FALSE);

    if (lpCmdLine && wcsstr(lpCmdLine, L"--apply-all-elevated") != nullptr) {
        int count = 0;
        for (size_t i = 0; i < g_allApps.size(); ++i) {
            if (ApplyRoundIcon(g_allApps[i], nullptr)) count++;
            g_progressRatio = (double)(i + 1) / (double)std::max<size_t>(1, g_allApps.size());
            g_statusText = L"Применение единых круглых иконок (Администратор): " + g_allApps[i].name;
            InvalidateRect(g_hMainWnd, NULL, FALSE);
            UpdateWindow(g_hMainWnd);
        }
        RefreshWindowsExplorerIconCache(true);
        g_progressRatio = 0.0;
        wchar_t doneMsg[256];
        wsprintfW(doneMsg, L"Все системные и пользовательские ярлыки (%d шт.) обновлены до одинаковых кругов!", count);
        g_statusText = doneMsg;
        InvalidateRect(g_hMainWnd, NULL, FALSE);
        MessageBoxW(g_hMainWnd, doneMsg, L"IconMaker Studio (Администратор)", MB_ICONINFORMATION);
    } else {
        wchar_t exeName[MAX_PATH] = {0};
        if (GetModuleFileNameW(NULL, exeName, MAX_PATH - 1)) {
            std::wstring lowerExe = PathFindFileNameW(exeName);
            for (auto& c : lowerExe) c = towlower(c);
            if (lowerExe.find(L"setup") != std::wstring::npos) {
                InstallSelfAndShortcuts(g_hMainWnd, false);
            }
        }
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
