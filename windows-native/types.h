#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <objbase.h>
#include <objidl.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <gdiplus.h>
#include <string>
#include <vector>

using namespace Gdiplus;

enum BgPreset {
    PRESET_GOOGLE_WHITE = 0,
    PRESET_MATERIAL_PASTEL = 1,
    PRESET_DARK_MATERIAL = 2,
    PRESET_ADAPTIVE_EDGE = 3,
    PRESET_CUSTOM_COLOR = 4
};

enum IconShape {
    SHAPE_CIRCLE = 0,
    SHAPE_SQUIRCLE = 1,
    SHAPE_ROUNDED_RECT = 2,
    SHAPE_PEBBLE = 3,
    SHAPE_HEXAGON = 4
};

struct RoundStyle {
    BgPreset preset = PRESET_GOOGLE_WHITE;
    IconShape shape = SHAPE_CIRCLE;
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
