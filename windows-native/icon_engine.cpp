#include "icon_engine.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace IconEngine {

void AddRoundedRect(GraphicsPath& path, RectF r, REAL radius) {
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

void FillRoundRect(Graphics& g, RectF r, REAL radius, Color fill) {
    GraphicsPath path;
    AddRoundedRect(path, r, radius);
    SolidBrush br(fill);
    g.FillPath(&br, &path);
}

void DrawRoundRect(Graphics& g, RectF r, REAL radius, Color stroke, REAL width) {
    GraphicsPath path;
    AddRoundedRect(path, r, radius);
    Pen pen(stroke, width);
    g.DrawPath(&pen, &path);
}

Bitmap* BitmapFromHIcon32(HICON hIcon, int targetSize) {
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

Bitmap* ExtractDirectPeOrIco256(const std::wstring& filePath, int iconIndex) {
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

Bitmap* TrimTransparentEdges(Bitmap* src) {
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
    double opticalRadiusNorm;
    double minRadialCover;
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

Bitmap* RenderGoogleRoundIcon(Bitmap* sourceBitmap, const RoundStyle& style, int canvasSize) {
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

static bool EncodeBitmapToPngBytes(Bitmap* bmp, const CLSID* pngClsid, std::vector<uint8_t>& outBytes) {
    if (!bmp || !pngClsid) return false;
    IStream* pStream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(NULL, TRUE, &pStream)) || !pStream) return false;
    if (bmp->Save(pStream, pngClsid, NULL) != Ok) {
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
    bih.biHeight = sz * 2;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = (DWORD)(xorBytes + andBytes);
    memcpy(outBytes.data(), &bih, sizeof(bih));

    uint8_t* xorDst = outBytes.data() + sizeof(BITMAPINFOHEADER);
    uint8_t* andDst = xorDst + xorBytes;

    for (int y = 0; y < sz; ++y) {
        int srcY = (sz - 1) - y;
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

bool SaveMultiSizeIco(Bitmap* sourceOrigBitmap, const RoundStyle& style, const std::wstring& icoPath, const CLSID* pngClsid) {
    if (!sourceOrigBitmap) return false;
    const int sizes[] = { 256, 128, 96, 64, 48, 40, 32, 24, 16 };
    const int count = (int)(sizeof(sizes) / sizeof(sizes[0]));
    std::vector<std::vector<uint8_t>> frames(count);

    for (int i = 0; i < count; ++i) {
        Bitmap* rendered = RenderGoogleRoundIcon(sourceOrigBitmap, style, sizes[i]);
        bool ok = false;
        if (sizes[i] >= 256) {
            ok = EncodeBitmapToPngBytes(rendered, pngClsid, frames[i]);
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

Bitmap* LoadHighResIcon(const std::wstring& primaryPath, const std::wstring& fallbackPath, const std::wstring& safeAppName, const CLSID* pngClsid) {
    std::wstring origCachePng;
    if (!safeAppName.empty()) {
        wchar_t buf[MAX_PATH] = {0};
        GetEnvironmentVariableW(L"LOCALAPPDATA", buf, MAX_PATH);
        std::wstring dir = std::wstring(buf) + L"\\Modo\\backup";
        CreateDirectoryW((std::wstring(buf) + L"\\Modo").c_str(), NULL);
        CreateDirectoryW(dir.c_str(), NULL);
        origCachePng = dir + L"\\" + safeAppName + L"_orig.png";
        if (GetFileAttributesW(origCachePng.c_str()) != INVALID_FILE_ATTRIBUTES) {
            Bitmap* cachedOrig = Bitmap::FromFile(origCachePng.c_str());
            if (cachedOrig && cachedOrig->GetLastStatus() == Ok && cachedOrig->GetWidth() >= 16) {
                return cachedOrig;
            }
            delete cachedOrig;
        }
    }

    auto saveToOrigCache = [&](Bitmap* b) -> Bitmap* {
        if (b && !origCachePng.empty() && pngClsid && GetFileAttributesW(origCachePng.c_str()) == INVALID_FILE_ATTRIBUTES) {
            b->Save(origCachePng.c_str(), pngClsid, NULL);
        }
        return b;
    };

    std::wstring cleanPrimary = primaryPath;
    if (cleanPrimary.find(L"\\Modo\\icons\\") != std::wstring::npos ||
        cleanPrimary.find(L"\\IconMaker\\icons\\") != std::wstring::npos) {
        cleanPrimary.clear();
    }

    std::wstring ext = PathFindExtensionW(cleanPrimary.c_str());
    for (auto& c : ext) c = towlower(c);
    if (ext == L".png" || ext == L".bmp" || ext == L".jpg" || ext == L".jpeg") {
        Bitmap* bmp = Bitmap::FromFile(cleanPrimary.c_str());
        if (bmp && bmp->GetLastStatus() == Ok && bmp->GetWidth() > 0) return saveToOrigCache(bmp);
        delete bmp;
    }

    if (!cleanPrimary.empty()) {
        if (Bitmap* directBmp = ExtractDirectPeOrIco256(cleanPrimary, 0)) {
            return saveToOrigCache(directBmp);
        }
    }

    const std::wstring& pathToUse = (!cleanPrimary.empty() && GetFileAttributesW(cleanPrimary.c_str()) != INVALID_FILE_ATTRIBUTES)
        ? cleanPrimary
        : fallbackPath;

    if (!pathToUse.empty() && pathToUse.find(L"\\Modo\\icons\\") == std::wstring::npos && pathToUse.find(L"\\IconMaker\\icons\\") == std::wstring::npos) {
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
    SolidBrush bg(Color(255, 60, 60, 60));
    g.FillEllipse(&bg, 16, 16, 224, 224);
    return synth;
}

} // namespace IconEngine
