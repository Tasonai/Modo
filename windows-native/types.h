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

enum AppThemeMode {
    THEME_DARK = 0,
    THEME_LIGHT = 1
};

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

enum SurfaceMaterial {
    MATERIAL_SMOOTH = 0,
    MATERIAL_FROSTED_GLASS = 1,
    MATERIAL_BRUSHED_TITANIUM = 2,
    MATERIAL_NEUMORPHIC = 3,
    MATERIAL_PRISM_GLOW = 4,
    MATERIAL_LIQUID_CHROME = 5,
    MATERIAL_MATTE_CERAMIC = 6,
    MATERIAL_HOLOGRAPHIC = 7
};

enum GlyphStyleMode {
    GLYPH_ORIGINAL = 0,
    GLYPH_SILHOUETTE = 1,
    GLYPH_EMBOSSED = 2,
    GLYPH_OUTLINE = 3,
    GLYPH_DUOTONE = 4,
    GLYPH_GLOW = 5
};

enum GlyphGradientPreset {
    GLYPH_GRAD_NONE = 0,
    GLYPH_GRAD_GOLD = 1,
    GLYPH_GRAD_CYBER = 2,
    GLYPH_GRAD_SUNSET = 3,
    GLYPH_GRAD_EMERALD = 4,
    GLYPH_GRAD_TITANIUM = 5,
    GLYPH_GRAD_PURPLE_HAZE = 6
};

enum GlyphBadgeOverlay {
    BADGE_NONE = 0,
    BADGE_DOT = 1,
    BADGE_CHECKMARK = 2,
    BADGE_STAR = 3,
    BADGE_LOCK = 4,
    BADGE_ALERT = 5,
    BADGE_BETA = 6,
    BADGE_PRO = 7
};

enum GlyphBadgePosition {
    BADGE_POS_TOP_RIGHT = 0,
    BADGE_POS_BOTTOM_RIGHT = 1,
    BADGE_POS_TOP_LEFT = 2,
    BADGE_POS_BOTTOM_LEFT = 3
};

struct BrandPaletteWin {
    COLORREF primary;
    COLORREF secondary;
    COLORREF backgroundTint;
    COLORREF deepTone;
    COLORREF highlight;
};

enum ShowcaseTheme {
    SHOWCASE_THEME_AMBIENT_MESH = 0,
    SHOWCASE_THEME_STUDIO_DARK  = 1,
    SHOWCASE_THEME_CLEAN_WHITE  = 2,
    SHOWCASE_THEME_SUNSET_GLOW  = 3,
    SHOWCASE_THEME_CYBER_NEON   = 4
};

enum ShowcaseLayout {
    SHOWCASE_LAYOUT_HERO        = 0,
    SHOWCASE_LAYOUT_TRIPLE      = 1,
    SHOWCASE_LAYOUT_WINDOW_CARD = 2
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
    SurfaceMaterial material = MATERIAL_SMOOTH;
    GlyphStyleMode glyphStyle = GLYPH_ORIGINAL;
    double rotation = 0.0;     // degrees: 0, 90, 180, 270
    bool flipH = false;        // horizontal flip
    bool flipV = false;        // vertical flip
    COLORREF glyphColor = 0;   // 0 = none / original, otherwise RGB(...)
    GlyphGradientPreset glyphGrad = GLYPH_GRAD_NONE;
    GlyphBadgeOverlay badge = BADGE_NONE;
    GlyphBadgePosition badgePos = BADGE_POS_TOP_RIGHT;
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
    bool isFolder = false;
    bool isSystemPinnedTop = false;
    bool isApplied = false;
    double customRotation = 0.0;
    bool isFlippedH = false;
    bool isFlippedV = false;
    COLORREF customGlyphColor = 0;
    GlyphGradientPreset customGlyphGrad = GLYPH_GRAD_NONE;
    GlyphBadgeOverlay customBadge = BADGE_NONE;
    GlyphBadgePosition customBadgePos = BADGE_POS_TOP_RIGHT;
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
    SurfaceMaterial material;
    GlyphStyleMode glyphStyle;
};

static const IconPackDef kIconPacks[19] = {
    { L"Pixel Pure",   PRESET_GOOGLE_WHITE,    SHAPE_CIRCLE,   0.68, 0.40, true,  false, RGB(232, 240, 254), MATERIAL_SMOOTH, GLYPH_ORIGINAL },
    { L"Material You", PRESET_MATERIAL_PASTEL, SHAPE_CIRCLE,   0.70, 0.35, true,  false, RGB(232, 240, 254), MATERIAL_SMOOTH, GLYPH_ORIGINAL },
    { L"Modo Noir",    PRESET_DARK_MATERIAL,   SHAPE_CIRCLE,   0.66, 0.50, true,  false, RGB(232, 240, 254), MATERIAL_SMOOTH, GLYPH_ORIGINAL },
    { L"Atoma Mono",   PRESET_DARK_MATERIAL,   SHAPE_SQUIRCLE, 0.64, 0.30, false, true,  RGB(232, 240, 254), MATERIAL_SMOOTH, GLYPH_SILHOUETTE },
    { L"WhiteSur",     PRESET_GOOGLE_WHITE,    SHAPE_SQUIRCLE, 0.74, 0.38, true,  false, RGB(0, 122, 255),   MATERIAL_SMOOTH, GLYPH_ORIGINAL },
    { L"Tela",         PRESET_MATERIAL_PASTEL, SHAPE_CIRCLE,   0.70, 0.30, false, false, RGB(45, 114, 217),  MATERIAL_SMOOTH, GLYPH_ORIGINAL },
    { L"Lawnicons",    PRESET_DARK_MATERIAL,   SHAPE_CIRCLE,   0.65, 0.35, false, true,  RGB(56, 189, 248),  MATERIAL_SMOOTH, GLYPH_SILHOUETTE },
    { L"macOSicons",   PRESET_CUSTOM_COLOR,    SHAPE_SQUIRCLE, 0.72, 0.42, true,  false, RGB(243, 244, 246), MATERIAL_SMOOTH, GLYPH_ORIGINAL },
    { L"Tabler",       PRESET_DARK_MATERIAL,   SHAPE_SQUIRCLE, 0.64, 0.40, false, true,  RGB(24, 24, 27),    MATERIAL_SMOOTH, GLYPH_SILHOUETTE },
    { L"Simple Icons", PRESET_DARK_MATERIAL,   SHAPE_SQUIRCLE, 0.65, 0.25, false, true,  RGB(32, 33, 36),    MATERIAL_SMOOTH, GLYPH_SILHOUETTE },
    { L"Squircle",     PRESET_GOOGLE_WHITE,    SHAPE_SQUIRCLE, 0.72, 0.35, true,  false, RGB(232, 240, 254), MATERIAL_SMOOTH, GLYPH_ORIGINAL },
    { L"Paper",        PRESET_CUSTOM_COLOR,    SHAPE_SQUIRCLE, 0.68, 0.25, false, false, RGB(246, 243, 235), MATERIAL_NEUMORPHIC, GLYPH_ORIGINAL },
    { L"Cyber Hex",    PRESET_DARK_MATERIAL,   SHAPE_HEXAGON,  0.65, 0.55, true,  false, RGB(232, 240, 254), MATERIAL_PRISM_GLOW, GLYPH_GLOW },
    { L"Catppuccin",   PRESET_CUSTOM_COLOR,    SHAPE_SQUIRCLE, 0.68, 0.35, false, false, RGB(30, 30, 46),    MATERIAL_SMOOTH, GLYPH_ORIGINAL },
    { L"Nordic",       PRESET_CUSTOM_COLOR,    SHAPE_SQUIRCLE, 0.68, 0.30, false, false, RGB(46, 52, 64),    MATERIAL_FROSTED_GLASS, GLYPH_ORIGINAL },
    { L"Tokyo Night",  PRESET_CUSTOM_COLOR,    SHAPE_HEXAGON,  0.66, 0.50, true,  false, RGB(26, 27, 38),    MATERIAL_PRISM_GLOW, GLYPH_GLOW },
    { L"Dracula",      PRESET_CUSTOM_COLOR,    SHAPE_SQUIRCLE, 0.68, 0.40, false, false, RGB(40, 42, 54),    MATERIAL_SMOOTH, GLYPH_ORIGINAL },
    { L"Sunset",       PRESET_CUSTOM_COLOR,    SHAPE_PEBBLE,   0.70, 0.35, false, false, RGB(45, 27, 40),    MATERIAL_FROSTED_GLASS, GLYPH_ORIGINAL },
    { L"Fluent",       PRESET_GOOGLE_WHITE,    SHAPE_SQUIRCLE, 0.74, 0.45, true,  false, RGB(243, 244, 246), MATERIAL_BRUSHED_TITANIUM, GLYPH_ORIGINAL }
};
