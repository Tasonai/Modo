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
#include "modo_cli.h"
#include "simple_icons_data.h"

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
    W_SWATCH_WIN_ACCENT,
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
    // Icon pack pills (19 curated packs)
    W_PACK_0, W_PACK_1, W_PACK_2, W_PACK_3, W_PACK_4, W_PACK_5, W_PACK_6, W_PACK_7,
    W_PACK_8, W_PACK_9, W_PACK_10, W_PACK_11, W_PACK_12, W_PACK_13, W_PACK_14, W_PACK_15,
    W_PACK_16, W_PACK_17, W_PACK_18,
    // 3-Column View Mode & Tabs
    W_BTN_APP_VIEW_GRID,
    W_BTN_APP_VIEW_LIST,
    W_SLIDER_GRID_SIZE,
    W_GRID_AREA,
    W_INSP_TAB_STYLE,
    W_INSP_TAB_ARTWORK,
    W_INSP_TAB_GEOMETRY,
    W_INSP_TAB_OPTIONS,
    // Surface material pills (8 materials)
    W_MATERIAL_0, W_MATERIAL_1, W_MATERIAL_2, W_MATERIAL_3, W_MATERIAL_4, W_MATERIAL_5, W_MATERIAL_6, W_MATERIAL_7,
    // Glyph artwork style pills (6 modes)
    W_GLYPH_0, W_GLYPH_1, W_GLYPH_2, W_GLYPH_3, W_GLYPH_4, W_GLYPH_5,
    // Explorer quick restart
    W_BTN_RESTART_EXPLORER,
    // Artwork Tab controls
    W_BTN_ART_FILE,
    W_BTN_ART_PASTE,
    W_BTN_ART_LUCIDE,
    W_BTN_ART_BRANDS,
    W_BTN_ART_AI_PATTERNS,
    W_BTN_ART_MONOGRAM,
    W_BTN_ART_RESET,
    W_BTN_ART_ROT_L,
    W_BTN_ART_ROT_R,
    W_BTN_ART_FLIP_H,
    W_BTN_ART_FLIP_V,
    W_BTN_ART_RESET_TRANS,
    W_BTN_ART_GRAD_0, W_BTN_ART_GRAD_1, W_BTN_ART_GRAD_2, W_BTN_ART_GRAD_3, W_BTN_ART_GRAD_4, W_BTN_ART_GRAD_5, W_BTN_ART_GRAD_6,
    W_BTN_ART_BADGE_0, W_BTN_ART_BADGE_1, W_BTN_ART_BADGE_2, W_BTN_ART_BADGE_3, W_BTN_ART_BADGE_4, W_BTN_ART_BADGE_5, W_BTN_ART_BADGE_6, W_BTN_ART_BADGE_7,
    W_BTN_ART_BPOS_TR, W_BTN_ART_BPOS_BR, W_BTN_ART_BPOS_TL, W_BTN_ART_BPOS_BL,
    W_TOGGLE_PER_APP
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
static int g_sidebarScrollY = 0;
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

// 3D Parallax Tilt & Specular Lighting
#define TIMER_ID_TILT_SPRING 101
static bool g_heroTiltHovered = false;
static float g_heroTiltX = 0.0f; // -1.0 .. +1.0
static float g_heroTiltY = 0.0f; // -1.0 .. +1.0
static bool g_tiltTimerActive = false;
static bool g_mouseTracking = false;

static bool g_isEyedropperActive = false;
static POINT g_cursorClientPt = {0, 0};
static COLORREF g_eyedropperLiveColor = RGB(26, 115, 232);
static RoundStyle g_eyedropperPrevStyle;

static std::wstring g_statusText = L"Готово: выберите приложение слева или настройте параметры иконки";
static double g_progressRatio = 0.0;
static uint32_t g_iconRevCounter = 1;

static int g_inspectorScrollY = 0;
static int g_maxInspectorScroll = 0;
static const RectF* g_currentClipRect = nullptr;

static int g_activePack = -1;
static bool g_adjustThisAppOnly = false;

// Hero 256px preview cache (avoids pixel-shader re-render on 60 FPS parallax tilt)
static Bitmap* g_cachedHeroPreview = nullptr;
static int g_cachedHeroAppIdx = -1;
static Bitmap* g_cachedHeroArtPtr = nullptr;
static RoundStyle g_cachedHeroStyle;

static void InvalidateHeroCache() {
    if (g_cachedHeroPreview) {
        delete g_cachedHeroPreview;
        g_cachedHeroPreview = nullptr;
    }
    g_cachedHeroAppIdx = -1;
    g_cachedHeroArtPtr = nullptr;
}

// Undo / Redo history
struct StyleHistoryEntry {
    RoundStyle globalStyle;
    int appIdx = -1;
    bool appHadCustom = false;
    RoundStyle appCustomStyle;
};
static std::vector<StyleHistoryEntry> g_undoStack;
static std::vector<StyleHistoryEntry> g_redoStack;

enum AppGalleryMode { APP_VIEW_GRID = 0, APP_VIEW_LIST = 1 };
static int g_viewMode = APP_VIEW_GRID;
static int g_gridScrollY = 0;
static int g_gridIconSize = 56; // Grid Gallery icon size in px (48..96)

enum InspectorTab { INSP_TAB_STYLE = 0, INSP_TAB_ARTWORK = 1, INSP_TAB_GEOMETRY = 2, INSP_TAB_OPTIONS = 3 };
static int g_inspectorTab = INSP_TAB_STYLE;

static inline Bitmap* GetEffectiveArtwork(const AppEntry& app) {
    return app.customArtwork ? app.customArtwork : app.origBitmap;
}

static inline const RoundStyle& GetEffectiveStyle(const AppEntry& app) {
    return app.hasCustomStyle ? app.customStyle : g_style;
}

static inline const RoundStyle& CurrentInspectorStyle() {
    if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
        const AppEntry& app = g_allApps[g_selectedAppIndex];
        if (app.hasCustomStyle) return app.customStyle;
    }
    return g_style;
}

static inline bool IsStyleDarkSurface(const RoundStyle& st) {
    if (st.preset == PRESET_DARK_MATERIAL) return true;
    if (st.preset == PRESET_CUSTOM_COLOR) {
        double lum = 0.299 * GetRValue(st.customColor) + 0.587 * GetGValue(st.customColor) + 0.114 * GetBValue(st.customColor);
        return lum < 128.0;
    }
    return false;
}

// Forward declarations
static void InvalidateAllThumbs();
static void RebuildFilteredList();
static void SaveStyleConfig();
static void SavePerAppOverride(const AppEntry& app);

static void PushUndoSnapshot() {
    StyleHistoryEntry snap;
    snap.globalStyle = g_style;
    snap.appIdx = g_selectedAppIndex;
    if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
        snap.appHadCustom = g_allApps[g_selectedAppIndex].hasCustomStyle;
        snap.appCustomStyle = g_allApps[g_selectedAppIndex].customStyle;
    }
    if (!g_undoStack.empty()) {
        const auto& last = g_undoStack.back();
        if (last.globalStyle == snap.globalStyle &&
            last.appIdx == snap.appIdx &&
            last.appHadCustom == snap.appHadCustom &&
            (!snap.appHadCustom || last.appCustomStyle == snap.appCustomStyle)) {
            return;
        }
    }
    if (g_undoStack.size() >= 40) {
        g_undoStack.erase(g_undoStack.begin());
    }
    g_undoStack.push_back(snap);
    g_redoStack.clear();
}

static RoundStyle& BeginEditStyle(bool recordUndo = true) {
    if (recordUndo) PushUndoSnapshot();
    if (g_adjustThisAppOnly && g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
        AppEntry& app = g_allApps[g_selectedAppIndex];
        if (!app.hasCustomStyle) {
            app.customStyle = g_style;
            app.hasCustomStyle = true;
        }
        return app.customStyle;
    }
    return g_style;
}

static void CommitEditedStyle() {
    InvalidateHeroCache();
    if (g_adjustThisAppOnly && g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
        AppEntry& app = g_allApps[g_selectedAppIndex];
        if (app.cachedThumb) {
            delete app.cachedThumb;
            app.cachedThumb = nullptr;
        }
        SavePerAppOverride(app);
    } else {
        SaveStyleConfig();
        InvalidateAllThumbs();
    }
}

static bool PerformUndo(HWND hwnd) {
    if (g_undoStack.empty()) return false;
    StyleHistoryEntry cur;
    cur.globalStyle = g_style;
    cur.appIdx = g_selectedAppIndex;
    if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
        cur.appHadCustom = g_allApps[g_selectedAppIndex].hasCustomStyle;
        cur.appCustomStyle = g_allApps[g_selectedAppIndex].customStyle;
    }
    g_redoStack.push_back(cur);

    StyleHistoryEntry prev = g_undoStack.back();
    g_undoStack.pop_back();
    g_style = prev.globalStyle;
    if (prev.appIdx >= 0 && prev.appIdx < (int)g_allApps.size()) {
        AppEntry& app = g_allApps[prev.appIdx];
        app.hasCustomStyle = prev.appHadCustom;
        app.customStyle = prev.appCustomStyle;
        SavePerAppOverride(app);
    }
    SaveStyleConfig();
    InvalidateHeroCache();
    InvalidateAllThumbs();
    g_statusText = L"Отменено последнее изменение стиля";
    InvalidateRect(hwnd, NULL, FALSE);
    return true;
}

static bool PerformRedo(HWND hwnd) {
    if (g_redoStack.empty()) return false;
    StyleHistoryEntry cur;
    cur.globalStyle = g_style;
    cur.appIdx = g_selectedAppIndex;
    if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
        cur.appHadCustom = g_allApps[g_selectedAppIndex].hasCustomStyle;
        cur.appCustomStyle = g_allApps[g_selectedAppIndex].customStyle;
    }
    g_undoStack.push_back(cur);

    StyleHistoryEntry next = g_redoStack.back();
    g_redoStack.pop_back();
    g_style = next.globalStyle;
    if (next.appIdx >= 0 && next.appIdx < (int)g_allApps.size()) {
        AppEntry& app = g_allApps[next.appIdx];
        app.hasCustomStyle = next.appHadCustom;
        app.customStyle = next.appCustomStyle;
        SavePerAppOverride(app);
    }
    SaveStyleConfig();
    InvalidateHeroCache();
    InvalidateAllThumbs();
    g_statusText = L"Повторено изменение стиля";
    InvalidateRect(hwnd, NULL, FALSE);
    return true;
}

static void UpdateWindowImmersiveDarkMode(HWND hwnd, bool dark) {
    HMODULE hDwm = LoadLibraryW(L"dwmapi.dll");
    if (hDwm) {
        typedef HRESULT(WINAPI* PFN_DwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);
        PFN_DwmSetWindowAttribute fn = (PFN_DwmSetWindowAttribute)GetProcAddress(hDwm, "DwmSetWindowAttribute");
        if (fn) {
            BOOL bDark = dark ? TRUE : FALSE;
            fn(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &bDark, sizeof(bDark));
            fn(hwnd, 19 /* older Win10 build */, &bDark, sizeof(bDark));

            DWORD cornerPref = 2; // DWMWCP_ROUND (DWMWA_WINDOW_CORNER_PREFERENCE = 33)
            fn(hwnd, 33, &cornerPref, sizeof(cornerPref));

            // Windows 11 22H2+ System Backdrop (Mica = 2, Acrylic = 3, Tabbed/Mica Alt = 4)
            DWORD backdropType = 2; // DWMSBT_MAINWINDOW (Mica)
            fn(hwnd, 38 /* DWMWA_SYSTEMBACKDROP_TYPE */, &backdropType, sizeof(backdropType));

            // Windows 11 21H2 (Build 22000) Mica toggle
            BOOL bMica = TRUE;
            fn(hwnd, 1029 /* DWMWA_MICA_EFFECT */, &bMica, sizeof(bMica));
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
    InvalidateHeroCache();
    for (auto& app : g_allApps) {
        delete app.cachedThumb;
        app.cachedThumb = nullptr;
    }
}

static Bitmap* GetAppThumb(AppEntry& app) {
    Bitmap* art = GetEffectiveArtwork(app);
    if (!app.cachedThumb && art) {
        app.cachedThumb = RenderGoogleRoundIcon(art, GetEffectiveStyle(app), 128);
    }
    return app.cachedThumb;
}

static void WriteRoundStyleToIni(const wchar_t* section, const RoundStyle& st, const std::wstring& iniPath) {
    wchar_t buf[64];
    wsprintfW(buf, L"%d", (int)st.preset);
    WritePrivateProfileStringW(section, L"Preset", buf, iniPath.c_str());
    wsprintfW(buf, L"%d", (int)st.shape);
    WritePrivateProfileStringW(section, L"Shape", buf, iniPath.c_str());
    wsprintfW(buf, L"%d", (int)std::round(st.scale * 1000.0));
    WritePrivateProfileStringW(section, L"ScaleMilli", buf, iniPath.c_str());
    wsprintfW(buf, L"%d", (int)std::round(st.offsetX * 1000.0));
    WritePrivateProfileStringW(section, L"OffsetXMilli", buf, iniPath.c_str());
    wsprintfW(buf, L"%d", (int)std::round(st.offsetY * 1000.0));
    WritePrivateProfileStringW(section, L"OffsetYMilli", buf, iniPath.c_str());
    wsprintfW(buf, L"%d", (int)std::round(st.shadow * 1000.0));
    WritePrivateProfileStringW(section, L"ShadowMilli", buf, iniPath.c_str());
    WritePrivateProfileStringW(section, L"AutoTrim", st.autoTrim ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(section, L"InnerShadow", st.innerShadow ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(section, L"FullBleed", st.fullBleed ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(section, L"Monochrome", st.monochrome ? L"1" : L"0", iniPath.c_str());
    wsprintfW(buf, L"%u", (unsigned)st.customColor);
    WritePrivateProfileStringW(section, L"CustomColor", buf, iniPath.c_str());
    wsprintfW(buf, L"%d", (int)st.material);
    WritePrivateProfileStringW(section, L"Material", buf, iniPath.c_str());
    wsprintfW(buf, L"%d", (int)st.glyphStyle);
    WritePrivateProfileStringW(section, L"GlyphStyle", buf, iniPath.c_str());
    wsprintfW(buf, L"%d", (int)std::round(st.rotation));
    WritePrivateProfileStringW(section, L"RotationDeg", buf, iniPath.c_str());
    WritePrivateProfileStringW(section, L"FlipH", st.flipH ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(section, L"FlipV", st.flipV ? L"1" : L"0", iniPath.c_str());
    wsprintfW(buf, L"%u", (unsigned)st.glyphColor);
    WritePrivateProfileStringW(section, L"GlyphColor", buf, iniPath.c_str());
    wsprintfW(buf, L"%d", (int)st.glyphGrad);
    WritePrivateProfileStringW(section, L"GlyphGrad", buf, iniPath.c_str());
    wsprintfW(buf, L"%d", (int)st.badge);
    WritePrivateProfileStringW(section, L"Badge", buf, iniPath.c_str());
    wsprintfW(buf, L"%d", (int)st.badgePos);
    WritePrivateProfileStringW(section, L"BadgePos", buf, iniPath.c_str());
}

static void ReadRoundStyleFromIni(const wchar_t* section, RoundStyle& st, const std::wstring& iniPath) {
    st.preset = (BgPreset)std::clamp((int)GetPrivateProfileIntW(section, L"Preset", (INT)st.preset, iniPath.c_str()), 0, 4);
    st.shape = (IconShape)std::clamp((int)GetPrivateProfileIntW(section, L"Shape", (INT)st.shape, iniPath.c_str()), 0, 4);
    int sc = (int)GetPrivateProfileIntW(section, L"ScaleMilli", (INT)std::round(st.scale * 1000.0), iniPath.c_str());
    st.scale = std::clamp(sc / 1000.0, 0.30, 2.50);
    int ox = (int)GetPrivateProfileIntW(section, L"OffsetXMilli", (INT)std::round(st.offsetX * 1000.0), iniPath.c_str());
    st.offsetX = std::clamp(ox / 1000.0, -1.0, 1.0);
    int oy = (int)GetPrivateProfileIntW(section, L"OffsetYMilli", (INT)std::round(st.offsetY * 1000.0), iniPath.c_str());
    st.offsetY = std::clamp(oy / 1000.0, -1.0, 1.0);
    int sh = (int)GetPrivateProfileIntW(section, L"ShadowMilli", (INT)std::round(st.shadow * 1000.0), iniPath.c_str());
    st.shadow = std::clamp(sh / 1000.0, 0.0, 1.0);
    st.autoTrim = GetPrivateProfileIntW(section, L"AutoTrim", st.autoTrim ? 1 : 0, iniPath.c_str()) != 0;
    st.innerShadow = GetPrivateProfileIntW(section, L"InnerShadow", st.innerShadow ? 1 : 0, iniPath.c_str()) != 0;
    st.fullBleed = GetPrivateProfileIntW(section, L"FullBleed", st.fullBleed ? 1 : 0, iniPath.c_str()) != 0;
    st.monochrome = GetPrivateProfileIntW(section, L"Monochrome", st.monochrome ? 1 : 0, iniPath.c_str()) != 0;
    st.customColor = (COLORREF)GetPrivateProfileIntW(section, L"CustomColor", (INT)st.customColor, iniPath.c_str());
    st.material = (SurfaceMaterial)std::clamp((int)GetPrivateProfileIntW(section, L"Material", (INT)st.material, iniPath.c_str()), 0, 7);
    st.glyphStyle = (GlyphStyleMode)std::clamp((int)GetPrivateProfileIntW(section, L"GlyphStyle", (INT)st.glyphStyle, iniPath.c_str()), 0, 5);
    int rot = (int)GetPrivateProfileIntW(section, L"RotationDeg", (INT)std::round(st.rotation), iniPath.c_str());
    st.rotation = std::fmod((double)((rot % 360) + 360), 360.0);
    st.flipH = GetPrivateProfileIntW(section, L"FlipH", st.flipH ? 1 : 0, iniPath.c_str()) != 0;
    st.flipV = GetPrivateProfileIntW(section, L"FlipV", st.flipV ? 1 : 0, iniPath.c_str()) != 0;
    st.glyphColor = (COLORREF)GetPrivateProfileIntW(section, L"GlyphColor", (INT)st.glyphColor, iniPath.c_str());
    st.glyphGrad = (GlyphGradientPreset)std::clamp((int)GetPrivateProfileIntW(section, L"GlyphGrad", (INT)st.glyphGrad, iniPath.c_str()), 0, 6);
    st.badge = (GlyphBadgeOverlay)std::clamp((int)GetPrivateProfileIntW(section, L"Badge", (INT)st.badge, iniPath.c_str()), 0, 7);
    st.badgePos = (GlyphBadgePosition)std::clamp((int)GetPrivateProfileIntW(section, L"BadgePos", (INT)st.badgePos, iniPath.c_str()), 0, 3);
}

static std::wstring GetArtworkStorageDir() {
    std::wstring dir = GetAppDataRoot() + L"\\artwork";
    CreateDirectoryW(dir.c_str(), NULL);
    return dir;
}

static std::wstring GetAppOverrideKey(const AppEntry& app) {
    return SanitizeFileName(app.name);
}

static void SavePerAppOverride(const AppEntry& app) {
    std::wstring iniPath = GetAppDataRoot() + L"\\overrides.ini";
    std::wstring sec = GetAppOverrideKey(app);
    if (sec.empty()) return;
    if (!app.hasCustomStyle) {
        WritePrivateProfileStringW(sec.c_str(), NULL, NULL, iniPath.c_str());
        return;
    }
    WritePrivateProfileStringW(sec.c_str(), L"HasCustomStyle", L"1", iniPath.c_str());
    WriteRoundStyleToIni(sec.c_str(), app.customStyle, iniPath);
}

static void LoadAppOverrides(AppEntry& app) {
    std::wstring sec = GetAppOverrideKey(app);
    if (sec.empty()) return;

    std::wstring artPath = GetArtworkStorageDir() + L"\\" + sec + L".png";
    if (GetFileAttributesW(artPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        Bitmap* loaded = IconEngine::LoadSvgOrRasterArtworkFile(artPath, 256);
        if (loaded) {
            if (app.customArtwork) delete app.customArtwork;
            app.customArtwork = loaded;
        }
    }

    std::wstring iniPath = GetAppDataRoot() + L"\\overrides.ini";
    if (GetFileAttributesW(iniPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        if (GetPrivateProfileIntW(sec.c_str(), L"HasCustomStyle", 0, iniPath.c_str()) != 0) {
            app.hasCustomStyle = true;
            app.customStyle = g_style;
            ReadRoundStyleFromIni(sec.c_str(), app.customStyle, iniPath);
        }
    }
}

static void SetAppCustomArtwork(AppEntry& app, Bitmap* newArt) {
    if (app.customArtwork) {
        delete app.customArtwork;
        app.customArtwork = nullptr;
    }
    app.customArtwork = newArt;
    if (app.cachedThumb) {
        delete app.cachedThumb;
        app.cachedThumb = nullptr;
    }
    InvalidateHeroCache();

    std::wstring sec = GetAppOverrideKey(app);
    if (!sec.empty()) {
        std::wstring artPath = GetArtworkStorageDir() + L"\\" + sec + L".png";
        if (newArt) {
            newArt->Save(artPath.c_str(), &g_pngClsid, NULL);
        } else {
            DeleteFileW(artPath.c_str());
        }
    }
}

static void SaveStyleConfig() {
    std::wstring cfgPath = GetAppDataRoot() + L"\\style.cfg";
    wchar_t buf[64];
    WritePrivateProfileStringW(L"Style", L"Theme", g_isDarkMode ? L"dark" : L"light", cfgPath.c_str());
    WritePrivateProfileStringW(L"Style", L"AdjustPerApp", g_adjustThisAppOnly ? L"1" : L"0", cfgPath.c_str());
    WriteRoundStyleToIni(L"Style", g_style, cfgPath);
    wsprintfW(buf, L"%d", g_gridIconSize);
    WritePrivateProfileStringW(L"Style", L"GridIconSize", buf, cfgPath.c_str());
}

static void LoadStyleConfig() {
    std::wstring cfgPath = GetAppDataRoot() + L"\\style.cfg";
    if (GetFileAttributesW(cfgPath.c_str()) == INVALID_FILE_ATTRIBUTES) return;

    wchar_t themeBuf[32] = {0};
    GetPrivateProfileStringW(L"Style", L"Theme", L"dark", themeBuf, 31, cfgPath.c_str());
    g_isDarkMode = (_wcsicmp(themeBuf, L"light") != 0);
    g_adjustThisAppOnly = GetPrivateProfileIntW(L"Style", L"AdjustPerApp", 0, cfgPath.c_str()) != 0;

    ReadRoundStyleFromIni(L"Style", g_style, cfgPath);
    g_gridIconSize = std::clamp((int)GetPrivateProfileIntW(L"Style", L"GridIconSize", 56, cfgPath.c_str()), 48, 96);
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
    InvalidateHeroCache();
    for (auto& app : g_allApps) {
        delete app.origBitmap;
        delete app.customArtwork;
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

    for (auto& app : g_allApps) {
        LoadAppOverrides(app);
    }

    RebuildFilteredList();
}

static bool ApplyRoundIcon(AppEntry& app, bool* outNeedsAdmin = nullptr) {
    return ShellIntegration::ApplyRoundIconToEntry(app, GetEffectiveStyle(app), &g_pngClsid, g_iconRevCounter, outNeedsAdmin);
}

static bool AddFolderEntry(HWND hwnd, const std::wstring& folderPath) {
    if (folderPath.empty()) return false;
    DWORD attr = GetFileAttributesW(folderPath.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) return false;

    for (size_t i = 0; i < g_allApps.size(); ++i) {
        if (_wcsicmp(g_allApps[i].targetPath.c_str(), folderPath.c_str()) == 0) {
            g_selectedAppIndex = (int)i;
            InvalidateRect(hwnd, NULL, FALSE);
            return true;
        }
    }

    AppEntry entry;
    std::wstring fname = PathFindFileNameW(folderPath.c_str());
    if (fname.empty()) fname = folderPath;
    entry.name = fname;
    entry.targetPath = folderPath;
    entry.isFolder = true;
    entry.category = L"Папки";
    entry.origBitmap = LoadHighResIcon(entry.targetPath, L"");

    std::wstring iniPath = folderPath + L"\\desktop.ini";
    if (GetFileAttributesW(iniPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        wchar_t res[MAX_PATH] = { 0 };
        GetPrivateProfileStringW(L".ShellClassInfo", L"IconResource", L"", res, MAX_PATH, iniPath.c_str());
        if (!res[0]) {
            GetPrivateProfileStringW(L".ShellClassInfo", L"IconFile", L"", res, MAX_PATH, iniPath.c_str());
        }
        if (res[0] && ShellIntegration::IsModoManagedIconPath(res)) entry.isApplied = true;
    }

    LoadAppOverrides(entry);
    g_allApps.insert(g_allApps.begin(), entry);
    RebuildFilteredList();
    g_selectedAppIndex = 0;
    InvalidateHeroCache();
    g_statusText = L"Добавлена папка для оформления: " + entry.name;
    InvalidateRect(hwnd, NULL, FALSE);
    return true;
}

static void AddCustomFolder(HWND hwnd) {
    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = hwnd;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;
    bi.lpszTitle = L"Выберите папку для применения адаптивного стиля Modo:";
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        wchar_t folderPath[MAX_PATH] = { 0 };
        if (SHGetPathFromIDListW(pidl, folderPath) && folderPath[0]) {
            AddFolderEntry(hwnd, folderPath);
        }
        CoTaskMemFree(pidl);
    }
}

static bool AddFileOrAppPath(HWND hwnd, const std::wstring& filePath) {
    if (filePath.empty()) return false;
    DWORD attr = GetFileAttributesW(filePath.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        return AddFolderEntry(hwnd, filePath);
    }
    std::wstring path = filePath;
    std::wstring fname = PathFindFileNameW(filePath.c_str());
    size_t dot = fname.find_last_of(L'.');
    std::wstring title = (dot != std::wstring::npos) ? fname.substr(0, dot) : fname;
    std::wstring ext = PathFindExtensionW(filePath.c_str());
    for (auto& c : ext) c = towlower(c);

    // If it's an image or vector file (.png, .svg, .jpg, .ico, .bmp)
    if (ext == L".png" || ext == L".svg" || ext == L".ico" || ext == L".jpg" || ext == L".jpeg" || ext == L".bmp") {
        Bitmap* bmp = IconEngine::LoadSvgOrRasterArtworkFile(path, 256);
        if (bmp && bmp->GetLastStatus() == Ok) {
            if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
                AppEntry& app = g_allApps[g_selectedAppIndex];
                SetAppCustomArtwork(app, bmp);
                g_statusText = L"Пользовательский рисунок назначен: " + app.name;
                InvalidateRect(hwnd, NULL, FALSE);
                return true;
            } else {
                AppEntry entry;
                entry.name = title;
                entry.targetPath = path;
                entry.category = L"Изображения";
                entry.origBitmap = bmp;
                LoadAppOverrides(entry);
                g_allApps.insert(g_allApps.begin(), entry);
                RebuildFilteredList();
                g_selectedAppIndex = 0;
                InvalidateHeroCache();
                g_statusText = L"Добавлено изображение: " + entry.name;
                InvalidateRect(hwnd, NULL, FALSE);
                return true;
            }
        }
        delete bmp;
    }

    // Check if an existing app already matches this .lnk or .exe path
    std::wstring resolvedTarget = path;
    if (ext == L".lnk") {
        std::wstring customIcon;
        ResolveShortcut(path, resolvedTarget, customIcon);
        if (resolvedTarget.empty()) resolvedTarget = path;
    }
    for (size_t i = 0; i < g_allApps.size(); ++i) {
        if ((!g_allApps[i].lnkPath.empty() && _wcsicmp(g_allApps[i].lnkPath.c_str(), path.c_str()) == 0) ||
            (!g_allApps[i].targetPath.empty() && _wcsicmp(g_allApps[i].targetPath.c_str(), resolvedTarget.c_str()) == 0)) {
            g_selectedAppIndex = (int)i;
            RebuildFilteredList();
            InvalidateHeroCache();
            g_statusText = L"Выбрано: " + g_allApps[i].name;
            InvalidateRect(hwnd, NULL, FALSE);
            return true;
        }
    }

    // Executable or shortcut
    AppEntry entry;
    entry.name = title;
    if (ext == L".lnk") {
        entry.lnkPath = path;
        entry.allLnkPaths.push_back(path);
        entry.targetPath = resolvedTarget;
    } else {
        entry.targetPath = path;
    }
    entry.category = (ext == L".lnk") ? L"Ярлыки" : L"Добавлено вручную";
    entry.origBitmap = LoadHighResIcon(entry.targetPath, entry.lnkPath, SanitizeFileName(entry.name), &g_pngClsid);
    LoadAppOverrides(entry);
    g_allApps.insert(g_allApps.begin(), entry);
    RebuildFilteredList();
    g_selectedAppIndex = 0;
    InvalidateHeroCache();
    g_statusText = L"Добавлено: " + entry.name;
    InvalidateRect(hwnd, NULL, FALSE);
    return true;
}

static void AddCustomAppOrImage(HWND hwnd) {
    wchar_t filePath[MAX_PATH] = {0};
    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"Приложения и значки (*.exe;*.lnk;*.png;*.svg;*.ico;*.jpg)\0*.exe;*.lnk;*.png;*.svg;*.ico;*.jpg\0Все файлы (*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Выберите приложение (.exe/.lnk) или рисунок (.png/.svg/.ico)";

    if (GetOpenFileNameW(&ofn) && filePath[0]) {
        AddFileOrAppPath(hwnd, filePath);
    }
}

static Bitmap* CreateBitmapFromClipboard() {
    if (!OpenClipboard(NULL)) return nullptr;
    Bitmap* bmp = nullptr;
    HANDLE hDib = GetClipboardData(CF_DIB);
    if (hDib) {
        BITMAPINFO* bi = (BITMAPINFO*)GlobalLock(hDib);
        if (bi) {
            void* pBits = (BYTE*)bi + bi->bmiHeader.biSize + (bi->bmiHeader.biClrUsed * sizeof(RGBQUAD));
            if (bi->bmiHeader.biBitCount == 32) {
                int w = bi->bmiHeader.biWidth;
                int h = std::abs(bi->bmiHeader.biHeight);
                bmp = new Bitmap(w, h, PixelFormat32bppARGB);
                BitmapData bd;
                Rect rc(0, 0, w, h);
                if (bmp->LockBits(&rc, ImageLockModeWrite, PixelFormat32bppARGB, &bd) == Ok) {
                    bool bottomUp = (bi->bmiHeader.biHeight > 0);
                    for (int y = 0; y < h; ++y) {
                        int srcY = bottomUp ? (h - 1 - y) : y;
                        uint8_t* pDst = (uint8_t*)bd.Scan0 + y * bd.Stride;
                        const uint8_t* pSrc = (const uint8_t*)pBits + srcY * (w * 4);
                        memcpy(pDst, pSrc, w * 4);
                    }
                    bmp->UnlockBits(&bd);
                }
            } else {
                HDC hdc = GetDC(NULL);
                HBITMAP hBmp = CreateDIBitmap(hdc, &bi->bmiHeader, CBM_INIT, pBits, bi, DIB_RGB_COLORS);
                ReleaseDC(NULL, hdc);
                if (hBmp) {
                    bmp = Bitmap::FromHBITMAP(hBmp, NULL);
                    DeleteObject(hBmp);
                }
            }
            GlobalUnlock(hDib);
        }
    } else {
        HBITMAP hBmp = (HBITMAP)GetClipboardData(CF_BITMAP);
        if (hBmp) {
            bmp = Bitmap::FromHBITMAP(hBmp, NULL);
        }
    }
    CloseClipboard();
    return bmp;
}

static Bitmap* PromptUserForArtworkFile(HWND hwnd) {
    wchar_t filePath[MAX_PATH] = {0};
    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"Рисунки и векторные значки (*.png;*.svg;*.ico;*.jpg;*.jpeg;*.bmp)\0*.png;*.svg;*.ico;*.jpg;*.jpeg;*.bmp\0Все файлы (*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Выберите изображение или SVG для рисунка значка (.png, .svg, .ico, .jpg)";

    if (GetOpenFileNameW(&ofn) && filePath[0]) {
        return IconEngine::LoadSvgOrRasterArtworkFile(filePath, 256);
    }
    return nullptr;
}

static void ExportStandardIcons(HWND hwnd) {
    if (g_selectedAppIndex < 0 || g_selectedAppIndex >= (int)g_allApps.size()) {
        MessageBoxW(hwnd, L"Выберите приложение в галерее или списке для экспорта.", L"Modo Studio", MB_OK | MB_ICONWARNING);
        return;
    }
    BROWSEINFOW bi = {0};
    bi.hwndOwner = hwnd;
    bi.lpszTitle = L"Выберите папку для экспорта иконки (.ico и 1024px .png):";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return;

    wchar_t outDir[MAX_PATH] = {0};
    if (SHGetPathFromIDListW(pidl, outDir) && outDir[0]) {
        std::wstring dir = outDir;
        AppEntry& app = g_allApps[g_selectedAppIndex];
        const RoundStyle& eff = GetEffectiveStyle(app);
        std::wstring safe = SanitizeFileName(app.name);
        Bitmap* art = GetEffectiveArtwork(app);
        Bitmap* master1024 = RenderGoogleRoundIcon(art, eff, 1024);
        std::wstring icoFile = dir + L"\\" + safe + L".ico";
        std::wstring pngFile = dir + L"\\" + safe + L".png";
        SaveMultiSizeIco(art, eff, icoFile, &g_pngClsid);
        if (master1024) {
            master1024->Save(pngFile.c_str(), &g_pngClsid, NULL);
            delete master1024;
        }
        g_statusText = L"Экспортированы .ico + 1024px .png в " + dir;
        InvalidateRect(hwnd, NULL, FALSE);
        MessageBoxW(hwnd, (L"Иконка (.ico + 1024px .png) успешно сохранена в:\n" + dir).c_str(),
                    L"Экспорт завершён", MB_ICONINFORMATION);
    }
    CoTaskMemFree(pidl);
}

static void ExportWebFaviconKitDialog(HWND hwnd) {
    if (g_selectedAppIndex < 0 || g_selectedAppIndex >= (int)g_allApps.size()) {
        MessageBoxW(hwnd, L"Выберите приложение в галерее или списке для экспорта.", L"Modo Studio", MB_OK | MB_ICONWARNING);
        return;
    }
    BROWSEINFOW bi = {0};
    bi.hwndOwner = hwnd;
    bi.lpszTitle = L"Выберите папку для создания пакета Web Favicon Kit & Manifest:";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return;

    wchar_t outDir[MAX_PATH] = {0};
    if (SHGetPathFromIDListW(pidl, outDir) && outDir[0]) {
        std::wstring dir = outDir;
        AppEntry& app = g_allApps[g_selectedAppIndex];
        Bitmap* art = GetEffectiveArtwork(app);
        if (ExportWebFaviconKit(art, GetEffectiveStyle(app), app.name, dir, &g_pngClsid)) {
            g_statusText = L"Web Favicon Kit успешно сохранён в " + dir;
            InvalidateRect(hwnd, NULL, FALSE);
            MessageBoxW(hwnd, (L"Пакет Web Favicon Kit (favicon.ico, PNG 16/32/180/192/512, site.webmanifest, HTML-теги) успешно создан в:\n" + dir).c_str(),
                        L"Экспорт Web Favicon Kit", MB_ICONINFORMATION);
        } else {
            MessageBoxW(hwnd, L"Не удалось создать пакет веб-иконок.", L"Ошибка экспорта", MB_OK | MB_ICONERROR);
        }
    }
    CoTaskMemFree(pidl);
}

static void ExportXcodeCatalogDialog(HWND hwnd) {
    if (g_selectedAppIndex < 0 || g_selectedAppIndex >= (int)g_allApps.size()) {
        MessageBoxW(hwnd, L"Выберите приложение в галерее или списке для экспорта.", L"Modo Studio", MB_OK | MB_ICONWARNING);
        return;
    }
    BROWSEINFOW bi = {0};
    bi.hwndOwner = hwnd;
    bi.lpszTitle = L"Выберите папку для создания каталога Xcode Asset Catalog (.appiconset):";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return;

    wchar_t outDir[MAX_PATH] = {0};
    if (SHGetPathFromIDListW(pidl, outDir) && outDir[0]) {
        std::wstring dir = outDir;
        AppEntry& app = g_allApps[g_selectedAppIndex];
        Bitmap* art = GetEffectiveArtwork(app);
        if (ExportXcodeAssetCatalog(art, GetEffectiveStyle(app), app.name, dir, &g_pngClsid)) {
            g_statusText = L"Xcode Asset Catalog успешно сохранён в " + dir;
            InvalidateRect(hwnd, NULL, FALSE);
            MessageBoxW(hwnd, (L"Каталог Xcode Asset Catalog (.appiconset с 11 размерами и Contents.json) успешно создан в:\n" + dir).c_str(),
                        L"Экспорт Xcode Catalog", MB_ICONINFORMATION);
        } else {
            MessageBoxW(hwnd, L"Не удалось создать каталог Xcode.", L"Ошибка экспорта", MB_OK | MB_ICONERROR);
        }
    }
    CoTaskMemFree(pidl);
}

static void ExportShowcaseBannerDialog(HWND hwnd, ShowcaseTheme theme = SHOWCASE_THEME_AMBIENT_MESH, ShowcaseLayout layout = SHOWCASE_LAYOUT_HERO) {
    if (g_selectedAppIndex < 0 || g_selectedAppIndex >= (int)g_allApps.size()) {
        MessageBoxW(hwnd, L"Выберите приложение в галерее или списке для создания баннера.", L"Modo Studio", MB_OK | MB_ICONWARNING);
        return;
    }
    AppEntry& app = g_allApps[g_selectedAppIndex];
    Bitmap* art = GetEffectiveArtwork(app);
    std::wstring defName = SanitizeFileName(app.name) + L"-showcase.png";

    wchar_t szFile[MAX_PATH] = {0};
    wcsncpy(szFile, defName.c_str(), MAX_PATH - 1);

    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"PNG Showcase Banner (*.png)\0*.png\0Все файлы (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"Сохранить презентационный баннер Showcase (1920x1080):";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"png";

    if (GetSaveFileNameW(&ofn)) {
        if (ExportShowcaseBanner(art, GetEffectiveStyle(app), app.name,
                                 theme, layout,
                                 szFile, &g_pngClsid)) {
            g_statusText = L"Showcase баннер успешно сохранён: " + std::wstring(szFile);
            InvalidateRect(hwnd, NULL, FALSE);
            MessageBoxW(hwnd, (L"Showcase презентация (1920x1080 PNG)\nуспешно сформирована и сохранена в:\n" + std::wstring(szFile)).c_str(),
                        L"Экспорт Showcase баннера", MB_ICONINFORMATION);
        } else {
            MessageBoxW(hwnd, L"Не удалось создать Showcase баннер.", L"Ошибка экспорта", MB_OK | MB_ICONERROR);
        }
    }
}

static void ShowExportMenu(HWND hwnd, POINT screenPt) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 1, L"Стандартный экспорт (.ico + 1024px .png)\tCtrl+E");
    AppendMenuW(hMenu, MF_STRING, 2, L"Веб-набор (Web Favicon Kit & Manifest)...");
    AppendMenuW(hMenu, MF_STRING, 3, L"Xcode Asset Catalog (.appiconset)...");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    HMENU hShowcaseSub = CreatePopupMenu();
    AppendMenuW(hShowcaseSub, MF_STRING, 10, L"Ambient Mesh (Глубокий индиго)...");
    AppendMenuW(hShowcaseSub, MF_STRING, 11, L"Studio Obsidian (Тёмная студия)...");
    AppendMenuW(hShowcaseSub, MF_STRING, 12, L"Clean Minimal (Светлый минимализм)...");
    AppendMenuW(hShowcaseSub, MF_STRING, 13, L"Sunset Gradient (Тёплый закат)...");
    AppendMenuW(hShowcaseSub, MF_STRING, 14, L"Cyber Glow (Кибер-бирюза)...");
    AppendMenuW(hShowcaseSub, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hShowcaseSub, MF_STRING, 15, L"Трио вариантов (Light, Dark, Adaptive)...");
    AppendMenuW(hShowcaseSub, MF_STRING, 16, L"Карточка окна (Acrylic Window Card)...");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hShowcaseSub, L"Showcase & README Баннер (1920x1080)");

    SetForegroundWindow(hwnd);
    int cmd = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
                            screenPt.x, screenPt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);

    if (cmd == 1) {
        ExportStandardIcons(hwnd);
    } else if (cmd == 2) {
        ExportWebFaviconKitDialog(hwnd);
    } else if (cmd == 3) {
        ExportXcodeCatalogDialog(hwnd);
    } else if (cmd == 10) {
        ExportShowcaseBannerDialog(hwnd, SHOWCASE_THEME_AMBIENT_MESH, SHOWCASE_LAYOUT_HERO);
    } else if (cmd == 11) {
        ExportShowcaseBannerDialog(hwnd, SHOWCASE_THEME_STUDIO_DARK, SHOWCASE_LAYOUT_HERO);
    } else if (cmd == 12) {
        ExportShowcaseBannerDialog(hwnd, SHOWCASE_THEME_CLEAN_WHITE, SHOWCASE_LAYOUT_HERO);
    } else if (cmd == 13) {
        ExportShowcaseBannerDialog(hwnd, SHOWCASE_THEME_SUNSET_GLOW, SHOWCASE_LAYOUT_HERO);
    } else if (cmd == 14) {
        ExportShowcaseBannerDialog(hwnd, SHOWCASE_THEME_CYBER_NEON, SHOWCASE_LAYOUT_HERO);
    } else if (cmd == 15) {
        ExportShowcaseBannerDialog(hwnd, SHOWCASE_THEME_AMBIENT_MESH, SHOWCASE_LAYOUT_TRIPLE);
    } else if (cmd == 16) {
        ExportShowcaseBannerDialog(hwnd, SHOWCASE_THEME_AMBIENT_MESH, SHOWCASE_LAYOUT_WINDOW_CARD);
    }
}

static COLORREF GetWindowsAccentColor() {
    static COLORREF s_cachedAccent = RGB(26, 115, 232);
    static ULONGLONG s_lastCheckTick = 0;
    ULONGLONG now = GetTickCount64();
    if (s_lastCheckTick != 0 && (now - s_lastCheckTick) < 5000ULL) {
        return s_cachedAccent;
    }
    s_lastCheckTick = now;

    HMODULE hDwm = LoadLibraryW(L"dwmapi.dll");
    if (hDwm) {
        typedef HRESULT(WINAPI* PFN_DwmGetColorizationColor)(DWORD*, BOOL*);
        PFN_DwmGetColorizationColor fn = (PFN_DwmGetColorizationColor)GetProcAddress(hDwm, "DwmGetColorizationColor");
        if (fn) {
            DWORD colorization = 0;
            BOOL opaque = FALSE;
            if (SUCCEEDED(fn(&colorization, &opaque))) {
                FreeLibrary(hDwm);
                BYTE r = (BYTE)((colorization >> 16) & 0xFF);
                BYTE g = (BYTE)((colorization >> 8) & 0xFF);
                BYTE b = (BYTE)(colorization & 0xFF);
                s_cachedAccent = RGB(r, g, b);
                return s_cachedAccent;
            }
        }
        FreeLibrary(hDwm);
    }
    s_cachedAccent = GetSysColor(COLOR_HIGHLIGHT);
    return s_cachedAccent;
}

static void BatchConvertImagesDialog(HWND hwnd) {
    BROWSEINFOW biSrc = {0};
    biSrc.hwndOwner = hwnd;
    biSrc.lpszTitle = L"Выберите исходную папку с изображениями (.png, .svg, .jpg, .bmp, .ico):";
    biSrc.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidlSrc = SHBrowseForFolderW(&biSrc);
    if (!pidlSrc) return;

    wchar_t srcDir[MAX_PATH] = {0};
    if (!SHGetPathFromIDListW(pidlSrc, srcDir) || !srcDir[0]) {
        CoTaskMemFree(pidlSrc);
        return;
    }
    CoTaskMemFree(pidlSrc);

    std::wstring outDir = std::wstring(srcDir) + L"\\Modo-Icons";
    CreateDirectoryW(outDir.c_str(), NULL);

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW((std::wstring(srcDir) + L"\\*.*").c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        MessageBoxW(hwnd, L"В выбранной папке не найдено файлов.", L"Пакетная конвертация", MB_OK | MB_ICONWARNING);
        return;
    }

    std::vector<std::wstring> imgFiles;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::wstring fname = fd.cFileName;
        std::wstring ext = PathFindExtensionW(fname.c_str());
        for (auto& c : ext) c = (wchar_t)towlower(c);
        if (ext == L".png" || ext == L".svg" || ext == L".ico" || ext == L".jpg" || ext == L".jpeg" || ext == L".bmp") {
            imgFiles.push_back(fname);
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);

    if (imgFiles.empty()) {
        MessageBoxW(hwnd, L"В папке не найдено подходящих графических файлов (.png, .svg, .jpg, .bmp, .ico).", L"Пакетная конвертация", MB_OK | MB_ICONINFORMATION);
        return;
    }

    int processed = 0;
    for (size_t i = 0; i < imgFiles.size(); ++i) {
        const std::wstring& fname = imgFiles[i];
        std::wstring srcPath = std::wstring(srcDir) + L"\\" + fname;
        Bitmap* bmp = IconEngine::LoadSvgOrRasterArtworkFile(srcPath, 256);
        if (bmp && bmp->GetLastStatus() == Ok) {
            size_t dot = fname.find_last_of(L'.');
            std::wstring baseName = (dot != std::wstring::npos) ? fname.substr(0, dot) : fname;
            std::wstring icoPath = outDir + L"\\" + baseName + L".ico";
            std::wstring pngPath = outDir + L"\\" + baseName + L".png";

            SaveMultiSizeIco(bmp, g_style, icoPath, &g_pngClsid);
            Bitmap* master1024 = RenderGoogleRoundIcon(bmp, g_style, 1024);
            if (master1024) {
                master1024->Save(pngPath.c_str(), &g_pngClsid, NULL);
                delete master1024;
            }
            delete bmp;
            processed++;
        } else {
            delete bmp;
        }
        g_progressRatio = (double)(i + 1) / (double)imgFiles.size();
        g_statusText = L"Пакетная конвертация: " + fname;
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
    }

    g_progressRatio = 0.0;
    g_statusText = L"Пакетная конвертация завершена: " + std::to_wstring(processed) + L" иконок";
    InvalidateRect(hwnd, NULL, FALSE);

    std::wstring doneMsg = L"Успешно обработано и сохранено: " + std::to_wstring(processed) +
                           L" значков (.ico + 1024px .png)\n\nПапка назначения:\n" + outDir;
    MessageBoxW(hwnd, doneMsg.c_str(), L"Пакетная конвертация завершена", MB_ICONINFORMATION);
}

// ============================================================================
// macOS HIG GDI+ CANVAS RENDERER (ZERO EMOJIS, PURE DESIGN CANONS)
// ============================================================================

static void RegisterHit(WidgetID id, RectF r) {
    if (g_currentClipRect) {
        REAL top = std::max(r.Y, g_currentClipRect->Y);
        REAL bot = std::min(r.GetBottom(), g_currentClipRect->GetBottom());
        if (bot <= top) {
            return; // Out of view in scrolled container
        }
        r.Y = top;
        r.Height = bot - top;
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
    GLYPH_SHAPE_HEXAGON,
    GLYPH_EXPLORER,
    GLYPH_PHOTO,
    GLYPH_CLIPBOARD,
    GLYPH_VECTOR,
    GLYPH_FONT,
    GLYPH_BRAND,
    GLYPH_SPARKLE
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
    case GLYPH_EXPLORER: {
        g.DrawLine(&p, r.X + 1.0f, r.Y + 3.0f, r.X + 5.0f, r.Y + 3.0f);
        g.DrawLine(&p, r.X + 5.0f, r.Y + 3.0f, r.X + 7.0f, r.Y + 5.0f);
        g.DrawLine(&p, r.X + 7.0f, r.Y + 5.0f, r.GetRight() - 1.0f, r.Y + 5.0f);
        g.DrawLine(&p, r.GetRight() - 1.0f, r.Y + 5.0f, r.GetRight() - 1.0f, r.GetBottom() - 1.0f);
        g.DrawLine(&p, r.GetRight() - 1.0f, r.GetBottom() - 1.0f, r.X + 1.0f, r.GetBottom() - 1.0f);
        g.DrawLine(&p, r.X + 1.0f, r.GetBottom() - 1.0f, r.X + 1.0f, r.Y + 3.0f);
        break;
    }
    case GLYPH_PHOTO: {
        DrawRoundRect(g, RectF(r.X + 1.0f, r.Y + 2.0f, r.Width - 2.0f, r.Height - 4.0f), 2.0f, clr, 1.2f);
        g.FillEllipse(&b, r.X + 3.5f, r.Y + 4.0f, 2.5f, 2.5f);
        PointF pts[3] = { PointF(r.X + 2.0f, r.GetBottom() - 3.0f), PointF(r.X + 6.0f, r.Y + 6.0f), PointF(r.X + 10.0f, r.GetBottom() - 3.0f) };
        g.DrawLines(&p, pts, 3);
        break;
    }
    case GLYPH_CLIPBOARD: {
        DrawRoundRect(g, RectF(r.X + 2.0f, r.Y + 3.0f, r.Width - 4.0f, r.Height - 4.0f), 2.0f, clr, 1.2f);
        g.DrawLine(&p, r.X + 5.0f, r.Y + 1.5f, r.GetRight() - 5.0f, r.Y + 1.5f);
        break;
    }
    case GLYPH_VECTOR: {
        PointF pts[4] = { PointF(r.X + r.Width * 0.5f, r.Y + 1.0f), PointF(r.GetRight() - 1.0f, r.Y + r.Height * 0.5f),
                          PointF(r.X + r.Width * 0.5f, r.GetBottom() - 1.0f), PointF(r.X + 1.0f, r.Y + r.Height * 0.5f) };
        g.DrawPolygon(&p, pts, 4);
        break;
    }
    case GLYPH_FONT: {
        FontFamily ff(L"Segoe UI");
        Font fn(&ff, 9.0f, FontStyleBold, UnitPixel);
        StringFormat sf; sf.SetAlignment(StringAlignmentCenter); sf.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(L"A", -1, &fn, r, &sf, &b);
        break;
    }
    case GLYPH_BRAND: {
        PointF tag[5] = { {r.X + 2.0f, r.Y + 2.0f}, {r.GetRight() - 5.0f, r.Y + 2.0f}, {r.GetRight() - 1.0f, r.Y + r.Height*0.5f}, {r.GetRight() - 5.0f, r.GetBottom() - 2.0f}, {r.X + 2.0f, r.GetBottom() - 2.0f} };
        g.DrawPolygon(&p, tag, 5);
        g.FillEllipse(&b, r.X + 4.5f, r.Y + r.Height*0.5f - 1.5f, 3.0f, 3.0f);
        break;
    }
    case GLYPH_SPARKLE: {
        PointF sp[9] = {
            {r.X + r.Width*0.5f, r.Y + 1.0f}, {r.X + r.Width*0.62f, r.Y + r.Height*0.38f},
            {r.GetRight() - 1.0f, r.Y + r.Height*0.5f}, {r.X + r.Width*0.62f, r.Y + r.Height*0.62f},
            {r.X + r.Width*0.5f, r.GetBottom() - 1.0f}, {r.X + r.Width*0.38f, r.Y + r.Height*0.62f},
            {r.X + 1.0f, r.Y + r.Height*0.5f}, {r.X + r.Width*0.38f, r.Y + r.Height*0.38f},
            {r.X + r.Width*0.5f, r.Y + 1.0f}
        };
        g.FillPolygon(&b, sp, 8);
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

static const wchar_t* GetWidgetTooltip(WidgetID id) {
    switch (id) {
        case W_TRAFFIC_CLOSE: return L"Закрыть Modo Studio";
        case W_TRAFFIC_MIN: return L"Свернуть окно в панель задач";
        case W_TRAFFIC_ZOOM: return L"Развернуть / Восстановить окно";
        case W_BTN_MODO_MENU: return L"Главное меню Modo Studio (Alt / Alt+M / F1)";
        case W_BTN_THEME_DARK: return L"Включить тёмную монохромную тему оформления";
        case W_BTN_THEME_LIGHT: return L"Включить светлую монохромную тему оформления";
        case W_BTN_ADMIN: return L"Запуск от имени администратора для модификации системных ярлыков";
        case W_BTN_SHORTCUTS: return L"Создать ярлыки Modo Studio на Рабочем столе и в меню Пуск";
        case W_BTN_RESCAN: return L"Обновить список установленных приложений и кэш (F5)";
        case W_BTN_ADD_FILE: return L"Добавить стороннее приложение или изображение (Ctrl+N / Ctrl+O)";
        case W_TAB_ALL: return L"Показать все обнаруженные приложения";
        case W_TAB_APPLIED: return L"Показать только приложения со стилизованными иконками Modo";
        case W_TAB_SYSTEM: return L"Показать встроенные системные приложения Windows";
        case W_TAB_THIRDPARTY: return L"Показать сторонние пользовательские приложения";
        case W_SEARCH_BOX: return L"Поиск приложения по названию или пути (Ctrl+F)";
        case W_SEARCH_CLEAR: return L"Очистить строку поиска (Esc)";
        case W_BTN_APP_VIEW_GRID: return L"Галерея карточек с крупными значками (Ctrl+1)";
        case W_BTN_APP_VIEW_LIST: return L"Компактный детальный список приложений (Ctrl+2)";
        case W_SLIDER_GRID_SIZE: return L"Размер значков карточек в сетке: 48-96px (Ctrl++ / Ctrl+-)";
        case W_INSP_TAB_STYLE: return L"Вкладка: Стиль, форма подложки и цвет";
        case W_INSP_TAB_ARTWORK: return L"Вкладка: Рисунок значка, замена, градиенты и бейджи";
        case W_INSP_TAB_GEOMETRY: return L"Вкладка: Геометрия, масштаб и смещение значка";
        case W_INSP_TAB_OPTIONS: return L"Вкладка: Тени, автообрезка полей и монохром";
        case W_BTN_RESTART_EXPLORER: return L"Перезапустить Проводник Windows и сбросить кэш иконок";
        case W_BTN_ART_FILE: return L"Загрузить картинку или векторный значок из файла (.png, .svg, .ico, .jpg)";
        case W_BTN_ART_PASTE: return L"Вставить изображение рисунка из буфера обмена Windows";
        case W_BTN_ART_LUCIDE: return L"Выбрать открытый векторный значок из каталога Lucide";
        case W_BTN_ART_BRANDS: return L"Выбрать официальный векторный логотип из каталога брендов Simple Icons (65+ брендов)";
        case W_BTN_ART_AI_PATTERNS: return L"Выбрать процедурный геометрический AI-паттерн (Sparkle, Orbit, Hexacore...)";
        case W_BTN_ART_MONOGRAM: return L"Создать типографическую буквенную монограмму";
        case W_BTN_ART_RESET: return L"Вернуть исходный системный рисунок приложения";
        case W_BTN_ART_ROT_L: return L"Повернуть рисунок на 90 градусов против часовой стрелки";
        case W_BTN_ART_ROT_R: return L"Повернуть рисунок на 90 градусов по часовой стрелке";
        case W_BTN_ART_FLIP_H: return L"Отразить рисунок по горизонтали зеркально";
        case W_BTN_ART_FLIP_V: return L"Отразить рисунок по вертикали зеркально";
        case W_BTN_ART_RESET_TRANS: return L"Сбросить поворот, отражение и градиенты рисунка";
        case W_BTN_EYEDROPPER: return L"Экранная пипетка: захват цвета из любой точки экрана (Esc для отмены)";
        case W_BTN_PALETTE: return L"Выбор произвольного цвета фона в палитре Windows";
        case W_SWATCH_WIN_ACCENT: return L"Цвет акцента текущей темы Windows (Ctrl+Shift+A)";
        case W_SLIDER_SCALE: return L"Масштаб исходного значка внутри подложки";
        case W_SLIDER_OFFX: return L"Горизонтальное смещение значка влево/вправо";
        case W_SLIDER_OFFY: return L"Вертикальное смещение значка вверх/вниз";
        case W_BTN_CENTER: return L"Центрировать значок по осям X и Y";
        case W_SLIDER_SHADOW: return L"Интенсивность внешней тени подложки";
        case W_TOGGLE_AUTOTRIM: return L"Автоматическая обрезка пустых прозрачных полей значка";
        case W_TOGGLE_INNERSHAD: return L"Мягкая внутренняя тень по контуру формы";
        case W_TOGGLE_FULLBLEED: return L"Режим заполнения формы целиком без внутренних полей";
        case W_TOGGLE_MONO: return L"Преобразование цветного значка в монохромный трафарет";
        case W_BTN_APPLY_ONE: return L"Применить стилизованную иконку к выбранному приложению (Enter)";
        case W_BTN_APPLY_ALL: return L"Пакетно применить текущий стиль ко всем отфильтрованным (Shift+Enter)";
        case W_BTN_RESTORE_ONE: return L"Восстановить оригинальную иконку выбранного приложения (Delete)";
        case W_BTN_RESTORE_ALL: return L"Сбросить кастомные иконки для всех приложений";
        case W_BTN_EXPORT: return L"Меню экспорта: .ico, Web Favicon Kit, Xcode Catalog (Ctrl+E)";
        case W_SHAPE_0: return L"Форма подложки: Круг (macOS канон)";
        case W_SHAPE_1: return L"Форма подложки: Сквиркл (Суперэллипс iOS/macOS)";
        case W_SHAPE_2: return L"Форма подложки: Скруглённый прямоугольник (Windows)";
        case W_SHAPE_3: return L"Форма подложки: Галька (Pebble)";
        case W_SHAPE_4: return L"Форма подложки: Шестиугольник (Hexagon)";
        case W_TOGGLE_PER_APP: return L"Режим правки: индивидуальный стиль для выбранного приложения или общий для всех";
        case W_MATERIAL_0: return L"Материал: Гладкий плоский фон (Smooth Flat)";
        case W_MATERIAL_1: return L"Материал: Матовое стекло с бликом (VisionOS Frosted Glass)";
        case W_MATERIAL_2: return L"Материал: Шлифованный металл с микротекстурой (Brushed Titanium)";
        case W_MATERIAL_3: return L"Материал: Мягкий объёмный рельеф (Neumorphic Emboss)";
        case W_MATERIAL_4: return L"Материал: Голографическая дисперсия света (Prism Chromatic)";
        case W_MATERIAL_5: return L"Материал: Жидкий хром со сверкающими бликами (Liquid Chrome)";
        case W_MATERIAL_6: return L"Материал: Матовая керамика с мягкой диффузией (Matte Ceramic)";
        case W_MATERIAL_7: return L"Материал: Радужная голографическая фольга (Holographic Foil)";
        case W_GLYPH_0: return L"Рисунок значка: Оригинал (Original Artwork)";
        case W_GLYPH_1: return L"Рисунок значка: Силуэтная заливка (Silhouette Solid Fill)";
        case W_GLYPH_2: return L"Рисунок значка: Тактильный 3D рельеф (Embossed 3D)";
        case W_GLYPH_3: return L"Рисунок значка: Контурная обводка (Outline Contour)";
        case W_GLYPH_4: return L"Рисунок значка: Гармоничный дуотон (Duotone Harmony)";
        case W_GLYPH_5: return L"Рисунок значка: Неоновое свечение (Neon Halo Glow)";
        default:
            if (id >= W_PACK_0 && id <= W_PACK_18) {
                int p = id - W_PACK_0;
                int totalPacks = (int)(sizeof(kIconPacks) / sizeof(kIconPacks[0]));
                if (p < totalPacks) {
                    return kIconPacks[p].name;
                }
            }
            return nullptr;
    }
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
    Font fHero(&ff, 15.0f, FontStyleBold, UnitPixel);
    Font fLogo(&ff, 14.0f, FontStyleBold, UnitPixel);
    Font fSection(&ff, 11.0f, FontStyleBold, UnitPixel);
    Font fBodyBold(&ff, 12.0f, FontStyleBold, UnitPixel);
    Font fBody(&ff, 11.5f, FontStyleRegular, UnitPixel);
    Font fSmallBold(&ff, 10.5f, FontStyleBold, UnitPixel);
    Font fSmall(&ff, 10.5f, FontStyleRegular, UnitPixel);

    SolidBrush primBr(theme.textPrimary);
    SolidBrush secBr(theme.textSecondary);
    SolidBrush mutBr(theme.textMuted);
    Pen divPen(theme.divider, 1.0f);

    // 1. Full Canvas
    g.Clear(theme.canvas);

    // 2. Windows Top Header Bar (54px)
    SolidBrush topBarBr(theme.topBar);
    g.FillRectangle(&topBarBr, 0, 0, clientW, 54);
    g.DrawLine(&divPen, 0, 54, clientW, 54);

    // Modo Brand Mark + Title + Dropdown Menu
    RectF brandR(14.0f, 13.0f, 28.0f, 28.0f);
    DrawModoBrandMark(g, brandR, theme);

    g.DrawString(L"Modo Studio", -1, &fLogo, PointF(50.0f, 12.0f), NULL, &primBr);
    g.DrawString(L"Студия адаптивных иконок", -1, &fSmall, PointF(50.0f, 31.0f), NULL, &secBr);

    RectF menuR(224.0f, 13.0f, 72.0f, 28.0f);
    DrawStudioPillButton(g, W_BTN_MODO_MENU, menuR, L"Меню", fSmallBold, theme, false, false, false);

    bool compactHeader = (clientW < 1060);
    RectF expR(302.0f, 13.0f, compactHeader ? 108.0f : 168.0f, 28.0f);
    DrawStudioPillButton(g, W_BTN_RESTART_EXPLORER, expR, compactHeader ? L"Проводник" : L"Перезапуск Проводника", fSmallBold, theme, false, false, false, GLYPH_EXPLORER);

    // Top-right Toolbar Controls
    RectF themeR((REAL)(clientW - 556), 13.0f, 130.0f, 28.0f);
    DrawSegmentedThemeSwitch(g, themeR, g_isDarkMode, theme, fSmallBold);

    RectF adminR((REAL)(clientW - 420), 13.0f, 122.0f, 28.0f);
    DrawStudioPillButton(g, W_BTN_ADMIN, adminR, L"Администратор", fSmallBold, theme, false, false, false, GLYPH_ADMIN);

    RectF scR((REAL)(clientW - 292), 13.0f, 92.0f, 28.0f);
    DrawStudioPillButton(g, W_BTN_SHORTCUTS, scR, L"Ярлыки", fSmallBold, theme, false, false, false, GLYPH_SHORTCUTS);

    RectF rescanR((REAL)(clientW - 194), 13.0f, 92.0f, 28.0f);
    DrawStudioPillButton(g, W_BTN_RESCAN, rescanR, L"Обновить", fSmallBold, theme, false, false, false, GLYPH_REFRESH);

    RectF addR((REAL)(clientW - 96), 13.0f, 84.0f, 28.0f);
    DrawStudioPillButton(g, W_BTN_ADD_FILE, addR, L"+ Файл", fSmallBold, theme, false, false, false, GLYPH_ADD_FILE);

    // 3-COLUMN ARCHITECTURE GEOMETRY
    int sideW = 210;
    int inspW = 360;
    int galX = sideW;
    int galW = std::max(200, clientW - sideW - inspW);
    int inspX = clientW - inspW;
    int contentY = 54;
    int contentH = clientH - 54 - 32;

    // =========================================================================
    // COLUMN 1: LEFT NAVIGATION SIDEBAR (Width: 210px)
    // =========================================================================
    SolidBrush sideBr(theme.sidebar);
    g.FillRectangle(&sideBr, 0, contentY, sideW, contentH);
    g.DrawLine(&divPen, sideW, contentY, sideW, contentY + contentH);

    int totalApps = (int)g_allApps.size();
    int appliedTotal = 0;
    int sysTotal = 0;
    int thirdTotal = 0;
    for (const auto& a : g_allApps) {
        if (a.isApplied) appliedTotal++;
        if (a.isFileExplorer || a.isRecycleBin || !a.systemClsid.empty() || a.category.find(L"Windows") != std::wstring::npos) {
            sysTotal++;
        } else {
            thirdTotal++;
        }
    }

    // Section 1: Library
    g.DrawString(L"БИБЛИОТЕКА", -1, &fSection, PointF(16.0f, (REAL)(contentY + 12)), NULL, &mutBr);

    struct SideNavItem {
        WidgetID id;
        const wchar_t* title;
        int count;
        int filterId;
    } libNav[4] = {
        { W_TAB_ALL,         L"Все программы",  totalApps,   0 },
        { W_TAB_APPLIED,     L"Стилизованные",  appliedTotal, 1 },
        { W_TAB_SYSTEM,      L"Системные",      sysTotal,    2 },
        { W_TAB_THIRDPARTY,  L"Сторонние",      thirdTotal,  3 }
    };

    REAL navY = (REAL)(contentY + 30);
    for (int i = 0; i < 4; ++i) {
        RectF r(8.0f, navY + i * 32.0f, (REAL)(sideW - 16), 28.0f);
        RegisterHit(libNav[i].id, r);
        bool isSel = (g_sidebarFilter == libNav[i].filterId);
        bool isHov = (g_hoveredWidget == libNav[i].id);

        if (isSel) {
            FillRoundRect(g, r, 6.0f, theme.selRowBg);
            DrawRoundRect(g, r, 6.0f, theme.selRowBorder, 1.0f);
            RectF bar(r.X + 2.0f, r.Y + 6.0f, 3.0f, r.Height - 12.0f);
            FillRoundRect(g, bar, 1.5f, theme.isDark ? Color(255, 255, 255, 255) : Color(255, 0, 122, 255));
        } else if (isHov) {
            FillRoundRect(g, r, 6.0f, theme.hovRowBg);
        }

        StringFormat sfL; sfL.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(libNav[i].title, -1, isSel ? &fBodyBold : &fBody, RectF(r.X + 12.0f, r.Y, r.Width - 54.0f, r.Height), &sfL, isSel ? &primBr : &secBr);

        wchar_t cbuf[32];
        wsprintfW(cbuf, L"%d", libNav[i].count);
        StringFormat sfR; sfR.SetAlignment(StringAlignmentFar); sfR.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(cbuf, -1, &fSmallBold, RectF(r.X, r.Y, r.Width - 10.0f, r.Height), &sfR, &mutBr);
    }

    // Section 2: Icon Packs & Presets
    REAL packsHeaderY = navY + 4 * 32.0f + 12.0f;
    g.DrawLine(&divPen, 16, (INT)packsHeaderY, sideW - 16, (INT)packsHeaderY);
    g.DrawString(L"ПАКИ И ТЕМЫ", -1, &fSection, PointF(16.0f, packsHeaderY + 10.0f), NULL, &mutBr);

    int totalPacks = (int)(sizeof(kIconPacks) / sizeof(kIconPacks[0]));
    REAL packAreaY = packsHeaderY + 28.0f;
    REAL packAreaH = (REAL)(contentH - (packAreaY - contentY) - 10.0f);
    RectF packScrollArea(0.0f, packAreaY, (REAL)sideW, packAreaH);

    REAL totalPacksH = totalPacks * 30.0f + 10.0f;
    int maxSidebarScroll = std::max(0, (int)(totalPacksH - packAreaH));
    g_sidebarScrollY = std::clamp(g_sidebarScrollY, 0, maxSidebarScroll);

    GraphicsState sideState = g.Save();
    g.SetClip(packScrollArea);

    for (int p = 0; p < totalPacks; ++p) {
        REAL py = packAreaY + p * 30.0f - g_sidebarScrollY;
        if (py + 26.0f < packAreaY || py > packAreaY + packAreaH) continue;

        RectF pr(8.0f, py, (REAL)(sideW - 16), 26.0f);
        RegisterHit((WidgetID)(W_PACK_0 + p), pr);
        bool isAct = (g_activePack == p);
        bool isHov = (g_hoveredWidget == (WidgetID)(W_PACK_0 + p));

        if (isAct) {
            FillRoundRect(g, pr, 6.0f, theme.selRowBg);
            DrawRoundRect(g, pr, 6.0f, theme.selRowBorder, 1.0f);
        } else if (isHov) {
            FillRoundRect(g, pr, 6.0f, theme.hovRowBg);
        }

        // Color dot: derived from pack preset / custom color
        COLORREF dotColor = kIconPacks[p].customColor;
        if (kIconPacks[p].preset == PRESET_GOOGLE_WHITE) dotColor = RGB(66, 133, 244);
        else if (kIconPacks[p].preset == PRESET_MATERIAL_PASTEL) dotColor = RGB(136, 192, 208);
        else if (kIconPacks[p].preset == PRESET_DARK_MATERIAL) dotColor = RGB(56, 189, 248);
        Color dc(255, GetRValue(dotColor), GetGValue(dotColor), GetBValue(dotColor));
        SolidBrush db(dc);
        g.FillEllipse(&db, pr.X + 10.0f, pr.Y + 9.0f, 8.0f, 8.0f);

        StringFormat sfP; sfP.SetLineAlignment(StringAlignmentCenter);
        g.DrawString(kIconPacks[p].name, -1, isAct ? &fBodyBold : &fBody, RectF(pr.X + 24.0f, pr.Y, pr.Width - 50.0f, pr.Height), &sfP, isAct ? &primBr : &secBr);

        if (isAct) {
            RectF cr(pr.GetRight() - 18.0f, pr.Y + 7.0f, 12.0f, 12.0f);
            DrawGlyphIcon(g, GLYPH_CHECKMARK, cr, theme.isDark ? Color(255, 52, 199, 89) : Color(255, 0, 122, 255));
        }
    }

    if (maxSidebarScroll > 0) {
        REAL trackH = packAreaH;
        REAL thumbH = std::max(24.0f, trackH * (trackH / totalPacksH));
        REAL thumbY = packAreaY + (trackH - thumbH) * ((REAL)g_sidebarScrollY / (REAL)maxSidebarScroll);
        FillRoundRect(g, RectF(sideW - 5.0f, thumbY, 3.0f, thumbH), 1.5f, theme.isDark ? Color(120, 110, 118, 135) : Color(120, 160, 165, 175));
    }

    g.Restore(sideState);

    // =========================================================================
    // COLUMN 2: CENTER GALLERY / LIST (Flexible Width)
    // =========================================================================
    g.DrawLine(&divPen, inspX, contentY, inspX, contentY + contentH);

    // Gallery Top Bar (Height: 46px)
    int galTopH = 46;
    RectF galHeaderR((REAL)galX, (REAL)contentY, (REAL)galW, (REAL)galTopH);
    g.DrawLine(&divPen, galX, contentY + galTopH, galX + galW, contentY + galTopH);

    // Active Category Title
    const wchar_t* filterTitle = (g_sidebarFilter == 1) ? L"Стилизованные иконки"
                               : (g_sidebarFilter == 2) ? L"Системные программы"
                               : (g_sidebarFilter == 3) ? L"Сторонние приложения"
                               : L"Все приложения";
    g.DrawString(filterTitle, -1, &fBodyBold, PointF((REAL)(galX + 16), (REAL)(contentY + 14)), NULL, &primBr);

    wchar_t cntSub[64];
    wsprintfW(cntSub, L"%d шт.", (int)g_filteredIndices.size());
    g.DrawString(cntSub, -1, &fSmall, PointF((REAL)(galX + 16 + (int)wcslen(filterTitle) * 7 + 10), (REAL)(contentY + 16)), NULL, &mutBr);

    // Search Field (X: galX + galW - 270 .. galX + galW - 110)
    REAL sW = 150.0f;
    RectF searchRect((REAL)(galX + galW - 270), (REAL)(contentY + 9), sW, 28.0f);
    RegisterHit(W_SEARCH_BOX, searchRect);
    FillRoundRect(g, searchRect, 6.0f, theme.wellBg);
    DrawRoundRect(g, searchRect, 6.0f, g_searchFocused ? theme.textPrimary : theme.controlBorder, 1.0f);

    RectF sLoupe(searchRect.X + 7.0f, searchRect.Y + 8.0f, 12.0f, 12.0f);
    Pen lPen(theme.textMuted, 1.3f);
    g.DrawEllipse(&lPen, sLoupe.X, sLoupe.Y, 8.0f, 8.0f);
    g.DrawLine(&lPen, sLoupe.X + 6.0f, sLoupe.Y + 6.0f, sLoupe.GetRight(), sLoupe.GetBottom());

    StringFormat sfSCenter; sfSCenter.SetLineAlignment(StringAlignmentCenter); sfSCenter.SetFormatFlags(StringFormatFlagsNoWrap);
    if (g_searchText.empty()) {
        g.DrawString(L"Поиск...", -1, &fSmall, RectF(searchRect.X + 24.0f, searchRect.Y, searchRect.Width - 30.0f, searchRect.Height), &sfSCenter, &mutBr);
    } else {
        std::wstring disp = g_searchText + (g_searchFocused ? L"|" : L"");
        g.DrawString(disp.c_str(), -1, &fSmallBold, RectF(searchRect.X + 24.0f, searchRect.Y, searchRect.Width - 44.0f, searchRect.Height), &sfSCenter, &primBr);
        RectF clearRect(searchRect.GetRight() - 22.0f, searchRect.Y + 4.0f, 20.0f, 20.0f);
        RegisterHit(W_SEARCH_CLEAR, clearRect);
        Pen xp(theme.textMuted, 1.3f);
        g.DrawLine(&xp, clearRect.X + 5.0f, clearRect.Y + 5.0f, clearRect.GetRight() - 5.0f, clearRect.GetBottom() - 5.0f);
        g.DrawLine(&xp, clearRect.GetRight() - 5.0f, clearRect.Y + 5.0f, clearRect.X + 5.0f, clearRect.GetBottom() - 5.0f);
    }

    // Segmented View Mode Toggle: [ ⊞ Сетка | ☰ Список ] (X: galX + galW - 106, W: 96)
    RectF viewSwitchR((REAL)(galX + galW - 106), (REAL)(contentY + 9), 96.0f, 28.0f);
    FillRoundRect(g, viewSwitchR, 6.0f, theme.controlBg);
    DrawRoundRect(g, viewSwitchR, 6.0f, theme.controlBorder, 1.0f);

    REAL segW = (viewSwitchR.Width - 4.0f) * 0.5f;
    RectF gridSegR(viewSwitchR.X + 2.0f, viewSwitchR.Y + 2.0f, segW, viewSwitchR.Height - 4.0f);
    RectF listSegR(viewSwitchR.X + 2.0f + segW, viewSwitchR.Y + 2.0f, segW, viewSwitchR.Height - 4.0f);

    RegisterHit(W_BTN_APP_VIEW_GRID, gridSegR);
    RegisterHit(W_BTN_APP_VIEW_LIST, listSegR);

    if (g_viewMode == APP_VIEW_GRID) {
        FillRoundRect(g, gridSegR, 5.0f, theme.isDark ? Color(255, 68, 68, 74) : Color(255, 216, 216, 224));
        DrawRoundRect(g, gridSegR, 5.0f, theme.isDark ? Color(255, 120, 120, 128) : Color(255, 160, 160, 170), 1.0f);
    } else {
        FillRoundRect(g, listSegR, 5.0f, theme.isDark ? Color(255, 68, 68, 74) : Color(255, 216, 216, 224));
        DrawRoundRect(g, listSegR, 5.0f, theme.isDark ? Color(255, 120, 120, 128) : Color(255, 160, 160, 170), 1.0f);
    }

    StringFormat sfC; sfC.SetAlignment(StringAlignmentCenter); sfC.SetLineAlignment(StringAlignmentCenter);
    g.DrawString(L"Сетка", -1, &fSmallBold, gridSegR, &sfC, (g_viewMode == APP_VIEW_GRID) ? &primBr : &secBr);
    g.DrawString(L"Список", -1, &fSmallBold, listSegR, &sfC, (g_viewMode == APP_VIEW_LIST) ? &primBr : &secBr);

    // Gallery Main Content (Grid vs List)
    int galBodyY = contentY + galTopH;
    int galBodyH = contentH - galTopH - 26; // reserve 26px for bottom hint
    RectF galArea((REAL)galX, (REAL)galBodyY, (REAL)galW, (REAL)galBodyH);

    GraphicsState galState = g.Save();
    g.SetClip(galArea);

    if (g_viewMode == APP_VIEW_GRID) {
        // --- VIEW MODE A: GRID GALLERY (APP STORE STYLE) ---
        RegisterHit(W_GRID_AREA, galArea);

        int iconSize = g_gridIconSize;
        int cardW = iconSize + 48;
        int cardH = iconSize + 58;
        int gapX = 10;
        int gapY = 10;
        int numCols = std::max(1, (galW - 20) / (cardW + gapX));
        int totalRows = ((int)g_filteredIndices.size() + numCols - 1) / numCols;
        int totalGridH = totalRows * (cardH + gapY) + 20;

        int maxGridScroll = std::max(0, totalGridH - galBodyH);
        g_gridScrollY = std::clamp(g_gridScrollY, 0, maxGridScroll);

        for (size_t i = 0; i < g_filteredIndices.size(); ++i) {
            int appIdx = g_filteredIndices[i];
            int col = (int)i % numCols;
            int row = (int)i / numCols;

            REAL cx = galX + 10.0f + col * (cardW + gapX);
            REAL cy = galBodyY + 10.0f + row * (cardH + gapY) - g_gridScrollY;

            if (cy + cardH < galBodyY || cy > galBodyY + galBodyH) continue;

            RectF cardR(cx, cy, (REAL)cardW, (REAL)cardH);
            bool isSel = (appIdx == g_selectedAppIndex);
            bool isHov = ((int)i == g_hoveredListRow);

            if (isSel) {
                FillRoundRect(g, cardR, 8.0f, theme.selRowBg);
                DrawRoundRect(g, cardR, 8.0f, theme.selRowBorder, 1.5f);
            } else if (isHov) {
                FillRoundRect(g, cardR, 8.0f, theme.hovRowBg);
                DrawRoundRect(g, cardR, 8.0f, theme.cardBorder, 1.0f);
            } else {
                FillRoundRect(g, cardR, 8.0f, theme.cardBg);
                DrawRoundRect(g, cardR, 8.0f, theme.cardBorder, 1.0f);
            }

            AppEntry& app = g_allApps[appIdx];
            if (Bitmap* thumb = GetAppThumb(app)) {
                RectF iconR(cx + (cardW - iconSize) * 0.5f, cy + 8.0f, (REAL)iconSize, (REAL)iconSize);
                g.DrawImage(thumb, iconR);
            }

            // Title
            RectF titleR(cx + 6.0f, cy + 8.0f + iconSize + 4.0f, (REAL)(cardW - 12), 16.0f);
            StringFormat sfTrim;
            sfTrim.SetAlignment(StringAlignmentCenter);
            sfTrim.SetTrimming(StringTrimmingEllipsisCharacter);
            sfTrim.SetFormatFlags(StringFormatFlagsNoWrap);
            g.DrawString(app.name.c_str(), -1, isSel ? &fBodyBold : &fBody, titleR, &sfTrim, isSel ? &primBr : &secBr);

            // Badge
            if (app.isApplied) {
                RectF badgeR(cx + (cardW - 46) * 0.5f, cy + 8.0f + iconSize + 22.0f, 46.0f, 15.0f);
                FillRoundRect(g, badgeR, 7.5f, theme.badgeBg);
                SolidBrush bText(theme.badgeText);
                StringFormat sfB; sfB.SetAlignment(StringAlignmentCenter); sfB.SetLineAlignment(StringAlignmentCenter);
                g.DrawString(L"Modo", -1, &fSmallBold, badgeR, &sfB, &bText);
            }
        }

        if (maxGridScroll > 0) {
            REAL trackH = (REAL)galBodyH;
            REAL thumbH = std::max(28.0f, trackH * (trackH / (REAL)totalGridH));
            REAL thumbY = (REAL)galBodyY + (trackH - thumbH) * ((REAL)g_gridScrollY / (REAL)maxGridScroll);
            FillRoundRect(g, RectF(galX + galW - 5.0f, thumbY, 4.0f, thumbH), 2.0f, theme.isDark ? Color(140, 110, 118, 135) : Color(140, 160, 165, 175));
        }

    } else {
        // --- VIEW MODE B: COMPACT LIST (MAC HIG STYLE) ---
        RegisterHit(W_APP_LIST_AREA, galArea);

        const int rowH = 48;
        int totalRowsH = (int)g_filteredIndices.size() * rowH;
        int maxListScroll = std::max(0, totalRowsH - galBodyH);
        g_listScrollY = std::clamp(g_listScrollY, 0, maxListScroll);

        for (size_t i = 0; i < g_filteredIndices.size(); ++i) {
            int appIdx = g_filteredIndices[i];
            REAL ry = galBodyY + (REAL)(i * rowH - g_listScrollY);
            if (ry + rowH < galBodyY || ry > galBodyY + galBodyH) continue;

            RectF rowRect((REAL)(galX + 8), ry + 2.0f, (REAL)(galW - 16), (REAL)(rowH - 4));
            bool isSel = (appIdx == g_selectedAppIndex);
            bool isHov = ((int)i == g_hoveredListRow);

            if (isSel) {
                FillRoundRect(g, rowRect, 6.0f, theme.selRowBg);
                DrawRoundRect(g, rowRect, 6.0f, theme.selRowBorder, 1.0f);
                RectF bar(rowRect.X + 2.0f, rowRect.Y + 8.0f, 3.0f, rowRect.Height - 16.0f);
                FillRoundRect(g, bar, 1.5f, theme.isDark ? Color(255, 255, 255, 255) : Color(255, 0, 122, 255));
            } else if (isHov) {
                FillRoundRect(g, rowRect, 6.0f, theme.hovRowBg);
            }

            AppEntry& app = g_allApps[appIdx];
            if (Bitmap* thumb = GetAppThumb(app)) {
                g.DrawImage(thumb, RectF(rowRect.X + 10.0f, rowRect.Y + 5.0f, 34.0f, 34.0f));
            }

            REAL textMaxW = app.isApplied ? (rowRect.Width - 110.0f) : (rowRect.Width - 54.0f);
            RectF titleR(rowRect.X + 52.0f, rowRect.Y + 5.0f, textMaxW, 16.0f);
            RectF subR(rowRect.X + 52.0f, rowRect.Y + 22.0f, textMaxW, 14.0f);
            StringFormat sfTrim;
            sfTrim.SetTrimming(StringTrimmingEllipsisCharacter);
            sfTrim.SetFormatFlags(StringFormatFlagsNoWrap);

            g.DrawString(app.name.c_str(), -1, &fBodyBold, titleR, &sfTrim, &primBr);

            wchar_t subBuf[128];
            if (app.isFolder) {
                wsprintfW(subBuf, L"Папка · desktop.ini");
            } else {
                wsprintfW(subBuf, L"%s · %u ярл.", app.category.c_str(), (unsigned)std::max<size_t>(1, app.allLnkPaths.size()));
            }
            g.DrawString(subBuf, -1, &fSmall, subR, &sfTrim, &mutBr);

            if (app.isApplied) {
                RectF badgeR(rowRect.GetRight() - 56.0f, rowRect.Y + 12.0f, 48.0f, 18.0f);
                FillRoundRect(g, badgeR, 9.0f, theme.badgeBg);
                SolidBrush bText(theme.badgeText);
                StringFormat sfB; sfB.SetAlignment(StringAlignmentCenter); sfB.SetLineAlignment(StringAlignmentCenter);
                g.DrawString(L"Modo", -1, &fSmallBold, badgeR, &sfB, &bText);
            }
        }

        if (maxListScroll > 0) {
            REAL trackH = (REAL)galBodyH;
            REAL thumbH = std::max(28.0f, trackH * (trackH / (REAL)totalRowsH));
            REAL thumbY = (REAL)galBodyY + (trackH - thumbH) * ((REAL)g_listScrollY / (REAL)maxListScroll);
            FillRoundRect(g, RectF(galX + galW - 5.0f, thumbY, 4.0f, thumbH), 2.0f, theme.isDark ? Color(140, 110, 118, 135) : Color(140, 160, 165, 175));
        }
    }

    g.Restore(galState);

    // Gallery Footer Sub-bar (Height: 26px)
    g.DrawLine(&divPen, galX, contentY + contentH - 26, galX + galW, contentY + contentH - 26);
    wchar_t galFoot[128];
    wsprintfW(galFoot, L"%d приложений · %d стилизовано", totalApps, appliedTotal);
    g.DrawString(galFoot, -1, &fSmall, PointF((REAL)(galX + 16), (REAL)(contentY + contentH - 20)), NULL, &mutBr);

    if (g_viewMode == APP_VIEW_GRID) {
        // macOS-style Grid Icon Size Slider in footer (48px .. 96px)
        REAL sWidth = 84.0f;
        REAL sRight = (REAL)(galX + galW - 54);
        REAL sLeft = sRight - sWidth;
        REAL sY = (REAL)(contentY + contentH - 22);

        // Small dot indicator on left
        SolidBrush dotBr(theme.textMuted);
        g.FillEllipse(&dotBr, sLeft - 12.0f, sY + 6.0f, 4.0f, 4.0f);

        // Slider
        double normGridSize = (double)(g_gridIconSize - 48) / (96.0 - 48.0);
        DrawStudioSlider(g, W_SLIDER_GRID_SIZE, RectF(sLeft, sY, sWidth, 18.0f), normGridSize, theme);

        // Large dot indicator on right
        g.FillEllipse(&dotBr, sRight + 6.0f, sY + 4.5f, 7.0f, 7.0f);

        // Pixel label: "56px"
        wchar_t szSize[32];
        wsprintfW(szSize, L"%dpx", g_gridIconSize);
        g.DrawString(szSize, -1, &fSmall, PointF(sRight + 18.0f, (REAL)(contentY + contentH - 20)), NULL, &secBr);
    }

    // =========================================================================
    // COLUMN 3: RIGHT FOCUSED INSPECTOR (Width: 360px)
    // =========================================================================
    SolidBrush inspBg(theme.cardBg);
    g.FillRectangle(&inspBg, inspX, contentY, inspW, contentH);

    // 1. Hero Preview Stage (Height: 204px)
    int heroH = 204;
    RectF heroR((REAL)inspX, (REAL)contentY, (REAL)inspW, (REAL)heroH);
    g.DrawLine(&divPen, inspX, contentY + heroH, inspX + inspW, contentY + heroH);

    const RoundStyle& curSt = CurrentInspectorStyle();
    bool hasSelApp = (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size());

    std::wstring selTitle = hasSelApp
        ? g_allApps[g_selectedAppIndex].name
        : L"Приложение не выбрано";

    RectF titleArea((REAL)(inspX + 16), (REAL)(contentY + 12), (REAL)(inspW - 146), 20.0f);
    StringFormat sfHT; sfHT.SetTrimming(StringTrimmingEllipsisCharacter); sfHT.SetFormatFlags(StringFormatFlagsNoWrap);
    g.DrawString(selTitle.c_str(), -1, &fHero, titleArea, &sfHT, &primBr);

    bool perAppBadgeOn = g_adjustThisAppOnly || (hasSelApp && g_allApps[g_selectedAppIndex].hasCustomStyle);
    DrawStudioPillButton(g, W_TOGGLE_PER_APP,
                         RectF((REAL)(inspX + inspW - 122), (REAL)(contentY + 10), 106.0f, 24.0f),
                         g_adjustThisAppOnly ? L"Индив. стиль" : L"Общий стиль",
                         fSmallBold, theme, false, perAppBadgeOn, false);

    // Telemetry
    wchar_t teleBuf[128];
    wsprintfW(teleBuf, L"Масштаб: %d%% · X: %+d%% · Y: %+d%%",
              (int)std::round(curSt.scale * 100.0),
              (int)std::round(curSt.offsetX * 100.0),
              (int)std::round(curSt.offsetY * 100.0));
    g.DrawString(teleBuf, -1, &fSmall, PointF((REAL)(inspX + 16), (REAL)(contentY + 34)), NULL, &secBr);

    // Interactive Stage Podiums
    REAL stagePodiumY = (REAL)(contentY + 54);
    REAL podiumSize = 98.0f;
    REAL leftPodiumX = (REAL)(inspX + 32);
    REAL rightPodiumX = (REAL)(inspX + inspW - 32 - podiumSize);

    auto drawLuxuryPodium = [&](REAL px, REAL py, REAL pw, REAL ph) {
        RectF pr(px, py, pw, ph);
        RectF shR(px - 2.0f, py + 3.0f, pw + 4.0f, ph + 4.0f);
        FillRoundRect(g, shR, 12.0f, theme.isDark ? Color(50, 0, 0, 0) : Color(25, 0, 0, 0));
        FillRoundRect(g, pr, 10.0f, theme.wellBg);
        DrawRoundRect(g, pr, 10.0f, theme.wellBorder, 1.0f);
    };

    drawLuxuryPodium(leftPodiumX, stagePodiumY, podiumSize, podiumSize);
    drawLuxuryPodium(rightPodiumX, stagePodiumY, podiumSize, podiumSize);

    RectF roundStageRect(rightPodiumX, stagePodiumY, podiumSize, podiumSize);
    RegisterHit(W_STAGE_ROUND_DRAG, roundStageRect);

    if (hasSelApp) {
        AppEntry& app = g_allApps[g_selectedAppIndex];
        Bitmap* art = GetEffectiveArtwork(app);
        if (art) {
            g.DrawImage(art, RectF(leftPodiumX + 10.0f, stagePodiumY + 10.0f, podiumSize - 20.0f, podiumSize - 20.0f));
            if (!g_cachedHeroPreview || g_cachedHeroAppIdx != g_selectedAppIndex ||
                g_cachedHeroArtPtr != art || g_cachedHeroStyle != curSt) {
                delete g_cachedHeroPreview;
                g_cachedHeroPreview = RenderGoogleRoundIcon(art, curSt, 256);
                g_cachedHeroAppIdx = g_selectedAppIndex;
                g_cachedHeroArtPtr = art;
                g_cachedHeroStyle = curSt;
            }
            Bitmap* hiRound = g_cachedHeroPreview;
            if (hiRound) {
                float tx = g_heroTiltX;
                float ty = g_heroTiltY;
                bool isTilted = (g_heroTiltHovered || g_isDraggingStage || std::abs(tx) > 0.005f || std::abs(ty) > 0.005f);
                float tiltMag = std::min(1.0f, std::sqrt(tx * tx + ty * ty));

                // Center of the stage icon
                REAL cx = rightPodiumX + podiumSize * 0.5f;
                REAL cy = stagePodiumY + podiumSize * 0.5f;

                // On hover/tilt, icon lifts towards viewer (scale up by ~3.5%)
                REAL scale = isTilted ? (1.0f + 0.035f * tiltMag) : 1.0f;
                REAL baseSize = podiumSize - 10.0f; // 88px
                REAL curSize = baseSize * scale;
                REAL halfW = curSize * 0.5f;
                REAL halfH = curSize * 0.5f;

                // Parallax translation offset: shifts slightly in direction of tilt
                REAL shiftX = tx * 4.5f;
                REAL shiftY = ty * 4.5f;
                REAL finalCX = cx + shiftX;
                REAL finalCY = cy + shiftY;

                // 1. Dynamic Parallax Drop Shadow: projects opposite to tilt and spreads when lifted
                REAL shOffX = -tx * 7.5f;
                REAL shOffY = 4.0f - ty * 7.5f;
                REAL shSpread = isTilted ? 4.0f * tiltMag : 0.0f;
                RectF shadowRect(finalCX - halfW + shOffX - shSpread * 0.5f,
                                 finalCY - halfH + shOffY - shSpread * 0.5f,
                                 curSize + shSpread,
                                 curSize + shSpread);
                BYTE shAlpha = (BYTE)(theme.isDark ? (isTilted ? 75 : 60) : (isTilted ? 45 : 30));
                GraphicsPath shPath;
                IconEngine::AddShapePath(shPath, curSt.shape, shadowRect);
                SolidBrush shBr(Color(shAlpha, 0, 0, 0));
                g.FillPath(&shBr, &shPath);

                // 2. 3D Perspective Projection Quad (3 parallelogram points for affine transformation)
                REAL shearX = ty * 3.5f;
                REAL shearY = tx * 3.5f;
                REAL compX = 1.0f - std::abs(tx) * 0.045f;
                REAL compY = 1.0f - std::abs(ty) * 0.045f;

                PointF pts[3] = {
                    PointF(finalCX - halfW * compX - shearX, finalCY - halfH * compY - shearY), // Top-Left
                    PointF(finalCX + halfW * compX - shearX, finalCY - halfH * compY + shearY), // Top-Right
                    PointF(finalCX - halfW * compX + shearX, finalCY + halfH * compY - shearY)  // Bottom-Left
                };

                g.DrawImage(hiRound, pts, 3);

                // 3. Dynamic Specular Lighting Glare & Glint
                if (isTilted && tiltMag > 0.02f) {
                    // Plate shape clipping path matching the tilted icon
                    RectF plateBounds(finalCX - halfW, finalCY - halfH, curSize, curSize);
                    GraphicsPath plateClip;
                    IconEngine::AddShapePath(plateClip, curSt.shape, plateBounds);

                    g.SetClip(&plateClip, CombineModeReplace);

                    // Dynamic specular glare shifted opposite to the tilt vector (light source simulation)
                    REAL glareCX = finalCX - tx * halfW * 0.55f;
                    REAL glareCY = finalCY - ty * halfH * 0.55f;
                    REAL glareRad = curSize * 0.75f;

                    GraphicsPath glareEll;
                    glareEll.AddEllipse(glareCX - glareRad, glareCY - glareRad, glareRad * 2.0f, glareRad * 2.0f);
                    PathGradientBrush glareBr(&glareEll);
                    BYTE glareAlpha = (BYTE)(theme.isDark ? (65 + 40 * tiltMag) : (45 + 30 * tiltMag));
                    Color centerCol(glareAlpha, 255, 255, 255);
                    Color surroundCol(0, 255, 255, 255);
                    int surroundCnt = 1;
                    glareBr.SetCenterColor(centerCol);
                    glareBr.SetSurroundColors(&surroundCol, &surroundCnt);
                    g.FillPath(&glareBr, &glareEll);

                    // Subtle crest edge highlight (rim reflection)
                    PointF crestStart(finalCX - tx * halfW, finalCY - ty * halfH);
                    PointF crestEnd(finalCX + tx * halfW, finalCY + ty * halfH);
                    LinearGradientBrush rimBr(crestStart, crestEnd, Color((BYTE)(95 * tiltMag), 255, 255, 255), Color(0, 255, 255, 255));
                    Pen rimPen(&rimBr, 1.25f);
                    g.DrawPath(&rimPen, &plateClip);

                    g.ResetClip();
                }
            }
        }
    }

    g.DrawString(L"Оригинал", -1, &fSmallBold, PointF(leftPodiumX + podiumSize * 0.5f, stagePodiumY + podiumSize + 6.0f), &sfC, &mutBr);
    g.DrawString(L"Адаптивный результат", -1, &fSmallBold, PointF(rightPodiumX + podiumSize * 0.5f, stagePodiumY + podiumSize + 6.0f), &sfC, &secBr);

    // Connector Arrow (pure GDI+ vector chevron, zero broken font glyphs)
    REAL midConnX = inspX + inspW * 0.5f;
    FillRoundRect(g, RectF(midConnX - 24.0f, stagePodiumY + 34.0f, 48.0f, 26.0f), 13.0f, theme.controlBg);
    DrawRoundRect(g, RectF(midConnX - 24.0f, stagePodiumY + 34.0f, 48.0f, 26.0f), 13.0f, theme.controlBorder, 1.0f);
    {
        Pen arrPen(theme.textPrimary, 1.6f);
        arrPen.SetStartCap(LineCapRound);
        arrPen.SetEndCap(LineCapRound);
        REAL ay = stagePodiumY + 47.0f;
        g.DrawLine(&arrPen, midConnX - 8.0f, ay, midConnX + 8.0f, ay);
        g.DrawLine(&arrPen, midConnX + 3.0f, ay - 4.5f, midConnX + 8.0f, ay);
        g.DrawLine(&arrPen, midConnX + 3.0f, ay + 4.5f, midConnX + 8.0f, ay);
    }

    // 2. Segmented Inspector Tabs (Height: 30px)
    REAL inspTabY = (REAL)(contentY + heroH + 10);
    RectF tabSwitchR((REAL)(inspX + 16), inspTabY, (REAL)(inspW - 32), 28.0f);
    FillRoundRect(g, tabSwitchR, 6.0f, theme.controlBg);
    DrawRoundRect(g, tabSwitchR, 6.0f, theme.controlBorder, 1.0f);

    REAL iSegW = (tabSwitchR.Width - 8.0f) / 4.0f;
    RectF tabStyleR(tabSwitchR.X + 2.0f, tabSwitchR.Y + 2.0f, iSegW, tabSwitchR.Height - 4.0f);
    RectF tabArtR(tabSwitchR.X + 3.0f + iSegW, tabSwitchR.Y + 2.0f, iSegW, tabSwitchR.Height - 4.0f);
    RectF tabGeoR(tabSwitchR.X + 4.0f + iSegW * 2.0f, tabSwitchR.Y + 2.0f, iSegW, tabSwitchR.Height - 4.0f);
    RectF tabOptR(tabSwitchR.X + 5.0f + iSegW * 3.0f, tabSwitchR.Y + 2.0f, iSegW, tabSwitchR.Height - 4.0f);

    RegisterHit(W_INSP_TAB_STYLE, tabStyleR);
    RegisterHit(W_INSP_TAB_ARTWORK, tabArtR);
    RegisterHit(W_INSP_TAB_GEOMETRY, tabGeoR);
    RegisterHit(W_INSP_TAB_OPTIONS, tabOptR);

    auto drawActiveTab = [&](RectF tr) {
        FillRoundRect(g, tr, 5.0f, theme.isDark ? Color(255, 68, 68, 74) : Color(255, 216, 216, 224));
        DrawRoundRect(g, tr, 5.0f, theme.isDark ? Color(255, 120, 120, 128) : Color(255, 160, 160, 170), 1.0f);
    };

    if (g_inspectorTab == INSP_TAB_STYLE) drawActiveTab(tabStyleR);
    else if (g_inspectorTab == INSP_TAB_ARTWORK) drawActiveTab(tabArtR);
    else if (g_inspectorTab == INSP_TAB_GEOMETRY) drawActiveTab(tabGeoR);
    else drawActiveTab(tabOptR);

    g.DrawString(L"Стиль", -1, &fSmallBold, tabStyleR, &sfC, (g_inspectorTab == INSP_TAB_STYLE) ? &primBr : &secBr);
    g.DrawString(L"Рисунок", -1, &fSmallBold, tabArtR, &sfC, (g_inspectorTab == INSP_TAB_ARTWORK) ? &primBr : &secBr);
    g.DrawString(L"Геом.", -1, &fSmallBold, tabGeoR, &sfC, (g_inspectorTab == INSP_TAB_GEOMETRY) ? &primBr : &secBr);
    g.DrawString(L"Опции", -1, &fSmallBold, tabOptR, &sfC, (g_inspectorTab == INSP_TAB_OPTIONS) ? &primBr : &secBr);

    // 3. Scrollable Tab Content Area (clipped strictly above Sticky Bottom Action Dock)
    REAL dockH = 104.0f;
    REAL dockY = (REAL)(contentY + contentH - dockH);
    REAL inspScrollTop = inspTabY + 34.0f;
    REAL availTabH = std::max(40.0f, dockY - inspScrollTop);
    REAL rawTabH = (g_inspectorTab == INSP_TAB_STYLE)    ? 292.0f
                 : (g_inspectorTab == INSP_TAB_ARTWORK)  ? 300.0f
                 : (g_inspectorTab == INSP_TAB_GEOMETRY) ? 196.0f
                                                         : 148.0f;
    g_maxInspectorScroll = std::max(0, (int)std::ceil(rawTabH - availTabH + 12.0f));
    g_inspectorScrollY = std::clamp(g_inspectorScrollY, 0, g_maxInspectorScroll);

    RectF inspScrollArea((REAL)inspX, inspScrollTop, (REAL)inspW, availTabH);
    GraphicsState inspClipState = g.Save();
    g.SetClip(inspScrollArea);
    g_currentClipRect = &inspScrollArea;

    REAL tabBodyY = inspTabY + 38.0f - (REAL)g_inspectorScrollY;

    if (g_inspectorTab == INSP_TAB_STYLE) {
        // --- TAB 1: СТИЛЬ И ФОРМА ---
        g.DrawString(L"Форма адаптивного контейнера", -1, &fSmallBold, PointF((REAL)(inspX + 16), tabBodyY), NULL, &primBr);
        const wchar_t* shapeNames[5] = { L"Круг", L"Сквиркл", L"Квадрат", L"Галька", L"Шестигранник" };
        IconGlyphType shapeGlyphs[5] = { GLYPH_SHAPE_CIRCLE, GLYPH_SHAPE_SQUIRCLE, GLYPH_SHAPE_ROUNDRECT, GLYPH_SHAPE_PEBBLE, GLYPH_SHAPE_HEXAGON };
        REAL shW = (inspW - 32.0f - 4.0f * 4.0f) / 5.0f;
        for (int p = 0; p < 5; ++p) {
            RectF sr((REAL)(inspX + 16) + p * (shW + 4.0f), tabBodyY + 18.0f, shW, 26.0f);
            DrawStudioPillButton(g, (WidgetID)(W_SHAPE_0 + p), sr, shapeNames[p], fSmallBold, theme, false, (int)curSt.shape == p, false, shapeGlyphs[p]);
        }

        // Surface presets
        REAL presetY = tabBodyY + 54.0f;
        g.DrawString(L"Подложка поверхности", -1, &fSmallBold, PointF((REAL)(inspX + 16), presetY), NULL, &primBr);
        const wchar_t* presetNames[5] = { L"Google White", L"Material Pastel", L"Dark Material", L"Adaptive Edge", L"Цвет" };
        REAL prW = (inspW - 32.0f - 4.0f * 4.0f) / 5.0f;
        for (int p = 0; p < 5; ++p) {
            RectF pr((REAL)(inspX + 16) + p * (prW + 4.0f), presetY + 18.0f, prW, 26.0f);
            DrawStudioPillButton(g, (WidgetID)(W_PRESET_0 + p), pr, presetNames[p], fSmallBold, theme, false, (int)curSt.preset == p);
        }

        // Color Bar & Swatches
        REAL colorY = presetY + 54.0f;
        DrawStudioPillButton(g, W_BTN_EYEDROPPER, RectF((REAL)(inspX + 16), colorY, 100.0f, 26.0f),
                             g_isEyedropperActive ? L"Точка..." : L"Пипетка", fSmallBold, theme,
                             g_isEyedropperActive, g_isEyedropperActive, false, GLYPH_EYEDROPPER);
        DrawStudioPillButton(g, W_BTN_PALETTE, RectF((REAL)(inspX + 122), colorY, 90.0f, 26.0f), L"Палитра", fSmallBold, theme, false, false, false, GLYPH_PALETTE);

        Color curC(255, GetRValue(curSt.customColor), GetGValue(curSt.customColor), GetBValue(curSt.customColor));
        RectF hexBox((REAL)(inspX + 218), colorY, (REAL)(inspW - 234), 26.0f);
        FillRoundRect(g, hexBox, 5.0f, theme.controlBg);
        DrawRoundRect(g, hexBox, 5.0f, theme.controlBorder, 1.0f);
        SolidBrush curSwBr(curC);
        g.FillEllipse(&curSwBr, hexBox.X + 6.0f, hexBox.Y + 5.0f, 16.0f, 16.0f);
        wchar_t hexStr[32];
        wsprintfW(hexStr, L"#%02X%02X%02X", GetRValue(curSt.customColor), GetGValue(curSt.customColor), GetBValue(curSt.customColor));
        g.DrawString(hexStr, -1, &fSmallBold, RectF(hexBox.X + 26.0f, hexBox.Y, hexBox.Width - 28.0f, hexBox.Height), &sfSCenter, &primBr);

        REAL swRowY = colorY + 34.0f;
        REAL swW = (inspW - 32.0f - 8.0f * 6.0f) / 9.0f;
        for (int s = 0; s < 8; ++s) {
            RectF sr((REAL)(inspX + 16) + s * (swW + 6.0f), swRowY, swW, swW);
            RegisterHit((WidgetID)(W_SWATCH_0 + s), sr);
            Color sc(255, GetRValue(kSwatchPalette[s]), GetGValue(kSwatchPalette[s]), GetBValue(kSwatchPalette[s]));
            SolidBrush sbr(sc);
            g.FillEllipse(&sbr, sr);
            bool match = (curSt.customColor == kSwatchPalette[s] && curSt.preset == PRESET_CUSTOM_COLOR);
            Pen sp(match ? (theme.isDark ? Color(255, 255, 255, 255) : Color(255, 0, 0, 0)) : Color(120, 180, 180, 185), match ? 2.5f : 1.0f);
            g.DrawEllipse(&sp, sr);
        }

        // 9th Swatch: Live Windows System Accent Color
        COLORREF winAccent = GetWindowsAccentColor();
        RectF winSr((REAL)(inspX + 16) + 8 * (swW + 6.0f), swRowY, swW, swW);
        RegisterHit(W_SWATCH_WIN_ACCENT, winSr);
        Color wsc(255, GetRValue(winAccent), GetGValue(winAccent), GetBValue(winAccent));
        SolidBrush wsbr(wsc);
        g.FillEllipse(&wsbr, winSr);
        bool winMatch = (curSt.customColor == winAccent && curSt.preset == PRESET_CUSTOM_COLOR);
        Pen wsp(winMatch ? (theme.isDark ? Color(255, 255, 255, 255) : Color(255, 0, 0, 0)) : Color(140, 180, 180, 185), winMatch ? 2.5f : 1.0f);
        g.DrawEllipse(&wsp, winSr);
        SolidBrush wDotBr(theme.isDark ? Color(220, 255, 255, 255) : Color(220, 0, 0, 0));
        g.FillEllipse(&wDotBr, winSr.X + winSr.Width * 0.5f - 1.5f, winSr.Y + winSr.Height * 0.5f - 1.5f, 3.0f, 3.0f);

        // Surface Material Shaders (8 tactile finishes)
        REAL matY = swRowY + swW + 12.0f;
        g.DrawString(L"Материал и текстура подложки", -1, &fSmallBold, PointF((REAL)(inspX + 16), matY), NULL, &primBr);
        const wchar_t* matNames[8] = { L"Гладкий", L"Стекло", L"Титан", L"Эмбосс", L"Призма", L"Хром", L"Керамика", L"Фольга" };
        REAL matW4 = (inspW - 32.0f - 3.0f * 4.0f) / 4.0f;
        for (int m = 0; m < 8; ++m) {
            int row = m / 4;
            int col = m % 4;
            RectF mr((REAL)(inspX + 16) + col * (matW4 + 4.0f), matY + 16.0f + row * 24.0f, matW4, 20.0f);
            DrawStudioPillButton(g, (WidgetID)(W_MATERIAL_0 + m), mr, matNames[m], fSmallBold, theme, false, (int)curSt.material == m);
        }

        // Glyph Artwork Style (6 rendering modes)
        REAL glyphY = matY + 16.0f + 2 * 24.0f + 8.0f;
        g.DrawString(L"Стиль рисунка значка", -1, &fSmallBold, PointF((REAL)(inspX + 16), glyphY), NULL, &primBr);
        const wchar_t* glyphNames[6] = { L"Оригинал", L"Силуэт", L"3D Рельеф", L"Контур", L"Дуотон", L"Неон" };
        REAL glyW3 = (inspW - 32.0f - 2.0f * 4.0f) / 3.0f;
        for (int gl = 0; gl < 6; ++gl) {
            int row = gl / 3;
            int col = gl % 3;
            RectF gr((REAL)(inspX + 16) + col * (glyW3 + 4.0f), glyphY + 16.0f + row * 24.0f, glyW3, 20.0f);
            DrawStudioPillButton(g, (WidgetID)(W_GLYPH_0 + gl), gr, glyphNames[gl], fSmallBold, theme, false, (int)curSt.glyphStyle == gl);
        }

    } else if (g_inspectorTab == INSP_TAB_ARTWORK) {
        // --- TAB 2: РИСУНОК ЗНАЧКА ---
        g.DrawString(L"Каталоги и замена рисунка значка", -1, &fSmallBold, PointF((REAL)(inspX + 16), tabBodyY), NULL, &primBr);
        REAL artW3 = (inspW - 32.0f - 2.0f * 4.0f) / 3.0f;
        RectF btnFileR((REAL)(inspX + 16), tabBodyY + 18.0f, artW3, 26.0f);
        RectF btnPasteR((REAL)(inspX + 16) + (artW3 + 4.0f), tabBodyY + 18.0f, artW3, 26.0f);
        RectF btnLucideR((REAL)(inspX + 16) + 2 * (artW3 + 4.0f), tabBodyY + 18.0f, artW3, 26.0f);

        DrawStudioPillButton(g, W_BTN_ART_FILE, btnFileR, L"Файл...", fSmallBold, theme, false, false, false, GLYPH_PHOTO);
        DrawStudioPillButton(g, W_BTN_ART_PASTE, btnPasteR, L"Буфер", fSmallBold, theme, false, false, false, GLYPH_CLIPBOARD);
        DrawStudioPillButton(g, W_BTN_ART_LUCIDE, btnLucideR, L"Lucide...", fSmallBold, theme, false, false, false, GLYPH_VECTOR);

        REAL row2Y = tabBodyY + 48.0f;
        RectF btnBrandsR((REAL)(inspX + 16), row2Y, artW3, 26.0f);
        RectF btnPatternsR((REAL)(inspX + 16) + (artW3 + 4.0f), row2Y, artW3, 26.0f);
        RectF btnMonoR((REAL)(inspX + 16) + 2 * (artW3 + 4.0f), row2Y, artW3, 26.0f);

        DrawStudioPillButton(g, W_BTN_ART_BRANDS, btnBrandsR, L"Бренды...", fSmallBold, theme, false, false, false, GLYPH_BRAND);
        DrawStudioPillButton(g, W_BTN_ART_AI_PATTERNS, btnPatternsR, L"AI Формы...", fSmallBold, theme, false, false, false, GLYPH_SPARKLE);
        DrawStudioPillButton(g, W_BTN_ART_MONOGRAM, btnMonoR, L"Текст...", fSmallBold, theme, false, false, false, GLYPH_FONT);

        REAL resetArtY = row2Y + 30.0f;
        DrawStudioPillButton(g, W_BTN_ART_RESET, RectF((REAL)(inspX + 16), resetArtY, (REAL)(inspW - 32), 24.0f),
                             L"Вернуть системный значок", fSmall, theme, false, false, false, GLYPH_REVERT);

        // 2. Поворот и зеркальное отражение
        REAL transY = resetArtY + 30.0f;
        wchar_t transLbl[96];
        wsprintfW(transLbl, L"Поворот и отражение (%d°%ls%ls)",
                  (int)std::round(curSt.rotation),
                  curSt.flipH ? L" · Flip H" : L"",
                  curSt.flipV ? L" · Flip V" : L"");
        g.DrawString(transLbl, -1, &fSmallBold, PointF((REAL)(inspX + 16), transY), NULL, &primBr);

        REAL rotW5 = (inspW - 32.0f - 4.0f * 4.0f) / 5.0f;
        DrawStudioPillButton(g, W_BTN_ART_ROT_L, RectF((REAL)(inspX + 16), transY + 18.0f, rotW5, 24.0f), L"-90°", fSmallBold, theme);
        DrawStudioPillButton(g, W_BTN_ART_ROT_R, RectF((REAL)(inspX + 16) + (rotW5 + 4.0f), transY + 18.0f, rotW5, 24.0f), L"+90°", fSmallBold, theme);
        DrawStudioPillButton(g, W_BTN_ART_FLIP_H, RectF((REAL)(inspX + 16) + 2 * (rotW5 + 4.0f), transY + 18.0f, rotW5, 24.0f), L"Flip H", fSmallBold, theme, false, curSt.flipH);
        DrawStudioPillButton(g, W_BTN_ART_FLIP_V, RectF((REAL)(inspX + 16) + 3 * (rotW5 + 4.0f), transY + 18.0f, rotW5, 24.0f), L"Flip V", fSmallBold, theme, false, curSt.flipV);
        DrawStudioPillButton(g, W_BTN_ART_RESET_TRANS, RectF((REAL)(inspX + 16) + 4 * (rotW5 + 4.0f), transY + 18.0f, rotW5, 24.0f), L"0°", fSmallBold, theme);

        // 3. Градиентная маска рисунка (7 градиентов)
        REAL gradY = transY + 46.0f;
        g.DrawString(L"Градиентная заливка рисунка", -1, &fSmallBold, PointF((REAL)(inspX + 16), gradY), NULL, &primBr);
        const wchar_t* gradNames[7] = { L"Нет", L"Gold", L"Cyber", L"Sunset", L"Emerald", L"Titan", L"Amethyst" };
        REAL grW4 = (inspW - 32.0f - 3.0f * 4.0f) / 4.0f;
        for (int gr = 0; gr < 7; ++gr) {
            int row = gr / 4;
            int col = gr % 4;
            RectF grR((REAL)(inspX + 16) + col * (grW4 + 4.0f), gradY + 18.0f + row * 24.0f, grW4, 20.0f);
            DrawStudioPillButton(g, (WidgetID)(W_BTN_ART_GRAD_0 + gr), grR, gradNames[gr], fSmallBold, theme, false, (int)curSt.glyphGrad == gr);
        }

        // 4. Бейдж статуса на значке (8 бейджей + 4 позиции)
        REAL badgeY = gradY + 18.0f + 2 * 24.0f + 6.0f;
        g.DrawString(L"Бейдж статуса поверх рисунка", -1, &fSmallBold, PointF((REAL)(inspX + 16), badgeY), NULL, &primBr);
        const wchar_t* badgeNames[8] = { L"Нет", L"Точка", L"Галочка", L"Звезда", L"Замок", L"Alert", L"Beta", L"Pro" };
        REAL bdW4 = (inspW - 32.0f - 3.0f * 4.0f) / 4.0f;
        for (int b = 0; b < 8; ++b) {
            int row = b / 4;
            int col = b % 4;
            RectF bdR((REAL)(inspX + 16) + col * (bdW4 + 4.0f), badgeY + 18.0f + row * 24.0f, bdW4, 20.0f);
            DrawStudioPillButton(g, (WidgetID)(W_BTN_ART_BADGE_0 + b), bdR, badgeNames[b], fSmallBold, theme, false, (int)curSt.badge == b);
        }

        // Badge positions (4 positions)
        REAL bposRowY = badgeY + 18.0f + 2 * 24.0f + 6.0f;
        const wchar_t* bposNames[4] = { L"Справа вверх", L"Справа низ", L"Слева вверх", L"Слева низ" };
        REAL bpW4 = (inspW - 32.0f - 3.0f * 4.0f) / 4.0f;
        for (int bp = 0; bp < 4; ++bp) {
            RectF bpR((REAL)(inspX + 16) + bp * (bpW4 + 4.0f), bposRowY, bpW4, 20.0f);
            DrawStudioPillButton(g, (WidgetID)(W_BTN_ART_BPOS_TR + bp), bpR, bposNames[bp], fSmall, theme, false, (int)curSt.badgePos == bp);
        }

    } else if (g_inspectorTab == INSP_TAB_GEOMETRY) {
        // --- TAB 3: ГЕОМЕТРИЯ И СЛАЙДЕРЫ ---
        wchar_t lblS[64]; wsprintfW(lblS, L"Масштаб значка: %d%%", (int)std::round(curSt.scale * 100.0));
        g.DrawString(lblS, -1, &fSmallBold, PointF((REAL)(inspX + 16), tabBodyY), NULL, &primBr);
        DrawStudioSlider(g, W_SLIDER_SCALE, RectF((REAL)(inspX + 16), tabBodyY + 18.0f, (REAL)(inspW - 100), 20.0f), (curSt.scale - 0.30) / (2.50 - 0.30), theme);
        DrawStudioPillButton(g, W_BTN_SC_100, RectF((REAL)(inspX + inspW - 74), tabBodyY + 16.0f, 58.0f, 24.0f), L"100%", fSmallBold, theme, false, std::abs(curSt.scale - 1.00) < 0.01);

        // Offset X
        REAL offXY = tabBodyY + 48.0f;
        wchar_t lblX[64]; wsprintfW(lblX, L"Смещение по X: %+d%%", (int)std::round(curSt.offsetX * 100.0));
        g.DrawString(lblX, -1, &fSmallBold, PointF((REAL)(inspX + 16), offXY), NULL, &primBr);
        DrawStudioSlider(g, W_SLIDER_OFFX, RectF((REAL)(inspX + 16), offXY + 18.0f, (REAL)(inspW - 100), 20.0f), (curSt.offsetX + 1.0) * 0.5, theme);
        DrawStudioPillButton(g, W_BTN_CENTER, RectF((REAL)(inspX + inspW - 74), offXY + 16.0f, 58.0f, 24.0f), L"Центр", fSmallBold, theme);

        // Offset Y
        REAL offYY = offXY + 48.0f;
        wchar_t lblY[64]; wsprintfW(lblY, L"Смещение по Y: %+d%%", (int)std::round(curSt.offsetY * 100.0));
        g.DrawString(lblY, -1, &fSmallBold, PointF((REAL)(inspX + 16), offYY), NULL, &primBr);
        DrawStudioSlider(g, W_SLIDER_OFFY, RectF((REAL)(inspX + 16), offYY + 18.0f, (REAL)(inspW - 32), 20.0f), (curSt.offsetY + 1.0) * 0.5, theme);

        // Outer Shadow
        REAL shdY = offYY + 48.0f;
        wchar_t lblSh[64]; wsprintfW(lblSh, L"Внешняя тень: %d%%", (int)std::round(curSt.shadow * 100.0));
        g.DrawString(lblSh, -1, &fSmallBold, PointF((REAL)(inspX + 16), shdY), NULL, &primBr);
        DrawStudioSlider(g, W_SLIDER_SHADOW, RectF((REAL)(inspX + 16), shdY + 18.0f, (REAL)(inspW - 32), 20.0f), curSt.shadow, theme);

    } else {
        // --- TAB 4: ОПЦИИ И ЭФФЕКТЫ ---
        DrawStudioToggle(g, W_TOGGLE_AUTOTRIM,  RectF((REAL)(inspX + 16), tabBodyY + 4.0f,   (REAL)(inspW - 32), 26.0f), L"Авто-обрезка прозрачных полей", curSt.autoTrim, fBody, theme);
        DrawStudioToggle(g, W_TOGGLE_INNERSHAD, RectF((REAL)(inspX + 16), tabBodyY + 36.0f,  (REAL)(inspW - 32), 26.0f), L"Внутренняя тень иконки",        curSt.innerShadow, fBody, theme);
        DrawStudioToggle(g, W_TOGGLE_FULLBLEED, RectF((REAL)(inspX + 16), tabBodyY + 68.0f,  (REAL)(inspW - 32), 26.0f), L"Квадратные иконки на всю форму", curSt.fullBleed, fBody, theme);
        DrawStudioToggle(g, W_TOGGLE_MONO,      RectF((REAL)(inspX + 16), tabBodyY + 100.0f, (REAL)(inspW - 32), 26.0f), L"Монохромный тон Material You",  curSt.monochrome, fBody, theme);
    }

    if (g_maxInspectorScroll > 0) {
        REAL trackH = availTabH - 4.0f;
        REAL thumbH = std::max(24.0f, trackH * (availTabH / (rawTabH + 12.0f)));
        REAL thumbY = inspScrollTop + 2.0f + (trackH - thumbH) * ((REAL)g_inspectorScrollY / (REAL)g_maxInspectorScroll);
        FillRoundRect(g, RectF((REAL)(inspX + inspW - 5), thumbY, 3.0f, thumbH), 1.5f,
                      theme.isDark ? Color(130, 120, 125, 140) : Color(130, 160, 165, 175));
    }

    g_currentClipRect = nullptr;
    g.Restore(inspClipState);

    // 4. Sticky Bottom Action Dock (Height: 104px, solid surface)
    RectF dockR((REAL)inspX, dockY, (REAL)inspW, dockH);
    g.FillRectangle(&inspBg, dockR);
    g.DrawLine(&divPen, inspX, (INT)dockY, inspX + inspW, (INT)dockY);

    // Row 1: Primary Apply Button
    DrawStudioPillButton(g, W_BTN_APPLY_ONE, RectF((REAL)(inspX + 16), dockY + 8.0f, (REAL)(inspW - 32), 34.0f),
                         L"Применить к выбранному", fBodyBold, theme, true, false, false, GLYPH_CHECKMARK);

    // Row 2: Batch Apply & Original
    REAL halfBtnW = (inspW - 32.0f - 8.0f) * 0.5f;
    DrawStudioPillButton(g, W_BTN_APPLY_ALL, RectF((REAL)(inspX + 16), dockY + 46.0f, halfBtnW, 26.0f),
                         L"Ко всем", fSmallBold, theme, false, true, false, GLYPH_BATCH);
    DrawStudioPillButton(g, W_BTN_RESTORE_ONE, RectF((REAL)(inspX + 24) + halfBtnW, dockY + 46.0f, halfBtnW, 26.0f),
                         L"Оригинал", fSmallBold, theme, false, false, false, GLYPH_REVERT);

    // Row 3: Reset All & Export
    DrawStudioPillButton(g, W_BTN_RESTORE_ALL, RectF((REAL)(inspX + 16), dockY + 76.0f, halfBtnW, 24.0f),
                         L"Сбросить всё", fSmall, theme, false, false, true, GLYPH_TRASH);
    DrawStudioPillButton(g, W_BTN_EXPORT, RectF((REAL)(inspX + 24) + halfBtnW, dockY + 76.0f, halfBtnW, 24.0f),
                         L"Экспорт...", fSmall, theme, false, false, false, GLYPH_EXPORT);

    // =========================================================================
    // BOTTOM STATUS BAR (Height: 32px)
    // =========================================================================
    SolidBrush footBr(theme.statusBar);
    g.FillRectangle(&footBr, 0, clientH - 32, clientW, 32);
    g.DrawLine(&divPen, 0, clientH - 32, clientW, clientH - 32);

    StringFormat sfTrimEllipsis;
    sfTrimEllipsis.SetTrimming(StringTrimmingEllipsisCharacter);
    sfTrimEllipsis.SetLineAlignment(StringAlignmentCenter);
    sfTrimEllipsis.SetFormatFlags(StringFormatFlagsNoWrap);

    const wchar_t* pTip = GetWidgetTooltip(g_hoveredWidget);
    std::wstring dispStatus = pTip ? pTip : g_statusText;
    g.DrawString(dispStatus.c_str(), -1, &fSmall, RectF(16.0f, (REAL)(clientH - 32), (REAL)(clientW - 390), 32.0f), &sfTrimEllipsis, pTip ? &primBr : &mutBr);

    if (g_progressRatio > 0.001) {
        RectF progBg((REAL)(clientW - 220), (REAL)(clientH - 20), 200.0f, 8.0f);
        FillRoundRect(g, progBg, 4.0f, theme.controlBg);
        RectF progFg(progBg.X, progBg.Y, (REAL)(progBg.Width * std::clamp(g_progressRatio, 0.0, 1.0)), 8.0f);
        FillRoundRect(g, progFg, 4.0f, theme.primaryBtnBg);
    } else {
        StringFormat sfRight;
        sfRight.SetAlignment(StringAlignmentFar);
        sfRight.SetLineAlignment(StringAlignmentCenter);
        sfRight.SetFormatFlags(StringFormatFlagsNoWrap);
        g.DrawString(L"Ctrl+1 Сетка · Ctrl+2 Список · Enter Применить · Del Сброс", -1, &fSmall,
                     RectF((REAL)(clientW - 380), (REAL)(clientH - 32), 364.0f, 32.0f), &sfRight, &secBr);
    }

    // Floating Eyedropper Magnifier Loupe
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
        g.DrawString(lhex, -1, &fBodyBold, RectF(lx + 42.0f, ly, 80.0f, 42.0f), &sfSCenter, &primBr);
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
    if (id == W_SLIDER_GRID_SIZE) {
        g_gridIconSize = std::clamp((int)std::round(48.0 + norm * (96.0 - 48.0)), 48, 96);
        if (isFinalRelease) SaveStyleConfig();
    } else {
        RoundStyle& st = BeginEditStyle(false);
        if (id == W_SLIDER_SCALE) {
            st.scale = 0.30 + norm * (2.50 - 0.30);
        } else if (id == W_SLIDER_OFFX) {
            st.offsetX = -1.0 + norm * 2.0;
        } else if (id == W_SLIDER_OFFY) {
            st.offsetY = -1.0 + norm * 2.0;
        } else if (id == W_SLIDER_SHADOW) {
            st.shadow = norm;
        }
        InvalidateHeroCache();
        if (isFinalRelease) {
            CommitEditedStyle();
        }
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
    IDM_FILE_EXPORT_WEB_FAVICON_KIT,
    IDM_FILE_EXPORT_XCODE_CATALOG,
    IDM_FILE_EXPORT_SHOWCASE,
    IDM_FILE_RESCAN,

    // Edit
    IDM_EDIT_UNDO,
    IDM_EDIT_REDO,
    IDM_EDIT_FIND,
    IDM_EDIT_CLEAR_SEARCH,

    // View
    IDM_VIEW_DARK_THEME,
    IDM_VIEW_LIGHT_THEME,
    IDM_VIEW_MODE_GRID,
    IDM_VIEW_MODE_LIST,
    IDM_VIEW_ZOOM_IN,
    IDM_VIEW_ZOOM_OUT,
    IDM_VIEW_FILTER_ALL,
    IDM_VIEW_FILTER_APPLIED,
    IDM_VIEW_FILTER_SYSTEM,
    IDM_VIEW_FILTER_THIRDPARTY,

    // Style
    IDM_STYLE_APPLY_ONE,
    IDM_STYLE_APPLY_ALL,
    IDM_STYLE_RESTORE_ONE,
    IDM_STYLE_RESTORE_ALL,
    IDM_STYLE_SHAPE_CIRCLE,
    IDM_STYLE_SHAPE_SQUIRCLE,
    IDM_STYLE_SHAPE_PEBBLE,
    IDM_STYLE_SHAPE_HEXAGON,
    IDM_STYLE_SHAPE_ROUNDEDRECT,
    IDM_STYLE_MATERIAL_SMOOTH,
    IDM_STYLE_MATERIAL_FROSTED_GLASS,
    IDM_STYLE_MATERIAL_BRUSHED_TITANIUM,
    IDM_STYLE_MATERIAL_NEUMORPHIC,
    IDM_STYLE_MATERIAL_PRISM_GLOW,
    IDM_STYLE_MATERIAL_LIQUID_CHROME,
    IDM_STYLE_MATERIAL_MATTE_CERAMIC,
    IDM_STYLE_MATERIAL_HOLOGRAPHIC,
    IDM_STYLE_GLYPH_ORIGINAL,
    IDM_STYLE_GLYPH_SILHOUETTE,
    IDM_STYLE_GLYPH_EMBOSSED,
    IDM_STYLE_GLYPH_OUTLINE,
    IDM_STYLE_GLYPH_DUOTONE,
    IDM_STYLE_GLYPH_GLOW,
    IDM_STYLE_GLYPH_ROTATE_90,
    IDM_STYLE_GLYPH_FLIP_H,
    IDM_STYLE_GLYPH_FLIP_V,
    IDM_STYLE_GLYPH_RESET_TRANSFORM,
    IDM_STYLE_GLYPH_GRAD_NONE,
    IDM_STYLE_GLYPH_GRAD_GOLD,
    IDM_STYLE_GLYPH_GRAD_CYBER,
    IDM_STYLE_GLYPH_GRAD_SUNSET,
    IDM_STYLE_GLYPH_GRAD_EMERALD,
    IDM_STYLE_GLYPH_GRAD_TITANIUM,
    IDM_STYLE_GLYPH_GRAD_PURPLE,
    IDM_STYLE_GLYPH_BADGE_NONE,
    IDM_STYLE_GLYPH_BADGE_DOT,
    IDM_STYLE_GLYPH_BADGE_CHECK,
    IDM_STYLE_GLYPH_BADGE_STAR,
    IDM_STYLE_GLYPH_BADGE_LOCK,
    IDM_STYLE_GLYPH_BADGE_ALERT,
    IDM_STYLE_GLYPH_BADGE_BETA,
    IDM_STYLE_GLYPH_BADGE_PRO,
    IDM_STYLE_GLYPH_BADGE_POS_TR,
    IDM_STYLE_GLYPH_BADGE_POS_BR,
    IDM_STYLE_GLYPH_BADGE_POS_TL,
    IDM_STYLE_GLYPH_BADGE_POS_BL,
    IDM_STYLE_EYEDROPPER,
    IDM_STYLE_WINDOWS_ACCENT,
    IDM_STYLE_RESET_DEFAULT,
    IDM_STYLE_PACK_BASE = 10100,

    // Tools
    IDM_TOOLS_CREATE_MONOGRAM,
    IDM_TOOLS_SURPRISE_ARTWORK,
    IDM_TOOLS_BATCH_CONVERT,
    IDM_TOOLS_CONTEXT_MENU,
    IDM_TOOLS_ADMIN,
    IDM_TOOLS_SHORTCUTS,
    IDM_TOOLS_RESTART_EXPLORER,
    IDM_TOOLS_SYSTEM_THEMING,
    IDM_TOOLS_SHORTCUTS_INFO,
    IDM_TOOLS_IMPORT_PACK = 10390,
    IDM_TOOLS_LUCIDE_BASE = 10400,
    IDM_TOOLS_BRANDS_BASE = 10600,
    IDM_TOOLS_PATTERNS_BASE = 10800
};

static const struct { const wchar_t* id; const wchar_t* label; int cat; } kLucideMenuIcons[] = {
    // 0: Разработка и система
    { L"terminal",   L"Терминал (Terminal)", 0 },
    { L"code",       L"Исходный код (Code)", 0 },
    { L"git",        L"Ветка Git (Branch)", 0 },
    { L"database",   L"База данных (Database)", 0 },
    { L"server",     L"Сервер (Server)", 0 },
    { L"cpu",        L"Процессор (CPU)", 0 },
    { L"layers",     L"Слои архитектуры (Layers)", 0 },
    { L"package",    L"Пакет / Сборка (Package)", 0 },
    { L"sparkles",   L"Искусственный интеллект (Sparkles)", 0 },
    // 1: Интернет и связь
    { L"globe",      L"Интернет и веб (Globe)", 1 },
    { L"wifi",       L"Беспроводная сеть (Wi-Fi)", 1 },
    { L"cloud",      L"Облако (Cloud)", 1 },
    { L"download",   L"Загрузка (Download)", 1 },
    { L"upload",     L"Отправка (Upload)", 1 },
    { L"mail",       L"Почта (Mail)", 1 },
    { L"chat",       L"Диалог и чат (Chat)", 1 },
    { L"bell",       L"Уведомления (Bell)", 1 },
    // 2: Медиа и творчество
    { L"music",      L"Музыка (Music)", 2 },
    { L"headphones", L"Наушники и аудио (Headphones)", 2 },
    { L"play",       L"Воспроизведение (Play)", 2 },
    { L"camera",     L"Фотокамера (Camera)", 2 },
    { L"video",      L"Видеокамера (Video)", 2 },
    { L"image",      L"Галерея и графика (Image)", 2 },
    { L"gamepad",    L"Игры и геймпад (Gamepad)", 2 },
    // 3: Файлы и интерфейс
    { L"home",       L"Домашний экран (Home)", 3 },
    { L"folder",     L"Папка (Folder)", 3 },
    { L"file-text",  L"Документ и заметки (File Text)", 3 },
    { L"search",     L"Поиск (Search)", 3 },
    { L"compass",    L"Навигатор (Compass)", 3 },
    { L"calendar",   L"Календарь (Calendar)", 3 },
    { L"clock",      L"Часы и таймер (Clock)", 3 },
    { L"user",       L"Профиль (User)", 3 },
    { L"star",       L"Избранное (Star)", 3 },
    { L"heart",      L"Лайк / Здоровье (Heart)", 3 },
    { L"trash",      L"Корзина (Trash)", 3 },
    // 4: Безопасность и инструменты
    { L"settings",   L"Настройки (Settings)", 4 },
    { L"wrench",     L"Инструменты и сервис (Wrench)", 4 },
    { L"shield",     L"Защита и безопасность (Shield)", 4 },
    { L"lock",       L"Замок (Lock)", 4 },
    { L"key",        L"Ключ доступа (Key)", 4 },
    { L"eye",        L"Обзор и приватность (Eye)", 4 },
    { L"rocket",     L"Быстрый запуск (Rocket)", 4 },
    { L"zap",        L"Молния / Производительность (Zap)", 4 }
};

static HMENU BuildLucidePopupMenu() {
    HMENU hRoot = CreatePopupMenu();
    const wchar_t* catTitles[5] = {
        L"Разработка и система",
        L"Интернет и связь",
        L"Медиа и творчество",
        L"Файлы и интерфейс",
        L"Безопасность и утилиты"
    };
    HMENU hCats[5] = {
        CreatePopupMenu(),
        CreatePopupMenu(),
        CreatePopupMenu(),
        CreatePopupMenu(),
        CreatePopupMenu()
    };
    int lucideCount = (int)(sizeof(kLucideMenuIcons) / sizeof(kLucideMenuIcons[0]));
    for (int i = 0; i < lucideCount; ++i) {
        int c = std::clamp(kLucideMenuIcons[i].cat, 0, 4);
        AppendMenuW(hCats[c], MF_STRING, IDM_TOOLS_LUCIDE_BASE + i, kLucideMenuIcons[i].label);
    }
    for (int c = 0; c < 5; ++c) {
        AppendMenuW(hRoot, MF_POPUP, (UINT_PTR)hCats[c], catTitles[c]);
    }
    return hRoot;
}

static void ExecuteMenuCommand(HWND hwnd, int cmd);
static void HandleWidgetClick(HWND hwnd, WidgetID id, POINT pt);

static void ShowSimpleIconsMenu(HWND hwnd, POINT pt) {
    if (g_selectedAppIndex < 0 || g_selectedAppIndex >= (int)g_allApps.size()) {
        MessageBoxW(hwnd, L"Выберите приложение в галерее для замены логотипа.", L"Бренды Simple Icons", MB_OK | MB_ICONINFORMATION);
        return;
    }

    HMENU hRoot = CreatePopupMenu();
    HMENU hWeb = CreatePopupMenu();
    HMENU hDev = CreatePopupMenu();
    HMENU hComms = CreatePopupMenu();
    HMENU hMedia = CreatePopupMenu();
    HMENU hAI = CreatePopupMenu();

    for (int i = 0; i < kSimpleIconsCount; ++i) {
        std::wstring cat = kSimpleIconsCurated[i].category;
        HMENU hTarget = hAI;
        if (cat == L"Браузеры и веб") hTarget = hWeb;
        else if (cat == L"Разработка и код") hTarget = hDev;
        else if (cat == L"Мессенджеры и соцсети") hTarget = hComms;
        else if (cat == L"Медиа, игры и дизайн") hTarget = hMedia;

        AppendMenuW(hTarget, MF_STRING, IDM_TOOLS_BRANDS_BASE + i, kSimpleIconsCurated[i].title);
    }

    AppendMenuW(hRoot, MF_POPUP, (UINT_PTR)hWeb, L"Браузеры и веб");
    AppendMenuW(hRoot, MF_POPUP, (UINT_PTR)hDev, L"Разработка и код");
    AppendMenuW(hRoot, MF_POPUP, (UINT_PTR)hComms, L"Мессенджеры и соцсети");
    AppendMenuW(hRoot, MF_POPUP, (UINT_PTR)hMedia, L"Медиа, игры и дизайн");
    AppendMenuW(hRoot, MF_POPUP, (UINT_PTR)hAI, L"Продуктивность и ИИ");

    int cmd = TrackPopupMenu(hRoot, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hRoot);
    if (cmd != 0) ExecuteMenuCommand(hwnd, cmd);
}

static void ShowAIPatternsMenu(HWND hwnd, POINT pt) {
    if (g_selectedAppIndex < 0 || g_selectedAppIndex >= (int)g_allApps.size()) {
        MessageBoxW(hwnd, L"Выберите приложение в галерее для создания паттерна.", L"AI Паттерны", MB_OK | MB_ICONINFORMATION);
        return;
    }

    HMENU hMenu = CreatePopupMenu();
    const wchar_t* pNames[8] = {
        L"Celestial Sparkle (ИИ звезда / вспышка)",
        L"Iris Aperture (Камера / апертура)",
        L"Planetary Orbit (Планетарная орбита / атом)",
        L"Hexacore (Квантовое шестигранное ядро)",
        L"Faceted Prism (Хроматическая призма)",
        L"Cyber Shield (Кибер-щит)",
        L"Code Terminal (Командный терминал)",
        L"Sound Waveform (Аудио-волна / эквалайзер)"
    };
    for (int i = 0; i < 8; ++i) {
        AppendMenuW(hMenu, MF_STRING, IDM_TOOLS_PATTERNS_BASE + i, pNames[i]);
    }

    int cmd = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMenu);
    if (cmd != 0) ExecuteMenuCommand(hwnd, cmd);
}

struct MonogramModalState {
    HWND hEdit = NULL;
    HWND hCombo = NULL;
    std::wstring letters;
    int frameStyle = 0;
    COLORREF primaryColor = RGB(10, 132, 255);
    bool accepted = false;
    bool finished = false;
};

static LRESULT CALLBACK MonogramModalWndProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    MonogramModalState* st = (MonogramModalState*)GetWindowLongPtrW(hDlg, GWLP_USERDATA);
    switch (msg) {
        case WM_NCCREATE: {
            CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
            SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            return TRUE;
        }
        case WM_COMMAND: {
            if (!st) break;
            int id = LOWORD(wParam);
            if (id == 3003) { // Pick Color
                static COLORREF cust[16] = {0};
                CHOOSECOLORW cc = {0};
                cc.lStructSize = sizeof(cc);
                cc.hwndOwner = hDlg;
                cc.lpCustColors = cust;
                cc.rgbResult = st->primaryColor;
                cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                if (ChooseColorW(&cc)) {
                    st->primaryColor = cc.rgbResult;
                }
                return 0;
            } else if (id == IDOK) {
                wchar_t buf[16] = {0};
                GetWindowTextW(st->hEdit, buf, 15);
                std::wstring trimmed;
                for (wchar_t ch : std::wstring(buf)) {
                    if (ch > 32) trimmed.push_back(ch);
                    if (trimmed.size() >= 3) break;
                }
                if (trimmed.empty()) {
                    MessageBoxW(hDlg, L"Введите от 1 до 3 символов для монограммы.", L"Монограмма", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                st->letters = trimmed;
                int sel = (int)SendMessageW(st->hCombo, CB_GETCURSEL, 0, 0);
                st->frameStyle = std::clamp(sel, 0, 3);
                st->accepted = true;
                st->finished = true;
                DestroyWindow(hDlg);
                return 0;
            } else if (id == IDCANCEL) {
                st->finished = true;
                DestroyWindow(hDlg);
                return 0;
            }
            break;
        }
        case WM_CLOSE:
            if (st) st->finished = true;
            DestroyWindow(hDlg);
            return 0;
    }
    return DefWindowProcW(hDlg, msg, wParam, lParam);
}

static void PromptMonogramDialog(HWND hwnd) {
    if (g_selectedAppIndex < 0 || g_selectedAppIndex >= (int)g_allApps.size()) {
        MessageBoxW(hwnd, L"Выберите приложение в галерее для создания монограммы.", L"Создание монограммы", MB_OK | MB_ICONINFORMATION);
        return;
    }
    AppEntry& app = g_allApps[g_selectedAppIndex];

    MonogramModalState st;
    if (!app.name.empty()) {
        st.letters += (wchar_t)towupper(app.name[0]);
        size_t sp = app.name.find(L' ');
        if (sp != std::wstring::npos && sp + 1 < app.name.length()) {
            st.letters += (wchar_t)towupper(app.name[sp + 1]);
        }
    } else {
        st.letters = L"M";
    }
    const RoundStyle& curStyle = GetEffectiveStyle(app);
    if (curStyle.preset == PRESET_CUSTOM_COLOR) {
        st.primaryColor = curStyle.customColor;
    }

    static bool s_reg = false;
    HINSTANCE hInst = GetModuleHandleW(NULL);
    if (!s_reg) {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.lpfnWndProc = MonogramModalWndProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = L"ModoMonogramModalClass";
        RegisterClassExW(&wc);
        s_reg = true;
    }

    RECT prc = {0};
    GetWindowRect(hwnd, &prc);
    int dlgW = 380, dlgH = 236;
    int dlgX = prc.left + ((prc.right - prc.left) - dlgW) / 2;
    int dlgY = prc.top + ((prc.bottom - prc.top) - dlgH) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"ModoMonogramModalClass",
        L"Создать типографическую монограмму",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        dlgX, dlgY, dlgW, dlgH,
        hwnd, NULL, hInst, &st
    );
    if (!hDlg) return;

    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HWND hLbl1 = CreateWindowExW(0, L"STATIC", L"Символы монограммы (1-3 знака):", WS_CHILD | WS_VISIBLE, 16, 16, 336, 18, hDlg, NULL, hInst, NULL);
    st.hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", st.letters.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 16, 38, 190, 26, hDlg, (HMENU)3001, hInst, NULL);
    SendMessageW(st.hEdit, EM_SETLIMITTEXT, 3, 0);

    HWND hColBtn = CreateWindowExW(0, L"BUTTON", L"Цвет акцента...", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 218, 37, 134, 28, hDlg, (HMENU)3003, hInst, NULL);

    HWND hLbl2 = CreateWindowExW(0, L"STATIC", L"Оформление контура:", WS_CHILD | WS_VISIBLE, 16, 78, 336, 18, hDlg, NULL, hInst, NULL);
    st.hCombo = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST, 16, 100, 336, 140, hDlg, (HMENU)3002, hInst, NULL);
    SendMessageW(st.hCombo, CB_ADDSTRING, 0, (LPARAM)L"Чистая типографика (без рамки)");
    SendMessageW(st.hCombo, CB_ADDSTRING, 0, (LPARAM)L"Двойное кольцо (Circle Ring)");
    SendMessageW(st.hCombo, CB_ADDSTRING, 0, (LPARAM)L"Контур сквиркла (Squircle Frame)");
    SendMessageW(st.hCombo, CB_ADDSTRING, 0, (LPARAM)L"Шестигранный контур (Hex Frame)");
    SendMessageW(st.hCombo, CB_SETCURSEL, 0, 0);

    HWND hOk = CreateWindowExW(0, L"BUTTON", L"Создать", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 168, 150, 90, 30, hDlg, (HMENU)IDOK, hInst, NULL);
    HWND hCancel = CreateWindowExW(0, L"BUTTON", L"Отмена", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 266, 150, 86, 30, hDlg, (HMENU)IDCANCEL, hInst, NULL);

    HWND ctrls[] = { hLbl1, st.hEdit, hColBtn, hLbl2, st.hCombo, hOk, hCancel };
    for (HWND hc : ctrls) SendMessageW(hc, WM_SETFONT, (WPARAM)hFont, TRUE);

    EnableWindow(hwnd, FALSE);
    ShowWindow(hDlg, SW_SHOW);
    SetFocus(st.hEdit);
    SendMessageW(st.hEdit, EM_SETSEL, 0, -1);

    MSG m;
    while (!st.finished && GetMessageW(&m, NULL, 0, 0)) {
        if (!IsDialogMessageW(hDlg, &m)) {
            TranslateMessage(&m);
            DispatchMessageW(&m);
        }
    }
    EnableWindow(hwnd, TRUE);
    SetForegroundWindow(hwnd);

    if (st.accepted) {
        BYTE r = GetRValue(st.primaryColor);
        BYTE g = GetGValue(st.primaryColor);
        BYTE b = GetBValue(st.primaryColor);
        Color pColor(255, r, g, b);
        Color sColor(
            255,
            (BYTE)std::clamp((int)r - 28, 0, 255),
            (BYTE)std::clamp((int)g - 36, 0, 255),
            (BYTE)std::clamp((int)b + 32, 0, 255)
        );
        SetAppCustomArtwork(app, IconEngine::RenderMonogramBitmap(st.letters, pColor, sColor, 256, st.frameStyle));
        g_statusText = L"Создана монограмма '" + st.letters + L"' для " + app.name;
        InvalidateRect(hwnd, NULL, FALSE);
    }
}

static std::wstring NormalizeMatchKey(const std::wstring& s) {
    std::wstring out;
    out.reserve(s.size());
    for (wchar_t ch : s) {
        wchar_t lo = (wchar_t)towlower(ch);
        if ((lo >= L'a' && lo <= L'z') || (lo >= L'0' && lo <= L'9') || (lo >= 0x0400 && lo <= 0x04FF)) {
            out.push_back(lo);
        }
    }
    return out;
}

static bool ApplyModoPackJsonFile(const std::wstring& jsonPath) {
    HANDLE hFile = CreateFileW(jsonPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD fSize = GetFileSize(hFile, NULL);
    if (fSize == 0 || fSize == INVALID_FILE_SIZE || fSize > 4 * 1024 * 1024) {
        CloseHandle(hFile);
        return false;
    }
    std::string raw(fSize, '\0');
    DWORD rd = 0;
    bool ok = ReadFile(hFile, &raw[0], fSize, &rd, NULL) && rd > 0;
    CloseHandle(hFile);
    if (!ok) return false;

    auto findStringVal = [&raw](const char* key, std::string& outVal) -> bool {
        std::string needle = std::string("\"") + key + "\"";
        size_t pos = raw.find(needle);
        if (pos == std::string::npos) return false;
        pos = raw.find(':', pos + needle.size());
        if (pos == std::string::npos) return false;
        size_t q1 = raw.find('"', pos + 1);
        if (q1 == std::string::npos) return false;
        size_t q2 = raw.find('"', q1 + 1);
        if (q2 == std::string::npos) return false;
        outVal = raw.substr(q1 + 1, q2 - q1 - 1);
        return true;
    };
    auto findDoubleVal = [&raw](const char* key, double& outVal) -> bool {
        std::string needle = std::string("\"") + key + "\"";
        size_t pos = raw.find(needle);
        if (pos == std::string::npos) return false;
        pos = raw.find(':', pos + needle.size());
        if (pos == std::string::npos) return false;
        pos++;
        while (pos < raw.size() && (raw[pos] == ' ' || raw[pos] == '\t' || raw[pos] == '\r' || raw[pos] == '\n')) pos++;
        if (pos >= raw.size()) return false;
        char* endPtr = nullptr;
        double v = strtod(raw.c_str() + pos, &endPtr);
        if (endPtr == raw.c_str() + pos) return false;
        outVal = v;
        return true;
    };
    auto findBoolVal = [&raw](const char* key, bool& outVal) -> bool {
        std::string needle = std::string("\"") + key + "\"";
        size_t pos = raw.find(needle);
        if (pos == std::string::npos) return false;
        pos = raw.find(':', pos + needle.size());
        if (pos == std::string::npos) return false;
        pos++;
        while (pos < raw.size() && (raw[pos] == ' ' || raw[pos] == '\t' || raw[pos] == '\r' || raw[pos] == '\n')) pos++;
        if (raw.compare(pos, 4, "true") == 0) { outVal = true; return true; }
        if (raw.compare(pos, 5, "false") == 0) { outVal = false; return true; }
        return false;
    };

    bool anyApplied = false;
    RoundStyle parsed = CurrentInspectorStyle();

    std::string sVal;
    if (findStringVal("shape", sVal)) {
        if (sVal == "circle") { parsed.shape = SHAPE_CIRCLE; anyApplied = true; }
        else if (sVal == "squircle") { parsed.shape = SHAPE_SQUIRCLE; anyApplied = true; }
        else if (sVal == "roundedRect" || sVal == "rounded_rect") { parsed.shape = SHAPE_ROUNDED_RECT; anyApplied = true; }
        else if (sVal == "pebble") { parsed.shape = SHAPE_PEBBLE; anyApplied = true; }
        else if (sVal == "hexagon") { parsed.shape = SHAPE_HEXAGON; anyApplied = true; }
    }
    if (findStringVal("background", sVal) || findStringVal("preset", sVal)) {
        if (sVal == "auto" || sVal == "autoAdaptive" || sVal == "adaptiveEdge") { parsed.preset = PRESET_ADAPTIVE_EDGE; anyApplied = true; }
        else if (sVal == "white" || sVal == "googleWhite") { parsed.preset = PRESET_GOOGLE_WHITE; anyApplied = true; }
        else if (sVal == "dark" || sVal == "darkMaterial") { parsed.preset = PRESET_DARK_MATERIAL; anyApplied = true; }
        else if (sVal == "pastel" || sVal == "materialPastel" || sVal == "materialYouPastel") { parsed.preset = PRESET_MATERIAL_PASTEL; anyApplied = true; }
        else if (sVal == "custom" || sVal == "customColor") { parsed.preset = PRESET_CUSTOM_COLOR; anyApplied = true; }
    }
    if (findStringVal("customHex", sVal) || findStringVal("color", sVal)) {
        if (!sVal.empty() && sVal[0] == '#') sVal.erase(0, 1);
        if (sVal.size() == 6) {
            unsigned int rgb = strtoul(sVal.c_str(), nullptr, 16);
            parsed.customColor = RGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
            parsed.preset = PRESET_CUSTOM_COLOR;
            anyApplied = true;
        }
    }
    if (findStringVal("surfaceMaterial", sVal) || findStringVal("material", sVal)) {
        if (sVal == "smooth") { parsed.material = MATERIAL_SMOOTH; anyApplied = true; }
        else if (sVal == "frostedGlass") { parsed.material = MATERIAL_FROSTED_GLASS; anyApplied = true; }
        else if (sVal == "brushedTitanium") { parsed.material = MATERIAL_BRUSHED_TITANIUM; anyApplied = true; }
        else if (sVal == "neumorphic") { parsed.material = MATERIAL_NEUMORPHIC; anyApplied = true; }
        else if (sVal == "prismGlow") { parsed.material = MATERIAL_PRISM_GLOW; anyApplied = true; }
        else if (sVal == "liquidChrome") { parsed.material = MATERIAL_LIQUID_CHROME; anyApplied = true; }
        else if (sVal == "matteCeramic") { parsed.material = MATERIAL_MATTE_CERAMIC; anyApplied = true; }
        else if (sVal == "holographic") { parsed.material = MATERIAL_HOLOGRAPHIC; anyApplied = true; }
    }

    double dVal = 0.0;
    if (findDoubleVal("scale", dVal)) { parsed.scale = std::clamp(dVal, 0.30, 2.50); anyApplied = true; }
    if (findDoubleVal("shadow", dVal)) { parsed.shadow = std::clamp(dVal, 0.0, 1.0); anyApplied = true; }

    bool bVal = false;
    if (findBoolVal("innerShadow", bVal)) { parsed.innerShadow = bVal; anyApplied = true; }
    if (findBoolVal("monochrome", bVal)) { parsed.monochrome = bVal; anyApplied = true; }
    if (findBoolVal("autoTrim", bVal)) { parsed.autoTrim = bVal; anyApplied = true; }
    if (findBoolVal("fullBleed", bVal)) { parsed.fullBleed = bVal; anyApplied = true; }

    if (anyApplied) {
        RoundStyle& dst = BeginEditStyle();
        dst = parsed;
        g_activePack = -1;
        CommitEditedStyle();
    }
    return anyApplied;
}

static void CollectPackFilesRecursive(const std::wstring& dir, int depth, std::vector<std::wstring>& outImages, std::wstring& outJson) {
    if (depth > 4 || outImages.size() >= 1000) return;
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;
        std::wstring full = dir + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            CollectPackFilesRecursive(full, depth + 1, outImages, outJson);
        } else {
            std::wstring ext = PathFindExtensionW(fd.cFileName);
            for (auto& c : ext) c = (wchar_t)towlower(c);
            if (ext == L".png" || ext == L".svg" || ext == L".ico") {
                outImages.push_back(full);
            } else if (outJson.empty() && (ext == L".modo-pack" || ext == L".modopack" || ext == L".json")) {
                outJson = full;
            }
        }
    } while (FindNextFileW(hFind, &fd) && outImages.size() < 1000);
    FindClose(hFind);
}

static void RemoveTempDirectoryRecursive(const std::wstring& dir, int depth = 0) {
    if (depth > 6 || dir.empty()) return;
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
            std::wstring full = dir + L"\\" + fd.cFileName;
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                RemoveTempDirectoryRecursive(full, depth + 1);
            } else {
                SetFileAttributesW(full.c_str(), FILE_ATTRIBUTE_NORMAL);
                DeleteFileW(full.c_str());
            }
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }
    RemoveDirectoryW(dir.c_str());
}

static void ImportIconPackDialog(HWND hwnd) {
    wchar_t filePath[MAX_PATH] = {0};
    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"Пакеты иконок Modo (*.modo-pack;*.json;*.zip)\0*.modo-pack;*.modopack;*.json;*.zip\0Все файлы (*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Выберите пакет стиля (.modo-pack / .json) или архив иконок (.zip)";

    if (!GetOpenFileNameW(&ofn) || !filePath[0]) return;

    std::wstring chosen = filePath;
    std::wstring base = PathFindFileNameW(filePath);
    std::wstring ext = PathFindExtensionW(filePath);
    for (auto& c : ext) c = (wchar_t)towlower(c);

    if (ext == L".modo-pack" || ext == L".modopack" || ext == L".json") {
        if (ApplyModoPackJsonFile(chosen)) {
            g_statusText = L"Применён пакет стиля: " + base;
            InvalidateRect(hwnd, NULL, FALSE);
            MessageBoxW(hwnd, (L"Параметры оформления из пакета '" + base + L"' успешно применены.").c_str(),
                        L"Импорт пакета стиля", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(hwnd, L"Не удалось распознать параметры стиля в выбранном JSON/.modo-pack файле.",
                        L"Ошибка импорта", MB_OK | MB_ICONWARNING);
        }
        return;
    }

    if (ext == L".zip") {
        wchar_t tempBase[MAX_PATH] = {0};
        if (!GetTempPathW(MAX_PATH, tempBase) || !tempBase[0]) {
            MessageBoxW(hwnd, L"Не удалось получить временную директорию системы.", L"Импорт архива", MB_OK | MB_ICONERROR);
            return;
        }
        wchar_t tmpDirBuf[MAX_PATH] = {0};
        wsprintfW(tmpDirBuf, L"%lsmodo_pack_%u_%u", tempBase, (unsigned)GetTickCount(), (unsigned)GetCurrentProcessId());
        std::wstring tmpDir = tmpDirBuf;
        if (!CreateDirectoryW(tmpDir.c_str(), NULL)) {
            MessageBoxW(hwnd, L"Не удалось подготовить временную папку для распаковки.", L"Импорт архива", MB_OK | MB_ICONERROR);
            return;
        }

        wchar_t sysDir[MAX_PATH] = {0};
        GetSystemDirectoryW(sysDir, MAX_PATH);
        std::wstring tarExe = std::wstring(sysDir) + L"\\tar.exe";
        std::wstring cmdLine = L"\"" + tarExe + L"\" -xf \"" + chosen + L"\" -C \"" + tmpDir + L"\"";
        std::vector<wchar_t> mutableCmd(cmdLine.begin(), cmdLine.end());
        mutableCmd.push_back(L'\0');

        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = {0};
        bool extracted = false;
        if (CreateProcessW(tarExe.c_str(), mutableCmd.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            DWORD waitRes = WaitForSingleObject(pi.hProcess, 20000);
            DWORD exitCode = 1;
            if (waitRes == WAIT_OBJECT_0 && GetExitCodeProcess(pi.hProcess, &exitCode) && exitCode == 0) {
                extracted = true;
            } else if (waitRes == WAIT_TIMEOUT) {
                TerminateProcess(pi.hProcess, 1);
            }
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }

        if (!extracted) {
            RemoveTempDirectoryRecursive(tmpDir);
            MessageBoxW(hwnd, L"Не удалось распаковать ZIP-архив системной утилитой tar.exe.", L"Импорт архива", MB_OK | MB_ICONERROR);
            return;
        }

        std::vector<std::wstring> imgFiles;
        std::wstring jsonPack;
        CollectPackFilesRecursive(tmpDir, 0, imgFiles, jsonPack);

        bool appliedStyleJson = false;
        if (!jsonPack.empty()) {
            appliedStyleJson = ApplyModoPackJsonFile(jsonPack);
        }

        int matchedApps = 0;
        for (const auto& imgPath : imgFiles) {
            std::wstring fn = PathFindFileNameW(imgPath.c_str());
            size_t dot = fn.find_last_of(L'.');
            std::wstring stem = (dot != std::wstring::npos) ? fn.substr(0, dot) : fn;
            std::wstring key = NormalizeMatchKey(stem);
            if (key.empty()) continue;

            for (auto& app : g_allApps) {
                std::wstring appKey = NormalizeMatchKey(app.name);
                std::wstring exeStem;
                if (!app.targetPath.empty()) {
                    std::wstring exeFn = PathFindFileNameW(app.targetPath.c_str());
                    size_t edot = exeFn.find_last_of(L'.');
                    exeStem = NormalizeMatchKey((edot != std::wstring::npos) ? exeFn.substr(0, edot) : exeFn);
                }
                if (key == appKey || (!exeStem.empty() && key == exeStem)) {
                    Bitmap* bmp = IconEngine::LoadSvgOrRasterArtworkFile(imgPath, 256);
                    if (bmp) {
                        SetAppCustomArtwork(app, bmp);
                        matchedApps++;
                    }
                }
            }
        }

        RemoveTempDirectoryRecursive(tmpDir);
        InvalidateRect(hwnd, NULL, FALSE);

        std::wstring summary = L"Архив: " + base + L"\n"
                             + L"Найдено графических файлов (.png/.svg/.ico): " + std::to_wstring(imgFiles.size()) + L"\n"
                             + L"Сопоставлено с приложениями в галерее: " + std::to_wstring(matchedApps);
        if (appliedStyleJson) {
            summary += L"\nПрименены параметры стиля из манифеста пакета.";
        }
        if (matchedApps == 0 && !imgFiles.empty()) {
            summary += L"\n\nПодсказка: чтобы значки сопоставлялись автоматически, имена файлов в архиве должны совпадать с названиями приложений или их .exe (например, Discord.png, Telegram.svg, chrome.png).";
        }
        g_statusText = L"Импорт '" + base + L"': сопоставлено значков — " + std::to_wstring(matchedApps);
        MessageBoxW(hwnd, summary.c_str(), L"Импорт пакета иконок", MB_OK | MB_ICONINFORMATION);
        return;
    }

    MessageBoxW(hwnd, L"Поддерживаются пакеты .modo-pack, .json и архивы .zip.", L"Импорт пакета", MB_OK | MB_ICONINFORMATION);
}

static bool IsExplorerContextMenuRegistered() {
    HKEY hKey = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\*\\shell\\ModoStudio", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

static void ExecuteMenuCommand(HWND hwnd, int cmd) {
    if (cmd == 0) return;

    POINT dummyPt = {0, 0};
    switch (cmd) {
        case IDM_FILE_ADD_CUSTOM:
            AddCustomAppOrImage(hwnd);
            break;
        case IDM_FILE_ADD_FOLDER:
            AddCustomFolder(hwnd);
            break;
        case IDM_FILE_EXPORT:
            ExportStandardIcons(hwnd);
            break;
        case IDM_FILE_EXPORT_WEB_FAVICON_KIT:
            ExportWebFaviconKitDialog(hwnd);
            break;
        case IDM_FILE_EXPORT_XCODE_CATALOG:
            ExportXcodeCatalogDialog(hwnd);
            break;
        case IDM_FILE_EXPORT_SHOWCASE:
            ExportShowcaseBannerDialog(hwnd);
            break;
        case IDM_FILE_RESCAN:
            HandleWidgetClick(hwnd, W_BTN_RESCAN, dummyPt);
            break;
        case IDM_APP_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        case IDM_EDIT_UNDO:
            PerformUndo(hwnd);
            break;
        case IDM_EDIT_REDO:
            PerformRedo(hwnd);
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
        case IDM_VIEW_MODE_GRID:
            g_viewMode = APP_VIEW_GRID;
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case IDM_VIEW_MODE_LIST:
            g_viewMode = APP_VIEW_LIST;
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case IDM_VIEW_ZOOM_IN:
            g_gridIconSize = std::clamp(g_gridIconSize + 4, 48, 96);
            SaveStyleConfig();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case IDM_VIEW_ZOOM_OUT:
            g_gridIconSize = std::clamp(g_gridIconSize - 4, 48, 96);
            SaveStyleConfig();
            InvalidateRect(hwnd, NULL, FALSE);
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
        case IDM_STYLE_RESTORE_ALL:
            HandleWidgetClick(hwnd, W_BTN_RESTORE_ALL, dummyPt);
            break;
        case IDM_STYLE_SHAPE_CIRCLE:
            HandleWidgetClick(hwnd, W_SHAPE_0, dummyPt);
            break;
        case IDM_STYLE_SHAPE_SQUIRCLE:
            HandleWidgetClick(hwnd, W_SHAPE_1, dummyPt);
            break;
        case IDM_STYLE_SHAPE_ROUNDEDRECT:
            HandleWidgetClick(hwnd, W_SHAPE_2, dummyPt);
            break;
        case IDM_STYLE_SHAPE_PEBBLE:
            HandleWidgetClick(hwnd, W_SHAPE_3, dummyPt);
            break;
        case IDM_STYLE_SHAPE_HEXAGON:
            HandleWidgetClick(hwnd, W_SHAPE_4, dummyPt);
            break;
        case IDM_STYLE_MATERIAL_SMOOTH:
        case IDM_STYLE_MATERIAL_FROSTED_GLASS:
        case IDM_STYLE_MATERIAL_BRUSHED_TITANIUM:
        case IDM_STYLE_MATERIAL_NEUMORPHIC:
        case IDM_STYLE_MATERIAL_PRISM_GLOW:
        case IDM_STYLE_MATERIAL_LIQUID_CHROME:
        case IDM_STYLE_MATERIAL_MATTE_CERAMIC:
        case IDM_STYLE_MATERIAL_HOLOGRAPHIC: {
            RoundStyle& st = BeginEditStyle();
            st.material = (SurfaceMaterial)(cmd - IDM_STYLE_MATERIAL_SMOOTH);
            g_activePack = -1;
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case IDM_STYLE_GLYPH_ORIGINAL:
        case IDM_STYLE_GLYPH_SILHOUETTE:
        case IDM_STYLE_GLYPH_EMBOSSED:
        case IDM_STYLE_GLYPH_OUTLINE:
        case IDM_STYLE_GLYPH_DUOTONE:
        case IDM_STYLE_GLYPH_GLOW: {
            RoundStyle& st = BeginEditStyle();
            st.glyphStyle = (GlyphStyleMode)(cmd - IDM_STYLE_GLYPH_ORIGINAL);
            g_activePack = -1;
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case IDM_STYLE_GLYPH_ROTATE_90: {
            RoundStyle& st = BeginEditStyle();
            st.rotation = std::fmod(st.rotation + 90.0, 360.0);
            int deg = (int)std::round(st.rotation);
            CommitEditedStyle();
            g_statusText = L"Поворот рисунка: " + std::to_wstring(deg) + L" град.";
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case IDM_STYLE_GLYPH_FLIP_H: {
            RoundStyle& st = BeginEditStyle();
            st.flipH = !st.flipH;
            bool on = st.flipH;
            CommitEditedStyle();
            g_statusText = on ? L"Отражение по горизонтали включено" : L"Отражение по горизонтали выключено";
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case IDM_STYLE_GLYPH_FLIP_V: {
            RoundStyle& st = BeginEditStyle();
            st.flipV = !st.flipV;
            bool on = st.flipV;
            CommitEditedStyle();
            g_statusText = on ? L"Отражение по вертикали включено" : L"Отражение по вертикали выключено";
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case IDM_STYLE_GLYPH_RESET_TRANSFORM: {
            RoundStyle& st = BeginEditStyle();
            st.rotation = 0.0;
            st.flipH = false;
            st.flipV = false;
            st.glyphColor = 0;
            st.glyphGrad = GLYPH_GRAD_NONE;
            CommitEditedStyle();
            g_statusText = L"Трансформации рисунка сброшены";
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case IDM_STYLE_GLYPH_GRAD_NONE: {
            RoundStyle& st = BeginEditStyle();
            st.glyphGrad = GLYPH_GRAD_NONE;
            CommitEditedStyle();
            g_statusText = L"Градиент рисунка выключен";
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case IDM_STYLE_GLYPH_GRAD_GOLD:
        case IDM_STYLE_GLYPH_GRAD_CYBER:
        case IDM_STYLE_GLYPH_GRAD_SUNSET:
        case IDM_STYLE_GLYPH_GRAD_EMERALD:
        case IDM_STYLE_GLYPH_GRAD_TITANIUM:
        case IDM_STYLE_GLYPH_GRAD_PURPLE: {
            RoundStyle& st = BeginEditStyle();
            st.glyphGrad = (GlyphGradientPreset)(cmd - IDM_STYLE_GLYPH_GRAD_NONE);
            CommitEditedStyle();
            g_statusText = L"Применен градиент рисунка";
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case IDM_STYLE_GLYPH_BADGE_NONE:
        case IDM_STYLE_GLYPH_BADGE_DOT:
        case IDM_STYLE_GLYPH_BADGE_CHECK:
        case IDM_STYLE_GLYPH_BADGE_STAR:
        case IDM_STYLE_GLYPH_BADGE_LOCK:
        case IDM_STYLE_GLYPH_BADGE_ALERT:
        case IDM_STYLE_GLYPH_BADGE_BETA:
        case IDM_STYLE_GLYPH_BADGE_PRO: {
            RoundStyle& st = BeginEditStyle();
            st.badge = (GlyphBadgeOverlay)(cmd - IDM_STYLE_GLYPH_BADGE_NONE);
            bool none = (st.badge == BADGE_NONE);
            CommitEditedStyle();
            g_statusText = none ? L"Бейдж статуса отключен" : L"Применен бейдж статуса рисунка";
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case IDM_STYLE_GLYPH_BADGE_POS_TR:
        case IDM_STYLE_GLYPH_BADGE_POS_BR:
        case IDM_STYLE_GLYPH_BADGE_POS_TL:
        case IDM_STYLE_GLYPH_BADGE_POS_BL: {
            RoundStyle& st = BeginEditStyle();
            st.badgePos = (GlyphBadgePosition)(cmd - IDM_STYLE_GLYPH_BADGE_POS_TR);
            CommitEditedStyle();
            g_statusText = L"Обновлена позиция бейджа статуса";
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case IDM_TOOLS_CREATE_MONOGRAM:
            PromptMonogramDialog(hwnd);
            break;
        case IDM_TOOLS_SURPRISE_ARTWORK: {
            RoundStyle& st = BeginEditStyle();
            int randomGlyph = rand() % 6;
            st.glyphStyle = (GlyphStyleMode)randomGlyph;
            CommitEditedStyle();
            g_statusText = L"Применён случайный стиль рисунка #" + std::to_wstring(randomGlyph + 1);
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case IDM_STYLE_EYEDROPPER:
            HandleWidgetClick(hwnd, W_BTN_EYEDROPPER, dummyPt);
            break;
        case IDM_STYLE_WINDOWS_ACCENT: {
            COLORREF accent = GetWindowsAccentColor();
            RoundStyle& st = BeginEditStyle();
            st.customColor = accent;
            st.preset = PRESET_CUSTOM_COLOR;
            g_activePack = -1;
            CommitEditedStyle();
            wchar_t buf[128];
            wsprintfW(buf, L"Применён акцентный цвет темы Windows (#%02X%02X%02X)",
                      GetRValue(accent), GetGValue(accent), GetBValue(accent));
            g_statusText = buf;
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case IDM_STYLE_RESET_DEFAULT:
            HandleWidgetClick(hwnd, W_PACK_0, dummyPt);
            break;
        case IDM_TOOLS_BATCH_CONVERT:
            BatchConvertImagesDialog(hwnd);
            break;
        case IDM_TOOLS_CONTEXT_MENU: {
            bool currentlyOn = IsExplorerContextMenuRegistered();
            wchar_t exePath[MAX_PATH * 2] = {0};
            GetModuleFileNameW(NULL, exePath, MAX_PATH * 2 - 1);
            if (currentlyOn) {
                if (MessageBoxW(hwnd,
                                L"Интеграция с контекстным меню Проводника сейчас активна.\n\nОтключить пункт «Оформить иконку в Modo» в контекстном меню Windows?",
                                L"Контекстное меню Проводника",
                                MB_YESNO | MB_ICONQUESTION) == IDYES) {
                    ShellIntegration::RegisterShellContextMenu(L"", false);
                    g_statusText = L"Интеграция с контекстным меню Проводника отключена";
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            } else {
                if (ShellIntegration::RegisterShellContextMenu(exePath, true)) {
                    g_statusText = L"Пункт «Оформить иконку в Modo» добавлен в меню Проводника";
                    InvalidateRect(hwnd, NULL, FALSE);
                    MessageBoxW(hwnd,
                                L"Интеграция с контекстным меню Проводника включена.\n\nТеперь вы можете нажать правой кнопкой мыши на любой файл, ярлык или папку в Проводнике и открыть их напрямую в Modo Studio.",
                                L"Контекстное меню Проводника",
                                MB_OK | MB_ICONINFORMATION);
                } else {
                    MessageBoxW(hwnd, L"Не удалось обновить ключи реестра текущего пользователя.", L"Ошибка интеграции", MB_OK | MB_ICONERROR);
                }
            }
            break;
        }
        case IDM_TOOLS_ADMIN:
            HandleWidgetClick(hwnd, W_BTN_ADMIN, dummyPt);
            break;
        case IDM_TOOLS_SHORTCUTS:
            HandleWidgetClick(hwnd, W_BTN_SHORTCUTS, dummyPt);
            break;
        case IDM_TOOLS_IMPORT_PACK:
            ImportIconPackDialog(hwnd);
            break;
        case IDM_TOOLS_RESTART_EXPLORER:
            HandleWidgetClick(hwnd, W_BTN_RESTART_EXPLORER, dummyPt);
            break;
        case IDM_TOOLS_SYSTEM_THEMING: {
            std::wstring msg = std::wstring(L"Запустить мастер глобальной стилизации (1-Click System Theming)?\n\n"
                               L"Выбранная форма и стиль оформления будут применены ко всем отфильтрованным объектам (")
                             + std::to_wstring(g_filteredIndices.size())
                             + L" шт.) с автоматическим обновлением кэша иконок Windows.\n\nПродолжить?";
            if (MessageBoxW(hwnd, msg.c_str(), L"Modo — Мастер глобальной стилизации", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                HandleWidgetClick(hwnd, W_BTN_APPLY_ALL, dummyPt);
            }
            break;
        }
        case IDM_TOOLS_SHORTCUTS_INFO:
            MessageBoxW(
                hwnd,
                L"Горячие клавиши Modo Studio (Windows):\n\n"
                L"  - Ctrl+Z / Ctrl+Y: Отменить / повторить изменение стиля\n"
                L"  - Ctrl+1 / Ctrl+2: Сетка карточек / Компактный список\n"
                L"  - Ctrl++ / Ctrl+-: Изменить масштаб карточек сетки\n"
                L"  - Стрелки: Навигация по приложениям\n"
                L"  - Enter: Применить адаптивную иконку к выбранному\n"
                L"  - Shift+Enter: Применить ко всем отфильтрованным\n"
                L"  - Delete: Восстановить оригинальную иконку\n"
                L"  - Shift+Delete: Восстановить оригиналы для всех\n"
                L"  - Ctrl+R: Повернуть рисунок на +90 градусов\n"
                L"  - Ctrl+Shift+M: Создать типографическую монограмму\n"
                L"  - Ctrl+Shift+R: Случайный стиль рисунка\n"
                L"  - Ctrl+Shift+T: Мастер глобальной стилизации\n"
                L"  - Ctrl+Shift+A: Применить акцентный цвет Windows\n"
                L"  - Ctrl+Shift+B: Пакетная конвертация изображений\n"
                L"  - Ctrl+N / Ctrl+O: Добавить приложение или изображение\n"
                L"  - Ctrl+Shift+O: Добавить папку для стилизации\n"
                L"  - Ctrl+E: Экспорт иконки (.ICO + 1024px .PNG)\n"
                L"  - Ctrl+F: Поиск приложения в списке\n"
                L"  - F5: Обновить список приложений\n"
                L"  - Alt / Alt+M / F1: Главное меню Modo\n"
                L"  - Esc: Сброс поиска / отмена пипетки",
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
                HandleWidgetClick(hwnd, (WidgetID)(W_PACK_0 + packIdx), dummyPt);
                break;
            }
            int lucideCount = (int)(sizeof(kLucideMenuIcons) / sizeof(kLucideMenuIcons[0]));
            if (cmd >= IDM_TOOLS_LUCIDE_BASE && cmd < IDM_TOOLS_LUCIDE_BASE + lucideCount) {
                int idx = cmd - IDM_TOOLS_LUCIDE_BASE;
                if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
                    auto& app = g_allApps[g_selectedAppIndex];
                    bool darkPlate = IsStyleDarkSurface(GetEffectiveStyle(app));
                    Color strokeCol = darkPlate ? Color(255, 244, 244, 248) : Color(255, 30, 30, 34);
                    SetAppCustomArtwork(app, IconEngine::RenderLucideVectorBitmap(kLucideMenuIcons[idx].id, strokeCol, 256));
                    g_statusText = std::wstring(L"Применен вектор Lucide: ") + kLucideMenuIcons[idx].label;
                    InvalidateRect(hwnd, NULL, FALSE);
                } else {
                    MessageBoxW(hwnd, L"Пожалуйста, выберите приложение в списке для замены рисунка.", L"Вектор Lucide", MB_OK | MB_ICONINFORMATION);
                }
                break;
            }
            if (cmd >= IDM_TOOLS_BRANDS_BASE && cmd < IDM_TOOLS_BRANDS_BASE + kSimpleIconsCount) {
                int idx = cmd - IDM_TOOLS_BRANDS_BASE;
                if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
                    auto& app = g_allApps[g_selectedAppIndex];
                    bool darkPlate = IsStyleDarkSurface(GetEffectiveStyle(app));
                    SetAppCustomArtwork(app, IconEngine::RenderSimpleIconBitmap(kSimpleIconsCurated[idx].slug, Color(0, 0, 0, 0), 256, darkPlate));
                    g_statusText = std::wstring(L"Применен официальный вектор бренда: ") + kSimpleIconsCurated[idx].title;
                    InvalidateRect(hwnd, NULL, FALSE);
                } else {
                    MessageBoxW(hwnd, L"Пожалуйста, выберите приложение в списке для замены рисунка.", L"Бренды Simple Icons", MB_OK | MB_ICONINFORMATION);
                }
                break;
            }
            if (cmd >= IDM_TOOLS_PATTERNS_BASE && cmd < IDM_TOOLS_PATTERNS_BASE + 8) {
                int idx = cmd - IDM_TOOLS_PATTERNS_BASE;
                if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
                    auto& app = g_allApps[g_selectedAppIndex];
                    const RoundStyle& eff = GetEffectiveStyle(app);
                    Color pCol(255, 10, 132, 255);
                    Color sCol(255, 94, 92, 230);
                    if (eff.preset == PRESET_CUSTOM_COLOR) {
                        pCol = Color(255, GetRValue(eff.customColor), GetGValue(eff.customColor), GetBValue(eff.customColor));
                    }
                    SetAppCustomArtwork(app, IconEngine::RenderProceduralGeometricBitmap(idx, pCol, sCol, 256));
                    g_statusText = L"Применен процедурный AI-паттерн #" + std::to_wstring(idx + 1) + L" для " + app.name;
                    InvalidateRect(hwnd, NULL, FALSE);
                } else {
                    MessageBoxW(hwnd, L"Пожалуйста, выберите приложение в списке для замены рисунка.", L"AI Паттерны", MB_OK | MB_ICONINFORMATION);
                }
                break;
            }
            break;
        }
    }
}

static void ShowModoAppMenu(HWND hwnd, POINT screenPt) {
    const RoundStyle& curSt = CurrentInspectorStyle();
    HMENU hMainMenu = CreatePopupMenu();

    // 1. File Menu
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_ADD_CUSTOM, L"Добавить приложение или файл...\tCtrl+N");
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_ADD_FOLDER, L"Добавить папку для стилизации...\tCtrl+Shift+O");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_EXPORT, L"Экспорт иконки (.ICO, .PNG)...\tCtrl+E");
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_EXPORT_WEB_FAVICON_KIT, L"Экспорт Web Favicon Kit & Manifest...");
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_EXPORT_XCODE_CATALOG, L"Экспорт Xcode Asset Catalog (.appiconset)...");
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_EXPORT_SHOWCASE, L"Showcase & README Баннер (1920x1080)...");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_RESCAN, L"Обновить список приложений\tF5");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFileMenu, MF_STRING, IDM_APP_EXIT, L"Закрыть Modo\tAlt+F4");
    AppendMenuW(hMainMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"Файл");

    // 2. Edit Menu
    HMENU hEditMenu = CreatePopupMenu();
    AppendMenuW(hEditMenu, (g_undoStack.empty() ? MF_GRAYED : MF_ENABLED) | MF_STRING, IDM_EDIT_UNDO, L"Отменить изменение стиля\tCtrl+Z");
    AppendMenuW(hEditMenu, (g_redoStack.empty() ? MF_GRAYED : MF_ENABLED) | MF_STRING, IDM_EDIT_REDO, L"Повторить изменение стиля\tCtrl+Y");
    AppendMenuW(hEditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_FIND, L"Найти приложение...\tCtrl+F");
    AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_CLEAR_SEARCH, L"Сбросить поиск\tEsc");
    AppendMenuW(hMainMenu, MF_POPUP, (UINT_PTR)hEditMenu, L"Правка");

    // 3. View Menu
    HMENU hViewMenu = CreatePopupMenu();
    AppendMenuW(hViewMenu, (g_isDarkMode ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_VIEW_DARK_THEME, L"Тёмная тема (Монохром)");
    AppendMenuW(hViewMenu, (!g_isDarkMode ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_VIEW_LIGHT_THEME, L"Светлая тема (Монохром)");
    AppendMenuW(hViewMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hViewMenu, (g_viewMode == APP_VIEW_GRID ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_VIEW_MODE_GRID, L"Галерея карточек (Сетка)\tCtrl+1");
    AppendMenuW(hViewMenu, (g_viewMode == APP_VIEW_LIST ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_VIEW_MODE_LIST, L"Компактный список\tCtrl+2");
    AppendMenuW(hViewMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_ZOOM_IN, L"Увеличить значки сетки\tCtrl++");
    AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_ZOOM_OUT, L"Уменьшить значки сетки\tCtrl+-");
    AppendMenuW(hViewMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hViewMenu, (g_sidebarFilter == 0 ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_VIEW_FILTER_ALL, L"Все приложения");
    AppendMenuW(hViewMenu, (g_sidebarFilter == 1 ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_VIEW_FILTER_APPLIED, L"Стилизованные");
    AppendMenuW(hViewMenu, (g_sidebarFilter == 2 ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_VIEW_FILTER_SYSTEM, L"Системные приложения");
    AppendMenuW(hViewMenu, (g_sidebarFilter == 3 ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_VIEW_FILTER_THIRDPARTY, L"Сторонние приложения");
    AppendMenuW(hMainMenu, MF_POPUP, (UINT_PTR)hViewMenu, L"Вид");

    // 4. Style Menu
    HMENU hStyleMenu = CreatePopupMenu();
    AppendMenuW(hStyleMenu, MF_STRING, IDM_STYLE_APPLY_ONE, L"Применить к выбранному\tEnter");
    AppendMenuW(hStyleMenu, MF_STRING, IDM_STYLE_APPLY_ALL, L"Применить ко всем\tShift+Enter");
    AppendMenuW(hStyleMenu, MF_STRING, IDM_STYLE_RESTORE_ONE, L"Восстановить оригинал\tDelete");
    AppendMenuW(hStyleMenu, MF_STRING, IDM_STYLE_RESTORE_ALL, L"Восстановить оригиналы для всех...\tShift+Delete");
    AppendMenuW(hStyleMenu, MF_SEPARATOR, 0, NULL);

    HMENU hShapeSub = CreatePopupMenu();
    AppendMenuW(hShapeSub, (curSt.shape == SHAPE_CIRCLE ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_SHAPE_CIRCLE, L"Круг (Classic Circle)");
    AppendMenuW(hShapeSub, (curSt.shape == SHAPE_SQUIRCLE ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_SHAPE_SQUIRCLE, L"Сквиркл (Squircle)");
    AppendMenuW(hShapeSub, (curSt.shape == SHAPE_ROUNDED_RECT ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_SHAPE_ROUNDEDRECT, L"Скругленный прямоугольник (Rounded Rect)");
    AppendMenuW(hShapeSub, (curSt.shape == SHAPE_PEBBLE ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_SHAPE_PEBBLE, L"Галька (Pebble)");
    AppendMenuW(hShapeSub, (curSt.shape == SHAPE_HEXAGON ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_SHAPE_HEXAGON, L"Шестиугольник (Hexagon)");
    AppendMenuW(hStyleMenu, MF_POPUP, (UINT_PTR)hShapeSub, L"Форма подложки (Surface)");

    HMENU hMatSub = CreatePopupMenu();
    AppendMenuW(hMatSub, (curSt.material == MATERIAL_SMOOTH ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_MATERIAL_SMOOTH, L"Гладкий (Smooth Flat)");
    AppendMenuW(hMatSub, (curSt.material == MATERIAL_FROSTED_GLASS ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_MATERIAL_FROSTED_GLASS, L"Матовое стекло (VisionOS Frosted Glass)");
    AppendMenuW(hMatSub, (curSt.material == MATERIAL_BRUSHED_TITANIUM ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_MATERIAL_BRUSHED_TITANIUM, L"Шлифованный металл (Brushed Titanium)");
    AppendMenuW(hMatSub, (curSt.material == MATERIAL_NEUMORPHIC ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_MATERIAL_NEUMORPHIC, L"Неоморфизм (Neumorphic Emboss)");
    AppendMenuW(hMatSub, (curSt.material == MATERIAL_PRISM_GLOW ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_MATERIAL_PRISM_GLOW, L"Призма (Prism Chromatic)");
    AppendMenuW(hMatSub, (curSt.material == MATERIAL_LIQUID_CHROME ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_MATERIAL_LIQUID_CHROME, L"Жидкий хром (Liquid Chrome)");
    AppendMenuW(hMatSub, (curSt.material == MATERIAL_MATTE_CERAMIC ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_MATERIAL_MATTE_CERAMIC, L"Матовая керамика (Matte Ceramic)");
    AppendMenuW(hMatSub, (curSt.material == MATERIAL_HOLOGRAPHIC ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_MATERIAL_HOLOGRAPHIC, L"Голографическая фольга (Holo Foil)");
    AppendMenuW(hStyleMenu, MF_POPUP, (UINT_PTR)hMatSub, L"Материал и текстура");

    HMENU hGlyphSub = CreatePopupMenu();
    AppendMenuW(hGlyphSub, (curSt.glyphStyle == GLYPH_ORIGINAL ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_ORIGINAL, L"Оригинальный рисунок (Original Artwork)");
    AppendMenuW(hGlyphSub, (curSt.glyphStyle == GLYPH_SILHOUETTE ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_SILHOUETTE, L"Плоский силуэт (Silhouette)");
    AppendMenuW(hGlyphSub, (curSt.glyphStyle == GLYPH_EMBOSSED ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_EMBOSSED, L"3D Барельеф (Embossed)");
    AppendMenuW(hGlyphSub, (curSt.glyphStyle == GLYPH_OUTLINE ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_OUTLINE, L"Контурный штрих (Outline)");
    AppendMenuW(hGlyphSub, (curSt.glyphStyle == GLYPH_DUOTONE ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_DUOTONE, L"Дуотон пастель (Duotone)");
    AppendMenuW(hGlyphSub, (curSt.glyphStyle == GLYPH_GLOW ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_GLOW, L"Неоновое свечение (Neon Glow)");
    AppendMenuW(hGlyphSub, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hGlyphSub, MF_STRING, IDM_STYLE_GLYPH_ROTATE_90, L"Повернуть рисунок на +90 град.\tCtrl+R");
    AppendMenuW(hGlyphSub, (curSt.flipH ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_FLIP_H, L"Отразить по горизонтали (Flip H)");
    AppendMenuW(hGlyphSub, (curSt.flipV ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_FLIP_V, L"Отразить по вертикали (Flip V)");
    AppendMenuW(hGlyphSub, MF_STRING, IDM_STYLE_GLYPH_RESET_TRANSFORM, L"Сбросить трансформации рисунка");
    AppendMenuW(hGlyphSub, MF_SEPARATOR, 0, NULL);

    HMENU hTintSub = CreatePopupMenu();
    AppendMenuW(hTintSub, (curSt.glyphGrad == GLYPH_GRAD_NONE ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_GRAD_NONE, L"Оригинал (без градиента)");
    AppendMenuW(hTintSub, (curSt.glyphGrad == GLYPH_GRAD_GOLD ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_GRAD_GOLD, L"Liquid Gold");
    AppendMenuW(hTintSub, (curSt.glyphGrad == GLYPH_GRAD_CYBER ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_GRAD_CYBER, L"Cyber Neon");
    AppendMenuW(hTintSub, (curSt.glyphGrad == GLYPH_GRAD_SUNSET ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_GRAD_SUNSET, L"Sunset Glow");
    AppendMenuW(hTintSub, (curSt.glyphGrad == GLYPH_GRAD_EMERALD ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_GRAD_EMERALD, L"Emerald Chrome");
    AppendMenuW(hTintSub, (curSt.glyphGrad == GLYPH_GRAD_TITANIUM ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_GRAD_TITANIUM, L"Titanium Ice");
    AppendMenuW(hTintSub, (curSt.glyphGrad == GLYPH_GRAD_PURPLE_HAZE ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_GRAD_PURPLE, L"Purple Haze");
    AppendMenuW(hGlyphSub, MF_POPUP, (UINT_PTR)hTintSub, L"Градиентная заливка рисунка (Gradient Mask)");

    HMENU hBadgeSub = CreatePopupMenu();
    AppendMenuW(hBadgeSub, (curSt.badge == BADGE_NONE ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_BADGE_NONE, L"Без бейджа (None)");
    AppendMenuW(hBadgeSub, (curSt.badge == BADGE_DOT ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_BADGE_DOT, L"Точка уведомления (Notification Dot)");
    AppendMenuW(hBadgeSub, (curSt.badge == BADGE_CHECKMARK ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_BADGE_CHECK, L"Проверено (Verified Check)");
    AppendMenuW(hBadgeSub, (curSt.badge == BADGE_STAR ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_BADGE_STAR, L"Звезда / Избранное (Star)");
    AppendMenuW(hBadgeSub, (curSt.badge == BADGE_LOCK ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_BADGE_LOCK, L"Замок / Приватность (Lock)");
    AppendMenuW(hBadgeSub, (curSt.badge == BADGE_ALERT ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_BADGE_ALERT, L"Предупреждение (Alert)");
    AppendMenuW(hBadgeSub, (curSt.badge == BADGE_BETA ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_BADGE_BETA, L"Метка Beta (Beta Tag)");
    AppendMenuW(hBadgeSub, (curSt.badge == BADGE_PRO ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_BADGE_PRO, L"Метка Pro (Pro Tag)");
    AppendMenuW(hBadgeSub, MF_SEPARATOR, 0, NULL);

    HMENU hBadgePosSub = CreatePopupMenu();
    AppendMenuW(hBadgePosSub, (curSt.badgePos == BADGE_POS_TOP_RIGHT ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_BADGE_POS_TR, L"Сверху справа (Top Right)");
    AppendMenuW(hBadgePosSub, (curSt.badgePos == BADGE_POS_BOTTOM_RIGHT ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_BADGE_POS_BR, L"Снизу справа (Bottom Right)");
    AppendMenuW(hBadgePosSub, (curSt.badgePos == BADGE_POS_TOP_LEFT ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_BADGE_POS_TL, L"Сверху слева (Top Left)");
    AppendMenuW(hBadgePosSub, (curSt.badgePos == BADGE_POS_BOTTOM_LEFT ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, IDM_STYLE_GLYPH_BADGE_POS_BL, L"Снизу слева (Bottom Left)");
    AppendMenuW(hBadgeSub, MF_POPUP, (UINT_PTR)hBadgePosSub, L"Позиция бейджа (Position)");

    AppendMenuW(hGlyphSub, MF_POPUP, (UINT_PTR)hBadgeSub, L"Бейдж статуса (Badge Overlay)");
    AppendMenuW(hStyleMenu, MF_POPUP, (UINT_PTR)hGlyphSub, L"Оформление рисунка (Glyph Style)");

    HMENU hPackSub = CreatePopupMenu();
    int totalPacks = (int)(sizeof(kIconPacks) / sizeof(kIconPacks[0]));
    for (int i = 0; i < totalPacks; ++i) {
        UINT flags = (g_activePack == i ? MF_CHECKED : MF_UNCHECKED) | MF_STRING;
        AppendMenuW(hPackSub, flags, IDM_STYLE_PACK_BASE + i, kIconPacks[i].name);
    }
    AppendMenuW(hStyleMenu, MF_POPUP, (UINT_PTR)hPackSub, L"Пакеты стилей (Icon Packs)");

    AppendMenuW(hStyleMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hStyleMenu, MF_STRING, IDM_STYLE_EYEDROPPER, L"Пипетка цвета фона...");
    AppendMenuW(hStyleMenu, MF_STRING, IDM_STYLE_WINDOWS_ACCENT, L"Применить цвет темы Windows (Accent Color)\tCtrl+Shift+A");
    AppendMenuW(hStyleMenu, MF_STRING, IDM_STYLE_RESET_DEFAULT, L"Сбросить стиль по умолчанию");
    AppendMenuW(hMainMenu, MF_POPUP, (UINT_PTR)hStyleMenu, L"Стиль");

    // 5. Tools Menu
    HMENU hToolsMenu = CreatePopupMenu();
    AppendMenuW(hToolsMenu, MF_STRING, IDM_TOOLS_IMPORT_PACK, L"Импортировать пакет иконок (.zip / .modo-pack)...");
    AppendMenuW(hToolsMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hToolsMenu, MF_STRING, IDM_TOOLS_CREATE_MONOGRAM, L"Создать типографическую монограмму...\tCtrl+Shift+M");

    HMENU hLucideSub = BuildLucidePopupMenu();
    AppendMenuW(hToolsMenu, MF_POPUP, (UINT_PTR)hLucideSub, L"Каталог интерфейсных иконок Lucide (43 значка)...");

    HMENU hBrandsMenu = CreatePopupMenu();
    HMENU hWeb = CreatePopupMenu();
    HMENU hDev = CreatePopupMenu();
    HMENU hComms = CreatePopupMenu();
    HMENU hMedia = CreatePopupMenu();
    HMENU hAI = CreatePopupMenu();

    for (int i = 0; i < kSimpleIconsCount; ++i) {
        std::wstring cat = kSimpleIconsCurated[i].category;
        HMENU hTarget = hAI;
        if (cat == L"Браузеры и веб") hTarget = hWeb;
        else if (cat == L"Разработка и код") hTarget = hDev;
        else if (cat == L"Мессенджеры и соцсети") hTarget = hComms;
        else if (cat == L"Медиа, игры и дизайн") hTarget = hMedia;
        AppendMenuW(hTarget, MF_STRING, IDM_TOOLS_BRANDS_BASE + i, kSimpleIconsCurated[i].title);
    }
    AppendMenuW(hBrandsMenu, MF_POPUP, (UINT_PTR)hWeb, L"Браузеры и веб");
    AppendMenuW(hBrandsMenu, MF_POPUP, (UINT_PTR)hDev, L"Разработка и код");
    AppendMenuW(hBrandsMenu, MF_POPUP, (UINT_PTR)hComms, L"Мессенджеры и соцсети");
    AppendMenuW(hBrandsMenu, MF_POPUP, (UINT_PTR)hMedia, L"Медиа, игры и дизайн");
    AppendMenuW(hBrandsMenu, MF_POPUP, (UINT_PTR)hAI, L"Продуктивность и ИИ");
    AppendMenuW(hToolsMenu, MF_POPUP, (UINT_PTR)hBrandsMenu, L"Каталог брендов Simple Icons (65+ логотипов)...");

    HMENU hPatternsMenu = CreatePopupMenu();
    const wchar_t* pNames[8] = {
        L"Celestial Sparkle (ИИ звезда)",
        L"Iris Aperture (Апертура)",
        L"Planetary Orbit (Орбита / атом)",
        L"Hexacore (Квантовое ядро)",
        L"Faceted Prism (Призма)",
        L"Cyber Shield (Кибер-щит)",
        L"Code Terminal (Терминал)",
        L"Sound Waveform (Аудио-волна)"
    };
    for (int i = 0; i < 8; ++i) {
        AppendMenuW(hPatternsMenu, MF_STRING, IDM_TOOLS_PATTERNS_BASE + i, pNames[i]);
    }
    AppendMenuW(hToolsMenu, MF_POPUP, (UINT_PTR)hPatternsMenu, L"Процедурные AI Паттерны...");

    AppendMenuW(hToolsMenu, MF_STRING, IDM_TOOLS_SURPRISE_ARTWORK, L"Мне повезёт (Случайный стиль рисунка)\tCtrl+Shift+R");
    AppendMenuW(hToolsMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hToolsMenu, MF_STRING, IDM_TOOLS_BATCH_CONVERT, L"Пакетная конвертация изображений в иконки...\tCtrl+Shift+B");
    AppendMenuW(hToolsMenu, MF_STRING, IDM_TOOLS_SYSTEM_THEMING, L"Мастер глобальной стилизации (1-Click Theming)...\tCtrl+Shift+T");
    AppendMenuW(hToolsMenu, MF_SEPARATOR, 0, NULL);
    UINT ctxFlags = (IsExplorerContextMenuRegistered() ? MF_CHECKED : MF_UNCHECKED) | MF_STRING;
    AppendMenuW(hToolsMenu, ctxFlags, IDM_TOOLS_CONTEXT_MENU, L"Интеграция в контекстное меню Проводника");
    AppendMenuW(hToolsMenu, MF_STRING, IDM_TOOLS_ADMIN, L"Перезапустить от имени Администратора...");
    AppendMenuW(hToolsMenu, MF_STRING, IDM_TOOLS_SHORTCUTS, L"Создать ярлыки Modo на Рабочем столе...");
    AppendMenuW(hToolsMenu, MF_STRING, IDM_TOOLS_RESTART_EXPLORER, L"Перезапустить Проводник Windows...");
    AppendMenuW(hMainMenu, MF_POPUP, (UINT_PTR)hToolsMenu, L"Инструменты");

    // 6. Help Menu
    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenuW(hHelpMenu, MF_STRING, IDM_APP_CHECK_UPDATES, L"Проверить обновления...");
    AppendMenuW(hHelpMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hHelpMenu, MF_STRING, IDM_TOOLS_SHORTCUTS_INFO, L"Горячие клавиши...\tF1");
    AppendMenuW(hHelpMenu, MF_STRING, IDM_APP_ABOUT, L"О программе Modo Studio...");
    AppendMenuW(hMainMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"Справка");

    int cmd = TrackPopupMenu(
        hMainMenu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON,
        screenPt.x, screenPt.y,
        0, hwnd, NULL
    );
    DestroyMenu(hMainMenu);

    if (cmd != 0) {
        ExecuteMenuCommand(hwnd, cmd);
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

        case W_TOGGLE_PER_APP:
            g_adjustThisAppOnly = !g_adjustThisAppOnly;
            SaveStyleConfig();
            InvalidateHeroCache();
            g_statusText = g_adjustThisAppOnly
                ? L"Режим инспектора: индивидуальный стиль выбранного объекта"
                : L"Режим инспектора: единый глобальный стиль";
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case W_SEARCH_BOX:
            g_searchFocused = true;
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case W_SEARCH_CLEAR:
            g_searchText.clear();
            RebuildFilteredList();
            InvalidateHeroCache();
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case W_BTN_APP_VIEW_GRID:
            g_viewMode = APP_VIEW_GRID;
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case W_BTN_APP_VIEW_LIST:
            g_viewMode = APP_VIEW_LIST;
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case W_BTN_RESTART_EXPLORER: {
            if (MessageBoxW(
                    hwnd,
                    L"Перезапустить Проводник Windows и сбросить дисковый кэш иконок?\n\n"
                    L"Открытые окна папок Проводника будут временно закрыты и панель задач перезапустится.",
                    L"Modo Studio — Перезапуск Проводника",
                    MB_YESNO | MB_ICONQUESTION) == IDYES) {
                RefreshWindowsExplorerIconCache(true);
                g_statusText = L"Проводник Windows перезапущен, дисковый кэш иконок успешно очищен";
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }

        case W_INSP_TAB_STYLE:
            g_inspectorTab = INSP_TAB_STYLE;
            g_inspectorScrollY = 0;
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case W_INSP_TAB_ARTWORK:
            g_inspectorTab = INSP_TAB_ARTWORK;
            g_inspectorScrollY = 0;
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case W_INSP_TAB_GEOMETRY:
            g_inspectorTab = INSP_TAB_GEOMETRY;
            g_inspectorScrollY = 0;
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case W_INSP_TAB_OPTIONS:
            g_inspectorTab = INSP_TAB_OPTIONS;
            g_inspectorScrollY = 0;
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case W_BTN_ART_FILE: {
            if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
                AppEntry& app = g_allApps[g_selectedAppIndex];
                Bitmap* newBmp = PromptUserForArtworkFile(hwnd);
                if (newBmp) {
                    SetAppCustomArtwork(app, newBmp);
                    g_statusText = L"Загружен рисунок значка для: " + app.name;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            } else {
                MessageBoxW(hwnd, L"Выберите приложение в галерее для замены рисунка.", L"Замена рисунка", MB_OK | MB_ICONINFORMATION);
            }
            break;
        }

        case W_BTN_ART_PASTE: {
            if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
                AppEntry& app = g_allApps[g_selectedAppIndex];
                Bitmap* clipBmp = CreateBitmapFromClipboard();
                if (clipBmp) {
                    SetAppCustomArtwork(app, clipBmp);
                    g_statusText = L"Вставлен рисунок из буфера обмена для: " + app.name;
                    InvalidateRect(hwnd, NULL, FALSE);
                } else {
                    MessageBoxW(hwnd, L"В буфере обмена Windows нет подходящего изображения.", L"Буфер обмена", MB_OK | MB_ICONWARNING);
                }
            } else {
                MessageBoxW(hwnd, L"Выберите приложение в галерее для вставки рисунка.", L"Буфер обмена", MB_OK | MB_ICONINFORMATION);
            }
            break;
        }

        case W_BTN_ART_LUCIDE: {
            if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
                HMENU hLucideSub = BuildLucidePopupMenu();
                POINT screenPt = pt;
                ClientToScreen(hwnd, &screenPt);
                int cmd = TrackPopupMenu(hLucideSub, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON, screenPt.x, screenPt.y, 0, hwnd, NULL);
                DestroyMenu(hLucideSub);
                if (cmd != 0) ExecuteMenuCommand(hwnd, cmd);
            } else {
                MessageBoxW(hwnd, L"Выберите приложение в галерее для выбора вектора Lucide.", L"Каталог Lucide", MB_OK | MB_ICONINFORMATION);
            }
            break;
        }

        case W_BTN_ART_BRANDS: {
            POINT screenPt = pt;
            ClientToScreen(hwnd, &screenPt);
            ShowSimpleIconsMenu(hwnd, screenPt);
            break;
        }

        case W_BTN_ART_AI_PATTERNS: {
            POINT screenPt = pt;
            ClientToScreen(hwnd, &screenPt);
            ShowAIPatternsMenu(hwnd, screenPt);
            break;
        }

        case W_BTN_ART_MONOGRAM: {
            PromptMonogramDialog(hwnd);
            break;
        }

        case W_BTN_ART_RESET: {
            if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
                AppEntry& app = g_allApps[g_selectedAppIndex];
                SetAppCustomArtwork(app, nullptr);
                app.customRotation = 0.0;
                app.isFlippedH = false;
                app.isFlippedV = false;
                app.customGlyphGrad = GLYPH_GRAD_NONE;
                app.customBadge = BADGE_NONE;
                RoundStyle& st = BeginEditStyle();
                st.rotation = 0.0;
                st.flipH = false;
                st.flipV = false;
                st.glyphGrad = GLYPH_GRAD_NONE;
                st.badge = BADGE_NONE;
                CommitEditedStyle();
                g_statusText = L"Восстановлен исходный системный рисунок: " + app.name;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }

        case W_BTN_ART_ROT_L: {
            RoundStyle& st = BeginEditStyle();
            st.rotation = std::fmod(st.rotation - 90.0 + 360.0, 360.0);
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case W_BTN_ART_ROT_R: {
            RoundStyle& st = BeginEditStyle();
            st.rotation = std::fmod(st.rotation + 90.0, 360.0);
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case W_BTN_ART_FLIP_H: {
            RoundStyle& st = BeginEditStyle();
            st.flipH = !st.flipH;
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case W_BTN_ART_FLIP_V: {
            RoundStyle& st = BeginEditStyle();
            st.flipV = !st.flipV;
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case W_BTN_ART_RESET_TRANS: {
            RoundStyle& st = BeginEditStyle();
            st.rotation = 0.0;
            st.flipH = false;
            st.flipV = false;
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case W_BTN_ART_GRAD_0:
        case W_BTN_ART_GRAD_1:
        case W_BTN_ART_GRAD_2:
        case W_BTN_ART_GRAD_3:
        case W_BTN_ART_GRAD_4:
        case W_BTN_ART_GRAD_5:
        case W_BTN_ART_GRAD_6: {
            RoundStyle& st = BeginEditStyle();
            st.glyphGrad = (GlyphGradientPreset)(id - W_BTN_ART_GRAD_0);
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case W_BTN_ART_BADGE_0:
        case W_BTN_ART_BADGE_1:
        case W_BTN_ART_BADGE_2:
        case W_BTN_ART_BADGE_3:
        case W_BTN_ART_BADGE_4:
        case W_BTN_ART_BADGE_5:
        case W_BTN_ART_BADGE_6:
        case W_BTN_ART_BADGE_7: {
            RoundStyle& st = BeginEditStyle();
            st.badge = (GlyphBadgeOverlay)(id - W_BTN_ART_BADGE_0);
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case W_BTN_ART_BPOS_TR:
        case W_BTN_ART_BPOS_BR:
        case W_BTN_ART_BPOS_TL:
        case W_BTN_ART_BPOS_BL: {
            RoundStyle& st = BeginEditStyle();
            st.badgePos = (GlyphBadgePosition)(id - W_BTN_ART_BPOS_TR);
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case W_GRID_AREA: {
            RectF gridArea;
            if (GetWidgetRect(W_GRID_AREA, gridArea)) {
                int iconSize = g_gridIconSize;
                int cardW = iconSize + 48;
                int cardH = iconSize + 58;
                int gapX = 10;
                int gapY = 10;
                int numCols = std::max(1, ((int)gridArea.Width - 20) / (cardW + gapX));
                int relX = pt.x - (int)gridArea.X - 10;
                int relY = pt.y - (int)gridArea.Y + g_gridScrollY - 10;
                if (relX >= 0 && relY >= 0) {
                    int col = relX / (cardW + gapX);
                    int row = relY / (cardH + gapY);
                    int cardRelX = relX % (cardW + gapX);
                    int cardRelY = relY % (cardH + gapY);
                    if (col < numCols && cardRelX < cardW && cardRelY < cardH) {
                        int idx = row * numCols + col;
                        if (idx >= 0 && idx < (int)g_filteredIndices.size()) {
                            if (g_selectedAppIndex != g_filteredIndices[idx]) {
                                g_selectedAppIndex = g_filteredIndices[idx];
                                InvalidateHeroCache();
                            }
                            InvalidateRect(hwnd, NULL, FALSE);
                        }
                    }
                }
            }
            break;
        }

        case W_TAB_ALL:
            g_sidebarFilter = 0;
            RebuildFilteredList();
            InvalidateHeroCache();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case W_TAB_APPLIED:
            g_sidebarFilter = 1;
            RebuildFilteredList();
            InvalidateHeroCache();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case W_TAB_SYSTEM:
            g_sidebarFilter = 2;
            RebuildFilteredList();
            InvalidateHeroCache();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        case W_TAB_THIRDPARTY:
            g_sidebarFilter = 3;
            RebuildFilteredList();
            InvalidateHeroCache();
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case W_APP_LIST_AREA: {
            RectF listArea;
            if (GetWidgetRect(W_APP_LIST_AREA, listArea)) {
                int relY = pt.y - (int)listArea.Y + g_listScrollY;
                int row = relY / 48;
                if (row >= 0 && row < (int)g_filteredIndices.size()) {
                    if (g_selectedAppIndex != g_filteredIndices[row]) {
                        g_selectedAppIndex = g_filteredIndices[row];
                        InvalidateHeroCache();
                    }
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

        case W_SHAPE_0: case W_SHAPE_1: case W_SHAPE_2: case W_SHAPE_3: case W_SHAPE_4: {
            RoundStyle& st = BeginEditStyle();
            st.shape = (IconShape)(id - W_SHAPE_0);
            g_activePack = -1;
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case W_MATERIAL_0: case W_MATERIAL_1: case W_MATERIAL_2: case W_MATERIAL_3:
        case W_MATERIAL_4: case W_MATERIAL_5: case W_MATERIAL_6: case W_MATERIAL_7: {
            RoundStyle& st = BeginEditStyle();
            st.material = (SurfaceMaterial)(id - W_MATERIAL_0);
            g_activePack = -1;
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case W_GLYPH_0: case W_GLYPH_1: case W_GLYPH_2:
        case W_GLYPH_3: case W_GLYPH_4: case W_GLYPH_5: {
            RoundStyle& st = BeginEditStyle();
            st.glyphStyle = (GlyphStyleMode)(id - W_GLYPH_0);
            g_activePack = -1;
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case W_PACK_0:  case W_PACK_1:  case W_PACK_2:  case W_PACK_3:
        case W_PACK_4:  case W_PACK_5:  case W_PACK_6:  case W_PACK_7:
        case W_PACK_8:  case W_PACK_9:  case W_PACK_10: case W_PACK_11:
        case W_PACK_12: case W_PACK_13: case W_PACK_14: case W_PACK_15:
        case W_PACK_16: case W_PACK_17: case W_PACK_18: {
            int idx = id - W_PACK_0;
            int totalPacks = (int)(sizeof(kIconPacks) / sizeof(kIconPacks[0]));
            if (idx >= 0 && idx < totalPacks) {
                const IconPackDef& pk = kIconPacks[idx];
                RoundStyle& st = BeginEditStyle();
                st.preset = pk.preset;
                st.shape = pk.shape;
                st.scale = pk.scale;
                st.shadow = pk.shadow;
                st.innerShadow = pk.innerShadow;
                st.monochrome = pk.monochrome;
                st.customColor = pk.customColor;
                st.material = pk.material;
                st.glyphStyle = pk.glyphStyle;
                st.offsetX = 0.0;
                st.offsetY = 0.0;
                if (!g_adjustThisAppOnly) {
                    g_activePack = idx;
                }
                CommitEditedStyle();
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }

        case W_PRESET_0:
        case W_PRESET_1:
        case W_PRESET_2:
        case W_PRESET_3:
        case W_PRESET_4: {
            RoundStyle& st = BeginEditStyle();
            st.preset = (BgPreset)(id - W_PRESET_0);
            g_activePack = -1;
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case W_BTN_EYEDROPPER: {
            const RoundStyle& cur = CurrentInspectorStyle();
            g_eyedropperPrevStyle = cur;
            PushUndoSnapshot();
            g_isEyedropperActive = true;
            g_eyedropperLiveColor = cur.customColor;
            SetCapture(hwnd);
            SetCursor(LoadCursor(NULL, IDC_CROSS));
            g_statusText = L"Пипетка активна: кликните точку на экране (ПКМ или Esc для отмены)";
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case W_BTN_PALETTE: {
            static COLORREF cust[16] = {0};
            const RoundStyle& cur = CurrentInspectorStyle();
            CHOOSECOLORW cc = {0};
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = hwnd;
            cc.lpCustColors = cust;
            cc.rgbResult = cur.customColor;
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColorW(&cc)) {
                RoundStyle& st = BeginEditStyle();
                st.customColor = cc.rgbResult;
                st.preset = PRESET_CUSTOM_COLOR;
                g_activePack = -1;
                CommitEditedStyle();
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
        case W_SWATCH_7: {
            RoundStyle& st = BeginEditStyle();
            st.customColor = kSwatchPalette[id - W_SWATCH_0];
            st.preset = PRESET_CUSTOM_COLOR;
            g_activePack = -1;
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case W_SWATCH_WIN_ACCENT: {
            COLORREF accent = GetWindowsAccentColor();
            RoundStyle& st = BeginEditStyle();
            st.customColor = accent;
            st.preset = PRESET_CUSTOM_COLOR;
            g_activePack = -1;
            CommitEditedStyle();
            wchar_t buf[128];
            wsprintfW(buf, L"Выбран акцентный цвет темы Windows (#%02X%02X%02X)",
                      GetRValue(accent), GetGValue(accent), GetBValue(accent));
            g_statusText = buf;
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case W_BTN_SC_68:  { RoundStyle& st = BeginEditStyle(); st.scale = 0.68; CommitEditedStyle(); InvalidateRect(hwnd, NULL, FALSE); break; }
        case W_BTN_SC_100: { RoundStyle& st = BeginEditStyle(); st.scale = 1.00; CommitEditedStyle(); InvalidateRect(hwnd, NULL, FALSE); break; }
        case W_BTN_SC_135: { RoundStyle& st = BeginEditStyle(); st.scale = 1.35; CommitEditedStyle(); InvalidateRect(hwnd, NULL, FALSE); break; }
        case W_BTN_SC_175: { RoundStyle& st = BeginEditStyle(); st.scale = 1.75; CommitEditedStyle(); InvalidateRect(hwnd, NULL, FALSE); break; }

        case W_BTN_LEFT: {
            RoundStyle& st = BeginEditStyle();
            st.offsetX = std::clamp(st.offsetX - 0.05, -1.0, 1.0);
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case W_BTN_RIGHT: {
            RoundStyle& st = BeginEditStyle();
            st.offsetX = std::clamp(st.offsetX + 0.05, -1.0, 1.0);
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case W_BTN_DOWN: {
            RoundStyle& st = BeginEditStyle();
            st.offsetY = std::clamp(st.offsetY - 0.05, -1.0, 1.0);
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case W_BTN_UP: {
            RoundStyle& st = BeginEditStyle();
            st.offsetY = std::clamp(st.offsetY + 0.05, -1.0, 1.0);
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case W_BTN_CENTER: {
            RoundStyle& st = BeginEditStyle();
            st.offsetX = 0.0;
            st.offsetY = 0.0;
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case W_TOGGLE_AUTOTRIM: {
            RoundStyle& st = BeginEditStyle();
            st.autoTrim = !st.autoTrim;
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case W_TOGGLE_INNERSHAD: {
            RoundStyle& st = BeginEditStyle();
            st.innerShadow = !st.innerShadow;
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case W_TOGGLE_FULLBLEED: {
            RoundStyle& st = BeginEditStyle();
            st.fullBleed = !st.fullBleed;
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case W_TOGGLE_MONO: {
            RoundStyle& st = BeginEditStyle();
            st.monochrome = !st.monochrome;
            CommitEditedStyle();
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case W_BTN_APPLY_ONE: {
            if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
                SaveStyleConfig();
                AppEntry& app = g_allApps[g_selectedAppIndex];
                if (app.hasCustomStyle) SavePerAppOverride(app);
                bool needsAdmin = false;
                ApplyRoundIcon(app, &needsAdmin);
                RefreshWindowsExplorerIconCache(app.isFileExplorer);
                g_statusText = L"Готово: иконка применена к " + app.name;
                InvalidateRect(hwnd, NULL, FALSE);
                if (needsAdmin) {
                    int ans = MessageBoxW(
                        hwnd,
                        L"Этот ярлык находится в общей системной папке (Public Desktop / ProgramData).\n\n"
                        L"Обновить этот ярлык с правами Администратора?",
                        L"Modo Studio — Системный ярлык",
                        MB_YESNO | MB_ICONQUESTION
                    );
                    if (ans == IDYES) {
                        wchar_t selfPath[MAX_PATH * 2] = {0};
                        if (GetModuleFileNameW(NULL, selfPath, MAX_PATH * 2 - 1)) {
                            std::wstring targetArg = !app.lnkPath.empty() ? app.lnkPath : app.name;
                            std::wstring args = L"--apply-single-elevated \"" + targetArg + L"\"";
                            if ((INT_PTR)ShellExecuteW(hwnd, L"runas", selfPath, args.c_str(), NULL, SW_SHOWNORMAL) > 32) {
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
            bool anyExplorer = false;
            for (size_t i = 0; i < g_filteredIndices.size(); ++i) {
                AppEntry& app = g_allApps[g_filteredIndices[i]];
                if (app.isFileExplorer) anyExplorer = true;
                bool na = false;
                if (ApplyRoundIcon(app, &na)) count++;
                if (na) anyNeedsAdmin = true;
                g_progressRatio = (double)(i + 1) / (double)g_filteredIndices.size();
                g_statusText = L"Применение иконок: " + app.name;
                InvalidateRect(hwnd, NULL, FALSE);
                UpdateWindow(hwnd);
            }
            RefreshWindowsExplorerIconCache(anyExplorer);
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

            std::wstring msg = L"Готово: стиль применён к " + std::to_wstring(count) + L" объектам.";
            g_statusText = msg;
            InvalidateRect(hwnd, NULL, FALSE);
            MessageBoxW(hwnd, msg.c_str(), L"Modo Studio", MB_ICONINFORMATION);
            break;
        }

        case W_BTN_RESTORE_ONE: {
            if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
                AppEntry& app = g_allApps[g_selectedAppIndex];
                SetAppCustomArtwork(app, nullptr);
                app.hasCustomStyle = false;
                SavePerAppOverride(app);
                RestoreOriginalEntry(app);
                InvalidateHeroCache();
                RefreshWindowsExplorerIconCache(app.isFileExplorer);
                g_statusText = L"Восстановлена оригинальная иконка для " + app.name;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }

        case W_BTN_RESTORE_ALL: {
            if (MessageBoxW(
                    hwnd,
                    L"Сбросить пользовательские стили и вернуть оригинальные системные иконки для ВСЕХ объектов?",
                    L"Modo Studio — Восстановление всех иконок",
                    MB_YESNO | MB_ICONQUESTION) != IDYES) {
                break;
            }
            bool anyExplorer = false;
            for (auto& app : g_allApps) {
                if (app.isFileExplorer && app.isApplied) anyExplorer = true;
                SetAppCustomArtwork(app, nullptr);
                app.hasCustomStyle = false;
                SavePerAppOverride(app);
                RestoreOriginalEntry(app);
            }
            InvalidateAllThumbs();
            RefreshWindowsExplorerIconCache(anyExplorer);
            g_statusText = L"Оригинальные иконки восстановлены для всех приложений";
            InvalidateRect(hwnd, NULL, FALSE);
            MessageBoxW(hwnd, L"Оригинальные иконки восстановлены для всех приложений.", L"Modo Studio", MB_ICONINFORMATION);
            break;
        }

        case W_BTN_EXPORT: {
            RectF expR;
            if (GetWidgetRect(W_BTN_EXPORT, expR)) {
                POINT p = { (LONG)expR.X, (LONG)expR.Y - 2 };
                ClientToScreen(hwnd, &p);
                ShowExportMenu(hwnd, p);
            } else {
                ExportStandardIcons(hwnd);
            }
            break;
        }

        default:
            break;
    }
}

static LRESULT CALLBACK StudioWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ERASEBKGND:
            return 1;

        case WM_COMMAND:
            ExecuteMenuCommand(hwnd, LOWORD(wParam));
            return 0;

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

        case WM_DROPFILES: {
            HDROP hDrop = (HDROP)wParam;
            UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
            int added = 0;
            for (UINT i = 0; i < count; ++i) {
                wchar_t droppedPath[MAX_PATH * 2] = {0};
                if (DragQueryFileW(hDrop, i, droppedPath, MAX_PATH * 2 - 1)) {
                    if (AddFileOrAppPath(hwnd, droppedPath)) {
                        added++;
                    }
                }
            }
            DragFinish(hDrop);
            if (added > 1) {
                g_statusText = L"Перетащено объектов: " + std::to_wstring(added);
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            g_cursorClientPt = pt;

            if (g_isEyedropperActive) {
                POINT screenPt = pt;
                ClientToScreen(hwnd, &screenPt);
                COLORREF chosen = g_eyedropperLiveColor;
                HDC screenDC = GetDC(NULL);
                if (screenDC) {
                    COLORREF sampled = GetPixel(screenDC, screenPt.x, screenPt.y);
                    ReleaseDC(NULL, screenDC);
                    if (sampled != CLR_INVALID) {
                        chosen = sampled;
                    }
                }
                RoundStyle& st = BeginEditStyle(false);
                st.customColor = chosen;
                st.preset = PRESET_CUSTOM_COLOR;
                g_activePack = -1;
                CommitEditedStyle();
                g_isEyedropperActive = false;
                ReleaseCapture();
                wchar_t msgBuf[128];
                wsprintfW(msgBuf, L"Пипетка: выбран цвет #%02X%02X%02X",
                          GetRValue(chosen), GetGValue(chosen), GetBValue(chosen));
                g_statusText = msgBuf;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            WidgetID hit = HitTestWidget(pt);
            g_activeWidget = hit;
            if (hit != W_SEARCH_BOX) g_searchFocused = false;

            if (hit == W_STAGE_ROUND_DRAG) {
                PushUndoSnapshot();
                const RoundStyle& cur = CurrentInspectorStyle();
                g_isDraggingStage = true;
                g_dragStartPt = pt;
                g_dragStartOffX = cur.offsetX;
                g_dragStartOffY = cur.offsetY;
                SetCapture(hwnd);
                return 0;
            }

            if (hit == W_SLIDER_SCALE || hit == W_SLIDER_OFFX || hit == W_SLIDER_OFFY ||
                hit == W_SLIDER_SHADOW || hit == W_SLIDER_GRID_SIZE) {
                if (hit != W_SLIDER_GRID_SIZE) {
                    PushUndoSnapshot();
                }
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
                RoundStyle& st = BeginEditStyle();
                st.offsetX = 0.0;
                st.offsetY = 0.0;
                CommitEditedStyle();
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
                RoundStyle& st = BeginEditStyle(false);
                st = g_eyedropperPrevStyle;
                CommitEditedStyle();
                g_statusText = L"Пипетка отменена, предыдущий цвет восстановлен";
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            break;
        }

        case WM_MOUSEMOVE: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            g_cursorClientPt = pt;

            if (!g_mouseTracking) {
                TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
                TrackMouseEvent(&tme);
                g_mouseTracking = true;
            }

            if (g_isEyedropperActive) {
                POINT screenPt = pt;
                ClientToScreen(hwnd, &screenPt);
                HDC screenDC = GetDC(NULL);
                if (screenDC) {
                    COLORREF sampled = GetPixel(screenDC, screenPt.x, screenPt.y);
                    ReleaseDC(NULL, screenDC);
                    if (sampled != CLR_INVALID) {
                        g_eyedropperLiveColor = sampled;
                        RoundStyle& st = BeginEditStyle(false);
                        st.customColor = sampled;
                        st.preset = PRESET_CUSTOM_COLOR;
                        InvalidateHeroCache();
                    }
                }
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            if (g_isDraggingStage) {
                double dx = (double)(pt.x - g_dragStartPt.x) / 56.0;
                double dy = -(double)(pt.y - g_dragStartPt.y) / 56.0;
                RoundStyle& st = BeginEditStyle(false);
                st.offsetX = std::clamp(g_dragStartOffX + dx, -1.0, 1.0);
                st.offsetY = std::clamp(g_dragStartOffY + dy, -1.0, 1.0);
                InvalidateHeroCache();
                g_heroTiltX = std::clamp((float)((pt.x - g_dragStartPt.x) / 45.0), -1.0f, 1.0f);
                g_heroTiltY = std::clamp((float)((pt.y - g_dragStartPt.y) / 45.0), -1.0f, 1.0f);
                g_heroTiltHovered = true;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            if (g_activeWidget == W_SLIDER_SCALE || g_activeWidget == W_SLIDER_OFFX ||
                g_activeWidget == W_SLIDER_OFFY  || g_activeWidget == W_SLIDER_SHADOW ||
                g_activeWidget == W_SLIDER_GRID_SIZE) {
                UpdateSliderFromMouse(g_activeWidget, pt.x);
                return 0;
            }

            WidgetID prevHov = g_hoveredWidget;
            int prevRow = g_hoveredListRow;
            bool prevTiltHover = g_heroTiltHovered;
            float prevTiltX = g_heroTiltX;
            float prevTiltY = g_heroTiltY;

            g_hoveredWidget = HitTestWidget(pt);
            g_hoveredListRow = -1;

            // Interactive Hero Stage Parallax Tilt
            RectF stageRect;
            if (GetWidgetRect(W_STAGE_ROUND_DRAG, stageRect)) {
                if (pt.x >= stageRect.X && pt.x <= stageRect.GetRight() &&
                    pt.y >= stageRect.Y && pt.y <= stageRect.GetBottom()) {
                    float relX = ((float)pt.x - (stageRect.X + stageRect.Width * 0.5f)) / (stageRect.Width * 0.5f);
                    float relY = ((float)pt.y - (stageRect.Y + stageRect.Height * 0.5f)) / (stageRect.Height * 0.5f);
                    g_heroTiltX = std::clamp(relX, -1.0f, 1.0f);
                    g_heroTiltY = std::clamp(relY, -1.0f, 1.0f);
                    g_heroTiltHovered = true;
                    if (g_tiltTimerActive) {
                        KillTimer(hwnd, TIMER_ID_TILT_SPRING);
                        g_tiltTimerActive = false;
                    }
                } else if (g_heroTiltHovered && !g_isDraggingStage) {
                    g_heroTiltHovered = false;
                    if (!g_tiltTimerActive && (std::abs(g_heroTiltX) > 0.005f || std::abs(g_heroTiltY) > 0.005f)) {
                        g_tiltTimerActive = true;
                        SetTimer(hwnd, TIMER_ID_TILT_SPRING, 16, NULL);
                    }
                }
            }

            if (g_hoveredWidget == W_APP_LIST_AREA) {
                RectF listArea;
                if (GetWidgetRect(W_APP_LIST_AREA, listArea)) {
                    int row = (pt.y - (int)listArea.Y + g_listScrollY) / 48;
                    if (row >= 0 && row < (int)g_filteredIndices.size()) {
                        g_hoveredListRow = row;
                    }
                }
            } else if (g_hoveredWidget == W_GRID_AREA) {
                RectF gridArea;
                if (GetWidgetRect(W_GRID_AREA, gridArea)) {
                    int iconSize = g_gridIconSize;
                    int cardW = iconSize + 48;
                    int cardH = iconSize + 58;
                    int gapX = 10;
                    int gapY = 10;
                    int numCols = std::max(1, ((int)gridArea.Width - 20) / (cardW + gapX));
                    int relX = pt.x - (int)gridArea.X - 10;
                    int relY = pt.y - (int)gridArea.Y + g_gridScrollY - 10;
                    if (relX >= 0 && relY >= 0) {
                        int col = relX / (cardW + gapX);
                        int row = relY / (cardH + gapY);
                        int cardRelX = relX % (cardW + gapX);
                        int cardRelY = relY % (cardH + gapY);
                        if (col < numCols && cardRelX < cardW && cardRelY < cardH) {
                            int idx = row * numCols + col;
                            if (idx >= 0 && idx < (int)g_filteredIndices.size()) {
                                g_hoveredListRow = idx;
                            }
                        }
                    }
                }
            }

            bool tiltChanged = (prevTiltHover != g_heroTiltHovered) ||
                               (std::abs(prevTiltX - g_heroTiltX) > 0.01f) ||
                               (std::abs(prevTiltY - g_heroTiltY) > 0.01f);

            if (prevHov != g_hoveredWidget || prevRow != g_hoveredListRow || tiltChanged) {
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_MOUSELEAVE: {
            g_mouseTracking = false;
            if (g_heroTiltHovered && !g_isDraggingStage) {
                g_heroTiltHovered = false;
                if (!g_tiltTimerActive && (std::abs(g_heroTiltX) > 0.005f || std::abs(g_heroTiltY) > 0.005f)) {
                    g_tiltTimerActive = true;
                    SetTimer(hwnd, TIMER_ID_TILT_SPRING, 16, NULL);
                }
            }
            g_hoveredWidget = W_NONE;
            g_hoveredListRow = -1;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        case WM_TIMER: {
            if (wParam == TIMER_ID_TILT_SPRING) {
                bool stillMoving = false;
                if (!g_heroTiltHovered && !g_isDraggingStage) {
                    const float damping = 0.70f;
                    g_heroTiltX *= damping;
                    g_heroTiltY *= damping;
                    if (std::abs(g_heroTiltX) > 0.005f || std::abs(g_heroTiltY) > 0.005f) {
                        stillMoving = true;
                    } else {
                        g_heroTiltX = 0.0f;
                        g_heroTiltY = 0.0f;
                    }
                }
                InvalidateRect(hwnd, NULL, FALSE);
                if (!stillMoving) {
                    KillTimer(hwnd, TIMER_ID_TILT_SPRING);
                    g_tiltTimerActive = false;
                }
                return 0;
            }
            break;
        }

        case WM_LBUTTONUP: {
            if (g_isDraggingStage) {
                g_isDraggingStage = false;
                ReleaseCapture();
                CommitEditedStyle();
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                RectF stageRect;
                if (GetWidgetRect(W_STAGE_ROUND_DRAG, stageRect) &&
                    pt.x >= stageRect.X && pt.x <= stageRect.GetRight() &&
                    pt.y >= stageRect.Y && pt.y <= stageRect.GetBottom()) {
                    float relX = ((float)pt.x - (stageRect.X + stageRect.Width * 0.5f)) / (stageRect.Width * 0.5f);
                    float relY = ((float)pt.y - (stageRect.Y + stageRect.Height * 0.5f)) / (stageRect.Height * 0.5f);
                    g_heroTiltX = std::clamp(relX, -1.0f, 1.0f);
                    g_heroTiltY = std::clamp(relY, -1.0f, 1.0f);
                    g_heroTiltHovered = true;
                } else {
                    g_heroTiltHovered = false;
                    if (!g_tiltTimerActive && (std::abs(g_heroTiltX) > 0.005f || std::abs(g_heroTiltY) > 0.005f)) {
                        g_tiltTimerActive = true;
                        SetTimer(hwnd, TIMER_ID_TILT_SPRING, 16, NULL);
                    }
                }
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if (g_activeWidget != W_NONE) {
                if (g_activeWidget == W_SLIDER_SCALE || g_activeWidget == W_SLIDER_OFFX ||
                    g_activeWidget == W_SLIDER_OFFY  || g_activeWidget == W_SLIDER_SHADOW ||
                    g_activeWidget == W_SLIDER_GRID_SIZE) {
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
            RECT rc; GetClientRect(hwnd, &rc);
            int clientW = rc.right;

            if (pt.x < 210) {
                // Left sidebar icon packs scroll
                g_sidebarScrollY -= (delta > 0) ? 35 : -35;
                g_sidebarScrollY = std::max(0, g_sidebarScrollY);
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            } else if (pt.x >= 210 && pt.x < clientW - 360) {
                // Center gallery
                if (g_viewMode == APP_VIEW_GRID) {
                    g_gridScrollY -= (delta > 0) ? 65 : -65;
                    g_gridScrollY = std::max(0, g_gridScrollY);
                } else {
                    g_listScrollY -= (delta > 0) ? 48 : -48;
                    g_listScrollY = std::max(0, g_listScrollY);
                }
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            } else {
                // Right inspector: zoom over preview stage, otherwise scroll inspector tabs
                if (HitTestWidget(pt) == W_STAGE_ROUND_DRAG) {
                    double step = (delta > 0) ? 0.05 : -0.05;
                    RoundStyle& st = BeginEditStyle();
                    st.scale = std::clamp(st.scale + step, 0.30, 2.50);
                    CommitEditedStyle();
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
                g_inspectorScrollY -= (delta > 0) ? 36 : -36;
                g_inspectorScrollY = std::clamp(g_inspectorScrollY, 0, g_maxInspectorScroll);
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
                InvalidateHeroCache();
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            break;
        }

        case WM_KEYDOWN: {
            bool ctrlDown = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            bool shiftDown = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

            if (ctrlDown && !shiftDown && (wParam == 'Z' || wParam == 'z')) {
                PerformUndo(hwnd);
                return 0;
            }
            if (ctrlDown && ((wParam == 'Y' || wParam == 'y') || (shiftDown && (wParam == 'Z' || wParam == 'z')))) {
                PerformRedo(hwnd);
                return 0;
            }
            if (ctrlDown && wParam == '1') {
                g_viewMode = APP_VIEW_GRID;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if (ctrlDown && wParam == '2') {
                g_viewMode = APP_VIEW_LIST;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if (ctrlDown && (wParam == VK_OEM_PLUS || wParam == VK_ADD || wParam == 0xBB)) {
                g_gridIconSize = std::clamp(g_gridIconSize + 4, 48, 96);
                SaveStyleConfig();
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if (ctrlDown && (wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT || wParam == 0xBD)) {
                g_gridIconSize = std::clamp(g_gridIconSize - 4, 48, 96);
                SaveStyleConfig();
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if (wParam == VK_ESCAPE) {
                if (g_isEyedropperActive) {
                    g_isEyedropperActive = false;
                    ReleaseCapture();
                    RoundStyle& st = BeginEditStyle(false);
                    st = g_eyedropperPrevStyle;
                    CommitEditedStyle();
                    g_statusText = L"Пипетка отменена, предыдущий цвет восстановлен";
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
                if (g_searchFocused) {
                    g_searchFocused = false;
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
            }
            if (ctrlDown && shiftDown && (wParam == 'T' || wParam == 't')) {
                ExecuteMenuCommand(hwnd, IDM_TOOLS_SYSTEM_THEMING);
                return 0;
            }
            if (ctrlDown && shiftDown && (wParam == 'B' || wParam == 'b')) {
                BatchConvertImagesDialog(hwnd);
                return 0;
            }
            if (ctrlDown && shiftDown && (wParam == 'A' || wParam == 'a')) {
                ExecuteMenuCommand(hwnd, IDM_STYLE_WINDOWS_ACCENT);
                return 0;
            }
            if (ctrlDown && shiftDown && (wParam == 'M' || wParam == 'm')) {
                ExecuteMenuCommand(hwnd, IDM_TOOLS_CREATE_MONOGRAM);
                return 0;
            }
            if (ctrlDown && shiftDown && (wParam == 'R' || wParam == 'r')) {
                ExecuteMenuCommand(hwnd, IDM_TOOLS_SURPRISE_ARTWORK);
                return 0;
            }
            if (ctrlDown && !shiftDown && (wParam == 'R' || wParam == 'r')) {
                ExecuteMenuCommand(hwnd, IDM_STYLE_GLYPH_ROTATE_90);
                return 0;
            }
            if (ctrlDown && (wParam == 'F' || wParam == 'f')) {
                g_searchFocused = true;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if (ctrlDown && shiftDown && (wParam == 'O' || wParam == 'o')) {
                AddCustomFolder(hwnd);
                return 0;
            }
            if (ctrlDown && !shiftDown && (wParam == 'N' || wParam == 'n' || wParam == 'O' || wParam == 'o')) {
                AddCustomAppOrImage(hwnd);
                return 0;
            }
            if (ctrlDown && (wParam == 'E' || wParam == 'e')) {
                ExportStandardIcons(hwnd);
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
                if (wParam == VK_DOWN || wParam == VK_UP || wParam == VK_LEFT || wParam == VK_RIGHT) {
                    int pos = 0;
                    for (size_t i = 0; i < g_filteredIndices.size(); ++i) {
                        if (g_filteredIndices[i] == g_selectedAppIndex) { pos = (int)i; break; }
                    }
                    int nextPos = pos;
                    RECT rc;
                    GetClientRect(hwnd, &rc);
                    int clientW = rc.right - rc.left;
                    int clientH = rc.bottom - rc.top;
                    int sideW = 210;
                    int inspW = 360;
                    int galW = std::max(200, clientW - sideW - inspW);
                    int galBodyH = std::max(60, clientH - 54 - 32 - 46 - 26);

                    if (g_viewMode == APP_VIEW_GRID) {
                        int cardW = g_gridIconSize + 48;
                        int cardH = g_gridIconSize + 58;
                        int numCols = std::max(1, (galW - 20) / (cardW + 10));

                        if (wParam == VK_LEFT) nextPos = pos - 1;
                        else if (wParam == VK_RIGHT) nextPos = pos + 1;
                        else if (wParam == VK_UP) nextPos = pos - numCols;
                        else if (wParam == VK_DOWN) nextPos = pos + numCols;

                        nextPos = std::clamp(nextPos, 0, (int)g_filteredIndices.size() - 1);
                        if (g_selectedAppIndex != g_filteredIndices[nextPos]) {
                            g_selectedAppIndex = g_filteredIndices[nextPos];
                            InvalidateHeroCache();
                        }

                        // Auto-scroll to keep visible
                        int targetRow = nextPos / numCols;
                        int itemTop = 10 + targetRow * (cardH + 10);
                        int itemBottom = itemTop + cardH;
                        if (itemTop < g_gridScrollY) {
                            g_gridScrollY = itemTop;
                        } else if (itemBottom > g_gridScrollY + galBodyH) {
                            g_gridScrollY = std::max(0, itemBottom - galBodyH);
                        }
                    } else {
                        if (wParam == VK_UP || wParam == VK_LEFT) nextPos = pos - 1;
                        else if (wParam == VK_DOWN || wParam == VK_RIGHT) nextPos = pos + 1;

                        nextPos = std::clamp(nextPos, 0, (int)g_filteredIndices.size() - 1);
                        if (g_selectedAppIndex != g_filteredIndices[nextPos]) {
                            g_selectedAppIndex = g_filteredIndices[nextPos];
                            InvalidateHeroCache();
                        }

                        // Auto-scroll in list mode (48px row stride)
                        int itemTop = nextPos * 48;
                        int itemBottom = itemTop + 48;
                        if (itemTop < g_listScrollY) {
                            g_listScrollY = itemTop;
                        } else if (itemBottom > g_listScrollY + galBodyH) {
                            g_listScrollY = std::max(0, itemBottom - galBodyH);
                        }
                    }
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
                if (wParam == VK_RETURN) {
                    POINT dummyPt = {0, 0};
                    if (shiftDown) {
                        HandleWidgetClick(hwnd, W_BTN_APPLY_ALL, dummyPt);
                    } else {
                        HandleWidgetClick(hwnd, W_BTN_APPLY_ONE, dummyPt);
                    }
                    return 0;
                }
                if (wParam == VK_DELETE) {
                    POINT dummyPt = {0, 0};
                    if (shiftDown) {
                        HandleWidgetClick(hwnd, W_BTN_RESTORE_ALL, dummyPt);
                    } else {
                        HandleWidgetClick(hwnd, W_BTN_RESTORE_ONE, dummyPt);
                    }
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
            if (g_tiltTimerActive) {
                KillTimer(hwnd, TIMER_ID_TILT_SPRING);
                g_tiltTimerActive = false;
            }
            InvalidateHeroCache();
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

    std::wstring openTargetPath;
    std::wstring applySingleTarget;

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) {
        int cliExitCode = 0;
        if (ModoCLI::TryDispatchCLI(argc, argv, cliExitCode, &g_pngClsid)) {
            LocalFree(argv);
            GdiplusShutdown(g_gdiplusToken);
            CoUninitialize();
            return cliExitCode;
        }
        for (int i = 1; i < argc; ++i) {
            if (_wcsicmp(argv[i], L"--open-target") == 0 && i + 1 < argc) {
                openTargetPath = argv[++i];
            } else if (_wcsicmp(argv[i], L"--apply-single-elevated") == 0 && i + 1 < argc) {
                applySingleTarget = argv[++i];
            }
        }
        LocalFree(argv);
    }

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
    DragAcceptFiles(g_hMainWnd, TRUE);

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    ScanInstalledWindowsApps();

    if (!openTargetPath.empty()) {
        AddFileOrAppPath(g_hMainWnd, openTargetPath);
    }

    InvalidateRect(g_hMainWnd, NULL, FALSE);

    // Asynchronously check for updates in background (silent check)
    ModoUpdater::CheckForUpdates(g_hMainWnd, true);

    if (!applySingleTarget.empty()) {
        int foundIdx = -1;
        for (size_t i = 0; i < g_allApps.size(); ++i) {
            if (_wcsicmp(g_allApps[i].lnkPath.c_str(), applySingleTarget.c_str()) == 0 ||
                _wcsicmp(g_allApps[i].targetPath.c_str(), applySingleTarget.c_str()) == 0 ||
                _wcsicmp(g_allApps[i].name.c_str(), applySingleTarget.c_str()) == 0) {
                foundIdx = (int)i;
                break;
            }
        }
        if (foundIdx < 0 && GetFileAttributesW(applySingleTarget.c_str()) != INVALID_FILE_ATTRIBUTES) {
            if (AddFileOrAppPath(g_hMainWnd, applySingleTarget)) {
                foundIdx = g_selectedAppIndex;
            }
        }
        if (foundIdx >= 0 && foundIdx < (int)g_allApps.size()) {
            g_selectedAppIndex = foundIdx;
            InvalidateHeroCache();
            AppEntry& app = g_allApps[foundIdx];
            ApplyRoundIcon(app, nullptr);
            RefreshWindowsExplorerIconCache(app.isFileExplorer);
            std::wstring doneMsg = L"Системный ярлык обновлён: " + app.name;
            g_statusText = doneMsg;
            InvalidateRect(g_hMainWnd, NULL, FALSE);
            MessageBoxW(g_hMainWnd, doneMsg.c_str(), L"Modo Studio (Администратор)", MB_ICONINFORMATION);
        }
    } else if (lpCmdLine && wcsstr(lpCmdLine, L"--apply-all-elevated") != nullptr) {
        int count = 0;
        bool anyExplorer = false;
        for (size_t i = 0; i < g_allApps.size(); ++i) {
            if (g_allApps[i].isFileExplorer) anyExplorer = true;
            if (ApplyRoundIcon(g_allApps[i], nullptr)) count++;
            g_progressRatio = (double)(i + 1) / (double)std::max<size_t>(1, g_allApps.size());
            g_statusText = L"Применение иконок (Администратор): " + g_allApps[i].name;
            InvalidateRect(g_hMainWnd, NULL, FALSE);
            UpdateWindow(g_hMainWnd);
        }
        RefreshWindowsExplorerIconCache(anyExplorer);
        g_progressRatio = 0.0;
        std::wstring doneMsg = L"Все ярлыки (" + std::to_wstring(count) + L" шт.) успешно обновлены.";
        g_statusText = doneMsg;
        InvalidateRect(g_hMainWnd, NULL, FALSE);
        MessageBoxW(g_hMainWnd, doneMsg.c_str(), L"Modo Studio (Администратор)", MB_ICONINFORMATION);
    }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    InvalidateHeroCache();
    for (auto& app : g_allApps) {
        delete app.origBitmap;
        delete app.customArtwork;
        delete app.cachedThumb;
    }
    GdiplusShutdown(g_gdiplusToken);
    CoUninitialize();
    return (int)msg.wParam;
}
