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

static const int WIN_W = 1140;
static const int WIN_H = 760;

enum BgPreset {
    PRESET_GOOGLE_WHITE = 0,
    PRESET_MATERIAL_PASTEL = 1,
    PRESET_DARK_MATERIAL = 2,
    PRESET_ADAPTIVE_EDGE = 3,
    PRESET_CUSTOM_COLOR = 4
};

struct RoundStyle {
    BgPreset preset = PRESET_GOOGLE_WHITE;
    double scale = 0.68;       // 0.30 .. 2.50 (30% .. 250%)
    double offsetX = 0.0;      // -1.0 .. +1.0
    double offsetY = 0.0;      // -1.0 .. +1.0 (positive = up)
    double shadow = 0.45;      // 0.0 .. 1.0
    bool autoTrim = true;
    bool innerShadow = true;
    bool fullBleed = false;
    bool monochrome = false;
    COLORREF customColor = RGB(26, 115, 232);
};

struct AppEntry {
    std::wstring name;
    std::wstring searchKeywords;
    std::wstring lnkPath;
    std::vector<std::wstring> allLnkPaths;
    std::wstring targetPath;
    std::wstring category;
    std::wstring systemClsid;
    std::wstring defaultRestoreIcon;
    bool isFileExplorer = false;
    bool isRecycleBin = false;
    bool isSystemPinnedTop = false;
    bool isApplied = false;
    Bitmap* origBitmap = nullptr;
    Bitmap* cachedThumb = nullptr; // 64x64 cached circular preview for the list
};

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

static const COLORREF kSwatchPalette[8] = {
    RGB(232, 240, 254), // Google Ice
    RGB(230, 244, 234), // Pixel Mint
    RGB(254, 247, 224), // Warm Amber
    RGB(252, 232, 230), // Blush Rose
    RGB(243, 232, 253), // Material Lavender
    RGB(26, 115, 232),  // Google Blue
    RGB(30, 142, 62),   // Material Emerald
    RGB(32, 33, 36)     // Dark Graphite
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
static Bitmap* RenderGoogleRoundIcon(Bitmap* sourceBitmap, const RoundStyle& style, int canvasSize);
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

static std::wstring GetAppDataRoot() {
    wchar_t buf[MAX_PATH] = {0};
    SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, buf);
    std::wstring dir = std::wstring(buf) + L"\\IconMaker";
    CreateDirectoryW(dir.c_str(), NULL);
    CreateDirectoryW((dir + L"\\icons").c_str(), NULL);
    CreateDirectoryW((dir + L"\\backup").c_str(), NULL);
    return dir;
}

static std::wstring SanitizeFileName(const std::wstring& s) {
    std::wstring out;
    for (wchar_t c : s) {
        if (c == L'\\' || c == L'/' || c == L':' || c == L'*' ||
            c == L'?' || c == L'"' || c == L'<' || c == L'>' || c == L'|') {
            out.push_back(L'_');
        } else {
            out.push_back(c);
        }
    }
    if (out.empty()) out = L"AppIcon";
    return out;
}

// Rounded rectangle path helper
static void AddRoundedRect(GraphicsPath& path, RectF r, REAL radius) {
    if (radius <= 0.5f) {
        path.AddRectangle(r);
        return;
    }
    REAL d = std::min({ radius * 2.0f, r.Width, r.Height });
    path.AddArc(r.X, r.Y, d, d, 180.0f, 90.0f);
    path.AddArc(r.GetRight() - d, r.Y, d, d, 270.0f, 90.0f);
    path.AddArc(r.GetRight() - d, r.GetBottom() - d, d, d, 0.0f, 90.0f);
    path.AddArc(r.X, r.GetBottom() - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
}

static void FillRoundRect(Graphics& g, RectF r, REAL radius, Color fill) {
    GraphicsPath path;
    AddRoundedRect(path, r, radius);
    SolidBrush br(fill);
    g.FillPath(&br, &path);
}

static void DrawRoundRect(Graphics& g, RectF r, REAL radius, Color stroke, REAL width = 1.0f) {
    GraphicsPath path;
    AddRoundedRect(path, r, radius);
    Pen pen(stroke, width);
    g.DrawPath(&pen, &path);
}

// Convert HICON to 32-bit RGBA GDI+ Bitmap (properly reconstructing alpha for masked icons
// and stripping SHIL_JUMBO 256x256 top-left corner padding when legacy 32/48px icons are returned)
static Bitmap* BitmapFromHIcon32(HICON hIcon, int targetSize = 256) {
    if (!hIcon) return nullptr;
    ICONINFO ii = {0};
    if (!GetIconInfo(hIcon, &ii)) return nullptr;

    BITMAP bm = {0};
    GetObject(ii.hbmColor ? ii.hbmColor : ii.hbmMask, sizeof(bm), &bm);
    int w = bm.bmWidth > 0 ? bm.bmWidth : targetSize;
    int h = bm.bmHeight > 0 ? bm.bmHeight : targetSize;
    if (!ii.hbmColor && ii.hbmMask) h /= 2;
    if (w <= 0) w = targetSize;
    if (h <= 0) h = targetSize;

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<uint8_t> pixels(w * h * 4, 0);
    std::vector<uint8_t> maskPixels(w * h * 4, 0);
    HDC hdc = GetDC(NULL);
    bool hasAlpha = false;
    if (ii.hbmColor) {
        GetDIBits(hdc, ii.hbmColor, 0, h, pixels.data(), &bmi, DIB_RGB_COLORS);
        for (size_t i = 3; i < pixels.size(); i += 4) {
            if (pixels[i] != 0) { hasAlpha = true; break; }
        }
    }
    if (!hasAlpha && ii.hbmMask) {
        GetDIBits(hdc, ii.hbmMask, 0, h, maskPixels.data(), &bmi, DIB_RGB_COLORS);
        for (int i = 0; i < w * h; ++i) {
            bool transparent = (maskPixels[i * 4] > 128);
            pixels[i * 4 + 3] = transparent ? 0 : 255;
        }
        hasAlpha = true;
    }
    ReleaseDC(NULL, hdc);

    if (ii.hbmColor) DeleteObject(ii.hbmColor);
    if (ii.hbmMask) DeleteObject(ii.hbmMask);

    // Detect SHIL_JUMBO (256x256) bug where Windows places a 32x32 or 48x48 icon in the top-left corner
    // and leaves the rest of the 256x256 canvas empty (or with a faint 1px tile frame).
    if (w == 256 && h == 256) {
        int topLeftOpaque = 0;
        int restOpaque = 0;
        for (int y = 2; y < 254; ++y) {
            for (int x = 2; x < 254; ++x) {
                uint8_t a = pixels[(y * w + x) * 4 + 3];
                if (a > 48) {
                    if (x < 72 && y < 72) topLeftOpaque++;
                    else restOpaque++;
                }
            }
        }
        if (topLeftOpaque > 64 && restOpaque < 16) {
            int cropDim = 64;
            for (int y = 0; y < 72; ++y) {
                for (int x = 0; x < 72; ++x) {
                    if (pixels[(y * w + x) * 4 + 3] > 32) {
                        cropDim = std::max({ cropDim, x + 2, y + 2 });
                    }
                }
            }
            cropDim = std::min(cropDim, 72);
            Bitmap* cropped = new Bitmap(cropDim, cropDim, PixelFormat32bppARGB);
            BitmapData cbd;
            Rect cr(0, 0, cropDim, cropDim);
            if (cropped->LockBits(&cr, ImageLockModeWrite, PixelFormat32bppARGB, &cbd) == Ok) {
                for (int y = 0; y < cropDim; ++y) {
                    memcpy((uint8_t*)cbd.Scan0 + y * cbd.Stride, pixels.data() + y * w * 4, cropDim * 4);
                }
                cropped->UnlockBits(&cbd);
            }
            return cropped;
        }
    }

    Bitmap* out = new Bitmap(w, h, PixelFormat32bppARGB);
    BitmapData bd;
    Rect r(0, 0, w, h);
    if (out->LockBits(&r, ImageLockModeWrite, PixelFormat32bppARGB, &bd) == Ok) {
        for (int y = 0; y < h; ++y) {
            memcpy((uint8_t*)bd.Scan0 + y * bd.Stride, pixels.data() + y * w * 4, w * 4);
        }
        out->UnlockBits(&bd);
    }
    return out;
}

static Bitmap* ExtractDirectPeOrIco256(const std::wstring& filePath, int iconIndex = 0) {
    if (filePath.empty() || GetFileAttributesW(filePath.c_str()) == INVALID_FILE_ATTRIBUTES) return nullptr;
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (!hUser32) return nullptr;
    typedef UINT(WINAPI* PFN_PrivateExtractIconsW)(LPCWSTR, int, int, int, HICON*, UINT*, UINT, UINT);
    PFN_PrivateExtractIconsW fnExtract = (PFN_PrivateExtractIconsW)GetProcAddress(hUser32, "PrivateExtractIconsW");
    if (!fnExtract) return nullptr;

    HICON hIcon = nullptr;
    UINT iconId = 0;
    if (fnExtract(filePath.c_str(), iconIndex, 256, 256, &hIcon, &iconId, 1, LR_DEFAULTCOLOR) > 0 && hIcon) {
        Bitmap* bmp = BitmapFromHIcon32(hIcon, 256);
        DestroyIcon(hIcon);
        if (bmp && bmp->GetWidth() > 0) return bmp;
        delete bmp;
    }
    return nullptr;
}

static Bitmap* LoadHighResIcon(const std::wstring& primaryPath, const std::wstring& fallbackPath, const std::wstring& safeAppName = L"") {
    // 1. Always prefer the pristine cached original PNG in %LOCALAPPDATA%\IconMaker\backup\<SafeName>_orig.png
    // so re-applying icons never shrinks an already-converted circular icon ("circle inside a circle").
    std::wstring origCachePng;
    if (!safeAppName.empty()) {
        origCachePng = GetAppDataRoot() + L"\\backup\\" + safeAppName + L"_orig.png";
        if (GetFileAttributesW(origCachePng.c_str()) != INVALID_FILE_ATTRIBUTES) {
            Bitmap* cachedOrig = Bitmap::FromFile(origCachePng.c_str());
            if (cachedOrig && cachedOrig->GetLastStatus() == Ok && cachedOrig->GetWidth() >= 16) {
                return cachedOrig;
            }
            delete cachedOrig;
        }
    }

    auto saveToOrigCache = [&](Bitmap* b) -> Bitmap* {
        if (b && !origCachePng.empty() && GetFileAttributesW(origCachePng.c_str()) == INVALID_FILE_ATTRIBUTES) {
            b->Save(origCachePng.c_str(), &g_pngClsid, NULL);
        }
        return b;
    };

    std::wstring cleanPrimary = primaryPath;
    if (cleanPrimary.find(L"\\IconMaker\\icons\\") != std::wstring::npos) {
        cleanPrimary.clear();
    }

    std::wstring ext = PathFindExtensionW(cleanPrimary.c_str());
    for (auto& c : ext) c = towlower(c);
    if (ext == L".png" || ext == L".bmp" || ext == L".jpg" || ext == L".jpeg") {
        Bitmap* bmp = Bitmap::FromFile(cleanPrimary.c_str());
        if (bmp && bmp->GetLastStatus() == Ok && bmp->GetWidth() > 0) return saveToOrigCache(bmp);
        delete bmp;
    }

    // 2. Direct 256x256 PE/ICO resource extraction via PrivateExtractIconsW (avoids SHIL_JUMBO top-left bug!)
    if (!cleanPrimary.empty()) {
        if (Bitmap* directBmp = ExtractDirectPeOrIco256(cleanPrimary, 0)) {
            return saveToOrigCache(directBmp);
        }
    }

    const std::wstring& pathToUse = (!cleanPrimary.empty() && GetFileAttributesW(cleanPrimary.c_str()) != INVALID_FILE_ATTRIBUTES)
        ? cleanPrimary
        : fallbackPath;

    if (!pathToUse.empty() && pathToUse.find(L"\\IconMaker\\icons\\") == std::wstring::npos) {
        if (Bitmap* directBmp2 = ExtractDirectPeOrIco256(pathToUse, 0)) {
            return saveToOrigCache(directBmp2);
        }
    }

    SHFILEINFOW sfi = {0};
    if (!pathToUse.empty() && SHGetFileInfoW(pathToUse.c_str(), 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX)) {
        IImageList* pImgList = nullptr;
        static const GUID kIID_IImageList = { 0x46eb5926, 0x582e, 0x4017, { 0x9f, 0xdf, 0xe8, 0x99, 0x8d, 0xaa, 0x09, 0x50 } };
        if (SUCCEEDED(SHGetImageList(SHIL_JUMBO, kIID_IImageList, (void**)&pImgList)) && pImgList) {
            HICON hIcon = nullptr;
            if (SUCCEEDED(pImgList->GetIcon(sfi.iIcon, ILD_TRANSPARENT, &hIcon)) && hIcon) {
                Bitmap* bmp = BitmapFromHIcon32(hIcon, 256);
                DestroyIcon(hIcon);
                pImgList->Release();
                if (bmp) return saveToOrigCache(bmp);
            }
            pImgList->Release();
        }
    }

    HICON hLarge = nullptr;
    if (!pathToUse.empty() && ExtractIconExW(pathToUse.c_str(), 0, &hLarge, NULL, 1) > 0 && hLarge) {
        Bitmap* bmp = BitmapFromHIcon32(hLarge, 128);
        DestroyIcon(hLarge);
        if (bmp) return saveToOrigCache(bmp);
    }

    if (!pathToUse.empty() && SHGetFileInfoW(pathToUse.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON) && sfi.hIcon) {
        Bitmap* bmp = BitmapFromHIcon32(sfi.hIcon, 128);
        DestroyIcon(sfi.hIcon);
        if (bmp) return saveToOrigCache(bmp);
    }

    Bitmap* synth = new Bitmap(256, 256, PixelFormat32bppARGB);
    Graphics g(synth);
    g.SetSmoothingMode(SmoothingModeHighQuality);
    SolidBrush bg(Color(255, 26, 115, 232));
    g.FillEllipse(&bg, 16, 16, 224, 224);
    return synth;
}

// Shadow-aware tight crop: strips soft drop shadows (alpha < 108) and 1-2px outer box artifacts
// so every icon's bounding box reflects its true solid visual core.
static Bitmap* TrimTransparentEdges(Bitmap* src) {
    if (!src) return nullptr;
    int w = (int)src->GetWidth();
    int h = (int)src->GetHeight();
    if (w <= 12 || h <= 12) return src->Clone(0, 0, w, h, PixelFormat32bppARGB);

    BitmapData bd;
    Rect r(0, 0, w, h);
    if (src->LockBits(&r, ImageLockModeRead, PixelFormat32bppARGB, &bd) != Ok) {
        return src->Clone(0, 0, w, h, PixelFormat32bppARGB);
    }

    auto findBounds = [&](uint8_t alphaThreshold, int inset, int& minX, int& minY, int& maxX, int& maxY) -> int {
        minX = w; minY = h; maxX = -1; maxY = -1;
        int count = 0;
        for (int y = inset; y < h - inset; ++y) {
            const uint8_t* row = (const uint8_t*)bd.Scan0 + y * bd.Stride;
            for (int x = inset; x < w - inset; ++x) {
                if (row[x * 4 + 3] >= alphaThreshold) {
                    if (x < minX) minX = x;
                    if (x > maxX) maxX = x;
                    if (y < minY) minY = y;
                    if (y > maxY) maxY = y;
                    count++;
                }
            }
        }
        return count;
    };

    int minX = w, minY = h, maxX = -1, maxY = -1;
    int inset = (w >= 64 && h >= 64) ? 2 : 0;
    int solidCount = findBounds(108, inset, minX, minY, maxX, maxY);
    if (solidCount < 24 || (maxX - minX) < 8 || (maxY - minY) < 8) {
        findBounds(28, 0, minX, minY, maxX, maxY);
    }
    src->UnlockBits(&bd);

    if (minX >= maxX || minY >= maxY) {
        return src->Clone(0, 0, w, h, PixelFormat32bppARGB);
    }
    return src->Clone(minX, minY, maxX - minX + 1, maxY - minY + 1, PixelFormat32bppARGB);
}

struct ExtractedColors {
    Color dominant;
    Color edge;
    bool isSquareLike;
    bool isAlreadyCircular;
    double opticalRadiusNorm; // 0.50 = circle, ~0.60 = squircle, ~0.69 = full square
    double minRadialCover;    // minimum solid radius across cardinal + diagonal directions
};

static ExtractedColors AnalyzeBitmapColors(Bitmap* src) {
    ExtractedColors res = {
        Color(255, 26, 115, 232),
        Color(255, 240, 244, 249),
        false,
        false,
        0.50,
        0.50
    };
    if (!src) return res;

    const int S = 64;
    Bitmap sample(S, S, PixelFormat32bppARGB);
    {
        Graphics g(&sample);
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        g.DrawImage(src, Rect(0, 0, S, S));
    }

    BitmapData bd;
    Rect r(0, 0, S, S);
    if (sample.LockBits(&r, ImageLockModeRead, PixelFormat32bppARGB, &bd) != Ok) return res;

    double totR = 0, totG = 0, totB = 0, totW = 0;
    double edgR = 0, edgG = 0, edgB = 0, edgN = 0;
    int opaqueCount = 0;
    int cornerOpaque = 0;
    int innerDiscTotal = 0;
    int innerDiscOpaque = 0;
    std::vector<double> radii;
    radii.reserve(S * S);

    const double cx = (S - 1) * 0.5;
    const double cy = (S - 1) * 0.5;

    for (int y = 0; y < S; ++y) {
        const uint8_t* row = (const uint8_t*)bd.Scan0 + y * bd.Stride;
        for (int x = 0; x < S; ++x) {
            uint8_t b = row[x * 4 + 0];
            uint8_t g = row[x * 4 + 1];
            uint8_t rv = row[x * 4 + 2];
            uint8_t a = row[x * 4 + 3];

            double dx = (x - cx) / (double)S;
            double dy = (y - cy) / (double)S;
            double dist = std::sqrt(dx * dx + dy * dy);

            if (dist <= 0.44) {
                innerDiscTotal++;
                if (a >= 108) innerDiscOpaque++;
            }
            if ((x < 5 || x >= S - 5) && (y < 5 || y >= S - 5) && a >= 108) {
                cornerOpaque++;
            }

            if (a < 96) continue;
            opaqueCount++;
            radii.push_back(dist);

            double maxC = std::max({ (double)rv, (double)g, (double)b });
            double minC = std::min({ (double)rv, (double)g, (double)b });
            double sat = maxC > 0 ? (maxC - minC) / maxC : 0.0;
            double w = 0.25 + sat * 2.5;
            totR += rv * w; totG += g * w; totB += b * w; totW += w;

            if (dist >= 0.36 && dist <= 0.50) {
                edgR += rv; edgG += g; edgB += b; edgN += 1.0;
            }
        }
    }

    // Measure solid radial extent in 8 directions (0, 45, 90, 135, 180, 225, 270, 315 deg)
    const double dirs[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {0.7071, 0.7071}, {-0.7071, 0.7071}, {0.7071, -0.7071}, {-0.7071, -0.7071}
    };
    double minDirR = 0.70;
    for (int d = 0; d < 8; ++d) {
        double maxRForDir = 0.0;
        for (int step = 1; step <= 44; ++step) {
            double rad = step * (0.70 / 44.0);
            int px = (int)std::round(cx + dirs[d][0] * rad * S);
            int py = (int)std::round(cy + dirs[d][1] * rad * S);
            if (px < 0 || px >= S || py < 0 || py >= S) break;
            const uint8_t* row = (const uint8_t*)bd.Scan0 + py * bd.Stride;
            if (row[px * 4 + 3] >= 108) {
                maxRForDir = rad;
            }
        }
        minDirR = std::min(minDirR, maxRForDir);
    }

    sample.UnlockBits(&bd);

    if (totW > 0) {
        res.dominant = Color(255,
            (BYTE)std::clamp((int)std::round(totR / totW), 0, 255),
            (BYTE)std::clamp((int)std::round(totG / totW), 0, 255),
            (BYTE)std::clamp((int)std::round(totB / totW), 0, 255));
    }
    if (edgN > 0) {
        res.edge = Color(255,
            (BYTE)std::clamp((int)std::round(edgR / edgN), 0, 255),
            (BYTE)std::clamp((int)std::round(edgG / edgN), 0, 255),
            (BYTE)std::clamp((int)std::round(edgB / edgN), 0, 255));
    } else {
        res.edge = res.dominant;
    }

    if (!radii.empty()) {
        std::sort(radii.begin(), radii.end());
        size_t idx96 = std::min(radii.size() - 1, (radii.size() * 96) / 100);
        res.opticalRadiusNorm = std::clamp(radii[idx96], 0.42, 0.70);
    }

    double innerFill = innerDiscTotal > 0 ? (double)innerDiscOpaque / (double)innerDiscTotal : 0.0;
    double totalFill = (double)opaqueCount / (double)(S * S);

    res.isAlreadyCircular = (innerFill > 0.86 && cornerOpaque <= 4 && minDirR >= 0.42 && minDirR <= 0.54);
    res.isSquareLike = (innerFill > 0.84 && totalFill > 0.68);
    res.minRadialCover = std::clamp(minDirR, 0.36, 0.70);
    return res;
}

static Color BlendWithWhite(Color c, double whiteMix) {
    double m = std::clamp(whiteMix, 0.0, 1.0);
    BYTE r = (BYTE)std::clamp((int)std::round(c.GetR() * (1.0 - m) + 255.0 * m), 0, 255);
    BYTE g = (BYTE)std::clamp((int)std::round(c.GetG() * (1.0 - m) + 255.0 * m), 0, 255);
    BYTE b = (BYTE)std::clamp((int)std::round(c.GetB() * (1.0 - m) + 255.0 * m), 0, 255);
    return Color(255, r, g, b);
}

static Color DeepenColor(Color c, double factor = 0.42) {
    BYTE r = (BYTE)std::clamp((int)std::round(c.GetR() * factor), 0, 255);
    BYTE g = (BYTE)std::clamp((int)std::round(c.GetG() * factor), 0, 255);
    BYTE b = (BYTE)std::clamp((int)std::round(c.GetB() * factor), 0, 255);
    return Color(255, r, g, b);
}

static Bitmap* RenderGoogleRoundIcon(Bitmap* sourceBitmap, const RoundStyle& style, int canvasSize = 256) {
    Bitmap* out = new Bitmap(canvasSize, canvasSize, PixelFormat32bppARGB);
    if (!sourceBitmap) return out;

    Bitmap* trimmed = style.autoTrim ? TrimTransparentEdges(sourceBitmap)
                                     : sourceBitmap->Clone(0, 0, sourceBitmap->GetWidth(), sourceBitmap->GetHeight(), PixelFormat32bppARGB);
    ExtractedColors colors = AnalyzeBitmapColors(trimmed);

    Graphics g(out);
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    g.Clear(Color(0, 0, 0, 0));

    // Strictly uniform outer circle geometry across all icon sizes (93.75% of canvas = 240x240 in 256x256)
    REAL dim = (REAL)canvasSize;
    REAL margin = std::max(1.0f, std::round(dim * 0.03125f));
    RectF circleRect(margin, margin, dim - margin * 2.0f, dim - margin * 2.0f);

    if (style.shadow > 0.01 && canvasSize >= 32) {
        int layers = 6;
        REAL maxBlur = std::min(margin * 1.4f, dim * (REAL)(0.024 * style.shadow));
        REAL yOff = std::min(margin * 0.45f, dim * (REAL)(0.008 * style.shadow));
        for (int i = layers; i >= 1; --i) {
            REAL t = (REAL)i / (REAL)layers;
            REAL expand = maxBlur * t;
            BYTE alpha = (BYTE)std::clamp((int)(34.0 * style.shadow * (1.0 - t * 0.72)), 1, 110);
            SolidBrush sb(Color(alpha, 0, 0, 0));
            RectF sr(circleRect.X - expand * 0.5f,
                     circleRect.Y - expand * 0.5f + yOff * t,
                     circleRect.Width + expand,
                     circleRect.Height + expand);
            g.FillEllipse(&sb, sr);
        }
    }

    Color topFill(255, 255, 255, 255);
    Color botFill(255, 244, 246, 250);
    switch (style.preset) {
        case PRESET_GOOGLE_WHITE:
            topFill = Color(255, 255, 255, 255);
            botFill = Color(255, 243, 245, 249);
            break;
        case PRESET_MATERIAL_PASTEL:
            topFill = BlendWithWhite(colors.dominant, 0.82);
            botFill = BlendWithWhite(colors.dominant, 0.72);
            break;
        case PRESET_DARK_MATERIAL:
            topFill = Color(255, 40, 42, 46);
            botFill = Color(255, 27, 28, 32);
            break;
        case PRESET_ADAPTIVE_EDGE:
            topFill = BlendWithWhite(colors.edge, 0.22);
            botFill = colors.edge;
            break;
        case PRESET_CUSTOM_COLOR: {
            Color c(255, GetRValue(style.customColor), GetGValue(style.customColor), GetBValue(style.customColor));
            topFill = BlendWithWhite(c, 0.18);
            botFill = c;
            break;
        }
    }

    LinearGradientBrush discBrush(
        PointF(circleRect.X, circleRect.Y),
        PointF(circleRect.X, circleRect.GetBottom()),
        topFill,
        botFill
    );
    g.FillEllipse(&discBrush, circleRect);

    GraphicsPath clipPath;
    clipPath.AddEllipse(circleRect);
    GraphicsState state = g.Save();
    g.SetClip(&clipPath, CombineModeReplace);

    // Optical normalization: ensures squares, squircles, and circles have the exact same visual radius
    double opticalFactor = style.autoTrim
        ? std::clamp(0.50 / std::max(0.42, colors.opticalRadiusNorm), 0.72, 1.08)
        : 1.0;

    double effScale = style.scale * opticalFactor;
    if (style.fullBleed && (colors.isSquareLike || colors.isAlreadyCircular)) {
        double bleedCover = colors.isAlreadyCircular
            ? (0.515 / std::max(0.42, colors.minRadialCover))
            : std::max(1.36, 0.525 / std::max(0.36, colors.minRadialCover));
        effScale = std::max(effScale, bleedCover);
    }

    REAL srcW = (REAL)std::max(1u, trimmed->GetWidth());
    REAL srcH = (REAL)std::max(1u, trimmed->GetHeight());
    REAL diagNorm = 1.0f;
    if (!style.fullBleed) {
        // Normalize aspect ratio so wide, tall, and square icons have equal bounding diagonal
        REAL ar = std::max(srcW / srcH, srcH / srcW);
        if (ar > 1.05f) {
            diagNorm = std::clamp(std::sqrt(2.0f / (1.0f + 1.0f / (ar * ar))), 1.0f, 1.22f);
        }
    }

    REAL maxSpan = circleRect.Width * (REAL)effScale * diagNorm;
    REAL aspect = srcW / srcH;
    REAL drawW = (aspect >= 1.0f) ? maxSpan : maxSpan * aspect;
    REAL drawH = (aspect >= 1.0f) ? maxSpan / aspect : maxSpan;

    REAL shiftX = (REAL)style.offsetX * (circleRect.Width * 0.5f);
    REAL shiftY = -(REAL)style.offsetY * (circleRect.Height * 0.5f);
    REAL cx = circleRect.X + circleRect.Width * 0.5f + shiftX;
    REAL cy = circleRect.Y + circleRect.Height * 0.5f + shiftY;
    RectF glyphRect(cx - drawW * 0.5f, cy - drawH * 0.5f, drawW, drawH);

    if (style.innerShadow && !(style.fullBleed && (colors.isSquareLike || colors.isAlreadyCircular))) {
        ColorMatrix shadowMat = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.20f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 1.0f
        };
        ImageAttributes shadowAttr;
        shadowAttr.SetColorMatrix(&shadowMat, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
        RectF shRect(glyphRect.X, glyphRect.Y + dim * 0.010f, glyphRect.Width, glyphRect.Height);
        g.DrawImage(trimmed, shRect, 0, 0, srcW, srcH, UnitPixel, &shadowAttr);
    }

    if (style.monochrome) {
        Color tint = (style.preset == PRESET_DARK_MATERIAL)
            ? BlendWithWhite(colors.dominant, 0.72)
            : DeepenColor(colors.dominant, 0.42);
        REAL tr = tint.GetR() / 255.0f;
        REAL tg = tint.GetG() / 255.0f;
        REAL tb = tint.GetB() / 255.0f;
        ColorMatrix monoMat = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            tr,   tg,   tb,   0.0f, 1.0f
        };
        ImageAttributes monoAttr;
        monoAttr.SetColorMatrix(&monoMat, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
        g.DrawImage(trimmed, glyphRect, 0, 0, srcW, srcH, UnitPixel, &monoAttr);
    } else {
        g.DrawImage(trimmed, glyphRect, 0, 0, srcW, srcH, UnitPixel);
    }

    g.Restore(state);

    REAL penW = std::max(1.0f, dim * 0.004f);
    Color rimColor = (style.preset == PRESET_DARK_MATERIAL)
        ? Color(45, 255, 255, 255)
        : Color(26, 0, 0, 0);
    Pen rimPen(rimColor, penW);
    RectF rimRect = circleRect;
    rimRect.Inflate(-penW * 0.5f, -penW * 0.5f);
    g.DrawEllipse(&rimPen, rimRect);

    delete trimmed;
    return out;
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

static bool EncodeBitmapToPngBytes(Bitmap* bmp, std::vector<uint8_t>& outBytes) {
    if (!bmp) return false;
    IStream* pStream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(NULL, TRUE, &pStream)) || !pStream) return false;
    if (bmp->Save(pStream, &g_pngClsid, NULL) != Ok) {
        pStream->Release();
        return false;
    }

    HGLOBAL hMem = NULL;
    if (SUCCEEDED(GetHGlobalFromStream(pStream, &hMem)) && hMem) {
        SIZE_T sz = GlobalSize(hMem);
        void* ptr = GlobalLock(hMem);
        if (ptr && sz > 0) {
            outBytes.assign((const uint8_t*)ptr, (const uint8_t*)ptr + sz);
            GlobalUnlock(hMem);
        }
    }
    pStream->Release();
    return !outBytes.empty();
}

// Encodes a rendered square ARGB Bitmap into a standard Windows uncompressed 32-bpp ICO DIB frame
// (BITMAPINFOHEADER + bottom-up BGRA XOR bitmap + 1-bpp AND mask).
// Windows Explorer (user32/shell32) requires uncompressed DIB frames for sizes < 256 so it never
// falls back to drawing a small 32/48px icon unscaled inside a larger desktop slot!
static bool EncodeBitmapToDibFrame(Bitmap* bmp, std::vector<uint8_t>& outBytes) {
    if (!bmp) return false;
    int sz = (int)bmp->GetWidth();
    if (sz <= 0 || (int)bmp->GetHeight() != sz) return false;

    BitmapData bd;
    Rect r(0, 0, sz, sz);
    if (bmp->LockBits(&r, ImageLockModeRead, PixelFormat32bppARGB, &bd) != Ok) return false;

    int xorStride = sz * 4;
    int xorBytes = xorStride * sz;
    int andStride = ((sz + 31) / 32) * 4;
    int andBytes = andStride * sz;

    outBytes.resize(sizeof(BITMAPINFOHEADER) + xorBytes + andBytes, 0);

    BITMAPINFOHEADER bih = {0};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = sz;
    bih.biHeight = sz * 2; // XOR height + AND height
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = (DWORD)(xorBytes + andBytes);
    memcpy(outBytes.data(), &bih, sizeof(bih));

    uint8_t* xorDst = outBytes.data() + sizeof(BITMAPINFOHEADER);
    uint8_t* andDst = xorDst + xorBytes;

    for (int y = 0; y < sz; ++y) {
        int srcY = (sz - 1) - y; // bottom-up
        const uint8_t* srcRow = (const uint8_t*)bd.Scan0 + srcY * bd.Stride;
        uint8_t* dstRow = xorDst + y * xorStride;
        uint8_t* maskRow = andDst + y * andStride;
        memcpy(dstRow, srcRow, xorStride);
        for (int x = 0; x < sz; ++x) {
            uint8_t alpha = srcRow[x * 4 + 3];
            if (alpha < 12) {
                maskRow[x >> 3] |= (uint8_t)(0x80 >> (x & 7));
            }
        }
    }

    bmp->UnlockBits(&bd);
    return true;
}

static bool SaveMultiSizeIco(Bitmap* sourceOrigBitmap, const RoundStyle& style, const std::wstring& icoPath) {
    if (!sourceOrigBitmap) return false;
    const int sizes[] = { 256, 128, 96, 64, 48, 40, 32, 24, 16 };
    const int count = (int)(sizeof(sizes) / sizeof(sizes[0]));
    std::vector<std::vector<uint8_t>> frames(count);

    for (int i = 0; i < count; ++i) {
        Bitmap* rendered = RenderGoogleRoundIcon(sourceOrigBitmap, style, sizes[i]);
        bool ok = false;
        if (sizes[i] >= 256) {
            ok = EncodeBitmapToPngBytes(rendered, frames[i]);
        } else {
            ok = EncodeBitmapToDibFrame(rendered, frames[i]);
        }
        delete rendered;
        if (!ok) return false;
    }

    HANDLE hFile = CreateFileW(icoPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD written = 0;
    uint16_t header[3] = { 0, 1, (uint16_t)count };
    WriteFile(hFile, header, sizeof(header), &written, NULL);

    uint32_t offset = 6 + count * 16;
    for (int i = 0; i < count; ++i) {
        uint8_t entry[16] = {0};
        entry[0] = (sizes[i] >= 256) ? 0 : (uint8_t)sizes[i];
        entry[1] = (sizes[i] >= 256) ? 0 : (uint8_t)sizes[i];
        entry[4] = 1; entry[5] = 0;
        entry[6] = 32; entry[7] = 0;
        uint32_t len = (uint32_t)frames[i].size();
        memcpy(entry + 8, &len, 4);
        memcpy(entry + 12, &offset, 4);
        WriteFile(hFile, entry, 16, &written, NULL);
        offset += len;
    }

    for (int i = 0; i < count; ++i) {
        WriteFile(hFile, frames[i].data(), (DWORD)frames[i].size(), &written, NULL);
    }

    CloseHandle(hFile);
    return true;
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

static void TouchFileModificationTime(const std::wstring& path) {
    HANDLE h = CreateFileW(path.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        SYSTEMTIME st;
        FILETIME ft;
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &ft);
        SetFileTime(h, NULL, &ft, &ft);
        CloseHandle(h);
    }
}

static bool SetRegistryStringValue(HKEY hRoot, const std::wstring& subKey, const wchar_t* valueName, const std::wstring& val) {
    HKEY hKey = NULL;
    if (RegCreateKeyExW(hRoot, subKey.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        LONG rc = RegSetValueExW(hKey, valueName, 0, REG_SZ, (const BYTE*)val.c_str(), (DWORD)((val.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
        return rc == ERROR_SUCCESS;
    }
    return false;
}

static std::wstring GetRegistryStringValue(HKEY hRoot, const std::wstring& subKey, const wchar_t* valueName) {
    HKEY hKey = NULL;
    std::wstring out;
    if (RegOpenKeyExW(hRoot, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t buf[MAX_PATH * 2] = {0};
        DWORD cb = sizeof(buf) - sizeof(wchar_t);
        DWORD type = 0;
        if (RegQueryValueExW(hKey, valueName, NULL, &type, (LPBYTE)buf, &cb) == ERROR_SUCCESS &&
            (type == REG_SZ || type == REG_EXPAND_SZ)) {
            out = buf;
        }
        RegCloseKey(hKey);
    }
    return out;
}

static void RefreshWindowsExplorerIconCache(bool restartExplorerForTaskbar = false) {
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT, NULL, NULL);
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ShellState", SMTO_ABORTIFHUNG, 500, NULL);
    wchar_t sysDir[MAX_PATH] = {0};
    if (GetSystemDirectoryW(sysDir, MAX_PATH)) {
        std::wstring ie4 = std::wstring(sysDir) + L"\\ie4uinit.exe";
        if (GetFileAttributesW(ie4.c_str()) != INVALID_FILE_ATTRIBUTES) {
            ShellExecuteW(NULL, L"open", ie4.c_str(), L"-show", NULL, SW_HIDE);
        }
    }
    if (restartExplorerForTaskbar) {
        HWND hTray = FindWindowW(L"Shell_TrayWnd", NULL);
        if (hTray) {
            DWORD pid = 0;
            GetWindowThreadProcessId(hTray, &pid);
            PostMessageW(hTray, WM_QUIT, 0, 0);
            Sleep(450);
            if (FindWindowW(L"Shell_TrayWnd", NULL) != NULL && pid != 0) {
                HANDLE hProc = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
                if (hProc) {
                    TerminateProcess(hProc, 0);
                    WaitForSingleObject(hProc, 1000);
                    CloseHandle(hProc);
                }
            }
            Sleep(250);
            if (FindWindowW(L"Shell_TrayWnd", NULL) == NULL) {
                wchar_t winDir[MAX_PATH] = {0};
                GetWindowsDirectoryW(winDir, MAX_PATH);
                std::wstring expExe = std::wstring(winDir) + L"\\explorer.exe";
                ShellExecuteW(NULL, L"open", expExe.c_str(), NULL, NULL, SW_SHOWNORMAL);
            }
        }
    }
}

static bool ResolveShortcut(const std::wstring& lnkPath, std::wstring& outTarget, std::wstring& outCustomIcon) {
    std::wstring ext = PathFindExtensionW(lnkPath.c_str());
    if (_wcsicmp(ext.c_str(), L".url") == 0) {
        wchar_t iconFile[MAX_PATH] = {0};
        GetPrivateProfileStringW(L"InternetShortcut", L"IconFile", L"", iconFile, MAX_PATH, lnkPath.c_str());
        if (iconFile[0]) {
            wchar_t expanded[MAX_PATH] = {0};
            ExpandEnvironmentStringsW(iconFile, expanded, MAX_PATH);
            outCustomIcon = expanded;
            outTarget = expanded;
        }
        return true;
    }

    IShellLinkW* psl = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl)) || !psl) {
        return false;
    }
    IPersistFile* ppf = nullptr;
    if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)) && ppf) {
        if (SUCCEEDED(ppf->Load(lnkPath.c_str(), STGM_READ))) {
            wchar_t target[MAX_PATH] = {0};
            if (SUCCEEDED(psl->GetPath(target, MAX_PATH, NULL, SLGP_RAWPATH)) && target[0]) {
                wchar_t expanded[MAX_PATH] = {0};
                ExpandEnvironmentStringsW(target, expanded, MAX_PATH);
                outTarget = expanded;
            }
            wchar_t iconLoc[MAX_PATH] = {0};
            int iconIdx = 0;
            if (SUCCEEDED(psl->GetIconLocation(iconLoc, MAX_PATH, &iconIdx)) && iconLoc[0]) {
                wchar_t expandedIcon[MAX_PATH] = {0};
                ExpandEnvironmentStringsW(iconLoc, expandedIcon, MAX_PATH);
                outCustomIcon = expandedIcon;
            }
        }
        ppf->Release();
    }
    psl->Release();
    return !outTarget.empty() || !outCustomIcon.empty();
}

static bool SetShortcutIconLocation(const std::wstring& lnkPath, const std::wstring& icoPath) {
    DWORD attrs = GetFileAttributesW(lnkPath.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM))) {
        SetFileAttributesW(lnkPath.c_str(), attrs & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM));
    }

    std::wstring ext = PathFindExtensionW(lnkPath.c_str());
    if (_wcsicmp(ext.c_str(), L".url") == 0) {
        BOOL ok1 = WritePrivateProfileStringW(L"InternetShortcut", L"IconFile", icoPath.c_str(), lnkPath.c_str());
        BOOL ok2 = WritePrivateProfileStringW(L"InternetShortcut", L"IconIndex", L"0", lnkPath.c_str());
        WritePrivateProfileStringW(NULL, NULL, NULL, lnkPath.c_str());
        if (ok1 && ok2) {
            TouchFileModificationTime(lnkPath);
            return true;
        }
        return false;
    }

    IShellLinkW* psl = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl)) || !psl) {
        return false;
    }
    bool ok = false;
    IPersistFile* ppf = nullptr;
    if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)) && ppf) {
        if (SUCCEEDED(ppf->Load(lnkPath.c_str(), STGM_READWRITE))) {
            if (SUCCEEDED(psl->SetIconLocation(icoPath.c_str(), 0))) {
                if (SUCCEEDED(ppf->Save(lnkPath.c_str(), TRUE))) {
                    ok = true;
                }
            }
        }
        ppf->Release();
    }
    psl->Release();
    if (ok) {
        TouchFileModificationTime(lnkPath);
    }
    return ok;
}

static Bitmap* LoadSystemDllIcon(const std::wstring& dllPath, int negResId, const std::wstring& fallbackDll, int fallbackIdx, const std::wstring& safeCacheName) {
    std::wstring origCachePng = GetAppDataRoot() + L"\\backup\\" + safeCacheName + L"_orig.png";
    if (GetFileAttributesW(origCachePng.c_str()) != INVALID_FILE_ATTRIBUTES) {
        Bitmap* cached = Bitmap::FromFile(origCachePng.c_str());
        if (cached && cached->GetLastStatus() == Ok && cached->GetWidth() >= 16) {
            return cached;
        }
        delete cached;
    }
    Bitmap* b = ExtractDirectPeOrIco256(dllPath, negResId);
    if (!b && !fallbackDll.empty()) {
        b = ExtractDirectPeOrIco256(fallbackDll, fallbackIdx);
    }
    if (!b) {
        b = LoadHighResIcon(dllPath, fallbackDll, safeCacheName);
    } else {
        b->Save(origCachePng.c_str(), &g_pngClsid, NULL);
    }
    return b;
}

// Always add File Explorer (Проводник) and the 5 Windows System Desktop Icons at the top of the list
static void AddBuiltInSystemEntries() {
    wchar_t winDir[MAX_PATH] = {0};
    GetWindowsDirectoryW(winDir, MAX_PATH);
    std::wstring wDir = winDir[0] ? winDir : L"C:\\Windows";
    std::wstring explorerExe = wDir + L"\\explorer.exe";
    std::wstring imageresDll = wDir + L"\\System32\\imageres.dll";
    std::wstring shell32Dll  = wDir + L"\\System32\\shell32.dll";

    // 1. File Explorer (Проводник)
    {
        AppEntry fe;
        fe.name = L"File Explorer (Проводник)";
        fe.searchKeywords = L"file explorer проводник файловый менеджер папки windows explorer.exe";
        fe.targetPath = explorerExe;
        fe.category = L"Системный компонент Windows";
        fe.systemClsid = L"{52205fd8-5dfb-447d-801a-d0b52f2e83e1}";
        fe.defaultRestoreIcon = explorerExe + L",0";
        fe.isFileExplorer = true;
        fe.isSystemPinnedTop = true;

        // Collect all known File Explorer .lnk locations across Taskbar, Start Menu, and Desktop
        wchar_t appData[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData)) && appData[0]) {
            const wchar_t* relPaths[] = {
                L"\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar\\File Explorer.lnk",
                L"\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar\\Проводник.lnk",
                L"\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar\\Windows Explorer.lnk",
                L"\\Microsoft\\Windows\\Start Menu\\Programs\\System Tools\\File Explorer.lnk",
                L"\\Microsoft\\Windows\\Start Menu\\Programs\\System Tools\\Проводник.lnk",
                L"\\Microsoft\\Windows\\Start Menu\\Programs\\Windows System\\File Explorer.lnk",
                L"\\Microsoft\\Windows\\Start Menu\\Programs\\Windows System\\Проводник.lnk",
                L"\\Microsoft\\Windows\\Start Menu\\Programs\\Accessories\\System Tools\\File Explorer.lnk"
            };
            for (const wchar_t* rp : relPaths) {
                std::wstring p = std::wstring(appData) + rp;
                if (GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES) {
                    fe.allLnkPaths.push_back(p);
                    if (fe.lnkPath.empty()) fe.lnkPath = p;
                    std::wstring t, cIcon;
                    ResolveShortcut(p, t, cIcon);
                    if (cIcon.find(L"\\IconMaker\\icons\\") != std::wstring::npos) fe.isApplied = true;
                }
            }
        }
        wchar_t desk[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desk)) && desk[0]) {
            const wchar_t* deskNames[] = { L"\\File Explorer.lnk", L"\\Проводник.lnk", L"\\File Explorer (Проводник).lnk" };
            for (const wchar_t* dn : deskNames) {
                std::wstring p = std::wstring(desk) + dn;
                if (GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES) {
                    fe.allLnkPaths.push_back(p);
                    if (fe.lnkPath.empty()) fe.lnkPath = p;
                    std::wstring t, cIcon;
                    ResolveShortcut(p, t, cIcon);
                    if (cIcon.find(L"\\IconMaker\\icons\\") != std::wstring::npos) fe.isApplied = true;
                }
            }
        }
        std::wstring regCur = GetRegistryStringValue(
            HKEY_CURRENT_USER,
            L"Software\\Classes\\CLSID\\{52205fd8-5dfb-447d-801a-d0b52f2e83e1}\\DefaultIcon",
            NULL
        );
        if (regCur.find(L"\\IconMaker\\icons\\") != std::wstring::npos) fe.isApplied = true;

        fe.origBitmap = LoadSystemDllIcon(explorerExe, 0, shell32Dll, 4, L"System_FileExplorer");
        g_allApps.push_back(fe);
    }

    // 2. The 5 Windows Desktop Shell CLSID Icons (This PC, Recycle Bin, User's Files, Control Panel, Network)
    struct SysClsidDef {
        const wchar_t* name;
        const wchar_t* keywords;
        const wchar_t* clsid;
        int imageresNegId;
        int shell32FallbackIdx;
        const wchar_t* defaultIconStr;
        const wchar_t* cacheKey;
        bool isRecycleBin;
    };
    const SysClsidDef defs[] = {
        {
            L"Этот компьютер (This PC)",
            L"this pc этот компьютер мой компьютер my computer",
            L"{20D04FE0-3AEA-1069-A2D8-08002B30309D}",
            -109, 15,
            L"%SystemRoot%\\System32\\imageres.dll,-109",
            L"System_ThisPC",
            false
        },
        {
            L"Корзина (Recycle Bin)",
            L"recycle bin корзина мусорка",
            L"{645FF040-5081-101B-9F08-00AA002F954E}",
            -55, 31,
            L"%SystemRoot%\\System32\\imageres.dll,-55",
            L"System_RecycleBin",
            true
        },
        {
            L"Папка пользователя (User's Files)",
            L"user files папка пользователя домашняя папка документы",
            L"{59031A47-3F72-44A7-89C5-5595FE6B30EE}",
            -123, 43,
            L"%SystemRoot%\\System32\\imageres.dll,-123",
            L"System_UserFiles",
            false
        },
        {
            L"Панель управления (Control Panel)",
            L"control panel панель управления настройки",
            L"{5399E694-6CE5-4D6C-8FCE-1D8870FDCBA0}",
            -27, 21,
            L"%SystemRoot%\\System32\\imageres.dll,-27",
            L"System_ControlPanel",
            false
        },
        {
            L"Сеть (Network)",
            L"network сеть сетевое окружение",
            L"{F02C1A0D-BE21-4350-88B0-7367FC96EF3C}",
            -25, 17,
            L"%SystemRoot%\\System32\\imageres.dll,-25",
            L"System_Network",
            false
        }
    };

    for (const auto& d : defs) {
        AppEntry e;
        e.name = d.name;
        e.searchKeywords = d.keywords;
        e.targetPath = imageresDll;
        e.category = L"Значок Рабочего стола Windows";
        e.systemClsid = d.clsid;
        e.defaultRestoreIcon = d.defaultIconStr;
        e.isRecycleBin = d.isRecycleBin;
        e.isSystemPinnedTop = true;

        std::wstring regKey = std::wstring(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\") + d.clsid + L"\\DefaultIcon";
        std::wstring curVal = GetRegistryStringValue(HKEY_CURRENT_USER, regKey, NULL);
        if (curVal.find(L"\\IconMaker\\icons\\") != std::wstring::npos) {
            e.isApplied = true;
        }
        e.origBitmap = LoadSystemDllIcon(imageresDll, d.imageresNegId, shell32Dll, d.shell32FallbackIdx, d.cacheKey);
        g_allApps.push_back(e);
    }
}

static void ScanDirectoryForShortcuts(const std::wstring& dir, const std::wstring& categoryName, int depth = 0, bool isDesktopOrTaskbar = false) {
    if (depth > 3 || g_allApps.size() >= 170) return;
    std::wstring pattern = dir + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        std::wstring fullPath = dir + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            ScanDirectoryForShortcuts(fullPath, categoryName, depth + 1, isDesktopOrTaskbar);
        } else {
            std::wstring fname = fd.cFileName;
            bool isLnk = (fname.size() > 4 && _wcsicmp(fname.c_str() + fname.size() - 4, L".lnk") == 0);
            bool isUrl = (fname.size() > 4 && _wcsicmp(fname.c_str() + fname.size() - 4, L".url") == 0);
            if (isLnk || (isUrl && isDesktopOrTaskbar)) {
                std::wstring title = fname.substr(0, fname.size() - 4);
                std::wstring lowerTitle = title;
                for (auto& c : lowerTitle) c = towlower(c);
                if (lowerTitle.find(L"uninstall") != std::wstring::npos ||
                    lowerTitle.find(L"удалить") != std::wstring::npos ||
                    lowerTitle.find(L"деинстал") != std::wstring::npos) {
                    continue;
                }

                std::wstring target, customIcon;
                ResolveShortcut(fullPath, target, customIcon);

                // Merge File Explorer / Проводник shortcuts into our built-in File Explorer entry
                bool isExplorerLnk = (lowerTitle == L"file explorer" || lowerTitle == L"проводник" ||
                                      lowerTitle == L"windows explorer" || lowerTitle == L"file explorer (проводник)");
                if (!target.empty()) {
                    std::wstring baseExe = PathFindFileNameW(target.c_str());
                    if (_wcsicmp(baseExe.c_str(), L"explorer.exe") == 0) isExplorerLnk = true;
                }
                if (isExplorerLnk && !g_allApps.empty() && g_allApps[0].isFileExplorer) {
                    bool already = false;
                    for (const auto& p : g_allApps[0].allLnkPaths) {
                        if (_wcsicmp(p.c_str(), fullPath.c_str()) == 0) { already = true; break; }
                    }
                    if (!already) g_allApps[0].allLnkPaths.push_back(fullPath);
                    if (customIcon.find(L"\\IconMaker\\icons\\") != std::wstring::npos) {
                        g_allApps[0].isApplied = true;
                    }
                    continue;
                }

                if (isLnk && !isDesktopOrTaskbar && !target.empty()) {
                    std::wstring ext = PathFindExtensionW(target.c_str());
                    if (_wcsicmp(ext.c_str(), L".exe") != 0) continue;
                }

                bool exists = false;
                for (auto& existing : g_allApps) {
                    bool sameName = (_wcsicmp(existing.name.c_str(), title.c_str()) == 0);
                    bool sameTarget = (!target.empty() && !existing.targetPath.empty() &&
                                       _wcsicmp(existing.targetPath.c_str(), target.c_str()) == 0);
                    if (sameName || sameTarget) {
                        bool pathAlreadyAdded = false;
                        for (const auto& p : existing.allLnkPaths) {
                            if (_wcsicmp(p.c_str(), fullPath.c_str()) == 0) {
                                pathAlreadyAdded = true;
                                break;
                            }
                        }
                        if (!pathAlreadyAdded) {
                            existing.allLnkPaths.push_back(fullPath);
                        }
                        if (customIcon.find(L"\\IconMaker\\icons\\") != std::wstring::npos) {
                            existing.isApplied = true;
                        }
                        exists = true;
                        break;
                    }
                }
                if (exists) continue;

                AppEntry entry;
                entry.name = title;
                entry.lnkPath = fullPath;
                entry.allLnkPaths.push_back(fullPath);
                std::wstring iconSource = (!customIcon.empty() && customIcon.find(L"\\IconMaker\\icons\\") == std::wstring::npos)
                    ? customIcon
                    : (target.empty() ? fullPath : target);
                entry.targetPath = target.empty() ? iconSource : target;
                entry.category = categoryName;
                entry.isApplied = (customIcon.find(L"\\IconMaker\\icons\\") != std::wstring::npos);

                // If shortcut already has an IconMaker icon and we have a .bak shortcut, resolve original icon from .bak
                std::wstring safe = SanitizeFileName(entry.name);
                if (entry.isApplied) {
                    std::wstring bak0 = GetAppDataRoot() + L"\\backup\\" + safe + L"_0.lnk.bak";
                    if (GetFileAttributesW(bak0.c_str()) != INVALID_FILE_ATTRIBUTES) {
                        std::wstring bTarget, bIcon;
                        ResolveShortcut(bak0, bTarget, bIcon);
                        if (!bIcon.empty() && bIcon.find(L"\\IconMaker\\icons\\") == std::wstring::npos) {
                            iconSource = bIcon;
                        } else if (!bTarget.empty()) {
                            iconSource = bTarget;
                        }
                    }
                }

                entry.origBitmap = LoadHighResIcon(iconSource, entry.lnkPath, safe);
                g_allApps.push_back(entry);
            }
        }
    } while (FindNextFileW(hFind, &fd) && g_allApps.size() < 170);

    FindClose(hFind);
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

    // 1. Always add File Explorer (Проводник) and Windows Desktop System Icons first
    AddBuiltInSystemEntries();

    // 2. Scan Taskbar, Desktop, and Start Menu shortcuts
    wchar_t appData[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData)) && appData[0]) {
        std::wstring taskbarDir = std::wstring(appData) + L"\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar";
        if (GetFileAttributesW(taskbarDir.c_str()) != INVALID_FILE_ATTRIBUTES) {
            ScanDirectoryForShortcuts(taskbarDir, L"Панель задач", 0, true);
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
            ScanDirectoryForShortcuts(path, labels[i], 0, isDesk[i]);
        }
    }

    std::sort(g_allApps.begin(), g_allApps.end(), [](const AppEntry& a, const AppEntry& b) {
        if (a.isSystemPinnedTop != b.isSystemPinnedTop) return a.isSystemPinnedTop;
        if (a.isFileExplorer != b.isFileExplorer) return a.isFileExplorer;
        return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0;
    });

    RebuildFilteredList();
}

static bool ApplyRoundIconToEntry(AppEntry& app, bool* outNeedsAdmin = nullptr) {
    std::wstring root = GetAppDataRoot();
    std::wstring safe = SanitizeFileName(app.name);
    std::wstring iconsDir = root + L"\\icons";

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW((iconsDir + L"\\" + safe + L"*.ico").c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            DeleteFileW((iconsDir + L"\\" + fd.cFileName).c_str());
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }

    wchar_t revSuffix[64];
    wsprintfW(revSuffix, L"_%u_%u.ico", (unsigned)(GetTickCount() & 0xFFFF), ++g_iconRevCounter);
    std::wstring icoPath = iconsDir + L"\\" + safe + revSuffix;

    if (!SaveMultiSizeIco(app.origBitmap, g_style, icoPath)) return false;

    if (app.allLnkPaths.empty() && !app.lnkPath.empty()) {
        app.allLnkPaths.push_back(app.lnkPath);
    }

    int updatedCount = 0;
    bool permissionDenied = false;

    // 1. Update Registry CLSID DefaultIcon if this is a Windows System Icon or File Explorer
    if (!app.systemClsid.empty()) {
        std::wstring icoWithIdx = icoPath + L",0";
        std::wstring explorerClsidKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\" + app.systemClsid + L"\\DefaultIcon";
        std::wstring classesClsidKey  = L"Software\\Classes\\CLSID\\" + app.systemClsid + L"\\DefaultIcon";

        if (SetRegistryStringValue(HKEY_CURRENT_USER, explorerClsidKey, NULL, icoWithIdx)) {
            updatedCount++;
        }
        if (app.isRecycleBin) {
            SetRegistryStringValue(HKEY_CURRENT_USER, explorerClsidKey, L"empty", icoWithIdx);
            SetRegistryStringValue(HKEY_CURRENT_USER, explorerClsidKey, L"full",  icoWithIdx);
        }
        if (SetRegistryStringValue(HKEY_CURRENT_USER, classesClsidKey, NULL, icoWithIdx)) {
            updatedCount++;
        }
        if (app.isRecycleBin) {
            SetRegistryStringValue(HKEY_CURRENT_USER, classesClsidKey, L"empty", icoWithIdx);
            SetRegistryStringValue(HKEY_CURRENT_USER, classesClsidKey, L"full",  icoWithIdx);
        }
    }

    // 2. Special handling for File Explorer (Проводник): ensure Taskbar & Start Menu shortcuts are updated
    if (app.isFileExplorer) {
        std::wstring icoWithIdx = icoPath + L",0";
        SetRegistryStringValue(HKEY_CURRENT_USER, L"Software\\Classes\\Applications\\explorer.exe\\DefaultIcon", NULL, icoWithIdx);

        wchar_t appData[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData)) && appData[0]) {
            std::wstring tbDir = std::wstring(appData) + L"\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar";
            if (GetFileAttributesW(tbDir.c_str()) != INVALID_FILE_ATTRIBUTES) {
                std::wstring tbExplorerLnk = tbDir + L"\\File Explorer.lnk";
                if (GetFileAttributesW(tbExplorerLnk.c_str()) == INVALID_FILE_ATTRIBUTES) {
                    // Create Taskbar File Explorer shortcut if not present yet
                    IShellLinkW* psl = nullptr;
                    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl)) && psl) {
                        psl->SetPath(app.targetPath.c_str());
                        psl->SetIconLocation(icoPath.c_str(), 0);
                        IPersistFile* ppf = nullptr;
                        if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)) && ppf) {
                            if (SUCCEEDED(ppf->Save(tbExplorerLnk.c_str(), TRUE))) {
                                app.allLnkPaths.push_back(tbExplorerLnk);
                                updatedCount++;
                            }
                            ppf->Release();
                        }
                        psl->Release();
                    }
                }
            }
        }
    }

    // 3. Update all associated .lnk and .url shortcuts
    for (size_t idx = 0; idx < app.allLnkPaths.size(); ++idx) {
        const std::wstring& lnk = app.allLnkPaths[idx];
        if (lnk.empty() || GetFileAttributesW(lnk.c_str()) == INVALID_FILE_ATTRIBUTES) continue;

        wchar_t bakName[MAX_PATH];
        wsprintfW(bakName, L"%s_%u.lnk.bak", safe.c_str(), (unsigned)idx);
        std::wstring backupLnk = root + L"\\backup\\" + bakName;
        if (GetFileAttributesW(backupLnk.c_str()) == INVALID_FILE_ATTRIBUTES) {
            CopyFileW(lnk.c_str(), backupLnk.c_str(), TRUE);
        }

        if (SetShortcutIconLocation(lnk, icoPath)) {
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW | SHCNF_FLUSHNOWAIT, lnk.c_str(), NULL);
            updatedCount++;
        } else {
            permissionDenied = true;
        }
    }

    if (updatedCount == 0 && !permissionDenied && app.systemClsid.empty()) {
        wchar_t desktopDir[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopDir))) {
            std::wstring deskLnk = std::wstring(desktopDir) + L"\\" + safe + L".lnk";
            IShellLinkW* psl = nullptr;
            if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl)) && psl) {
                psl->SetPath(app.targetPath.c_str());
                psl->SetIconLocation(icoPath.c_str(), 0);
                IPersistFile* ppf = nullptr;
                if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)) && ppf) {
                    if (SUCCEEDED(ppf->Save(deskLnk.c_str(), TRUE))) {
                        app.allLnkPaths.push_back(deskLnk);
                        TouchFileModificationTime(deskLnk);
                        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW | SHCNF_FLUSHNOWAIT, deskLnk.c_str(), NULL);
                        updatedCount++;
                    }
                    ppf->Release();
                }
                psl->Release();
            }
        }
    }

    if (outNeedsAdmin && permissionDenied) *outNeedsAdmin = true;
    app.isApplied = (updatedCount > 0 && !permissionDenied);
    return updatedCount > 0;
}

static bool RestoreOriginalEntry(AppEntry& app) {
    if (!app.systemClsid.empty() && !app.defaultRestoreIcon.empty()) {
        std::wstring explorerClsidKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\" + app.systemClsid + L"\\DefaultIcon";
        std::wstring classesClsidKey  = L"Software\\Classes\\CLSID\\" + app.systemClsid + L"\\DefaultIcon";
        SetRegistryStringValue(HKEY_CURRENT_USER, explorerClsidKey, NULL, app.defaultRestoreIcon);
        SetRegistryStringValue(HKEY_CURRENT_USER, classesClsidKey,  NULL, app.defaultRestoreIcon);
        if (app.isRecycleBin) {
            SetRegistryStringValue(HKEY_CURRENT_USER, explorerClsidKey, L"empty", L"%SystemRoot%\\System32\\imageres.dll,-55");
            SetRegistryStringValue(HKEY_CURRENT_USER, explorerClsidKey, L"full",  L"%SystemRoot%\\System32\\imageres.dll,-54");
            SetRegistryStringValue(HKEY_CURRENT_USER, classesClsidKey,  L"empty", L"%SystemRoot%\\System32\\imageres.dll,-55");
            SetRegistryStringValue(HKEY_CURRENT_USER, classesClsidKey,  L"full",  L"%SystemRoot%\\System32\\imageres.dll,-54");
        }
    }
    if (app.isFileExplorer) {
        SetRegistryStringValue(HKEY_CURRENT_USER, L"Software\\Classes\\Applications\\explorer.exe\\DefaultIcon", NULL, app.defaultRestoreIcon);
    }

    if (app.allLnkPaths.empty() && !app.lnkPath.empty()) {
        app.allLnkPaths.push_back(app.lnkPath);
    }
    std::wstring root = GetAppDataRoot();
    std::wstring safe = SanitizeFileName(app.name);
    for (size_t idx = 0; idx < app.allLnkPaths.size(); ++idx) {
        const std::wstring& lnk = app.allLnkPaths[idx];
        wchar_t bakName[MAX_PATH];
        wsprintfW(bakName, L"%s_%u.lnk.bak", safe.c_str(), (unsigned)idx);
        std::wstring backupLnk = root + L"\\backup\\" + bakName;
        if (GetFileAttributesW(backupLnk.c_str()) != INVALID_FILE_ATTRIBUTES) {
            CopyFileW(backupLnk.c_str(), lnk.c_str(), FALSE);
            TouchFileModificationTime(lnk);
        } else if (!app.targetPath.empty()) {
            SetShortcutIconLocation(lnk, app.targetPath);
        }
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW | SHCNF_FLUSHNOWAIT, lnk.c_str(), NULL);
    }
    app.isApplied = false;
    return true;
}

static void InstallSelfAndShortcuts(HWND hwnd, bool showConfirmation) {
    wchar_t selfPath[MAX_PATH * 2] = {0};
    if (!GetModuleFileNameW(NULL, selfPath, MAX_PATH * 2 - 1)) return;

    wchar_t localAppData[MAX_PATH] = {0};
    if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData))) return;

    std::wstring progDir = std::wstring(localAppData) + L"\\Programs";
    CreateDirectoryW(progDir.c_str(), NULL);
    std::wstring appDir = progDir + L"\\IconMaker";
    CreateDirectoryW(appDir.c_str(), NULL);

    std::wstring installedExe = appDir + L"\\IconMaker.exe";
    if (_wcsicmp(selfPath, installedExe.c_str()) != 0) {
        CopyFileW(selfPath, installedExe.c_str(), FALSE);
    } else {
        installedExe = selfPath;
    }

    auto makeShortcut = [&](int csidl, const wchar_t* lnkName) {
        wchar_t folder[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(NULL, csidl, NULL, 0, folder)) && folder[0]) {
            std::wstring lnkPath = std::wstring(folder) + L"\\" + lnkName;
            IShellLinkW* psl = nullptr;
            if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl)) && psl) {
                psl->SetPath(installedExe.c_str());
                psl->SetWorkingDirectory(appDir.c_str());
                psl->SetIconLocation(installedExe.c_str(), 0);
                psl->SetDescription(L"IconMaker Studio — Круглые иконки Google Pixel");
                IPersistFile* ppf = nullptr;
                if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)) && ppf) {
                    ppf->Save(lnkPath.c_str(), TRUE);
                    ppf->Release();
                }
                psl->Release();
            }
        }
    };

    makeShortcut(CSIDL_DESKTOPDIRECTORY, L"IconMaker Studio.lnk");
    makeShortcut(CSIDL_PROGRAMS, L"IconMaker Studio.lnk");

    g_statusText = L"Ярлыки IconMaker Studio созданы на Рабочем столе и в меню «Пуск»";
    InvalidateRect(hwnd, NULL, FALSE);
    if (showConfirmation) {
        MessageBoxW(
            hwnd,
            L"IconMaker Studio успешно установлен!\n\n"
            L"• Папка: %LOCALAPPDATA%\\Programs\\IconMaker\\IconMaker.exe\n"
            L"• Ярлыки созданы на Рабочем столе и в меню «Пуск».",
            L"IconMaker Studio",
            MB_ICONINFORMATION
        );
    }
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

    // Fluent Design Dark Control Palette (Windows 11 specs)
    Color fill(255, 45, 45, 45);
    Color border(255, 68, 68, 68);
    Color textClr(255, 255, 255, 255);

    if (isPrimary) {
        // Windows 11 Primary Accent #0067C0
        fill = pressed ? Color(255, 0, 90, 168)
             : (hovered ? Color(255, 25, 120, 215) : Color(255, 0, 103, 192));
        border = Color(255, 76, 161, 255);
        textClr = Color(255, 255, 255, 255);
    } else if (isSelected) {
        // Subtle Selected Pill Accent
        fill = Color(255, 0, 90, 168);
        border = Color(255, 76, 161, 255);
        textClr = Color(255, 255, 255, 255);
    } else if (isDanger) {
        fill = hovered ? Color(255, 180, 40, 40) : Color(255, 140, 32, 32);
        border = Color(255, 220, 70, 70);
        textClr = Color(255, 255, 255, 255);
    } else if (pressed) {
        fill = Color(255, 38, 38, 38);
        border = Color(255, 80, 80, 80);
    } else if (hovered) {
        fill = Color(255, 54, 54, 54);
        border = Color(255, 88, 88, 88);
    }

    REAL rad = std::min(6.0f, r.Height * 0.5f); // Windows 11 4-6px corner radius
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

    // Track groove (Windows 11 Fluent Slider)
    RectF groove(trackRect.X, trackRect.Y + trackRect.Height * 0.5f - 2.5f, trackRect.Width, 5.0f);
    FillRoundRect(g, groove, 2.5f, Color(255, 52, 52, 52));
    DrawRoundRect(g, groove, 2.5f, Color(255, 72, 72, 72), 1.0f);

    // Active fill
    REAL fillW = std::max(5.0f, (REAL)(groove.Width * v));
    RectF activeBar(groove.X, groove.Y, fillW, groove.Height);
    FillRoundRect(g, activeBar, 2.5f, Color(255, 96, 205, 255)); // Win11 Accent Light

    // Thumb knob
    REAL thumbX = groove.X + (REAL)(groove.Width * v);
    REAL thumbY = groove.Y + groove.Height * 0.5f;
    REAL thumbR = hovered ? 8.0f : 7.0f;

    SolidBrush shadowBr(Color(90, 0, 0, 0));
    g.FillEllipse(&shadowBr, thumbX - thumbR, thumbY - thumbR + 1.5f, thumbR * 2.0f, thumbR * 2.0f);

    SolidBrush thumbBr(Color(255, 255, 255, 255));
    g.FillEllipse(&thumbBr, thumbX - thumbR, thumbY - thumbR, thumbR * 2.0f, thumbR * 2.0f);
    Pen ringPen(Color(255, 0, 103, 192), 2.5f);
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

    // Windows 11 ToggleSwitch (40x20 pill)
    RectF pill(rowRect.X, rowRect.Y + (rowRect.Height - 20.0f) * 0.5f, 40.0f, 20.0f);
    Color pillFill = isOn
        ? (hovered ? Color(255, 25, 120, 215) : Color(255, 0, 103, 192))
        : (hovered ? Color(255, 55, 55, 55) : Color(255, 42, 42, 42));
    FillRoundRect(g, pill, 10.0f, pillFill);
    DrawRoundRect(g, pill, 10.0f, isOn ? Color(255, 96, 205, 255) : Color(255, 78, 78, 78), 1.0f);

    REAL knobX = isOn ? (pill.GetRight() - 17.0f) : (pill.X + 3.0f);
    SolidBrush knobBr(Color(255, 255, 255, 255));
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
    SolidBrush brandCircle(Color(255, 0, 103, 192)); // Windows 11 Accent Blue #0067C0
    g.FillEllipse(&brandCircle, 18.0f, 14.0f, 28.0f, 28.0f);
    SolidBrush innerWhite(Color(255, 255, 255, 255));
    g.FillEllipse(&innerWhite, 25.0f, 21.0f, 14.0f, 14.0f);

    SolidBrush whiteBr(Color(255, 255, 255, 255));
    SolidBrush mutedBr(Color(255, 160, 160, 160));
    SolidBrush accentBr(Color(255, 96, 205, 255)); // Fluent Accent Light

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
    FillRoundRect(g, searchRect, 8.0f, Color(255, 14, 15, 18));
    DrawRoundRect(g, searchRect, 8.0f, g_searchFocused ? Color(255, 26, 115, 232) : Color(255, 46, 49, 58), 1.2f);

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
            // Windows 11 Selected Item Fill #0067C0 with rounded pill outline
            FillRoundRect(g, rowRect, 6.0f, Color(255, 0, 90, 168));
            DrawRoundRect(g, rowRect, 6.0f, Color(255, 96, 205, 255), 1.0f);
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
            FillRoundRect(g, badgeR, 10.0f, Color(55, 52, 168, 83));
            DrawRoundRect(g, badgeR, 10.0f, Color(160, 72, 204, 108), 1.0f);
            StringFormat sfB; sfB.SetAlignment(StringAlignmentCenter); sfB.SetLineAlignment(StringAlignmentCenter);
            SolidBrush greenText(Color(255, 126, 238, 154));
            g.DrawString(L"ROUND", -1, &fSmallBold, badgeR, &sfB, &greenText);
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
        Pen sp(match ? Color(255, 96, 205, 255) : Color(120, 255, 255, 255), match ? 2.5f : 1.0f);
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
                ApplyRoundIconToEntry(app, &needsAdmin);
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
                if (ApplyRoundIconToEntry(app, &na)) count++;
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
            if (ApplyRoundIconToEntry(g_allApps[i], nullptr)) count++;
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
