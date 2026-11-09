#include "icon_engine.h"
#include "shell_integration.h"
#include <algorithm>
#include <cmath>
#include <cwctype>
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
    BrandPaletteWin palette;
};

static ExtractedColors AnalyzeBitmapColors(Bitmap* src) {
    ExtractedColors res = {
        Color(255, 26, 115, 232),
        Color(255, 240, 244, 249),
        false,
        false,
        0.50,
        0.50,
        { RGB(26, 115, 232), RGB(66, 133, 244), RGB(232, 240, 254), RGB(13, 71, 161), RGB(255, 255, 255) }
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

    res.palette.primary = RGB(res.dominant.GetR(), res.dominant.GetG(), res.dominant.GetB());
    res.palette.secondary = RGB(res.edge.GetR(), res.edge.GetG(), res.edge.GetB());
    res.palette.backgroundTint = RGB(
        (BYTE)std::clamp((int)std::round(res.dominant.GetR() * 0.14 + 255.0 * 0.86), 0, 255),
        (BYTE)std::clamp((int)std::round(res.dominant.GetG() * 0.14 + 255.0 * 0.86), 0, 255),
        (BYTE)std::clamp((int)std::round(res.dominant.GetB() * 0.14 + 255.0 * 0.86), 0, 255)
    );
    res.palette.deepTone = RGB(
        (BYTE)std::clamp((int)std::round(res.dominant.GetR() * 0.22), 0, 255),
        (BYTE)std::clamp((int)std::round(res.dominant.GetG() * 0.22), 0, 255),
        (BYTE)std::clamp((int)std::round(res.dominant.GetB() * 0.22), 0, 255)
    );
    res.palette.highlight = RGB(255, 255, 255);
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

void AddShapePath(GraphicsPath& path, IconShape shape, RectF r) {
    switch (shape) {
        case SHAPE_CIRCLE:
            path.AddEllipse(r);
            break;
        case SHAPE_SQUIRCLE: {
            // Superellipse |x|^n + |y|^n = 1 with n = 5: continuous curvature, close to the macOS icon outline.
            const int steps = 256;
            const double power = 2.0 / 5.0;
            std::vector<PointF> pts(steps);
            for (int i = 0; i < steps; ++i) {
                double t = 2.0 * 3.14159265358979323846 * i / steps;
                double c = std::cos(t), s = std::sin(t);
                double x = (c < 0 ? -1.0 : 1.0) * std::pow(std::fabs(c), power);
                double y = (s < 0 ? -1.0 : 1.0) * std::pow(std::fabs(s), power);
                pts[i] = PointF((REAL)(r.X + r.Width * 0.5 + x * r.Width * 0.5),
                                (REAL)(r.Y + r.Height * 0.5 + y * r.Height * 0.5));
            }
            path.AddPolygon(pts.data(), steps);
            break;
        }
        case SHAPE_ROUNDED_RECT:
            AddRoundedRect(path, r, r.Width * 0.16f);
            break;
        case SHAPE_PEBBLE:
            AddRoundedRect(path, r, r.Width * 0.38f);
            break;
        case SHAPE_HEXAGON: {
            REAL cx = r.X + r.Width * 0.5f;
            REAL cy = r.Y + r.Height * 0.5f;
            REAL rad = std::min(r.Width, r.Height) * 0.5f;
            PointF pts[6];
            for (int i = 0; i < 6; ++i) {
                REAL angle = (REAL)(i * 3.14159265358979323846 / 3.0 - 3.14159265358979323846 / 6.0);
                pts[i] = PointF(cx + rad * std::cos(angle), cy + rad * std::sin(angle));
            }
            path.AddPolygon(pts, 6);
            break;
        }
    }
}

static void ApplySurfaceMaterialShader(Graphics& g, SurfaceMaterial mat, RectF rect) {
    if (mat == MATERIAL_SMOOTH) return;

    PointF startPt(rect.X, rect.GetBottom());
    PointF endPt(rect.GetRight(), rect.Y);

    switch (mat) {
        case MATERIAL_FROSTED_GLASS: {
            LinearGradientBrush sweep(startPt, endPt, Color(80, 255, 255, 255), Color(0, 255, 255, 255));
            Color sweepCols[3] = {
                Color(82, 255, 255, 255),
                Color(20, 255, 255, 255),
                Color(0, 255, 255, 255)
            };
            REAL sweepPos[3] = { 0.0f, 0.45f, 1.0f };
            sweep.SetInterpolationColors(sweepCols, sweepPos, 3);
            g.FillRectangle(&sweep, rect);

            RectF topGlow(rect.X, rect.Y, rect.Width, rect.Height * 0.5f);
            SolidBrush topBr(Color(22, 255, 255, 255));
            g.FillRectangle(&topBr, topGlow);
            break;
        }

        case MATERIAL_BRUSHED_TITANIUM: {
            LinearGradientBrush metal(startPt, endPt, Color(41, 255, 255, 255), Color(26, 255, 255, 255));
            Color metalCols[5] = {
                Color(41, 255, 255, 255),
                Color(31, 0, 0, 0),
                Color(56, 255, 255, 255),
                Color(36, 0, 0, 0),
                Color(26, 255, 255, 255)
            };
            REAL metalPos[5] = { 0.0f, 0.28f, 0.52f, 0.78f, 1.0f };
            metal.SetInterpolationColors(metalCols, metalPos, 5);
            g.FillRectangle(&metal, rect);

            Pen grainPen(Color(10, 255, 255, 255), 1.0f);
            REAL y = rect.Y + 4.0f;
            while (y < rect.GetBottom()) {
                g.DrawLine(&grainPen, rect.X, y, rect.GetRight(), y);
                y += 5.0f;
            }
            break;
        }

        case MATERIAL_NEUMORPHIC: {
            PointF nw(rect.X, rect.Y);
            PointF se(rect.GetRight(), rect.GetBottom());
            LinearGradientBrush neu(nw, se, Color(71, 255, 255, 255), Color(56, 0, 0, 0));
            Color neuCols[3] = {
                Color(71, 255, 255, 255),
                Color(0, 255, 255, 255),
                Color(56, 0, 0, 0)
            };
            REAL neuPos[3] = { 0.0f, 0.50f, 1.0f };
            neu.SetInterpolationColors(neuCols, neuPos, 3);
            g.FillRectangle(&neu, rect);
            break;
        }

        case MATERIAL_PRISM_GLOW: {
            PointF nw(rect.X, rect.Y);
            PointF se(rect.GetRight(), rect.GetBottom());
            LinearGradientBrush prism(nw, se, Color(46, 242, 89, 115), Color(46, 191, 89, 242));
            Color prismCols[5] = {
                Color(46, 242, 89, 115),
                Color(41, 242, 191, 64),
                Color(46, 76, 217, 140),
                Color(51, 64, 166, 242),
                Color(46, 191, 89, 242)
            };
            REAL prismPos[5] = { 0.0f, 0.25f, 0.50f, 0.75f, 1.0f };
            prism.SetInterpolationColors(prismCols, prismPos, 5);
            g.FillRectangle(&prism, rect);
            break;
        }

        case MATERIAL_LIQUID_CHROME: {
            LinearGradientBrush chrome(startPt, endPt, Color(102, 255, 255, 255), Color(89, 255, 255, 255));
            Color chromeCols[6] = {
                Color(102, 255, 255, 255),
                Color(38, 115, 122, 140),
                Color(89, 38, 46, 64),
                Color(128, 230, 240, 255),
                Color(51, 76, 84, 102),
                Color(89, 255, 255, 255)
            };
            REAL chromePos[6] = { 0.0f, 0.22f, 0.45f, 0.65f, 0.85f, 1.0f };
            chrome.SetInterpolationColors(chromeCols, chromePos, 6);
            g.FillRectangle(&chrome, rect);

            Pen arcPen(Color(76, 255, 255, 255), std::max(1.5f, rect.Width * 0.008f));
            g.DrawArc(&arcPen, rect.X, rect.Y + rect.Height * 0.2f, rect.Width, rect.Height * 0.6f, 190.0f, 160.0f);
            break;
        }

        case MATERIAL_MATTE_CERAMIC: {
            PointF top(rect.X + rect.Width * 0.5f, rect.Y);
            PointF bot(rect.X + rect.Width * 0.5f, rect.GetBottom());
            LinearGradientBrush ceramic(top, bot, Color(56, 255, 255, 255), Color(20, 0, 0, 0));
            Color ceramicCols[4] = {
                Color(56, 255, 255, 255),
                Color(20, 255, 250, 242),
                Color(0, 230, 230, 235),
                Color(20, 0, 0, 0)
            };
            REAL ceramicPos[4] = { 0.0f, 0.35f, 0.70f, 1.0f };
            ceramic.SetInterpolationColors(ceramicCols, ceramicPos, 4);
            g.FillRectangle(&ceramic, rect);
            break;
        }

        case MATERIAL_HOLOGRAPHIC: {
            PointF nw(rect.X, rect.Y);
            PointF se(rect.GetRight(), rect.GetBottom());
            LinearGradientBrush holo(nw, se, Color(71, 6, 182, 212), Color(76, 139, 92, 246));
            Color holoCols[5] = {
                Color(71, 6, 182, 212),
                Color(76, 236, 72, 153),
                Color(61, 245, 158, 11),
                Color(71, 16, 185, 129),
                Color(76, 139, 92, 246)
            };
            REAL holoPos[5] = { 0.0f, 0.25f, 0.50f, 0.75f, 1.0f };
            holo.SetInterpolationColors(holoCols, holoPos, 5);
            g.FillRectangle(&holo, rect);
            break;
        }

        default:
            break;
    }
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
            GraphicsPath shadowPath;
            AddShapePath(shadowPath, style.shape, sr);
            g.FillPath(&sb, &shadowPath);
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
    GraphicsPath platePath;
    AddShapePath(platePath, style.shape, circleRect);
    g.FillPath(&discBrush, &platePath);

    GraphicsPath clipPath;
    AddShapePath(clipPath, style.shape, circleRect);
    GraphicsState state = g.Save();
    g.SetClip(&clipPath, CombineModeReplace);

    ApplySurfaceMaterialShader(g, style.material, circleRect);

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

    GraphicsState gTransformState = g.Save();
    if (style.rotation != 0.0 || style.flipH || style.flipV) {
        g.TranslateTransform(cx, cy);
        if (style.rotation != 0.0) {
            g.RotateTransform((REAL)style.rotation);
        }
        REAL sx = style.flipH ? -1.0f : 1.0f;
        REAL sy = style.flipV ? -1.0f : 1.0f;
        if (sx != 1.0f || sy != 1.0f) {
            g.ScaleTransform(sx, sy);
        }
        g.TranslateTransform(-cx, -cy);
    }

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

    if (style.glyphGrad != GLYPH_GRAD_NONE) {
        Color c1(255, 255, 255, 255);
        Color c2(255, 255, 255, 255);
        switch (style.glyphGrad) {
            case GLYPH_GRAD_GOLD:
                c1 = Color(255, 0xF5, 0x9E, 0x0B);
                c2 = Color(255, 0xD9, 0x77, 0x06);
                break;
            case GLYPH_GRAD_CYBER:
                c1 = Color(255, 0x06, 0xB6, 0xD4);
                c2 = Color(255, 0xEC, 0x48, 0x99);
                break;
            case GLYPH_GRAD_SUNSET:
                c1 = Color(255, 0xFF, 0x51, 0x2F);
                c2 = Color(255, 0xDD, 0x24, 0x76);
                break;
            case GLYPH_GRAD_EMERALD:
                c1 = Color(255, 0x10, 0xB9, 0x81);
                c2 = Color(255, 0x05, 0x96, 0x69);
                break;
            case GLYPH_GRAD_TITANIUM:
                c1 = Color(255, 0xE2, 0xE8, 0xF0);
                c2 = Color(255, 0x94, 0xA3, 0xB8);
                break;
            case GLYPH_GRAD_PURPLE_HAZE:
                c1 = Color(255, 0x8B, 0x5C, 0xF6);
                c2 = Color(255, 0x3B, 0x82, 0xF6);
                break;
            default:
                break;
        }

        int gw = (int)std::max(1.0f, std::round(glyphRect.Width));
        int gh = (int)std::max(1.0f, std::round(glyphRect.Height));
        Bitmap gradBmp(gw, gh, PixelFormat32bppARGB);
        {
            Graphics gGrad(&gradBmp);
            LinearGradientBrush gradBrush(
                PointF(0, (REAL)gh),
                PointF((REAL)gw, 0),
                c1,
                c2
            );
            gGrad.FillRectangle(&gradBrush, 0, 0, gw, gh);
        }

        Bitmap alphaBmp(gw, gh, PixelFormat32bppARGB);
        {
            Graphics gAlpha(&alphaBmp);
            gAlpha.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            gAlpha.DrawImage(trimmed, RectF(0, 0, (REAL)gw, (REAL)gh), 0, 0, srcW, srcH, UnitPixel);
        }

        BitmapData bdGrad, bdAlpha;
        Rect rc(0, 0, gw, gh);
        if (gradBmp.LockBits(&rc, ImageLockModeRead | ImageLockModeWrite, PixelFormat32bppARGB, &bdGrad) == Ok) {
            if (alphaBmp.LockBits(&rc, ImageLockModeRead, PixelFormat32bppARGB, &bdAlpha) == Ok) {
                for (int y = 0; y < gh; ++y) {
                    uint8_t* pG = (uint8_t*)bdGrad.Scan0 + y * bdGrad.Stride;
                    const uint8_t* pA = (const uint8_t*)bdAlpha.Scan0 + y * bdAlpha.Stride;
                    for (int x = 0; x < gw; ++x) {
                        uint8_t a = pA[x * 4 + 3];
                        pG[x * 4 + 3] = (uint8_t)((pG[x * 4 + 3] * a) / 255);
                    }
                }
                alphaBmp.UnlockBits(&bdAlpha);
            }
            gradBmp.UnlockBits(&bdGrad);
        }

        g.DrawImage(&gradBmp, glyphRect.X, glyphRect.Y, glyphRect.Width, glyphRect.Height);
    } else if (style.glyphColor != 0) {
        REAL tr = GetRValue(style.glyphColor) / 255.0f;
        REAL tg = GetGValue(style.glyphColor) / 255.0f;
        REAL tb = GetBValue(style.glyphColor) / 255.0f;
        ColorMatrix customMat = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            tr,   tg,   tb,   0.0f, 1.0f
        };
        ImageAttributes customAttr;
        customAttr.SetColorMatrix(&customMat, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
        g.DrawImage(trimmed, glyphRect, 0, 0, srcW, srcH, UnitPixel, &customAttr);
    } else if (style.glyphStyle == GLYPH_SILHOUETTE) {
        Color tint = (style.preset == PRESET_DARK_MATERIAL)
            ? Color(255, 230, 230, 232)
            : colors.dominant;
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
    } else if (style.glyphStyle == GLYPH_EMBOSSED) {
        REAL offsetDist = std::max(1.5f, dim * 0.005f);
        // Shadow bevel bottom-right
        ColorMatrix shMat = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.35f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 1.0f
        };
        ImageAttributes shAttr;
        shAttr.SetColorMatrix(&shMat, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
        RectF shR(glyphRect.X, glyphRect.Y + offsetDist, glyphRect.Width, glyphRect.Height);
        g.DrawImage(trimmed, shR, 0, 0, srcW, srcH, UnitPixel, &shAttr);

        // Highlight bevel top-left
        ColorMatrix hiMat = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.45f, 0.0f,
            1.0f, 1.0f, 1.0f, 0.0f, 1.0f
        };
        ImageAttributes hiAttr;
        hiAttr.SetColorMatrix(&hiMat, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
        RectF hiR(glyphRect.X, glyphRect.Y - offsetDist, glyphRect.Width, glyphRect.Height);
        g.DrawImage(trimmed, hiR, 0, 0, srcW, srcH, UnitPixel, &hiAttr);

        // Central artwork
        g.DrawImage(trimmed, glyphRect, 0, 0, srcW, srcH, UnitPixel);
    } else if (style.glyphStyle == GLYPH_OUTLINE) {
        REAL strokeDist = std::max(1.5f, dim * 0.004f);
        ColorMatrix outMat = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.85f, 0.0f,
            1.0f, 1.0f, 1.0f, 0.0f, 1.0f
        };
        ImageAttributes outAttr;
        outAttr.SetColorMatrix(&outMat, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
        REAL offsets[8][2] = {
            {-strokeDist, 0}, {strokeDist, 0}, {0, -strokeDist}, {0, strokeDist},
            {-strokeDist * 0.7f, -strokeDist * 0.7f}, {strokeDist * 0.7f, strokeDist * 0.7f},
            {-strokeDist * 0.7f, strokeDist * 0.7f}, {strokeDist * 0.7f, -strokeDist * 0.7f}
        };
        for (int i = 0; i < 8; ++i) {
            RectF oR(glyphRect.X + offsets[i][0], glyphRect.Y + offsets[i][1], glyphRect.Width, glyphRect.Height);
            g.DrawImage(trimmed, oR, 0, 0, srcW, srcH, UnitPixel, &outAttr);
        }
        g.DrawImage(trimmed, glyphRect, 0, 0, srcW, srcH, UnitPixel);
    } else if (style.glyphStyle == GLYPH_DUOTONE) {
        REAL dr = GetRValue(colors.palette.deepTone) / 255.0f;
        REAL dg = GetGValue(colors.palette.deepTone) / 255.0f;
        REAL db = GetBValue(colors.palette.deepTone) / 255.0f;
        REAL lr = GetRValue(colors.palette.primary) / 255.0f;
        REAL lg = GetGValue(colors.palette.primary) / 255.0f;
        REAL lb = GetBValue(colors.palette.primary) / 255.0f;
        REAL scaleR = lr - dr;
        REAL scaleG = lg - dg;
        REAL scaleB = lb - db;

        ColorMatrix duoMat = {
            0.299f * scaleR, 0.299f * scaleG, 0.299f * scaleB, 0.0f, 0.0f,
            0.587f * scaleR, 0.587f * scaleG, 0.587f * scaleB, 0.0f, 0.0f,
            0.114f * scaleR, 0.114f * scaleG, 0.114f * scaleB, 0.0f, 0.0f,
            0.0f,            0.0f,            0.0f,            1.0f, 0.0f,
            dr,              dg,              db,              0.0f, 1.0f
        };
        ImageAttributes duoAttr;
        duoAttr.SetColorMatrix(&duoMat, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
        g.DrawImage(trimmed, glyphRect, 0, 0, srcW, srcH, UnitPixel, &duoAttr);
    } else if (style.glyphStyle == GLYPH_GLOW) {
        Color glowCol = colors.dominant;
        REAL gr = glowCol.GetR() / 255.0f;
        REAL gg = glowCol.GetG() / 255.0f;
        REAL gb = glowCol.GetB() / 255.0f;
        REAL glowDist = std::max(2.0f, dim * 0.015f);

        ColorMatrix glowMat = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.25f, 0.0f,
            gr,   gg,   gb,   0.0f, 1.0f
        };
        ImageAttributes glowAttr;
        glowAttr.SetColorMatrix(&glowMat, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
        REAL offsets[8][2] = {
            {-glowDist, 0}, {glowDist, 0}, {0, -glowDist}, {0, glowDist},
            {-glowDist * 0.7f, -glowDist * 0.7f}, {glowDist * 0.7f, glowDist * 0.7f},
            {-glowDist * 0.7f, glowDist * 0.7f}, {glowDist * 0.7f, -glowDist * 0.7f}
        };
        for (int i = 0; i < 8; ++i) {
            RectF gR(glyphRect.X + offsets[i][0], glyphRect.Y + offsets[i][1], glyphRect.Width, glyphRect.Height);
            g.DrawImage(trimmed, gR, 0, 0, srcW, srcH, UnitPixel, &glowAttr);
        }
        g.DrawImage(trimmed, glyphRect, 0, 0, srcW, srcH, UnitPixel);
    } else if (style.monochrome && colors.isSquareLike) {
        // Full-bleed artwork has no transparent glyph to silhouette; a flat tint would paint a
        // solid block. Desaturate instead so the artwork keeps its detail.
        ColorMatrix grayMat = {
            0.299f, 0.299f, 0.299f, 0.0f, 0.0f,
            0.587f, 0.587f, 0.587f, 0.0f, 0.0f,
            0.114f, 0.114f, 0.114f, 0.0f, 0.0f,
            0.0f,   0.0f,   0.0f,   1.0f, 0.0f,
            0.0f,   0.0f,   0.0f,   0.0f, 1.0f
        };
        ImageAttributes grayAttr;
        grayAttr.SetColorMatrix(&grayMat, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
        g.DrawImage(trimmed, glyphRect, 0, 0, srcW, srcH, UnitPixel, &grayAttr);
    } else if (style.monochrome) {
        Color tint = (style.preset == PRESET_DARK_MATERIAL)
            ? Color(255, 230, 230, 232)
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

    g.Restore(gTransformState);
    g.Restore(state);

    REAL penW = std::max(1.0f, dim * 0.004f);
    Color rimColor = (style.preset == PRESET_DARK_MATERIAL)
        ? Color(45, 255, 255, 255)
        : Color(26, 0, 0, 0);
    Pen rimPen(rimColor, penW);
    RectF rimRect = circleRect;
    rimRect.Inflate(-penW * 0.5f, -penW * 0.5f);
    GraphicsPath rimPath;
    AddShapePath(rimPath, style.shape, rimRect);
    g.DrawPath(&rimPen, &rimPath);

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

static bool SaveIcoWithSizes(Bitmap* sourceOrigBitmap, const RoundStyle& style, const std::wstring& icoPath, const int* sizes, int count, const CLSID* pngClsid) {
    if (!sourceOrigBitmap || count <= 0) return false;
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

bool SaveMultiSizeIco(Bitmap* sourceOrigBitmap, const RoundStyle& style, const std::wstring& icoPath, const CLSID* pngClsid) {
    const int sizes[] = { 256, 128, 96, 64, 48, 40, 32, 24, 16 };
    return SaveIcoWithSizes(sourceOrigBitmap, style, icoPath, sizes, (int)(sizeof(sizes)/sizeof(sizes[0])), pngClsid);
}

bool SaveFaviconIco(Bitmap* sourceOrigBitmap, const RoundStyle& style, const std::wstring& icoPath, const CLSID* pngClsid) {
    const int sizes[] = { 48, 32, 16 };
    return SaveIcoWithSizes(sourceOrigBitmap, style, icoPath, sizes, (int)(sizeof(sizes)/sizeof(sizes[0])), pngClsid);
}

static bool WriteUtf8File(const std::wstring& path, const std::string& content) {
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    WriteFile(hFile, content.data(), (DWORD)content.size(), &written, NULL);
    CloseHandle(hFile);
    return written == (DWORD)content.size();
}

bool ExportWebFaviconKit(Bitmap* src, const RoundStyle& style, const std::wstring& appName, const std::wstring& outDir, const CLSID* pngClsid) {
    if (!src || !pngClsid || outDir.empty()) return false;
    std::wstring clean = ShellIntegration::SanitizeFileName(appName);
    if (clean.empty()) clean = L"modo-app";
    for (auto& ch : clean) {
        if (ch == L' ') ch = L'-';
        else ch = towlower(ch);
    }
    std::wstring kitDir = outDir + L"\\" + clean + L"-favicons";
    CreateDirectoryW(kitDir.c_str(), NULL);

    // 1. Multi-resolution favicon.ico (16, 32, 48)
    SaveFaviconIco(src, style, kitDir + L"\\favicon.ico", pngClsid);

    // 2. Web PNGs
    struct WebSpec { const wchar_t* name; int px; };
    const WebSpec specs[] = {
        { L"favicon-16x16.png", 16 },
        { L"favicon-32x32.png", 32 },
        { L"apple-touch-icon.png", 180 },
        { L"android-chrome-192x192.png", 192 },
        { L"android-chrome-512x512.png", 512 }
    };

    for (const auto& sp : specs) {
        Bitmap* bmp = RenderGoogleRoundIcon(src, style, sp.px);
        if (bmp) {
            std::wstring fpath = kitDir + L"\\" + sp.name;
            bmp->Save(fpath.c_str(), pngClsid, NULL);
            delete bmp;
        }
    }

    // 3. App title UTF-8
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, appName.c_str(), -1, NULL, 0, NULL, NULL);
    std::string nameUtf8(utf8Len > 1 ? utf8Len - 1 : 0, '\0');
    if (!nameUtf8.empty()) {
        WideCharToMultiByte(CP_UTF8, 0, appName.c_str(), -1, nameUtf8.data(), utf8Len, NULL, NULL);
    }
    if (nameUtf8.empty()) nameUtf8 = "Modo";

    // 4. site.webmanifest
    std::string manifest =
        "{\n"
        "  \"name\": \"" + nameUtf8 + "\",\n"
        "  \"short_name\": \"" + nameUtf8 + "\",\n"
        "  \"icons\": [\n"
        "    {\n"
        "      \"src\": \"/android-chrome-192x192.png\",\n"
        "      \"sizes\": \"192x192\",\n"
        "      \"type\": \"image/png\"\n"
        "    },\n"
        "    {\n"
        "      \"src\": \"/android-chrome-512x512.png\",\n"
        "      \"sizes\": \"512x512\",\n"
        "      \"type\": \"image/png\"\n"
        "    }\n"
        "  ],\n"
        "  \"theme_color\": \"#ffffff\",\n"
        "  \"background_color\": \"#ffffff\",\n"
        "  \"display\": \"standalone\"\n"
        "}\n";
    WriteUtf8File(kitDir + L"\\site.webmanifest", manifest);

    // 5. html_snippet.html
    std::string snippet =
        "<!-- Modo Web Favicon Tags -->\n"
        "<link rel=\"apple-touch-icon\" sizes=\"180x180\" href=\"/apple-touch-icon.png\">\n"
        "<link rel=\"icon\" type=\"image/png\" sizes=\"32x32\" href=\"/favicon-32x32.png\">\n"
        "<link rel=\"icon\" type=\"image/png\" sizes=\"16x16\" href=\"/favicon-16x16.png\">\n"
        "<link rel=\"manifest\" href=\"/site.webmanifest\">\n"
        "<meta name=\"theme-color\" content=\"#ffffff\">\n";
    WriteUtf8File(kitDir + L"\\html_snippet.html", snippet);

    return true;
}

bool ExportXcodeAssetCatalog(Bitmap* src, const RoundStyle& style, const std::wstring& appName, const std::wstring& outDir, const CLSID* pngClsid) {
    if (!src || !pngClsid || outDir.empty()) return false;
    std::wstring clean;
    for (wchar_t c : appName) {
        if (iswalnum(c)) clean.push_back(c);
    }
    if (clean.empty()) clean = L"AppIcon";
    std::wstring setDir = outDir + L"\\" + clean + L".appiconset";
    CreateDirectoryW(setDir.c_str(), NULL);

    struct XcodeSpec { const wchar_t* filename; int px; };
    const XcodeSpec specs[] = {
        { L"icon_16x16.png", 16 },
        { L"icon_16x16@2x.png", 32 },
        { L"icon_32x32.png", 32 },
        { L"icon_32x32@2x.png", 64 },
        { L"icon_128x128.png", 128 },
        { L"icon_128x128@2x.png", 256 },
        { L"icon_256x256.png", 256 },
        { L"icon_256x256@2x.png", 512 },
        { L"icon_512x512.png", 512 },
        { L"icon_512x512@2x.png", 1024 },
        { L"icon_1024x1024.png", 1024 }
    };

    for (const auto& sp : specs) {
        Bitmap* bmp = RenderGoogleRoundIcon(src, style, sp.px);
        if (bmp) {
            std::wstring fpath = setDir + L"\\" + sp.filename;
            bmp->Save(fpath.c_str(), pngClsid, NULL);
            delete bmp;
        }
    }

    std::string contentsJson =
        "{\n"
        "  \"images\" : [\n"
        "    {\n"
        "      \"filename\" : \"icon_16x16.png\",\n"
        "      \"idiom\" : \"mac\",\n"
        "      \"scale\" : \"1x\",\n"
        "      \"size\" : \"16x16\"\n"
        "    },\n"
        "    {\n"
        "      \"filename\" : \"icon_16x16@2x.png\",\n"
        "      \"idiom\" : \"mac\",\n"
        "      \"scale\" : \"2x\",\n"
        "      \"size\" : \"16x16\"\n"
        "    },\n"
        "    {\n"
        "      \"filename\" : \"icon_32x32.png\",\n"
        "      \"idiom\" : \"mac\",\n"
        "      \"scale\" : \"1x\",\n"
        "      \"size\" : \"32x32\"\n"
        "    },\n"
        "    {\n"
        "      \"filename\" : \"icon_32x32@2x.png\",\n"
        "      \"idiom\" : \"mac\",\n"
        "      \"scale\" : \"2x\",\n"
        "      \"size\" : \"32x32\"\n"
        "    },\n"
        "    {\n"
        "      \"filename\" : \"icon_128x128.png\",\n"
        "      \"idiom\" : \"mac\",\n"
        "      \"scale\" : \"1x\",\n"
        "      \"size\" : \"128x128\"\n"
        "    },\n"
        "    {\n"
        "      \"filename\" : \"icon_128x128@2x.png\",\n"
        "      \"idiom\" : \"mac\",\n"
        "      \"scale\" : \"2x\",\n"
        "      \"size\" : \"128x128\"\n"
        "    },\n"
        "    {\n"
        "      \"filename\" : \"icon_256x256.png\",\n"
        "      \"idiom\" : \"mac\",\n"
        "      \"scale\" : \"1x\",\n"
        "      \"size\" : \"256x256\"\n"
        "    },\n"
        "    {\n"
        "      \"filename\" : \"icon_256x256@2x.png\",\n"
        "      \"idiom\" : \"mac\",\n"
        "      \"scale\" : \"2x\",\n"
        "      \"size\" : \"256x256\"\n"
        "    },\n"
        "    {\n"
        "      \"filename\" : \"icon_512x512.png\",\n"
        "      \"idiom\" : \"mac\",\n"
        "      \"scale\" : \"1x\",\n"
        "      \"size\" : \"512x512\"\n"
        "    },\n"
        "    {\n"
        "      \"filename\" : \"icon_512x512@2x.png\",\n"
        "      \"idiom\" : \"mac\",\n"
        "      \"scale\" : \"2x\",\n"
        "      \"size\" : \"512x512\"\n"
        "    },\n"
        "    {\n"
        "      \"filename\" : \"icon_1024x1024.png\",\n"
        "      \"idiom\" : \"universal\",\n"
        "      \"platform\" : \"ios\",\n"
        "      \"scale\" : \"1x\",\n"
        "      \"size\" : \"1024x1024\"\n"
        "    }\n"
        "  ],\n"
        "  \"info\" : {\n"
        "    \"author\" : \"Modo by Atoma\",\n"
        "    \"version\" : 1\n"
        "  }\n"
        "}\n";
    WriteUtf8File(setDir + L"\\Contents.json", contentsJson);

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

bool ExportShowcaseBanner(Bitmap* src, const RoundStyle& style, const std::wstring& appName,
                         ShowcaseTheme theme, ShowcaseLayout layout,
                         const std::wstring& outPath, const CLSID* pngClsid,
                         int width, int height) {
    if (!src || outPath.empty() || !pngClsid) return false;

    Bitmap canvas(width, height, PixelFormat32bppARGB);
    Graphics g(&canvas);
    g.SetSmoothingMode(SmoothingModeHighQuality);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    // 1. Draw Themed Background
    RectF fullRect(0.0f, 0.0f, (REAL)width, (REAL)height);
    bool isLight = (theme == SHOWCASE_THEME_CLEAN_WHITE);

    switch (theme) {
        case SHOWCASE_THEME_AMBIENT_MESH: {
            LinearGradientBrush bgBr(PointF(0.0f, 0.0f), PointF((REAL)width, (REAL)height),
                                    Color(255, 20, 26, 41), Color(255, 13, 16, 26));
            Color cols[3] = { Color(255, 20, 26, 41), Color(255, 38, 46, 71), Color(255, 13, 16, 26) };
            REAL pos[3] = { 0.0f, 0.52f, 1.0f };
            bgBr.SetInterpolationColors(cols, pos, 3);
            g.FillRectangle(&bgBr, fullRect);

            // Ambient central glow
            GraphicsPath glowPath;
            glowPath.AddEllipse((REAL)(width * 0.5f - 600), (REAL)(height * 0.40f - 400), 1200.0f, 800.0f);
            PathGradientBrush pgb(&glowPath);
            Color cCol(70, 75, 115, 245);
            Color sCol(0, 0, 0, 0);
            int cnt = 1;
            pgb.SetCenterColor(cCol);
            pgb.SetSurroundColors(&sCol, &cnt);
            g.FillPath(&pgb, &glowPath);
            break;
        }
        case SHOWCASE_THEME_STUDIO_DARK: {
            LinearGradientBrush bgBr(PointF(0.0f, 0.0f), PointF((REAL)width, (REAL)height),
                                    Color(255, 28, 30, 36), Color(255, 10, 10, 13));
            g.FillRectangle(&bgBr, fullRect);

            // Studio top spotlight
            GraphicsPath spotPath;
            spotPath.AddEllipse((REAL)(width * 0.5f - 550), (REAL)(height * 0.35f - 350), 1100.0f, 700.0f);
            PathGradientBrush pgb(&spotPath);
            Color cCol(50, 255, 255, 255);
            Color sCol(0, 0, 0, 0);
            int cnt = 1;
            pgb.SetCenterColor(cCol);
            pgb.SetSurroundColors(&sCol, &cnt);
            g.FillPath(&pgb, &spotPath);
            break;
        }
        case SHOWCASE_THEME_CLEAN_WHITE: {
            LinearGradientBrush bgBr(PointF(0.0f, 0.0f), PointF((REAL)width, (REAL)height),
                                    Color(255, 250, 250, 252), Color(255, 230, 232, 240));
            g.FillRectangle(&bgBr, fullRect);
            break;
        }
        case SHOWCASE_THEME_SUNSET_GLOW: {
            LinearGradientBrush bgBr(PointF(0.0f, 0.0f), PointF((REAL)width, (REAL)height),
                                    Color(255, 38, 15, 46), Color(255, 20, 10, 26));
            Color cols[3] = { Color(255, 38, 15, 46), Color(255, 82, 31, 56), Color(255, 20, 10, 26) };
            REAL pos[3] = { 0.0f, 0.55f, 1.0f };
            bgBr.SetInterpolationColors(cols, pos, 3);
            g.FillRectangle(&bgBr, fullRect);

            // Sunset radial glow
            GraphicsPath glowPath;
            glowPath.AddEllipse((REAL)(width * 0.5f - 600), (REAL)(height * 0.42f - 400), 1200.0f, 800.0f);
            PathGradientBrush pgb(&glowPath);
            Color cCol(80, 255, 110, 80);
            Color sCol(0, 0, 0, 0);
            int cnt = 1;
            pgb.SetCenterColor(cCol);
            pgb.SetSurroundColors(&sCol, &cnt);
            g.FillPath(&pgb, &glowPath);
            break;
        }
        case SHOWCASE_THEME_CYBER_NEON: {
            LinearGradientBrush bgBr(PointF(0.0f, 0.0f), PointF((REAL)width, (REAL)height),
                                    Color(255, 5, 20, 31), Color(255, 3, 8, 13));
            Color cols[3] = { Color(255, 5, 20, 31), Color(255, 10, 41, 56), Color(255, 3, 8, 13) };
            REAL pos[3] = { 0.0f, 0.50f, 1.0f };
            bgBr.SetInterpolationColors(cols, pos, 3);
            g.FillRectangle(&bgBr, fullRect);

            // Cyber neon glow
            GraphicsPath glowPath;
            glowPath.AddEllipse((REAL)(width * 0.5f - 550), (REAL)(height * 0.40f - 350), 1100.0f, 700.0f);
            PathGradientBrush pgb(&glowPath);
            Color cCol(75, 0, 225, 255);
            Color sCol(0, 0, 0, 0);
            int cnt = 1;
            pgb.SetCenterColor(cCol);
            pgb.SetSurroundColors(&sCol, &cnt);
            g.FillPath(&pgb, &glowPath);
            break;
        }
    }

    // Typography Setup
    FontFamily fam(L"Segoe UI");
    Font fHeroTitle(&fam, 52.0f, FontStyleBold, UnitPixel);
    Font fHeroSub(&fam, 24.0f, FontStyleRegular, UnitPixel);
    Font fBadge(&fam, 15.0f, FontStyleBold, UnitPixel);
    Font fLabel(&fam, 18.0f, FontStyleBold, UnitPixel);

    SolidBrush textPrimBr(isLight ? Color(255, 20, 20, 24) : Color(255, 255, 255, 255));
    SolidBrush textSecBr(isLight ? Color(255, 95, 95, 105) : Color(255, 160, 165, 180));
    StringFormat sfC; sfC.SetAlignment(StringAlignmentCenter); sfC.SetLineAlignment(StringAlignmentCenter);

    // Helpers for high-end diffuse shadows
    auto drawShowcaseIconWithShadow = [&](Bitmap* icon, RectF r, IconShape shape) {
        if (!icon) return;
        GraphicsPath shp1, shp2, shp3;
        AddShapePath(shp1, shape, RectF(r.X - 16.0f, r.Y + 24.0f, r.Width + 32.0f, r.Height + 32.0f));
        AddShapePath(shp2, shape, RectF(r.X - 8.0f, r.Y + 14.0f, r.Width + 16.0f, r.Height + 16.0f));
        AddShapePath(shp3, shape, RectF(r.X, r.Y + 6.0f, r.Width, r.Height));

        SolidBrush sb1(isLight ? Color(25, 0, 0, 0) : Color(45, 0, 0, 0));
        SolidBrush sb2(isLight ? Color(40, 0, 0, 0) : Color(65, 0, 0, 0));
        SolidBrush sb3(isLight ? Color(55, 0, 0, 0) : Color(90, 0, 0, 0));
        g.FillPath(&sb1, &shp1);
        g.FillPath(&sb2, &shp2);
        g.FillPath(&sb3, &shp3);

        g.DrawImage(icon, r);

        // Edge rim highlight
        GraphicsPath rimP;
        AddShapePath(rimP, shape, r);
        Pen rimPen(isLight ? Color(60, 0, 0, 0) : Color(80, 255, 255, 255), 1.5f);
        g.DrawPath(&rimPen, &rimP);
    };

    // 2. Render Layouts
    if (layout == SHOWCASE_LAYOUT_HERO) {
        REAL iconDim = 460.0f;
        REAL iconX = (width - iconDim) * 0.5f;
        REAL iconY = (REAL)(height * 0.20f);

        Bitmap* hiRound = RenderGoogleRoundIcon(src, style, 512);
        if (hiRound) {
            drawShowcaseIconWithShadow(hiRound, RectF(iconX, iconY, iconDim, iconDim), style.shape);
            delete hiRound;
        }

        // Title and Subtitle
        REAL textY = iconY + iconDim + 46.0f;
        RectF titleR(60.0f, textY, (REAL)(width - 120), 64.0f);
        g.DrawString(appName.c_str(), -1, &fHeroTitle, titleR, &sfC, &textPrimBr);

        RectF subR(60.0f, textY + 62.0f, (REAL)(width - 120), 34.0f);
        g.DrawString(L"Adaptive Icon · Designed with Modo Studio", -1, &fHeroSub, subR, &sfC, &textSecBr);

        // Badge pill
        REAL badgeW = 200.0f, badgeH = 32.0f;
        RectF badgeR((width - badgeW) * 0.5f, textY + 104.0f, badgeW, badgeH);
        FillRoundRect(g, badgeR, 16.0f, isLight ? Color(25, 0, 0, 0) : Color(50, 255, 255, 255));
        DrawRoundRect(g, badgeR, 16.0f, isLight ? Color(50, 0, 0, 0) : Color(80, 255, 255, 255), 1.0f);
        g.DrawString(L"MODO ADAPTIVE", -1, &fBadge, badgeR, &sfC, &textPrimBr);

    } else if (layout == SHOWCASE_LAYOUT_TRIPLE) {
        REAL iconDim = 320.0f;
        REAL spacing = 70.0f;
        REAL totalW = (iconDim * 3.0f) + (spacing * 2.0f);
        REAL startX = (width - totalW) * 0.5f;
        REAL iconY = 360.0f;

        // Title at top
        RectF titleR(60.0f, 90.0f, (REAL)(width - 120), 64.0f);
        g.DrawString(appName.c_str(), -1, &fHeroTitle, titleR, &sfC, &textPrimBr);

        RectF subR(60.0f, 154.0f, (REAL)(width - 120), 34.0f);
        g.DrawString(L"Light, Dark & Material You Variants · Designed with Modo Studio", -1, &fHeroSub, subR, &sfC, &textSecBr);

        // 1. Light variant
        RoundStyle lightSt = style;
        lightSt.preset = PRESET_GOOGLE_WHITE;
        Bitmap* lightBmp = RenderGoogleRoundIcon(src, lightSt, 512);
        if (lightBmp) {
            drawShowcaseIconWithShadow(lightBmp, RectF(startX, iconY, iconDim, iconDim), lightSt.shape);
            delete lightBmp;
        }
        RectF lbl1R(startX, iconY + iconDim + 24.0f, iconDim, 30.0f);
        g.DrawString(L"Light", -1, &fLabel, lbl1R, &sfC, &textSecBr);

        // 2. Active hero variant
        Bitmap* heroBmp = RenderGoogleRoundIcon(src, style, 512);
        if (heroBmp) {
            drawShowcaseIconWithShadow(heroBmp, RectF(startX + iconDim + spacing, iconY, iconDim, iconDim), style.shape);
            delete heroBmp;
        }
        RectF lbl2R(startX + iconDim + spacing, iconY + iconDim + 24.0f, iconDim, 30.0f);
        g.DrawString(L"Modo Adaptive", -1, &fLabel, lbl2R, &sfC, &textPrimBr);

        // 3. Dark variant
        RoundStyle darkSt = style;
        darkSt.preset = PRESET_DARK_MATERIAL;
        Bitmap* darkBmp = RenderGoogleRoundIcon(src, darkSt, 512);
        if (darkBmp) {
            drawShowcaseIconWithShadow(darkBmp, RectF(startX + (iconDim + spacing) * 2.0f, iconY, iconDim, iconDim), darkSt.shape);
            delete darkBmp;
        }
        RectF lbl3R(startX + (iconDim + spacing) * 2.0f, iconY + iconDim + 24.0f, iconDim, 30.0f);
        g.DrawString(L"Dark Material", -1, &fLabel, lbl3R, &sfC, &textSecBr);

    } else if (layout == SHOWCASE_LAYOUT_WINDOW_CARD) {
        REAL cardW = 1200.0f;
        REAL cardH = 700.0f;
        REAL cardX = (width - cardW) * 0.5f;
        REAL cardY = (height - cardH) * 0.5f;
        RectF cardR(cardX, cardY, cardW, cardH);

        // Window card diffuse shadow
        RectF shCard(cardX - 16.0f, cardY + 24.0f, cardW + 32.0f, cardH + 32.0f);
        FillRoundRect(g, shCard, 24.0f, Color(70, 0, 0, 0));

        // Window body
        FillRoundRect(g, cardR, 20.0f, isLight ? Color(255, 255, 255, 255) : Color(255, 28, 30, 36));
        DrawRoundRect(g, cardR, 20.0f, isLight ? Color(255, 218, 220, 228) : Color(255, 60, 62, 70), 1.5f);

        // Window Titlebar
        RectF tbR(cardX, cardY, cardW, 52.0f);
        FillRoundRect(g, tbR, 20.0f, isLight ? Color(255, 244, 244, 247) : Color(255, 38, 40, 48));
        SolidBrush tbFill(isLight ? Color(255, 244, 244, 247) : Color(255, 38, 40, 48));
        g.FillRectangle(&tbFill, cardX, cardY + 26.0f, cardW, 26.0f);
        Pen divPen(isLight ? Color(255, 220, 220, 225) : Color(255, 55, 55, 62), 1.0f);
        g.DrawLine(&divPen, cardX, cardY + 52.0f, cardX + cardW, cardY + 52.0f);

        // Window traffic lights
        FillRoundRect(g, RectF(cardX + 22.0f, cardY + 20.0f, 13.0f, 13.0f), 6.5f, Color(255, 255, 95, 87));
        FillRoundRect(g, RectF(cardX + 42.0f, cardY + 20.0f, 13.0f, 13.0f), 6.5f, Color(255, 254, 188, 46));
        FillRoundRect(g, RectF(cardX + 62.0f, cardY + 20.0f, 13.0f, 13.0f), 6.5f, Color(255, 40, 200, 64));

        std::wstring tbTitle = appName + L" — Modo Studio Presentation";
        Font fTb(&fam, 15.0f, FontStyleBold, UnitPixel);
        g.DrawString(tbTitle.c_str(), -1, &fTb, tbR, &sfC, &textSecBr);

        // Inside card: Hero icon on the left
        REAL inIconDim = 380.0f;
        REAL inIconX = cardX + 100.0f;
        REAL inIconY = cardY + 52.0f + (cardH - 52.0f - inIconDim) * 0.5f;

        Bitmap* hiRound = RenderGoogleRoundIcon(src, style, 512);
        if (hiRound) {
            drawShowcaseIconWithShadow(hiRound, RectF(inIconX, inIconY, inIconDim, inIconDim), style.shape);
            delete hiRound;
        }

        // Inside card: Typography on the right
        REAL textX = inIconX + inIconDim + 80.0f;
        REAL textW = cardX + cardW - textX - 80.0f;
        REAL textTopY = inIconY + 40.0f;

        StringFormat sfL; sfL.SetAlignment(StringAlignmentNear); sfL.SetLineAlignment(StringAlignmentNear);
        g.DrawString(appName.c_str(), -1, &fHeroTitle, RectF(textX, textTopY, textW, 64.0f), &sfL, &textPrimBr);
        g.DrawString(L"Pure Native Vector & Material Adaptive Icon", -1, &fHeroSub, RectF(textX, textTopY + 60.0f, textW, 36.0f), &sfL, &textSecBr);

        // Technical Spec Details Box
        REAL specY = textTopY + 115.0f;
        RectF specR(textX, specY, textW, 140.0f);
        FillRoundRect(g, specR, 10.0f, isLight ? Color(255, 244, 244, 248) : Color(255, 20, 21, 25));
        DrawRoundRect(g, specR, 10.0f, isLight ? Color(255, 222, 222, 228) : Color(255, 48, 50, 58), 1.0f);

        wchar_t szSpecs[256];
        wsprintfW(szSpecs, L"Shape: %s  ·  Scale: %d%%  ·  Pan: (%+d%%, %+d%%)\nSurface: %s  ·  Shadow Depth: %d%%\nResolution: 1024×1024 Master Multi-Size Suite",
                  (style.shape == SHAPE_CIRCLE ? L"Circle" : style.shape == SHAPE_SQUIRCLE ? L"Squircle" :
                   style.shape == SHAPE_ROUNDED_RECT ? L"Rounded Rect" : style.shape == SHAPE_PEBBLE ? L"Pebble" : L"Hexagon"),
                  (int)std::round(style.scale * 100.0),
                  (int)std::round(style.offsetX * 100.0),
                  (int)std::round(style.offsetY * 100.0),
                  (style.material == MATERIAL_FROSTED_GLASS ? L"Frosted Glass" :
                   style.material == MATERIAL_BRUSHED_TITANIUM ? L"Brushed Titanium" :
                   style.material == MATERIAL_NEUMORPHIC ? L"Neumorphic" :
                   style.material == MATERIAL_PRISM_GLOW ? L"Prism Glow" : L"Smooth Material"),
                  (int)std::round(style.shadow * 100.0));

        Font fSpec(&fam, 16.0f, FontStyleRegular, UnitPixel);
        g.DrawString(szSpecs, -1, &fSpec, RectF(specR.X + 18.0f, specR.Y + 18.0f, specR.Width - 36.0f, specR.Height - 36.0f), &sfL, &textSecBr);
    }

    return (canvas.Save(outPath.c_str(), pngClsid, NULL) == Ok);
}

static std::string Base64Encode(const uint8_t* data, size_t len) {
    static const char kBase64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string ret;
    ret.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3) {
        uint32_t val = (data[i] << 16);
        if (i + 1 < len) val |= (data[i + 1] << 8);
        if (i + 2 < len) val |= data[i + 2];
        ret.push_back(kBase64Chars[(val >> 18) & 0x3F]);
        ret.push_back(kBase64Chars[(val >> 12) & 0x3F]);
        ret.push_back((i + 1 < len) ? kBase64Chars[(val >> 6) & 0x3F] : '=');
        ret.push_back((i + 2 < len) ? kBase64Chars[val & 0x3F] : '=');
    }
    return ret;
}

static std::string BitmapToBase64Png(Bitmap* bmp, const CLSID* pngClsid) {
    if (!bmp || !pngClsid) return "";
    IStream* pStream = nullptr;
    if (CreateStreamOnHGlobal(NULL, TRUE, &pStream) != S_OK || !pStream) return "";
    if (bmp->Save(pStream, pngClsid, NULL) != Ok) {
        pStream->Release();
        return "";
    }
    HGLOBAL hGlobal = NULL;
    GetHGlobalFromStream(pStream, &hGlobal);
    if (!hGlobal) {
        pStream->Release();
        return "";
    }
    size_t size = GlobalSize(hGlobal);
    const uint8_t* pData = (const uint8_t*)GlobalLock(hGlobal);
    std::string b64;
    if (pData && size > 0) {
        b64 = Base64Encode(pData, size);
        GlobalUnlock(hGlobal);
    }
    pStream->Release();
    return b64;
}

bool ExportSvgVector(Bitmap* src, const RoundStyle& style, const std::wstring& appName,
                     const std::wstring& outDir, const CLSID* pngClsid) {
    CreateDirectoryW(outDir.c_str(), NULL);
    std::wstring safe = ShellIntegration::SanitizeFileName(appName);
    std::wstring svgPath = outDir + L"\\" + safe + L".svg";

    std::string topColor = "#FFFFFF";
    std::string bottomColor = "#F1F3F4";
    if (style.preset == PRESET_DARK_MATERIAL) {
        topColor = "#2D2E30"; bottomColor = "#1F2023";
    } else if (style.preset == PRESET_MATERIAL_PASTEL) {
        topColor = "#F8F9FA"; bottomColor = "#E8F0FE";
    } else if (style.preset == PRESET_CUSTOM_COLOR) {
        char cBuf[32];
        sprintf_s(cBuf, "#%02X%02X%02X", GetRValue(style.customColor), GetGValue(style.customColor), GetBValue(style.customColor));
        topColor = cBuf;
        bottomColor = cBuf;
    }

    std::string svg;
    svg += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    svg += "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" viewBox=\"0 0 512 512\" width=\"512\" height=\"512\">\n";
    svg += "  <defs>\n";
    svg += "    <filter id=\"modo-shadow\" x=\"-20%\" y=\"-20%\" width=\"140%\" height=\"140%\">\n";
    svg += "      <feDropShadow dx=\"0\" dy=\"16\" stdDeviation=\"20\" flood-color=\"#000000\" flood-opacity=\"0.35\" />\n";
    svg += "    </filter>\n";
    svg += "    <linearGradient id=\"bg-grad\" x1=\"0%\" y1=\"0%\" x2=\"0%\" y2=\"100%\">\n";
    svg += "      <stop offset=\"0%\" stop-color=\"" + topColor + "\" />\n";
    svg += "      <stop offset=\"100%\" stop-color=\"" + bottomColor + "\" />\n";
    svg += "    </linearGradient>\n";

    if (style.material == MATERIAL_FROSTED_GLASS) {
        svg += "    <linearGradient id=\"glass-sweep\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\">\n";
        svg += "      <stop offset=\"0%\" stop-color=\"#ffffff\" stop-opacity=\"0.35\" />\n";
        svg += "      <stop offset=\"45%\" stop-color=\"#ffffff\" stop-opacity=\"0.08\" />\n";
        svg += "      <stop offset=\"100%\" stop-color=\"#ffffff\" stop-opacity=\"0.0\" />\n";
        svg += "    </linearGradient>\n";
    } else if (style.material == MATERIAL_BRUSHED_TITANIUM) {
        svg += "    <linearGradient id=\"titanium-grad\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\">\n";
        svg += "      <stop offset=\"0%\" stop-color=\"#ffffff\" stop-opacity=\"0.20\" />\n";
        svg += "      <stop offset=\"28%\" stop-color=\"#000000\" stop-opacity=\"0.12\" />\n";
        svg += "      <stop offset=\"52%\" stop-color=\"#ffffff\" stop-opacity=\"0.24\" />\n";
        svg += "      <stop offset=\"78%\" stop-color=\"#000000\" stop-opacity=\"0.14\" />\n";
        svg += "      <stop offset=\"100%\" stop-color=\"#ffffff\" stop-opacity=\"0.10\" />\n";
        svg += "    </linearGradient>\n";
    } else if (style.material == MATERIAL_NEUMORPHIC) {
        svg += "    <linearGradient id=\"neu-grad\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\">\n";
        svg += "      <stop offset=\"0%\" stop-color=\"#ffffff\" stop-opacity=\"0.30\" />\n";
        svg += "      <stop offset=\"50%\" stop-color=\"#ffffff\" stop-opacity=\"0.0\" />\n";
        svg += "      <stop offset=\"100%\" stop-color=\"#000000\" stop-opacity=\"0.22\" />\n";
        svg += "    </linearGradient>\n";
    } else if (style.material == MATERIAL_PRISM_GLOW) {
        svg += "    <linearGradient id=\"prism-grad\" x1=\"0%\" y1=\"100%\" x2=\"100%\" y2=\"0%\">\n";
        svg += "      <stop offset=\"0%\" stop-color=\"#F43F5E\" stop-opacity=\"0.25\" />\n";
        svg += "      <stop offset=\"25%\" stop-color=\"#F59E0B\" stop-opacity=\"0.20\" />\n";
        svg += "      <stop offset=\"50%\" stop-color=\"#10B981\" stop-opacity=\"0.25\" />\n";
        svg += "      <stop offset=\"75%\" stop-color=\"#3B82F6\" stop-opacity=\"0.28\" />\n";
        svg += "      <stop offset=\"100%\" stop-color=\"#8B5CF6\" stop-opacity=\"0.25\" />\n";
        svg += "    </linearGradient>\n";
    } else if (style.material == MATERIAL_LIQUID_CHROME) {
        svg += "    <linearGradient id=\"chrome-grad\" x1=\"0%\" y1=\"100%\" x2=\"100%\" y2=\"0%\">\n";
        svg += "      <stop offset=\"0%\" stop-color=\"#ffffff\" stop-opacity=\"0.45\" />\n";
        svg += "      <stop offset=\"22%\" stop-color=\"#454855\" stop-opacity=\"0.15\" />\n";
        svg += "      <stop offset=\"45%\" stop-color=\"#151825\" stop-opacity=\"0.35\" />\n";
        svg += "      <stop offset=\"65%\" stop-color=\"#e8f0fe\" stop-opacity=\"0.50\" />\n";
        svg += "      <stop offset=\"85%\" stop-color=\"#303340\" stop-opacity=\"0.20\" />\n";
        svg += "      <stop offset=\"100%\" stop-color=\"#ffffff\" stop-opacity=\"0.40\" />\n";
        svg += "    </linearGradient>\n";
    } else if (style.material == MATERIAL_MATTE_CERAMIC) {
        svg += "    <linearGradient id=\"ceramic-grad\" x1=\"50%\" y1=\"0%\" x2=\"50%\" y2=\"100%\">\n";
        svg += "      <stop offset=\"0%\" stop-color=\"#ffffff\" stop-opacity=\"0.22\" />\n";
        svg += "      <stop offset=\"35%\" stop-color=\"#fdfbf7\" stop-opacity=\"0.08\" />\n";
        svg += "      <stop offset=\"70%\" stop-color=\"#e2e4e8\" stop-opacity=\"0.0\" />\n";
        svg += "      <stop offset=\"100%\" stop-color=\"#000000\" stop-opacity=\"0.08\" />\n";
        svg += "    </linearGradient>\n";
    } else if (style.material == MATERIAL_HOLOGRAPHIC) {
        svg += "    <linearGradient id=\"holo-grad\" x1=\"0%\" y1=\"0%\" x2=\"100%\" y2=\"100%\">\n";
        svg += "      <stop offset=\"0%\" stop-color=\"#06B6D4\" stop-opacity=\"0.28\" />\n";
        svg += "      <stop offset=\"25%\" stop-color=\"#EC4899\" stop-opacity=\"0.30\" />\n";
        svg += "      <stop offset=\"50%\" stop-color=\"#F59E0B\" stop-opacity=\"0.24\" />\n";
        svg += "      <stop offset=\"75%\" stop-color=\"#10B981\" stop-opacity=\"0.28\" />\n";
        svg += "      <stop offset=\"100%\" stop-color=\"#8B5CF6\" stop-opacity=\"0.30\" />\n";
        svg += "    </linearGradient>\n";
    }
    svg += "  </defs>\n\n";

    svg += "  <!-- Background Adaptive Plate -->\n";
    svg += "  <g filter=\"url(#modo-shadow)\">\n";
    if (style.shape == SHAPE_CIRCLE) {
        svg += "    <circle cx=\"256\" cy=\"256\" r=\"236\" fill=\"url(#bg-grad)\" />\n";
    } else if (style.shape == SHAPE_SQUIRCLE) {
        svg += "    <path d=\"M 256,20 C 370,20 492,142 492,256 C 492,370 370,492 256,492 C 142,492 20,370 20,256 C 20,142 142,20 256,20 Z\" fill=\"url(#bg-grad)\" />\n";
    } else if (style.shape == SHAPE_ROUNDED_RECT) {
        svg += "    <rect x=\"20\" y=\"20\" width=\"472\" height=\"472\" rx=\"92\" ry=\"92\" fill=\"url(#bg-grad)\" />\n";
    } else if (style.shape == SHAPE_HEXAGON) {
        svg += "    <polygon points=\"256,20 460,138 460,374 256,492 52,374 52,138\" fill=\"url(#bg-grad)\" />\n";
    } else { // SHAPE_PEBBLE
        svg += "    <path d=\"M 256,20 C 400,24 492,116 492,256 C 492,420 370,492 256,492 C 120,492 20,390 20,256 C 20,100 110,20 256,20 Z\" fill=\"url(#bg-grad)\" />\n";
    }

    if (style.material == MATERIAL_FROSTED_GLASS) {
        if (style.shape == SHAPE_CIRCLE) svg += "    <circle cx=\"256\" cy=\"256\" r=\"236\" fill=\"url(#glass-sweep)\" />\n";
        else if (style.shape == SHAPE_ROUNDED_RECT) svg += "    <rect x=\"20\" y=\"20\" width=\"472\" height=\"472\" rx=\"92\" ry=\"92\" fill=\"url(#glass-sweep)\" />\n";
        else svg += "    <path d=\"M 256,20 C 370,20 492,142 492,256 C 492,370 370,492 256,492 C 142,492 20,370 20,256 C 20,142 142,20 256,20 Z\" fill=\"url(#glass-sweep)\" />\n";
    } else if (style.material == MATERIAL_BRUSHED_TITANIUM) {
        if (style.shape == SHAPE_CIRCLE) svg += "    <circle cx=\"256\" cy=\"256\" r=\"236\" fill=\"url(#titanium-grad)\" />\n";
        else if (style.shape == SHAPE_ROUNDED_RECT) svg += "    <rect x=\"20\" y=\"20\" width=\"472\" height=\"472\" rx=\"92\" ry=\"92\" fill=\"url(#titanium-grad)\" />\n";
        else svg += "    <path d=\"M 256,20 C 370,20 492,142 492,256 C 492,370 370,492 256,492 C 142,492 20,370 20,256 C 20,142 142,20 256,20 Z\" fill=\"url(#titanium-grad)\" />\n";
    } else if (style.material == MATERIAL_NEUMORPHIC) {
        if (style.shape == SHAPE_CIRCLE) svg += "    <circle cx=\"256\" cy=\"256\" r=\"236\" fill=\"url(#neu-grad)\" />\n";
        else if (style.shape == SHAPE_ROUNDED_RECT) svg += "    <rect x=\"20\" y=\"20\" width=\"472\" height=\"472\" rx=\"92\" ry=\"92\" fill=\"url(#neu-grad)\" />\n";
        else svg += "    <path d=\"M 256,20 C 370,20 492,142 492,256 C 492,370 370,492 256,492 C 142,492 20,370 20,256 C 20,142 142,20 256,20 Z\" fill=\"url(#neu-grad)\" />\n";
    } else if (style.material == MATERIAL_PRISM_GLOW) {
        if (style.shape == SHAPE_CIRCLE) svg += "    <circle cx=\"256\" cy=\"256\" r=\"236\" fill=\"url(#prism-grad)\" />\n";
        else if (style.shape == SHAPE_ROUNDED_RECT) svg += "    <rect x=\"20\" y=\"20\" width=\"472\" height=\"472\" rx=\"92\" ry=\"92\" fill=\"url(#prism-grad)\" />\n";
        else svg += "    <path d=\"M 256,20 C 370,20 492,142 492,256 C 492,370 370,492 256,492 C 142,492 20,370 20,256 C 20,142 142,20 256,20 Z\" fill=\"url(#prism-grad)\" />\n";
    } else if (style.material == MATERIAL_LIQUID_CHROME) {
        if (style.shape == SHAPE_CIRCLE) svg += "    <circle cx=\"256\" cy=\"256\" r=\"236\" fill=\"url(#chrome-grad)\" />\n";
        else if (style.shape == SHAPE_ROUNDED_RECT) svg += "    <rect x=\"20\" y=\"20\" width=\"472\" height=\"472\" rx=\"92\" ry=\"92\" fill=\"url(#chrome-grad)\" />\n";
        else svg += "    <path d=\"M 256,20 C 370,20 492,142 492,256 C 492,370 370,492 256,492 C 142,492 20,370 20,256 C 20,142 142,20 256,20 Z\" fill=\"url(#chrome-grad)\" />\n";
    } else if (style.material == MATERIAL_MATTE_CERAMIC) {
        if (style.shape == SHAPE_CIRCLE) svg += "    <circle cx=\"256\" cy=\"256\" r=\"236\" fill=\"url(#ceramic-grad)\" />\n";
        else if (style.shape == SHAPE_ROUNDED_RECT) svg += "    <rect x=\"20\" y=\"20\" width=\"472\" height=\"472\" rx=\"92\" ry=\"92\" fill=\"url(#ceramic-grad)\" />\n";
        else svg += "    <path d=\"M 256,20 C 370,20 492,142 492,256 C 492,370 370,492 256,492 C 142,492 20,370 20,256 C 20,142 142,20 256,20 Z\" fill=\"url(#ceramic-grad)\" />\n";
    } else if (style.material == MATERIAL_HOLOGRAPHIC) {
        if (style.shape == SHAPE_CIRCLE) svg += "    <circle cx=\"256\" cy=\"256\" r=\"236\" fill=\"url(#holo-grad)\" />\n";
        else if (style.shape == SHAPE_ROUNDED_RECT) svg += "    <rect x=\"20\" y=\"20\" width=\"472\" height=\"472\" rx=\"92\" ry=\"92\" fill=\"url(#holo-grad)\" />\n";
        else svg += "    <path d=\"M 256,20 C 370,20 492,142 492,256 C 492,370 370,492 256,492 C 142,492 20,370 20,256 C 20,142 142,20 256,20 Z\" fill=\"url(#holo-grad)\" />\n";
    }
    svg += "  </g>\n\n";

    if (src) {
        std::string b64 = BitmapToBase64Png(src, pngClsid);
        if (!b64.empty()) {
            double iconW = 512.0 * style.scale;
            double iconH = 512.0 * style.scale;
            double iconX = 256.0 - iconW * 0.5 + (style.offsetX * 256.0);
            double iconY = 256.0 - iconH * 0.5 - (style.offsetY * 256.0);
            char imgTag[256];
            sprintf_s(imgTag, "  <!-- Centered Icon Artwork -->\n  <image x=\"%.1f\" y=\"%.1f\" width=\"%.1f\" height=\"%.1f\" href=\"data:image/png;base64,", iconX, iconY, iconW, iconH);
            svg += imgTag;
            svg += b64;
            svg += "\" />\n";
        }
    }

    svg += "</svg>\n";
    return WriteUtf8File(svgPath, svg);
}

Bitmap* RenderMonogramBitmap(const std::wstring& letters, Color primaryColor, Color secondaryColor, int size, int frameStyle) {
    Bitmap* bmp = new Bitmap(size, size, PixelFormat32bppARGB);
    Graphics g(bmp);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    g.Clear(Color(0, 0, 0, 0));

    RectF r(16.0f, 16.0f, (REAL)size - 32.0f, (REAL)size - 32.0f);

    switch (frameStyle) {
    case 1: { // Squircle Solid
        GraphicsPath path;
        AddShapePath(path, SHAPE_SQUIRCLE, r);
        SolidBrush fill(primaryColor);
        g.FillPath(&fill, &path);
        Pen outline(secondaryColor, 4.0f);
        g.DrawPath(&outline, &path);
        break;
    }
    case 2: { // Hexagon Frame
        GraphicsPath path;
        AddShapePath(path, SHAPE_HEXAGON, r);
        Pen pen(primaryColor, 8.0f);
        g.DrawPath(&pen, &path);
        break;
    }
    case 3: // Minimal (no frame)
        break;
    default: { // Circle Ring (case 0)
        Pen pen(primaryColor, 8.0f);
        g.DrawEllipse(&pen, r);
        SolidBrush dot(secondaryColor);
        g.FillEllipse(&dot, r.X + r.Width * 0.75f, r.Y + r.Height * 0.15f, 16.0f, 16.0f);
        break;
    }
    }

    // Text rendering
    std::wstring displayText = letters.empty() ? L"M" : letters.substr(0, 4);
    REAL fontSize = (displayText.length() == 1) ? ((REAL)size * 0.44f) :
                    (displayText.length() == 2) ? ((REAL)size * 0.34f) :
                    ((REAL)size * 0.24f);

    FontFamily ff(L"Segoe UI");
    Font font(&ff, fontSize, FontStyleBold, UnitPixel);

    StringFormat sf;
    sf.SetAlignment(StringAlignmentCenter);
    sf.SetLineAlignment(StringAlignmentCenter);

    Color textColor = (frameStyle == 1) ? Color(255, 255, 255, 255) : primaryColor;
    SolidBrush textBrush(textColor);
    RectF textRect(0.0f, 0.0f, (REAL)size, (REAL)size);
    g.DrawString(displayText.c_str(), (INT)displayText.length(), &font, textRect, &sf, &textBrush);

    return bmp;
}

Bitmap* RenderLucideVectorBitmap(const std::wstring& iconName, Color strokeColor, int size) {
    Bitmap* bmp = new Bitmap(size, size, PixelFormat32bppARGB);
    Graphics g(bmp);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.Clear(Color(0, 0, 0, 0));

    REAL s = (REAL)size / 24.0f;
    Pen pen(strokeColor, std::max(2.0f, s * 1.8f));
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    pen.SetLineJoin(LineJoinRound);

    std::wstring name = iconName;
    std::transform(name.begin(), name.end(), name.begin(), ::towlower);

    if (name == L"terminal" || name == L"cli" || name == L"console") {
        PointF p[3] = { {4.0f*s, 17.0f*s}, {10.0f*s, 11.0f*s}, {4.0f*s, 5.0f*s} };
        g.DrawLines(&pen, p, 3);
        g.DrawLine(&pen, 12.0f*s, 19.0f*s, 20.0f*s, 19.0f*s);
    } else if (name == L"code" || name == L"dev") {
        PointF p1[3] = { {16.0f*s, 18.0f*s}, {22.0f*s, 12.0f*s}, {16.0f*s, 6.0f*s} };
        PointF p2[3] = { {8.0f*s, 6.0f*s}, {2.0f*s, 12.0f*s}, {8.0f*s, 18.0f*s} };
        g.DrawLines(&pen, p1, 3);
        g.DrawLines(&pen, p2, 3);
    } else if (name == L"git" || name == L"git-branch") {
        g.DrawLine(&pen, 6.0f*s, 3.0f*s, 6.0f*s, 15.0f*s);
        g.DrawEllipse(&pen, 15.0f*s, 3.0f*s, 6.0f*s, 6.0f*s);
        g.DrawEllipse(&pen, 3.0f*s, 15.0f*s, 6.0f*s, 6.0f*s);
        g.DrawArc(&pen, 0.0f, 9.0f*s, 18.0f*s, 18.0f*s, 270.0f, 90.0f);
    } else if (name == L"database" || name == L"sql") {
        g.DrawEllipse(&pen, 3.0f*s, 2.0f*s, 18.0f*s, 6.0f*s);
        g.DrawLine(&pen, 3.0f*s, 5.0f*s, 3.0f*s, 19.0f*s);
        g.DrawLine(&pen, 21.0f*s, 5.0f*s, 21.0f*s, 19.0f*s);
        g.DrawArc(&pen, 3.0f*s, 9.0f*s, 18.0f*s, 6.0f*s, 0.0f, 180.0f);
        g.DrawArc(&pen, 3.0f*s, 16.0f*s, 18.0f*s, 6.0f*s, 0.0f, 180.0f);
    } else if (name == L"cpu" || name == L"chip") {
        g.DrawRectangle(&pen, 4.0f*s, 4.0f*s, 16.0f*s, 16.0f*s);
        g.DrawRectangle(&pen, 9.0f*s, 9.0f*s, 6.0f*s, 6.0f*s);
        g.DrawLine(&pen, 9.0f*s, 2.0f*s, 9.0f*s, 4.0f*s);
        g.DrawLine(&pen, 15.0f*s, 2.0f*s, 15.0f*s, 4.0f*s);
        g.DrawLine(&pen, 9.0f*s, 20.0f*s, 9.0f*s, 22.0f*s);
        g.DrawLine(&pen, 15.0f*s, 20.0f*s, 15.0f*s, 22.0f*s);
        g.DrawLine(&pen, 2.0f*s, 9.0f*s, 4.0f*s, 9.0f*s);
        g.DrawLine(&pen, 2.0f*s, 15.0f*s, 4.0f*s, 15.0f*s);
        g.DrawLine(&pen, 20.0f*s, 9.0f*s, 22.0f*s, 9.0f*s);
        g.DrawLine(&pen, 20.0f*s, 15.0f*s, 22.0f*s, 15.0f*s);
    } else if (name == L"shield" || name == L"security") {
        GraphicsPath sp;
        sp.AddLine(12.0f*s, 2.0f*s, 20.0f*s, 5.0f*s);
        sp.AddLine(20.0f*s, 5.0f*s, 20.0f*s, 13.0f*s);
        sp.AddBezier(20.0f*s, 13.0f*s, 17.0f*s, 18.0f*s, 14.0f*s, 21.0f*s, 12.0f*s, 22.0f*s);
        sp.AddBezier(12.0f*s, 22.0f*s, 10.0f*s, 21.0f*s, 7.0f*s, 18.0f*s, 4.0f*s, 13.0f*s);
        sp.AddLine(4.0f*s, 13.0f*s, 4.0f*s, 5.0f*s);
        sp.CloseFigure();
        g.DrawPath(&pen, &sp);
    } else if (name == L"music") {
        g.DrawLine(&pen, 9.0f*s, 18.0f*s, 9.0f*s, 5.0f*s);
        g.DrawLine(&pen, 9.0f*s, 5.0f*s, 21.0f*s, 3.0f*s);
        g.DrawLine(&pen, 21.0f*s, 3.0f*s, 21.0f*s, 16.0f*s);
        SolidBrush sb(strokeColor);
        g.FillEllipse(&sb, 3.0f*s, 15.0f*s, 6.0f*s, 6.0f*s);
        g.FillEllipse(&sb, 15.0f*s, 13.0f*s, 6.0f*s, 6.0f*s);
    } else if (name == L"camera") {
        g.DrawRectangle(&pen, 2.0f*s, 7.0f*s, 20.0f*s, 14.0f*s);
        g.DrawEllipse(&pen, 8.5f*s, 10.5f*s, 7.0f*s, 7.0f*s);
        g.DrawLine(&pen, 7.0f*s, 7.0f*s, 9.0f*s, 4.0f*s);
        g.DrawLine(&pen, 9.0f*s, 4.0f*s, 15.0f*s, 4.0f*s);
        g.DrawLine(&pen, 15.0f*s, 4.0f*s, 17.0f*s, 7.0f*s);
    } else if (name == L"video") {
        g.DrawRectangle(&pen, 2.0f*s, 6.0f*s, 14.0f*s, 12.0f*s);
        PointF lens[3] = { {16.0f*s, 12.0f*s}, {22.0f*s, 8.0f*s}, {22.0f*s, 16.0f*s} };
        g.DrawPolygon(&pen, lens, 3);
    } else if (name == L"mail") {
        g.DrawRectangle(&pen, 2.0f*s, 4.0f*s, 20.0f*s, 16.0f*s);
        PointF env[3] = { {2.0f*s, 7.0f*s}, {12.0f*s, 13.0f*s}, {22.0f*s, 7.0f*s} };
        g.DrawLines(&pen, env, 3);
    } else if (name == L"chat" || name == L"message") {
        GraphicsPath cp;
        cp.AddLine(21.0f*s, 15.0f*s, 21.0f*s, 3.0f*s);
        cp.AddLine(21.0f*s, 3.0f*s, 3.0f*s, 3.0f*s);
        cp.AddLine(3.0f*s, 3.0f*s, 3.0f*s, 17.0f*s);
        cp.AddLine(3.0f*s, 17.0f*s, 7.0f*s, 17.0f*s);
        cp.AddLine(7.0f*s, 17.0f*s, 3.0f*s, 21.0f*s);
        cp.AddLine(3.0f*s, 21.0f*s, 11.0f*s, 17.0f*s);
        cp.AddLine(11.0f*s, 17.0f*s, 19.0f*s, 17.0f*s);
        cp.CloseFigure();
        g.DrawPath(&pen, &cp);
    } else if (name == L"search") {
        g.DrawEllipse(&pen, 3.0f*s, 3.0f*s, 14.0f*s, 14.0f*s);
        g.DrawLine(&pen, 15.0f*s, 15.0f*s, 21.0f*s, 21.0f*s);
    } else if (name == L"rocket") {
        PointF rkp[5] = { {12.0f*s, 2.0f*s}, {18.0f*s, 9.0f*s}, {15.0f*s, 18.0f*s}, {9.0f*s, 18.0f*s}, {6.0f*s, 9.0f*s} };
        g.DrawPolygon(&pen, rkp, 5);
        g.DrawLine(&pen, 6.0f*s, 14.0f*s, 3.0f*s, 18.0f*s);
        g.DrawLine(&pen, 18.0f*s, 14.0f*s, 21.0f*s, 18.0f*s);
    } else if (name == L"zap" || name == L"bolt") {
        PointF zp[6] = { {13.0f*s, 2.0f*s}, {5.0f*s, 13.0f*s}, {11.0f*s, 13.0f*s}, {10.0f*s, 22.0f*s}, {19.0f*s, 10.0f*s}, {13.0f*s, 10.0f*s} };
        g.DrawPolygon(&pen, zp, 6);
    } else if (name == L"folder") {
        GraphicsPath fld;
        fld.AddLine(2.0f*s, 4.0f*s, 9.0f*s, 4.0f*s);
        fld.AddLine(9.0f*s, 4.0f*s, 11.0f*s, 7.0f*s);
        fld.AddLine(11.0f*s, 7.0f*s, 22.0f*s, 7.0f*s);
        fld.AddLine(22.0f*s, 7.0f*s, 22.0f*s, 20.0f*s);
        fld.AddLine(22.0f*s, 20.0f*s, 2.0f*s, 20.0f*s);
        fld.CloseFigure();
        g.DrawPath(&pen, &fld);
    } else if (name == L"globe" || name == L"web") {
        g.DrawEllipse(&pen, 2.0f*s, 2.0f*s, 20.0f*s, 20.0f*s);
        g.DrawLine(&pen, 2.0f*s, 12.0f*s, 22.0f*s, 12.0f*s);
        g.DrawEllipse(&pen, 6.5f*s, 2.0f*s, 11.0f*s, 20.0f*s);
    } else if (name == L"lock") {
        g.DrawRectangle(&pen, 3.0f*s, 11.0f*s, 18.0f*s, 11.0f*s);
        g.DrawArc(&pen, 7.0f*s, 3.0f*s, 10.0f*s, 16.0f*s, 180.0f, 180.0f);
    } else if (name == L"key") {
        g.DrawEllipse(&pen, 2.0f*s, 9.5f*s, 10.0f*s, 10.0f*s);
        g.DrawLine(&pen, 9.0f*s, 14.5f*s, 22.0f*s, 3.0f*s);
        g.DrawLine(&pen, 18.0f*s, 5.0f*s, 20.0f*s, 7.0f*s);
        g.DrawLine(&pen, 15.0f*s, 8.0f*s, 17.0f*s, 10.0f*s);
    } else { // sparkles / ai (default)
        PointF st[9] = {
            {12.0f*s, 3.0f*s}, {10.0f*s, 9.0f*s}, {3.0f*s, 12.0f*s}, {10.0f*s, 15.0f*s},
            {12.0f*s, 21.0f*s}, {14.0f*s, 15.0f*s}, {21.0f*s, 12.0f*s}, {14.0f*s, 9.0f*s},
            {12.0f*s, 3.0f*s}
        };
        g.DrawLines(&pen, st, 9);
        g.DrawLine(&pen, 5.0f*s, 3.0f*s, 5.0f*s, 7.0f*s);
        g.DrawLine(&pen, 3.0f*s, 5.0f*s, 7.0f*s, 5.0f*s);
        g.DrawLine(&pen, 19.0f*s, 17.0f*s, 19.0f*s, 21.0f*s);
        g.DrawLine(&pen, 17.0f*s, 19.0f*s, 21.0f*s, 19.0f*s);
    }

    return bmp;
}

} // namespace IconEngine
