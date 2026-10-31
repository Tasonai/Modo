/*
 * IconMaker for Windows (Google Pixel & Material You Circular Icon Studio)
 * 100% Native Win32 + GDI+ + Shell COM C++ Application (Zero external dependencies)
 *
 * Features:
 *   - Scans installed Windows apps & shortcuts (Desktop + Start Menu)
 *   - High-res 256x256 icon extraction via SHGetImageList (SHIL_JUMBO) & ExtractIconExW
 *   - Google Pixel / Material You circular icon renderer (GDI+ 1024x1024 / 256x256)
 *   - Zoom from 30% up to 250% (>100% inside circular clipping mask)
 *   - Pan Left / Right (X: -100%..+100%) and Up / Down (Y: -100%..+100%) via sliders,
 *     arrow buttons, and direct mouse drag + wheel zoom on the live preview
 *   - 1-click Apply to Selected / Apply to All / Restore Originals / Export .ICO & .PNG
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

// Control IDs
#define ID_EDIT_SEARCH      1001
#define ID_BTN_SCAN         1002
#define ID_BTN_ADD_FILE     1003
#define ID_LIST_APPS        1004
#define ID_BTN_INSTALL_SELF 1005

#define ID_CMB_PRESET       1010
#define ID_BTN_CUSTOM_CLR   1011
#define ID_BTN_EYEDROPPER   1012

#define ID_SLIDER_SCALE     1020
#define ID_BTN_SCALE_68     1021
#define ID_BTN_SCALE_100    1022
#define ID_BTN_SCALE_135    1023
#define ID_BTN_SCALE_175    1024

#define ID_SLIDER_OFFX      1030
#define ID_BTN_LEFT         1031
#define ID_BTN_RIGHT        1032

#define ID_SLIDER_OFFY      1040
#define ID_BTN_DOWN         1041
#define ID_BTN_UP           1042
#define ID_BTN_CENTER       1043

#define ID_SLIDER_SHADOW    1050

#define ID_CHK_AUTOTRIM     1060
#define ID_CHK_INNERSHAD    1061
#define ID_CHK_FULLBLEED    1062
#define ID_CHK_MONOCHROME   1063

#define ID_BTN_APPLY_ONE    1070
#define ID_BTN_APPLY_ALL    1071
#define ID_BTN_RESTORE_ONE  1072
#define ID_BTN_RESTORE_ALL  1073
#define ID_BTN_EXPORT       1074
#define ID_BTN_ADMIN        1075

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
    COLORREF customColor = RGB(232, 240, 254);
};

struct AppEntry {
    std::wstring name;
    std::wstring lnkPath;                    // Primary shortcut path (.lnk)
    std::vector<std::wstring> allLnkPaths;   // All shortcuts for this app (Desktop, Start Menu, Taskbar)
    std::wstring targetPath;                 // Target .exe or image path
    std::wstring category;
    bool isApplied = false;
    Bitmap* origBitmap = nullptr;
};

static ULONG_PTR g_gdiplusToken = 0;
static CLSID g_pngClsid = {0};
static RoundStyle g_style;
static std::vector<AppEntry> g_allApps;
static std::vector<int> g_filteredIndices;
static int g_selectedAppIndex = -1;

// Window handles
static HWND g_hMainWnd = nullptr;
static HWND g_hSearch = nullptr;
static HWND g_hList = nullptr;
static HWND g_hCmbPreset = nullptr;
static HWND g_hBtnColor = nullptr;
static HWND g_hLblScale = nullptr;
static HWND g_hSliderScale = nullptr;
static HWND g_hLblOffX = nullptr;
static HWND g_hSliderOffX = nullptr;
static HWND g_hLblOffY = nullptr;
static HWND g_hSliderOffY = nullptr;
static HWND g_hLblShadow = nullptr;
static HWND g_hSliderShadow = nullptr;
static HWND g_hChkAutoTrim = nullptr;
static HWND g_hChkInnerShad = nullptr;
static HWND g_hChkFullBleed = nullptr;
static HWND g_hChkMono = nullptr;
static HWND g_hStatus = nullptr;
static HWND g_hProgress = nullptr;

static HFONT g_fontTitle = nullptr;
static HFONT g_fontSection = nullptr;
static HFONT g_fontRegular = nullptr;
static HFONT g_fontSmall = nullptr;

// Preview canvas rectangle in main window client coordinates
static RECT g_previewRect = { 390, 62, 956, 282 };
static RECT g_roundHitRect = { 696, 86, 840, 230 };
static bool g_isDraggingPreview = false;
static bool g_isEyedropperActive = false;
static POINT g_dragStartPt = { 0, 0 };
static double g_dragStartOffX = 0.0;
static double g_dragStartOffY = 0.0;
static HBRUSH g_hBrWindowBg = nullptr;

// Forward declarations
static void RefreshFilteredList();
static void UpdateInspectorLabels();
static void InvalidatePreview();

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

// Convert HICON to 32-bit RGBA GDI+ Bitmap (preserving alpha channel)
static Bitmap* BitmapFromHIcon32(HICON hIcon, int targetSize = 256) {
    if (!hIcon) return nullptr;
    ICONINFO ii = {0};
    if (!GetIconInfo(hIcon, &ii)) return nullptr;

    BITMAP bm = {0};
    GetObject(ii.hbmColor ? ii.hbmColor : ii.hbmMask, sizeof(bm), &bm);
    int w = bm.bmWidth > 0 ? bm.bmWidth : targetSize;
    int h = bm.bmHeight > 0 ? bm.bmHeight : targetSize;

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<uint8_t> pixels(w * h * 4, 0);
    HDC hdc = GetDC(NULL);
    bool hasAlpha = false;
    if (ii.hbmColor) {
        GetDIBits(hdc, ii.hbmColor, 0, h, pixels.data(), &bmi, DIB_RGB_COLORS);
        for (size_t i = 3; i < pixels.size(); i += 4) {
            if (pixels[i] != 0) { hasAlpha = true; break; }
        }
    }
    ReleaseDC(NULL, hdc);

    Bitmap* out = new Bitmap(w, h, PixelFormat32bppARGB);
    if (hasAlpha) {
        BitmapData bd;
        Rect r(0, 0, w, h);
        if (out->LockBits(&r, ImageLockModeWrite, PixelFormat32bppARGB, &bd) == Ok) {
            for (int y = 0; y < h; ++y) {
                memcpy((uint8_t*)bd.Scan0 + y * bd.Stride, pixels.data() + y * w * 4, w * 4);
            }
            out->UnlockBits(&bd);
        }
    } else {
        Graphics g(out);
        g.Clear(Color(0, 0, 0, 0));
        HDC memDC = g.GetHDC();
        DrawIconEx(memDC, 0, 0, hIcon, w, h, 0, NULL, DI_NORMAL);
        g.ReleaseHDC(memDC);
    }

    if (ii.hbmColor) DeleteObject(ii.hbmColor);
    if (ii.hbmMask) DeleteObject(ii.hbmMask);
    return out;
}

// Extract 256x256 Jumbo Icon from a file or shortcut
static Bitmap* LoadHighResIcon(const std::wstring& primaryPath, const std::wstring& fallbackPath) {
    // Check if image file directly (.png, .ico, .bmp, .jpg)
    std::wstring ext = PathFindExtensionW(primaryPath.c_str());
    for (auto& c : ext) c = towlower(c);
    if (ext == L".png" || ext == L".bmp" || ext == L".jpg" || ext == L".jpeg") {
        Bitmap* bmp = Bitmap::FromFile(primaryPath.c_str());
        if (bmp && bmp->GetLastStatus() == Ok && bmp->GetWidth() > 0) return bmp;
        delete bmp;
    }

    const std::wstring& pathToUse = (!fallbackPath.empty() && GetFileAttributesW(fallbackPath.c_str()) != INVALID_FILE_ATTRIBUTES)
        ? fallbackPath
        : primaryPath;

    SHFILEINFOW sfi = {0};
    if (SHGetFileInfoW(pathToUse.c_str(), 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX)) {
        IImageList* pImgList = nullptr;
        static const GUID kIID_IImageList = { 0x46eb5926, 0x582e, 0x4017, { 0x9f, 0xdf, 0xe8, 0x99, 0x8d, 0xaa, 0x09, 0x50 } };
        if (SUCCEEDED(SHGetImageList(SHIL_JUMBO, kIID_IImageList, (void**)&pImgList)) && pImgList) {
            HICON hIcon = nullptr;
            if (SUCCEEDED(pImgList->GetIcon(sfi.iIcon, ILD_TRANSPARENT, &hIcon)) && hIcon) {
                Bitmap* bmp = BitmapFromHIcon32(hIcon, 256);
                DestroyIcon(hIcon);
                pImgList->Release();
                if (bmp) return bmp;
            }
            pImgList->Release();
        }
    }

    HICON hLarge = nullptr;
    if (ExtractIconExW(pathToUse.c_str(), 0, &hLarge, NULL, 1) > 0 && hLarge) {
        Bitmap* bmp = BitmapFromHIcon32(hLarge, 128);
        DestroyIcon(hLarge);
        if (bmp) return bmp;
    }

    if (SHGetFileInfoW(pathToUse.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON) && sfi.hIcon) {
        Bitmap* bmp = BitmapFromHIcon32(sfi.hIcon, 128);
        DestroyIcon(sfi.hIcon);
        if (bmp) return bmp;
    }

    // Fallback synthesized Google-style app icon
    Bitmap* synth = new Bitmap(256, 256, PixelFormat32bppARGB);
    Graphics g(synth);
    g.SetSmoothingMode(SmoothingModeHighQuality);
    SolidBrush bg(Color(255, 26, 115, 232));
    g.FillEllipse(&bg, 24, 24, 208, 208);
    return synth;
}

// Auto-trim transparent margins around a Bitmap
static Bitmap* TrimTransparentEdges(Bitmap* src) {
    if (!src) return nullptr;
    int w = (int)src->GetWidth();
    int h = (int)src->GetHeight();
    if (w <= 16 || h <= 16) return src->Clone(0, 0, w, h, PixelFormat32bppARGB);

    BitmapData bd;
    Rect r(0, 0, w, h);
    if (src->LockBits(&r, ImageLockModeRead, PixelFormat32bppARGB, &bd) != Ok) {
        return src->Clone(0, 0, w, h, PixelFormat32bppARGB);
    }

    int minX = w, minY = h, maxX = -1, maxY = -1;
    for (int y = 0; y < h; ++y) {
        const uint8_t* row = (const uint8_t*)bd.Scan0 + y * bd.Stride;
        for (int x = 0; x < w; ++x) {
            uint8_t a = row[x * 4 + 3];
            if (a > 28) {
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
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
};

static ExtractedColors AnalyzeBitmapColors(Bitmap* src) {
    ExtractedColors res = { Color(255, 26, 115, 232), Color(255, 240, 244, 249), false };
    if (!src) return res;

    const int S = 64;
    Bitmap sample(S, S, PixelFormat32bppARGB);
    {
        Graphics g(&sample);
        g.SetInterpolationMode(InterpolationModeBilinear);
        g.DrawImage(src, Rect(0, 0, S, S));
    }

    BitmapData bd;
    Rect r(0, 0, S, S);
    if (sample.LockBits(&r, ImageLockModeRead, PixelFormat32bppARGB, &bd) != Ok) return res;

    double totR = 0, totG = 0, totB = 0, totW = 0;
    double edgR = 0, edgG = 0, edgB = 0, edgN = 0;
    int opaqueCount = 0;

    for (int y = 0; y < S; ++y) {
        const uint8_t* row = (const uint8_t*)bd.Scan0 + y * bd.Stride;
        for (int x = 0; x < S; ++x) {
            uint8_t b = row[x * 4 + 0];
            uint8_t g = row[x * 4 + 1];
            uint8_t rv = row[x * 4 + 2];
            uint8_t a = row[x * 4 + 3];
            if (a < 64) continue;
            opaqueCount++;

            double maxC = std::max({ (double)rv, (double)g, (double)b });
            double minC = std::min({ (double)rv, (double)g, (double)b });
            double sat = maxC > 0 ? (maxC - minC) / maxC : 0.0;
            double w = 0.25 + sat * 2.5;
            totR += rv * w;
            totG += g * w;
            totB += b * w;
            totW += w;

            if (x < 6 || x >= S - 6 || y < 6 || y >= S - 6) {
                edgR += rv; edgG += g; edgB += b; edgN += 1.0;
            }
        }
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
    res.isSquareLike = (double(opaqueCount) / double(S * S)) > 0.72;
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

// Render the Google Pixel / Material You circular icon at `canvasSize`
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

    REAL dim = (REAL)canvasSize;
    REAL margin = dim * 0.075f;
    RectF circleRect(margin, margin, dim - margin * 2.0f, dim - margin * 2.0f);

    // 1. Soft multi-layer elevation drop shadow
    if (style.shadow > 0.01) {
        int layers = 10;
        REAL maxBlur = dim * (REAL)(0.045 * style.shadow);
        REAL yOff = dim * (REAL)(0.018 * style.shadow);
        for (int i = layers; i >= 1; --i) {
            REAL t = (REAL)i / (REAL)layers;
            REAL expand = maxBlur * t;
            BYTE alpha = (BYTE)std::clamp((int)(38.0 * style.shadow * (1.0 - t * 0.72)), 1, 120);
            SolidBrush sb(Color(alpha, 0, 0, 0));
            RectF sr(circleRect.X - expand * 0.5f,
                     circleRect.Y - expand * 0.5f + yOff * t,
                     circleRect.Width + expand,
                     circleRect.Height + expand);
            g.FillEllipse(&sb, sr);
        }
    }

    // 2. Surface background gradient
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

    // 3. Circular clipping mask for foreground glyph (supports >100% zoom up to 250% and X/Y pan)
    GraphicsPath clipPath;
    clipPath.AddEllipse(circleRect);
    GraphicsState state = g.Save();
    g.SetClip(&clipPath, CombineModeReplace);

    double effScale = (style.fullBleed && colors.isSquareLike)
        ? std::max(style.scale, 1.04)
        : style.scale;
    REAL maxSpan = circleRect.Width * (REAL)effScale;
    REAL srcW = (REAL)std::max(1u, trimmed->GetWidth());
    REAL srcH = (REAL)std::max(1u, trimmed->GetHeight());
    REAL aspect = srcW / srcH;
    REAL drawW = (aspect >= 1.0f) ? maxSpan : maxSpan * aspect;
    REAL drawH = (aspect >= 1.0f) ? maxSpan / aspect : maxSpan;

    // Note: in GDI+, Y increases downward, so positive offsetY (Up) subtracts from Y
    REAL shiftX = (REAL)style.offsetX * (circleRect.Width * 0.5f);
    REAL shiftY = -(REAL)style.offsetY * (circleRect.Height * 0.5f);
    REAL cx = circleRect.X + circleRect.Width * 0.5f + shiftX;
    REAL cy = circleRect.Y + circleRect.Height * 0.5f + shiftY;
    RectF glyphRect(cx - drawW * 0.5f, cy - drawH * 0.5f, drawW, drawH);

    // Optional inner glyph drop shadow
    if (style.innerShadow && !(style.fullBleed && colors.isSquareLike)) {
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

    // 4. Crisp circular rim border
    REAL penW = std::max(1.0f, dim * 0.004f);
    Color rimColor = (style.preset == PRESET_DARK_MATERIAL)
        ? Color(36, 255, 255, 255)
        : Color(22, 0, 0, 0);
    Pen rimPen(rimColor, penW);
    RectF rimRect = circleRect;
    rimRect.Inflate(-penW * 0.5f, -penW * 0.5f);
    g.DrawEllipse(&rimPen, rimRect);

    delete trimmed;
    return out;
}

// Encode a GDI+ Bitmap to an in-memory PNG byte buffer at `targetSize`
static bool EncodeBitmapToPngBytes(Bitmap* master, int targetSize, std::vector<uint8_t>& outBytes) {
    Bitmap scaled(targetSize, targetSize, PixelFormat32bppARGB);
    {
        Graphics g(&scaled);
        g.SetSmoothingMode(SmoothingModeHighQuality);
        g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        g.DrawImage(master, Rect(0, 0, targetSize, targetSize));
    }

    IStream* pStream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(NULL, TRUE, &pStream)) || !pStream) return false;
    if (scaled.Save(pStream, &g_pngClsid, NULL) != Ok) {
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

// Save multi-size Windows .ico (256, 128, 64, 48, 32, 16)
static bool SaveMultiSizeIco(Bitmap* master, const std::wstring& icoPath) {
    const int sizes[] = { 256, 128, 64, 48, 32, 16 };
    const int count = 6;
    std::vector<std::vector<uint8_t>> pngFrames(count);
    for (int i = 0; i < count; ++i) {
        if (!EncodeBitmapToPngBytes(master, sizes[i], pngFrames[i])) return false;
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
        entry[2] = 0;
        entry[3] = 0;
        entry[4] = 1; entry[5] = 0;   // planes = 1
        entry[6] = 32; entry[7] = 0;  // bpp = 32
        uint32_t len = (uint32_t)pngFrames[i].size();
        memcpy(entry + 8, &len, 4);
        memcpy(entry + 12, &offset, 4);
        WriteFile(hFile, entry, 16, &written, NULL);
        offset += len;
    }

    for (int i = 0; i < count; ++i) {
        WriteFile(hFile, pngFrames[i].data(), (DWORD)pngFrames[i].size(), &written, NULL);
    }

    CloseHandle(hFile);
    return true;
}

// Resolve a .lnk shortcut's target path
static bool ResolveShortcut(const std::wstring& lnkPath, std::wstring& outTarget, std::wstring& outCustomIcon) {
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
    return !outTarget.empty();
}

static bool SetShortcutIconLocation(const std::wstring& lnkPath, const std::wstring& icoPath) {
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
    return ok;
}

static void ScanDirectoryForShortcuts(const std::wstring& dir, const std::wstring& categoryName, int depth = 0) {
    if (depth > 3 || g_allApps.size() >= 120) return;
    std::wstring pattern = dir + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        std::wstring fullPath = dir + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            ScanDirectoryForShortcuts(fullPath, categoryName, depth + 1);
        } else {
            std::wstring fname = fd.cFileName;
            if (fname.size() > 4 && _wcsicmp(fname.c_str() + fname.size() - 4, L".lnk") == 0) {
                std::wstring title = fname.substr(0, fname.size() - 4);
                // Skip uninstallers
                std::wstring lowerTitle = title;
                for (auto& c : lowerTitle) c = towlower(c);
                if (lowerTitle.find(L"uninstall") != std::wstring::npos ||
                    lowerTitle.find(L"удалить") != std::wstring::npos ||
                    lowerTitle.find(L"деинстал") != std::wstring::npos) {
                    continue;
                }

                std::wstring target, customIcon;
                ResolveShortcut(fullPath, target, customIcon);
                if (!target.empty()) {
                    std::wstring ext = PathFindExtensionW(target.c_str());
                    if (_wcsicmp(ext.c_str(), L".exe") != 0) continue;
                }

                // If an app with the same title or target .exe already exists, attach this .lnk to its allLnkPaths!
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
                entry.targetPath = target.empty() ? fullPath : target;
                entry.category = categoryName;
                entry.isApplied = (customIcon.find(L"\\IconMaker\\icons\\") != std::wstring::npos);
                // Always load the original icon from the .exe targetPath so already-applied .lnk icons don't overwrite the source
                entry.origBitmap = LoadHighResIcon(entry.targetPath, entry.lnkPath);
                g_allApps.push_back(entry);
            }
        }
    } while (FindNextFileW(hFind, &fd) && g_allApps.size() < 120);

    FindClose(hFind);
}

static void ScanInstalledWindowsApps() {
    for (auto& app : g_allApps) {
        delete app.origBitmap;
    }
    g_allApps.clear();

    // 1. Scan Taskbar Pinned shortcuts (%APPDATA%\Microsoft\Internet Explorer\Quick Launch\User Pinned\TaskBar)
    wchar_t appData[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData)) && appData[0]) {
        std::wstring taskbarDir = std::wstring(appData) + L"\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar";
        if (GetFileAttributesW(taskbarDir.c_str()) != INVALID_FILE_ATTRIBUTES) {
            ScanDirectoryForShortcuts(taskbarDir, L"Панель задач", 0);
        }
    }

    // 2. Scan User & Public Desktop and Start Menu
    const int folders[] = {
        CSIDL_DESKTOPDIRECTORY,
        CSIDL_PROGRAMS,
        CSIDL_COMMON_DESKTOPDIRECTORY,
        CSIDL_COMMON_PROGRAMS
    };
    const wchar_t* labels[] = {
        L"Рабочий стол",
        L"Меню «Пуск»",
        L"Общий Рабочий стол",
        L"Программы Windows"
    };

    for (int i = 0; i < 4; ++i) {
        wchar_t path[MAX_PATH] = {0};
        if (SUCCEEDED(SHGetFolderPathW(NULL, folders[i], NULL, 0, path)) && path[0]) {
            ScanDirectoryForShortcuts(path, labels[i], 0);
        }
    }

    std::sort(g_allApps.begin(), g_allApps.end(), [](const AppEntry& a, const AppEntry& b) {
        return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0;
    });

    RefreshFilteredList();
}

static void RefreshFilteredList() {
    wchar_t query[256] = {0};
    if (g_hSearch) GetWindowTextW(g_hSearch, query, 255);
    std::wstring q = query;
    for (auto& c : q) c = towlower(c);

    g_filteredIndices.clear();
    SendMessageW(g_hList, LB_RESETCONTENT, 0, 0);

    for (size_t i = 0; i < g_allApps.size(); ++i) {
        std::wstring lowerName = g_allApps[i].name;
        for (auto& c : lowerName) c = towlower(c);
        if (!q.empty() && lowerName.find(q) == std::wstring::npos) continue;

        g_filteredIndices.push_back((int)i);
        std::wstring rowText = g_allApps[i].isApplied
            ? (L"● [Круглая]  " + g_allApps[i].name + L"  —  " + g_allApps[i].category)
            : (L"○ " + g_allApps[i].name + L"  —  " + g_allApps[i].category);
        SendMessageW(g_hList, LB_ADDSTRING, 0, (LPARAM)rowText.c_str());
    }

    if (!g_filteredIndices.empty()) {
        g_selectedAppIndex = g_filteredIndices[0];
        SendMessageW(g_hList, LB_SETCURSEL, 0, 0);
    } else {
        g_selectedAppIndex = -1;
    }
    InvalidatePreview();
}

static uint32_t g_iconRevCounter = 1;

static bool ApplyRoundIconToEntry(AppEntry& app, bool* outNeedsAdmin = nullptr) {
    std::wstring root = GetAppDataRoot();
    std::wstring safe = SanitizeFileName(app.name);
    std::wstring iconsDir = root + L"\\icons";

    // Remove previous revision .ico files for this app so disk stays clean
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW((iconsDir + L"\\" + safe + L"*.ico").c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            DeleteFileW((iconsDir + L"\\" + fd.cFileName).c_str());
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }

    // Use a unique filename revision so Windows Explorer icon cache reloads immediately!
    wchar_t revSuffix[64];
    wsprintfW(revSuffix, L"_%u_%u.ico", (unsigned)(GetTickCount() & 0xFFFF), ++g_iconRevCounter);
    std::wstring icoPath = iconsDir + L"\\" + safe + revSuffix;

    Bitmap* master = RenderGoogleRoundIcon(app.origBitmap, g_style, 512);
    bool saved = SaveMultiSizeIco(master, icoPath);
    delete master;
    if (!saved) return false;

    if (app.allLnkPaths.empty() && !app.lnkPath.empty()) {
        app.allLnkPaths.push_back(app.lnkPath);
    }

    int updatedCount = 0;
    bool permissionDenied = false;

    for (size_t idx = 0; idx < app.allLnkPaths.size(); ++idx) {
        const std::wstring& lnk = app.allLnkPaths[idx];
        if (lnk.empty() || GetFileAttributesW(lnk.c_str()) == INVALID_FILE_ATTRIBUTES) continue;

        wchar_t bakName[MAX_PATH];
        wsprintfW(bakName, L"%s_%u.lnk.bak", safe.c_str(), (unsigned)idx);
        std::wstring backupLnk = root + L"\\backup\\" + bakName;
        if (GetFileAttributesW(backupLnk.c_str()) == INVALID_FILE_ATTRIBUTES) {
            CopyFileW(lnk.c_str(), backupLnk.c_str(), TRUE);
        }

        // Clear read-only attribute if set
        DWORD attrs = GetFileAttributesW(lnk.c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY)) {
            SetFileAttributesW(lnk.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
        }

        if (SetShortcutIconLocation(lnk, icoPath)) {
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW | SHCNF_FLUSHNOWAIT, lnk.c_str(), NULL);
            updatedCount++;
        } else {
            permissionDenied = true;
        }
    }

    // If a system-wide shortcut in C:\Users\Public\Desktop or C:\ProgramData couldn't be written without Admin,
    // also create/update a shortcut directly on the user's personal Desktop so the round icon appears immediately!
    if (updatedCount == 0) {
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
                        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW | SHCNF_FLUSHNOWAIT, deskLnk.c_str(), NULL);
                        updatedCount++;
                    }
                    ppf->Release();
                }
                psl->Release();
            }
        }
    }

    if (outNeedsAdmin && permissionDenied) {
        *outNeedsAdmin = true;
    }
    app.isApplied = (updatedCount > 0);
    return app.isApplied;
}

static bool RestoreOriginalEntry(AppEntry& app) {
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
        } else if (!app.targetPath.empty()) {
            SetShortcutIconLocation(lnk, app.targetPath);
        }
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW | SHCNF_FLUSHNOWAIT, lnk.c_str(), NULL);
    }
    app.isApplied = false;
    return true;
}

static void DrawPreviewPanel(HDC hdc) {
    int w = g_previewRect.right - g_previewRect.left;
    int h = g_previewRect.bottom - g_previewRect.top;
    if (w <= 0 || h <= 0) return;

    Bitmap buffer(w, h, PixelFormat32bppARGB);
    Graphics g(&buffer);
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    // Card background
    SolidBrush cardBg(Color(255, 248, 250, 253));
    g.FillRectangle(&cardBg, 0, 0, w, h);
    Pen borderPen(Color(255, 218, 224, 233), 1.0f);
    g.DrawRectangle(&borderPen, 0, 0, w - 1, h - 1);

    // Subtle checkerboard pattern behind both icons
    auto drawChecker = [&](int bx, int by, int bw, int bh) {
        SolidBrush c1(Color(255, 255, 255, 255));
        SolidBrush c2(Color(255, 238, 241, 246));
        g.FillRectangle(&c1, bx, by, bw, bh);
        int step = 12;
        for (int y = 0; y < bh; y += step) {
            for (int x = 0; x < bw; x += step) {
                if (((x / step) + (y / step)) & 1) {
                    g.FillRectangle(&c2, bx + x, by + y, std::min(step, bw - x), std::min(step, bh - y));
                }
            }
        }
        Pen p(Color(255, 222, 227, 235), 1.0f);
        g.DrawRectangle(&p, bx, by, bw - 1, bh - 1);
    };

    int iconBox = 144;
    int leftX = 64;
    int rightX = 362;
    int topY = 24;

    drawChecker(leftX, topY, iconBox, iconBox);
    drawChecker(rightX, topY, iconBox, iconBox);

    if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
        AppEntry& app = g_allApps[g_selectedAppIndex];
        if (app.origBitmap) {
            g.DrawImage(app.origBitmap, Rect(leftX + 12, topY + 12, iconBox - 24, iconBox - 24));
            Bitmap* roundBmp = RenderGoogleRoundIcon(app.origBitmap, g_style, 256);
            g.DrawImage(roundBmp, Rect(rightX + 4, topY + 4, iconBox - 8, iconBox - 8));
            delete roundBmp;
        }
    }

    // Center arrow + scale indicator
    SolidBrush accentBrush(Color(255, 26, 115, 232));
    SolidBrush textDark(Color(255, 32, 33, 36));
    SolidBrush textMuted(Color(255, 95, 99, 104));

    FontFamily ff(L"Segoe UI");
    Font fontBold(&ff, 13.0f, FontStyleBold, UnitPixel);
    Font fontSub(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fontArrow(&ff, 22.0f, FontStyleBold, UnitPixel);

    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);

    g.DrawString(L"➔", -1, &fontArrow, PointF((REAL)(w / 2), (REAL)(topY + 52)), &sfCenter, &accentBrush);

    wchar_t scaleBuf[64];
    wsprintfW(scaleBuf, L"%d%%", (int)std::round(g_style.scale * 100.0));
    g.DrawString(scaleBuf, -1, &fontBold, PointF((REAL)(w / 2), (REAL)(topY + 84)), &sfCenter, &textMuted);

    g.DrawString(L"Оригинал Windows", -1, &fontBold, PointF((REAL)(leftX + iconBox / 2), (REAL)(topY + iconBox + 10)), &sfCenter, &textDark);
    g.DrawString(L"Исходный ярлык", -1, &fontSub, PointF((REAL)(leftX + iconBox / 2), (REAL)(topY + iconBox + 28)), &sfCenter, &textMuted);

    g.DrawString(L"Круглая Google Pixel", -1, &fontBold, PointF((REAL)(rightX + iconBox / 2), (REAL)(topY + iconBox + 10)), &sfCenter, &textDark);
    if (g_isEyedropperActive) {
        SolidBrush pickBrush(Color(255, 217, 48, 37));
        g.DrawString(L"🔍 ПИПЕТКА АКТИВНА: кликните в любую точку иконки или экрана", -1, &fontBold,
                     PointF((REAL)(w / 2), (REAL)(topY + iconBox + 28)), &sfCenter, &pickBrush);
    } else {
        g.DrawString(L"Тяните мышью внутри круга · Колесо = Зум", -1, &fontSub,
                     PointF((REAL)(rightX + iconBox / 2), (REAL)(topY + iconBox + 28)), &sfCenter, &accentBrush);
    }

    // Current custom color indicator swatch + #HEX in top-right corner of the macOS studio card
    Color curClr(255, GetRValue(g_style.customColor), GetGValue(g_style.customColor), GetBValue(g_style.customColor));
    SolidBrush swatchBr(curClr);
    Pen swatchPen(Color(120, 0, 0, 0), 1.0f);
    g.FillEllipse(&swatchBr, w - 96, 12, 18, 18);
    g.DrawEllipse(&swatchPen, w - 96, 12, 18, 18);
    wchar_t hexBuf[32];
    wsprintfW(hexBuf, L"#%02X%02X%02X", GetRValue(g_style.customColor), GetGValue(g_style.customColor), GetBValue(g_style.customColor));
    g.DrawString(hexBuf, -1, &fontSub, PointF((REAL)(w - 72), 12.0f), NULL, &textMuted);

    Graphics screenG(hdc);
    screenG.DrawImage(&buffer, (INT)g_previewRect.left, (INT)g_previewRect.top);
}

static void InvalidatePreview() {
    if (g_hMainWnd) {
        InvalidateRect(g_hMainWnd, &g_previewRect, FALSE);
    }
}

static void UpdateInspectorLabels() {
    wchar_t buf[128];
    int sc = (int)std::round(g_style.scale * 100.0);
    wsprintfW(buf, L"Масштаб внутри круга (%d%% — от 30%% до 250%%):", sc);
    SetWindowTextW(g_hLblScale, buf);
    SendMessageW(g_hSliderScale, TBM_SETPOS, TRUE, sc);

    int ox = (int)std::round(g_style.offsetX * 100.0);
    wsprintfW(buf, L"Смещение Влево / Вправо X (%d%%):", ox);
    SetWindowTextW(g_hLblOffX, buf);
    SendMessageW(g_hSliderOffX, TBM_SETPOS, TRUE, ox + 100);

    int oy = (int)std::round(g_style.offsetY * 100.0);
    wsprintfW(buf, L"Смещение Вверх / Вниз Y (%d%%):", oy);
    SetWindowTextW(g_hLblOffY, buf);
    SendMessageW(g_hSliderOffY, TBM_SETPOS, TRUE, oy + 100);

    int sh = (int)std::round(g_style.shadow * 100.0);
    wsprintfW(buf, L"Внешняя тень Google Pixel (%d%%):", sh);
    SetWindowTextW(g_hLblShadow, buf);
    SendMessageW(g_hSliderShadow, TBM_SETPOS, TRUE, sh);
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
            std::wstring customIcon;
            ResolveShortcut(path, entry.targetPath, customIcon);
            if (entry.targetPath.empty()) entry.targetPath = path;
        } else {
            entry.targetPath = path;
        }
        entry.category = L"Добавлено вручную";
        entry.origBitmap = LoadHighResIcon(entry.targetPath, entry.lnkPath);
        g_allApps.insert(g_allApps.begin(), entry);
        RefreshFilteredList();
        SetWindowTextW(g_hStatus, (L"Добавлено: " + entry.name).c_str());
    }
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
                psl->SetDescription(L"IconMaker — Круглые иконки в стиле Google Pixel");
                IPersistFile* ppf = nullptr;
                if (SUCCEEDED(psl->QueryInterface(IID_IPersistFile, (void**)&ppf)) && ppf) {
                    ppf->Save(lnkPath.c_str(), TRUE);
                    ppf->Release();
                }
                psl->Release();
            }
        }
    };

    makeShortcut(CSIDL_DESKTOPDIRECTORY, L"IconMaker — Круглые иконки.lnk");
    makeShortcut(CSIDL_PROGRAMS, L"IconMaker — Круглые иконки.lnk");

    if (g_hStatus) {
        SetWindowTextW(g_hStatus, L"Приложение установлено в %LOCALAPPDATA%\\Programs\\IconMaker, ярлыки созданы на Рабочем столе и в меню Пуск");
    }
    if (showConfirmation) {
        MessageBoxW(
            hwnd,
            L"IconMaker успешно установлен!\n\n"
            L"• Папка: %LOCALAPPDATA%\\Programs\\IconMaker\\IconMaker.exe\n"
            L"• Ярлыки созданы на Рабочем столе и в меню «Пуск».",
            L"Установка завершена — IconMaker",
            MB_ICONINFORMATION
        );
    }
}

static void ExportCurrentOrAllIcons(HWND hwnd) {
    BROWSEINFOW bi = {0};
    bi.hwndOwner = hwnd;
    bi.lpszTitle = L"Выберите папку для сохранения круглых иконок (.ICO и 1024x1024 .PNG):";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return;

    wchar_t outDir[MAX_PATH] = {0};
    if (SHGetPathFromIDListW(pidl, outDir) && outDir[0]) {
        int exported = 0;
        std::wstring dir = outDir;
        if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
            AppEntry& app = g_allApps[g_selectedAppIndex];
            std::wstring safe = SanitizeFileName(app.name);
            Bitmap* master1024 = RenderGoogleRoundIcon(app.origBitmap, g_style, 1024);
            std::wstring icoFile = dir + L"\\" + safe + L".ico";
            std::wstring pngFile = dir + L"\\" + safe + L".png";
            SaveMultiSizeIco(master1024, icoFile);
            master1024->Save(pngFile.c_str(), &g_pngClsid, NULL);
            delete master1024;
            exported = 1;
        }
        wchar_t msg[MAX_PATH + 128];
        wsprintfW(msg, L"Круглая иконка (.ICO + 1024px .PNG) сохранена в:\n%s", outDir);
        SetWindowTextW(g_hStatus, msg);
        MessageBoxW(hwnd, msg, L"Экспорт завершён", MB_ICONINFORMATION);
        (void)exported;
    }
    CoTaskMemFree(pidl);
}

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_fontTitle = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                      DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            g_fontSection = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            g_fontRegular = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            g_fontSmall = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                      DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            auto mkLabel = [&](const wchar_t* text, int x, int y, int w, int h, HFONT font) -> HWND {
                HWND hCtrl = CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, hwnd, NULL, NULL, NULL);
                SendMessageW(hCtrl, WM_SETFONT, (WPARAM)font, TRUE);
                return hCtrl;
            };
            auto mkBtn = [&](const wchar_t* text, int id, int x, int y, int w, int h, bool bold = false) -> HWND {
                HWND hCtrl = CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | (bold ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON),
                                             x, y, w, h, hwnd, (HMENU)(INT_PTR)id, NULL, NULL);
                SendMessageW(hCtrl, WM_SETFONT, (WPARAM)(bold ? g_fontSection : g_fontRegular), TRUE);
                return hCtrl;
            };

            // Header
            mkLabel(L"IconMaker — Круглые иконки Google Pixel", 16, 14, 380, 28, g_fontTitle);
            mkBtn(L"🛡 Права Админа", ID_BTN_ADMIN, 404, 14, 128, 30);
            mkBtn(L"📌 Создать ярлыки", ID_BTN_INSTALL_SELF, 536, 14, 148, 30);
            mkBtn(L"Обновить список", ID_BTN_SCAN, 690, 14, 130, 30);
            mkBtn(L"+ Файл / .EXE…", ID_BTN_ADD_FILE, 826, 14, 130, 30);

            // Left column: Search + App List
            mkLabel(L"Поиск приложений Windows:", 16, 54, 220, 20, g_fontRegular);
            g_hSearch = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                        16, 76, 356, 26, hwnd, (HMENU)ID_EDIT_SEARCH, NULL, NULL);
            SendMessageW(g_hSearch, WM_SETFONT, (WPARAM)g_fontRegular, TRUE);

            g_hList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", NULL,
                                      WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
                                      16, 110, 356, 524, hwnd, (HMENU)ID_LIST_APPS, NULL, NULL);
            SendMessageW(g_hList, WM_SETFONT, (WPARAM)g_fontRegular, TRUE);

            // Right column: Studio Inspector
            int rx = 390;
            int rw = 566;

            // Surface Preset Row + Eyedropper ("Пипетка") + Color Palette
            mkLabel(L"Стиль подложки:", rx, 296, 130, 22, g_fontSection);
            g_hCmbPreset = CreateWindowExW(0, L"COMBOBOX", NULL,
                                           WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                           rx + 134, 293, 218, 200, hwnd, (HMENU)ID_CMB_PRESET, NULL, NULL);
            SendMessageW(g_hCmbPreset, WM_SETFONT, (WPARAM)g_fontRegular, TRUE);
            SendMessageW(g_hCmbPreset, CB_ADDSTRING, 0, (LPARAM)L"Google White (Белый круг)");
            SendMessageW(g_hCmbPreset, CB_ADDSTRING, 0, (LPARAM)L"Material You Pastel (Пастель)");
            SendMessageW(g_hCmbPreset, CB_ADDSTRING, 0, (LPARAM)L"Dark Material (Тёмный)");
            SendMessageW(g_hCmbPreset, CB_ADDSTRING, 0, (LPARAM)L"Adaptive Edge (Край иконки)");
            SendMessageW(g_hCmbPreset, CB_ADDSTRING, 0, (LPARAM)L"Custom Color (Свой цвет)");
            SendMessageW(g_hCmbPreset, CB_SETCURSEL, 0, 0);

            mkBtn(L"🔍 Пипетка", ID_BTN_EYEDROPPER, rx + 358, 292, 104, 26);
            g_hBtnColor = mkBtn(L"Палитра…", ID_BTN_CUSTOM_CLR, rx + 468, 292, 98, 26);

            // Scale Slider (30% .. 250%)
            g_hLblScale = mkLabel(L"Масштаб внутри круга (68% — от 30% до 250%):", rx, 330, 340, 20, g_fontRegular);
            mkBtn(L"68%", ID_BTN_SCALE_68, rx + 356, 326, 48, 24);
            mkBtn(L"100%", ID_BTN_SCALE_100, rx + 408, 326, 50, 24);
            mkBtn(L"135%", ID_BTN_SCALE_135, rx + 462, 326, 50, 24);
            mkBtn(L"175%", ID_BTN_SCALE_175, rx + 516, 326, 50, 24);

            g_hSliderScale = CreateWindowExW(0, TRACKBAR_CLASSW, NULL,
                                             WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
                                             rx, 352, rw, 26, hwnd, (HMENU)ID_SLIDER_SCALE, NULL, NULL);
            SendMessageW(g_hSliderScale, TBM_SETRANGE, TRUE, MAKELONG(30, 250));
            SendMessageW(g_hSliderScale, TBM_SETPOS, TRUE, 68);

            // Offset X (-100% .. +100%)
            g_hLblOffX = mkLabel(L"Смещение Влево / Вправо X (0%):", rx, 384, 280, 20, g_fontRegular);
            mkBtn(L"← Влево", ID_BTN_LEFT, rx + 390, 380, 84, 24);
            mkBtn(L"Вправо →", ID_BTN_RIGHT, rx + 480, 380, 86, 24);

            g_hSliderOffX = CreateWindowExW(0, TRACKBAR_CLASSW, NULL,
                                            WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
                                            rx, 406, rw, 26, hwnd, (HMENU)ID_SLIDER_OFFX, NULL, NULL);
            SendMessageW(g_hSliderOffX, TBM_SETRANGE, TRUE, MAKELONG(0, 200));
            SendMessageW(g_hSliderOffX, TBM_SETPOS, TRUE, 100);

            // Offset Y (-100% .. +100%)
            g_hLblOffY = mkLabel(L"Смещение Вверх / Вниз Y (0%):", rx, 438, 260, 20, g_fontRegular);
            mkBtn(L"↓ Вниз", ID_BTN_DOWN, rx + 312, 434, 76, 24);
            mkBtn(L"↑ Вверх", ID_BTN_UP, rx + 392, 434, 76, 24);
            mkBtn(L"По центру", ID_BTN_CENTER, rx + 472, 434, 94, 24);

            g_hSliderOffY = CreateWindowExW(0, TRACKBAR_CLASSW, NULL,
                                            WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
                                            rx, 460, rw, 26, hwnd, (HMENU)ID_SLIDER_OFFY, NULL, NULL);
            SendMessageW(g_hSliderOffY, TBM_SETRANGE, TRUE, MAKELONG(0, 200));
            SendMessageW(g_hSliderOffY, TBM_SETPOS, TRUE, 100);

            // Elevation Shadow (0% .. 100%)
            g_hLblShadow = mkLabel(L"Внешняя тень Google Pixel (45%):", rx, 492, 320, 20, g_fontRegular);
            g_hSliderShadow = CreateWindowExW(0, TRACKBAR_CLASSW, NULL,
                                              WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
                                              rx, 512, rw, 26, hwnd, (HMENU)ID_SLIDER_SHADOW, NULL, NULL);
            SendMessageW(g_hSliderShadow, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
            SendMessageW(g_hSliderShadow, TBM_SETPOS, TRUE, 45);

            // Checkboxes
            auto mkChk = [&](const wchar_t* text, int id, int x, int y, int w, bool checked) -> HWND {
                HWND hCtrl = CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                             x, y, w, 22, hwnd, (HMENU)(INT_PTR)id, NULL, NULL);
                SendMessageW(hCtrl, WM_SETFONT, (WPARAM)g_fontRegular, TRUE);
                SendMessageW(hCtrl, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
                return hCtrl;
            };

            g_hChkAutoTrim  = mkChk(L"Авто-обрезка прозрачных полей", ID_CHK_AUTOTRIM, rx, 544, 275, true);
            g_hChkInnerShad = mkChk(L"Внутренняя тень иконки", ID_CHK_INNERSHAD, rx + 285, 544, 275, true);
            g_hChkFullBleed = mkChk(L"Квадратные иконки на весь круг", ID_CHK_FULLBLEED, rx, 568, 275, false);
            g_hChkMono      = mkChk(L"Монохромный тон Material You", ID_CHK_MONOCHROME, rx + 285, 568, 275, false);

            // Action Buttons
            mkBtn(L"Применить к выбранному", ID_BTN_APPLY_ONE, rx, 600, 205, 34, true);
            mkBtn(L"Применить ко ВСЕМ", ID_BTN_APPLY_ALL, rx + 212, 600, 175, 34);
            mkBtn(L"Экспорт .ICO/.PNG", ID_BTN_EXPORT, rx + 394, 600, 172, 34);

            mkBtn(L"Вернуть оригинал выбранному", ID_BTN_RESTORE_ONE, rx, 640, 278, 30);
            mkBtn(L"Вернуть оригиналы ВСЕМ приложениям", ID_BTN_RESTORE_ALL, rx + 288, 640, 278, 30);

            // Bottom status & progress bar
            g_hStatus = mkLabel(L"Готово — выберите приложение или перетащите иконку внутри круга мышью", 16, 680, 650, 22, g_fontSmall);
            g_hProgress = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
                                          WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
                                          680, 680, 276, 18, hwnd, NULL, NULL, NULL);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            DrawPreviewPanel(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetBkMode(hdcStatic, TRANSPARENT);
            SetTextColor(hdcStatic, RGB(29, 29, 31));
            if (!g_hBrWindowBg) g_hBrWindowBg = CreateSolidBrush(RGB(245, 245, 247));
            return (INT_PTR)g_hBrWindowBg;
        }

        case WM_SETCURSOR: {
            if (g_isEyedropperActive) {
                SetCursor(LoadCursor(NULL, IDC_CROSS));
                return TRUE;
            }
            break;
        }

        case WM_LBUTTONDOWN: {
            if (g_isEyedropperActive) {
                POINT screenPt = { 0, 0 };
                GetCursorPos(&screenPt);
                HDC screenDC = GetDC(NULL);
                if (screenDC) {
                    COLORREF sampled = GetPixel(screenDC, screenPt.x, screenPt.y);
                    ReleaseDC(NULL, screenDC);
                    if (sampled != CLR_INVALID) {
                        g_style.customColor = sampled;
                        g_style.preset = PRESET_CUSTOM_COLOR;
                        SendMessageW(g_hCmbPreset, CB_SETCURSEL, PRESET_CUSTOM_COLOR, 0);
                    }
                }
                g_isEyedropperActive = false;
                ReleaseCapture();
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                wchar_t msg[128];
                wsprintfW(msg, L"Пипетка: выбран цвет #%02X%02X%02X",
                          GetRValue(g_style.customColor), GetGValue(g_style.customColor), GetBValue(g_style.customColor));
                SetWindowTextW(g_hStatus, msg);
                InvalidatePreview();
                return 0;
            }
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            if (PtInRect(&g_roundHitRect, pt)) {
                g_isDraggingPreview = true;
                g_dragStartPt = pt;
                g_dragStartOffX = g_style.offsetX;
                g_dragStartOffY = g_style.offsetY;
                SetCapture(hwnd);
                return 0;
            }
            break;
        }

        case WM_RBUTTONDOWN: {
            if (g_isEyedropperActive) {
                g_isEyedropperActive = false;
                ReleaseCapture();
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                SetWindowTextW(g_hStatus, L"Пипетка отменена");
                InvalidatePreview();
                return 0;
            }
            break;
        }

        case WM_MOUSEMOVE: {
            if (g_isEyedropperActive) {
                SetCursor(LoadCursor(NULL, IDC_CROSS));
                POINT screenPt = { 0, 0 };
                GetCursorPos(&screenPt);
                HDC screenDC = GetDC(NULL);
                if (screenDC) {
                    COLORREF sampled = GetPixel(screenDC, screenPt.x, screenPt.y);
                    ReleaseDC(NULL, screenDC);
                    if (sampled != CLR_INVALID) {
                        g_style.customColor = sampled;
                        g_style.preset = PRESET_CUSTOM_COLOR;
                        SendMessageW(g_hCmbPreset, CB_SETCURSEL, PRESET_CUSTOM_COLOR, 0);
                        InvalidatePreview();
                    }
                }
                return 0;
            }
            if (g_isDraggingPreview) {
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                double dx = (double)(pt.x - g_dragStartPt.x) / 64.0;
                double dy = -(double)(pt.y - g_dragStartPt.y) / 64.0;
                g_style.offsetX = std::clamp(g_dragStartOffX + dx, -1.0, 1.0);
                g_style.offsetY = std::clamp(g_dragStartOffY + dy, -1.0, 1.0);
                UpdateInspectorLabels();
                InvalidatePreview();
                return 0;
            }
            break;
        }

        case WM_LBUTTONUP: {
            if (g_isDraggingPreview) {
                g_isDraggingPreview = false;
                ReleaseCapture();
                return 0;
            }
            break;
        }

        case WM_MOUSEWHEEL: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            if (PtInRect(&g_previewRect, pt)) {
                int delta = GET_WHEEL_DELTA_WPARAM(wParam);
                double step = (delta > 0) ? 0.06 : -0.06;
                g_style.scale = std::clamp(g_style.scale + step, 0.30, 2.50);
                UpdateInspectorLabels();
                InvalidatePreview();
                return 0;
            }
            break;
        }

        case WM_HSCROLL: {
            HWND hScroll = (HWND)lParam;
            if (hScroll == g_hSliderScale) {
                int pos = (int)SendMessageW(g_hSliderScale, TBM_GETPOS, 0, 0);
                g_style.scale = std::clamp(pos / 100.0, 0.30, 2.50);
            } else if (hScroll == g_hSliderOffX) {
                int pos = (int)SendMessageW(g_hSliderOffX, TBM_GETPOS, 0, 0);
                g_style.offsetX = std::clamp((pos - 100) / 100.0, -1.0, 1.0);
            } else if (hScroll == g_hSliderOffY) {
                int pos = (int)SendMessageW(g_hSliderOffY, TBM_GETPOS, 0, 0);
                g_style.offsetY = std::clamp((pos - 100) / 100.0, -1.0, 1.0);
            } else if (hScroll == g_hSliderShadow) {
                int pos = (int)SendMessageW(g_hSliderShadow, TBM_GETPOS, 0, 0);
                g_style.shadow = std::clamp(pos / 100.0, 0.0, 1.0);
            }
            UpdateInspectorLabels();
            InvalidatePreview();
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);

            if (id == ID_EDIT_SEARCH && code == EN_CHANGE) {
                RefreshFilteredList();
                return 0;
            }
            if (id == ID_LIST_APPS && code == LBN_SELCHANGE) {
                int sel = (int)SendMessageW(g_hList, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && sel < (int)g_filteredIndices.size()) {
                    g_selectedAppIndex = g_filteredIndices[sel];
                    InvalidatePreview();
                }
                return 0;
            }
            if (id == ID_CMB_PRESET && code == CBN_SELCHANGE) {
                int sel = (int)SendMessageW(g_hCmbPreset, CB_GETCURSEL, 0, 0);
                if (sel >= 0 && sel <= 4) {
                    g_style.preset = (BgPreset)sel;
                    InvalidatePreview();
                }
                return 0;
            }

            switch (id) {
                case ID_BTN_ADMIN: {
                    wchar_t selfPath[MAX_PATH * 2] = {0};
                    if (GetModuleFileNameW(NULL, selfPath, MAX_PATH * 2 - 1)) {
                        if ((INT_PTR)ShellExecuteW(hwnd, L"runas", selfPath, NULL, NULL, SW_SHOWNORMAL) > 32) {
                            PostQuitMessage(0);
                        }
                    }
                    break;
                }

                case ID_BTN_INSTALL_SELF:
                    InstallSelfAndShortcuts(hwnd, true);
                    break;

                case ID_BTN_SCAN:
                    SetWindowTextW(g_hStatus, L"Сканирование установленных приложений Windows…");
                    ScanInstalledWindowsApps();
                    SetWindowTextW(g_hStatus, L"Список установленных приложений обновлён");
                    break;

                case ID_BTN_ADD_FILE:
                    AddCustomAppOrImage(hwnd);
                    break;

                case ID_BTN_EYEDROPPER: {
                    g_isEyedropperActive = true;
                    SetCapture(hwnd);
                    SetCursor(LoadCursor(NULL, IDC_CROSS));
                    SetWindowTextW(g_hStatus, L"🔍 Пипетка активна: наведите курсор на любой цвет иконки или экрана и кликните ЛКМ (ПКМ — отмена)");
                    InvalidatePreview();
                    break;
                }

                case ID_BTN_CUSTOM_CLR: {
                    static COLORREF customCust[16] = {0};
                    CHOOSECOLORW cc = {0};
                    cc.lStructSize = sizeof(cc);
                    cc.hwndOwner = hwnd;
                    cc.lpCustColors = customCust;
                    cc.rgbResult = g_style.customColor;
                    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                    if (ChooseColorW(&cc)) {
                        g_style.customColor = cc.rgbResult;
                        g_style.preset = PRESET_CUSTOM_COLOR;
                        SendMessageW(g_hCmbPreset, CB_SETCURSEL, PRESET_CUSTOM_COLOR, 0);
                        InvalidatePreview();
                    }
                    break;
                }

                case ID_BTN_SCALE_68:  g_style.scale = 0.68; UpdateInspectorLabels(); InvalidatePreview(); break;
                case ID_BTN_SCALE_100: g_style.scale = 1.00; UpdateInspectorLabels(); InvalidatePreview(); break;
                case ID_BTN_SCALE_135: g_style.scale = 1.35; UpdateInspectorLabels(); InvalidatePreview(); break;
                case ID_BTN_SCALE_175: g_style.scale = 1.75; UpdateInspectorLabels(); InvalidatePreview(); break;

                case ID_BTN_LEFT:
                    g_style.offsetX = std::clamp(g_style.offsetX - 0.05, -1.0, 1.0);
                    UpdateInspectorLabels(); InvalidatePreview(); break;
                case ID_BTN_RIGHT:
                    g_style.offsetX = std::clamp(g_style.offsetX + 0.05, -1.0, 1.0);
                    UpdateInspectorLabels(); InvalidatePreview(); break;
                case ID_BTN_DOWN:
                    g_style.offsetY = std::clamp(g_style.offsetY - 0.05, -1.0, 1.0);
                    UpdateInspectorLabels(); InvalidatePreview(); break;
                case ID_BTN_UP:
                    g_style.offsetY = std::clamp(g_style.offsetY + 0.05, -1.0, 1.0);
                    UpdateInspectorLabels(); InvalidatePreview(); break;
                case ID_BTN_CENTER:
                    g_style.offsetX = 0.0; g_style.offsetY = 0.0;
                    UpdateInspectorLabels(); InvalidatePreview(); break;

                case ID_CHK_AUTOTRIM:
                    g_style.autoTrim = (SendMessageW(g_hChkAutoTrim, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    InvalidatePreview(); break;
                case ID_CHK_INNERSHAD:
                    g_style.innerShadow = (SendMessageW(g_hChkInnerShad, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    InvalidatePreview(); break;
                case ID_CHK_FULLBLEED:
                    g_style.fullBleed = (SendMessageW(g_hChkFullBleed, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    InvalidatePreview(); break;
                case ID_CHK_MONOCHROME:
                    g_style.monochrome = (SendMessageW(g_hChkMono, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    InvalidatePreview(); break;

                case ID_BTN_APPLY_ONE: {
                    if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
                        AppEntry& app = g_allApps[g_selectedAppIndex];
                        bool needsAdmin = false;
                        if (ApplyRoundIconToEntry(app, &needsAdmin)) {
                            SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT, NULL, NULL);
                            int curSel = (int)SendMessageW(g_hList, LB_GETCURSEL, 0, 0);
                            RefreshFilteredList();
                            if (curSel >= 0 && curSel < (int)g_filteredIndices.size()) {
                                SendMessageW(g_hList, LB_SETCURSEL, curSel, 0);
                                g_selectedAppIndex = g_filteredIndices[curSel];
                            }
                            std::wstring status = L"Применена круглая иконка к " + app.name;
                            if (needsAdmin) {
                                status += L" (для системных ярлыков в Общем Рабочем столе нажмите «🛡 Права Админа»)";
                            }
                            SetWindowTextW(g_hStatus, status.c_str());
                        }
                    }
                    break;
                }

                case ID_BTN_APPLY_ALL: {
                    if (g_filteredIndices.empty()) break;
                    SendMessageW(g_hProgress, PBM_SETRANGE32, 0, (LPARAM)g_filteredIndices.size());
                    int count = 0;
                    bool anyNeedsAdmin = false;
                    for (size_t i = 0; i < g_filteredIndices.size(); ++i) {
                        AppEntry& app = g_allApps[g_filteredIndices[i]];
                        bool na = false;
                        if (ApplyRoundIconToEntry(app, &na)) count++;
                        if (na) anyNeedsAdmin = true;
                        SendMessageW(g_hProgress, PBM_SETPOS, i + 1, 0);
                    }
                    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT, NULL, NULL);
                    RefreshFilteredList();
                    wchar_t msg[384];
                    if (anyNeedsAdmin) {
                        wsprintfW(msg,
                                  L"Круглые иконки применены к %d приложениям!\n\n"
                                  L"Примечание: часть ярлыков находится в системной папке C:\\Users\\Public\\Desktop или C:\\ProgramData.\n"
                                  L"Чтобы заменить их без создания копий, нажмите «🛡 Права Админа» вверху окна.",
                                  count);
                    } else {
                        wsprintfW(msg, L"Готово! Круглые иконки Google Pixel применены к %d приложениям (Рабочий стол, Пуск и Панель задач).", count);
                    }
                    SetWindowTextW(g_hStatus, msg);
                    MessageBoxW(hwnd, msg, L"IconMaker", MB_ICONINFORMATION);
                    break;
                }

                case ID_BTN_RESTORE_ONE: {
                    if (g_selectedAppIndex >= 0 && g_selectedAppIndex < (int)g_allApps.size()) {
                        AppEntry& app = g_allApps[g_selectedAppIndex];
                        RestoreOriginalEntry(app);
                        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
                        RefreshFilteredList();
                        SetWindowTextW(g_hStatus, (L"Восстановлена оригинальная иконка для " + app.name).c_str());
                    }
                    break;
                }

                case ID_BTN_RESTORE_ALL: {
                    for (auto& app : g_allApps) {
                        RestoreOriginalEntry(app);
                    }
                    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
                    RefreshFilteredList();
                    SetWindowTextW(g_hStatus, L"Оригинальные иконки Windows восстановлены для всех приложений");
                    MessageBoxW(hwnd, L"Оригинальные иконки восстановлены для всех приложений!", L"IconMaker", MB_ICONINFORMATION);
                    break;
                }

                case ID_BTN_EXPORT:
                    ExportCurrentOrAllIcons(hwnd);
                    break;
            }
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmdLine, int nCmdShow) {
    (void)hPrev; (void)lpCmdLine;

    SetProcessDPIAware();
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    GdiplusStartupInput gsi;
    GdiplusStartup(&g_gdiplusToken, &gsi, NULL);
    GetPngEncoderClsid(&g_pngClsid);

    g_hBrWindowBg = CreateSolidBrush(RGB(245, 245, 247)); // macOS Sonoma/Tahoe light window surface (#F5F5F7)
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(101));
    wc.hIconSm = LoadIconW(hInst, MAKEINTRESOURCEW(101));
    if (!wc.hIcon) wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = g_hBrWindowBg;
    wc.lpszClassName = L"IconMakerNativeWin32Class";
    RegisterClassExW(&wc);

    int winW = 984, winH = 748;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    g_hMainWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"IconMaker — Круглые иконки Google Pixel & Material You (Windows 10/11)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        std::max(0, (screenW - winW) / 2),
        std::max(0, (screenH - winH) / 2),
        winW,
        winH,
        NULL,
        NULL,
        hInst,
        NULL
    );

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    ScanInstalledWindowsApps();

    // If run from a file named *Setup*.exe, offer 1-click shortcut installation in-process (no external process spawning)
    wchar_t exeName[MAX_PATH] = {0};
    if (GetModuleFileNameW(NULL, exeName, MAX_PATH - 1)) {
        std::wstring lowerExe = PathFindFileNameW(exeName);
        for (auto& c : lowerExe) c = towlower(c);
        if (lowerExe.find(L"setup") != std::wstring::npos) {
            InstallSelfAndShortcuts(g_hMainWnd, false);
        }
    }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    for (auto& app : g_allApps) {
        delete app.origBitmap;
    }
    GdiplusShutdown(g_gdiplusToken);
    CoUninitialize();
    return (int)msg.wParam;
}
