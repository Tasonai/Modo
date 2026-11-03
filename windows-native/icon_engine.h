#pragma once

#include "types.h"

namespace IconEngine {

// Helper drawing utilities
void AddRoundedRect(GraphicsPath& path, RectF r, REAL radius);
void FillRoundRect(Graphics& g, RectF r, REAL radius, Color fill);
void DrawRoundRect(Graphics& g, RectF r, REAL radius, Color stroke, REAL width = 1.0f);

// Icon loading and processing
Bitmap* BitmapFromHIcon32(HICON hIcon, int targetSize = 256);
Bitmap* ExtractDirectPeOrIco256(const std::wstring& filePath, int iconIndex = 0);
Bitmap* LoadHighResIcon(const std::wstring& primaryPath, const std::wstring& fallbackPath, const std::wstring& safeAppName = L"", const CLSID* pngClsid = nullptr);
Bitmap* TrimTransparentEdges(Bitmap* src);

// Google Pixel & Material You Round Icon Generator
Bitmap* RenderGoogleRoundIcon(Bitmap* sourceBitmap, const RoundStyle& style, int canvasSize = 256);

// Multi-size .ico encoder
bool SaveMultiSizeIco(Bitmap* src, const RoundStyle& style, const std::wstring& outPath, const CLSID* pngClsid = nullptr);

} // namespace IconEngine
