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

// Curated icon packs (mirror of ModoKit IconPack).
struct IconPackDef {
    const wchar_t* name;
    BgPreset preset;
    IconShape shape;
    double scale;
    double shadow;
    bool innerShadow;
    bool monochrome;
    COLORREF customColor;
};

static const IconPackDef kIconPacks[13] = {
    { L"Pixel Pure",   PRESET_GOOGLE_WHITE,    SHAPE_CIRCLE,   0.68, 0.40, true,  false, RGB(232, 240, 254) },
    { L"Material You", PRESET_MATERIAL_PASTEL, SHAPE_CIRCLE,   0.70, 0.35, true,  false, RGB(232, 240, 254) },
    { L"Modo Noir",    PRESET_DARK_MATERIAL,   SHAPE_CIRCLE,   0.66, 0.50, true,  false, RGB(232, 240, 254) },
    { L"Atoma Mono",   PRESET_DARK_MATERIAL,   SHAPE_SQUIRCLE, 0.64, 0.30, false, true,  RGB(232, 240, 254) },
    { L"Squircle",     PRESET_GOOGLE_WHITE,    SHAPE_SQUIRCLE, 0.72, 0.35, true,  false, RGB(232, 240, 254) },
    { L"Paper",        PRESET_CUSTOM_COLOR,    SHAPE_SQUIRCLE, 0.68, 0.25, false, false, RGB(246, 243, 235) },
    { L"Cyber Hex",    PRESET_DARK_MATERIAL,   SHAPE_HEXAGON,  0.65, 0.55, true,  false, RGB(232, 240, 254) },
    { L"Catppuccin",   PRESET_CUSTOM_COLOR,    SHAPE_SQUIRCLE, 0.68, 0.35, false, false, RGB(30, 30, 46) },
    { L"Nordic",       PRESET_CUSTOM_COLOR,    SHAPE_SQUIRCLE, 0.68, 0.30, false, false, RGB(46, 52, 64) },
    { L"Tokyo Night",  PRESET_CUSTOM_COLOR,    SHAPE_HEXAGON,  0.66, 0.50, true,  false, RGB(26, 27, 38) },
    { L"Dracula",      PRESET_CUSTOM_COLOR,    SHAPE_SQUIRCLE, 0.68, 0.40, false, false, RGB(40, 42, 54) },
    { L"Sunset",       PRESET_CUSTOM_COLOR,    SHAPE_PEBBLE,   0.70, 0.35, false, false, RGB(45, 27, 40) },
    { L"Fluent",       PRESET_GOOGLE_WHITE,    SHAPE_SQUIRCLE, 0.74, 0.45, true,  false, RGB(243, 244, 246) }
};
