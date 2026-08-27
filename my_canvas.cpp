#include "include/GCanvas.h"
#include "include/GBitmap.h"
#include "include/GColor.h"
#include "include/GMath.h"
#include "include/GRect.h"

#include <algorithm>
#include <cmath>
#include <memory>

static unsigned to_byte(float value) {
    return static_cast<unsigned>(GRoundToInt(std::max(0.0f, std::min(1.0f, value)) * 255));
}

static GPixel color_to_pixel(const GColor& color, float coverage = 1) {
    const float alpha = std::max(0.0f, std::min(1.0f, color.a * coverage));
    const unsigned a = to_byte(alpha);
    return GPixel_PackARGB(a, to_byte(color.r * alpha), to_byte(color.g * alpha),
                           to_byte(color.b * alpha));
}

static GPixel over(GPixel source, GPixel destination) {
    const unsigned sourceA = GPixel_GetA(source);
    const unsigned inverseA = 255 - sourceA;
    const unsigned alpha = sourceA + (GPixel_GetA(destination) * inverseA + 127) / 255;
    const unsigned red = GPixel_GetR(source) + (GPixel_GetR(destination) * inverseA + 127) / 255;
    const unsigned green = GPixel_GetG(source) + (GPixel_GetG(destination) * inverseA + 127) / 255;
    const unsigned blue = GPixel_GetB(source) + (GPixel_GetB(destination) * inverseA + 127) / 255;
    return GPixel_PackARGB(std::min(255u, alpha), std::min(255u, red),
                           std::min(255u, green), std::min(255u, blue));
}

class BitmapCanvas final : public GCanvas {
public:
    explicit BitmapCanvas(const GBitmap& bitmap) : fBitmap(bitmap) {}

    void clear(const GColor& color) override {
        if (fBitmap.width() == 0) return;
        const GPixel pixel = color_to_pixel(color);
        for (int y = 0; y < fBitmap.height(); ++y) {
            GPixel* row = fBitmap.getAddr(0, y);
            std::fill(row, row + fBitmap.width(), pixel);
        }
    }

    void fillRect(const GRect& rect, const GColor& color) override {
        const int left = std::max(0, GCeilToInt(rect.left - 0.5f));
        const int top = std::max(0, GCeilToInt(rect.top - 0.5f));
        const int right = std::min(fBitmap.width(), GCeilToInt(rect.right - 0.5f));
        const int bottom = std::min(fBitmap.height(), GCeilToInt(rect.bottom - 0.5f));
        if (left >= right || top >= bottom) return;
        const GPixel source = color_to_pixel(color);
        for (int y = top; y < bottom; ++y) {
            for (int x = left; x < right; ++x) {
                *fBitmap.getAddr(x, y) = over(source, *fBitmap.getAddr(x, y));
            }
        }
    }

    void hairLine(GPoint p0, GPoint p1, const GColor& color) override {
        if (!clip(&p0, &p1) || (p0.x == p1.x && p0.y == p1.y)) return;
        const bool axisAligned = p0.x == p1.x || p0.y == p1.y;
        p0.x -= 0.5f; p0.y -= 0.5f;
        p1.x -= 0.5f; p1.y -= 0.5f;
        bool steep = std::fabs(p1.y - p0.y) > std::fabs(p1.x - p0.x);
        if (steep) { std::swap(p0.x, p0.y); std::swap(p1.x, p1.y); }
        if (p0.x > p1.x) std::swap(p0, p1);
        const float dx = p1.x - p0.x;
        const float gradient = dx == 0 ? 1 : (p1.y - p0.y) / dx;
        const int xStart = GCeilToInt(p0.x);
        const int xEnd = GFloorToInt(p1.x);
        float y = p0.y + (xStart - p0.x) * gradient;
        for (int x = xStart; x <= xEnd; ++x, y += gradient) {
            const int baseY = GFloorToInt(y);
            const float fraction = y - baseY;
            plot(steep, x, baseY, 1 - fraction, color, axisAligned);
            plot(steep, x, baseY + 1, fraction, color, axisAligned);
        }
    }

private:
    bool clip(GPoint* p0, GPoint* p1) const {
        const float dx = p1->x - p0->x, dy = p1->y - p0->y;
        float t0 = 0, t1 = 1;
        auto test = [&](float p, float q) {
            if (p == 0) return q >= 0;
            const float r = q / p;
            if (p < 0) { if (r > t1) return false; t0 = std::max(t0, r); }
            else { if (r < t0) return false; t1 = std::min(t1, r); }
            return true;
        };
        if (!test(-dx, p0->x) || !test(dx, fBitmap.width() - p0->x) ||
            !test(-dy, p0->y) || !test(dy, fBitmap.height() - p0->y) || t0 > t1) return false;
        GPoint delta = *p1 - *p0;
        *p0 += delta * t0;
        *p1 = *p0 + delta * (t1 - t0);
        return true;
    }

    void plot(bool steep, int x, int y, float coverage, const GColor& color, bool axisAligned) {
        if (coverage <= 0 || (axisAligned && coverage < 0.999999f)) return;
        if (steep) std::swap(x, y);
        if (x < 0 || x >= fBitmap.width() || y < 0 || y >= fBitmap.height()) return;
        GPixel* pixel = fBitmap.getAddr(x, y);
        *pixel = over(color_to_pixel(color, coverage), *pixel);
    }

    GBitmap fBitmap;
};

std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& bitmap) {
    return std::make_unique<BitmapCanvas>(bitmap);
}

std::string GDrawSomething(GCanvas* canvas, GISize dimension) {
    canvas->clear({0.035f, 0.05f, 0.08f, 1});
    const float side = std::min(dimension.width, dimension.height) * 0.72f;
    const GPoint center = {dimension.width * 0.5f, dimension.height * 0.5f};
    const float left = center.x - side * 0.5f, top = center.y - side * 0.5f;
    const GColor colors[] = {{0.95f, 0.22f, 0.18f, 1}, {0.98f, 0.72f, 0.16f, 1},
                             {0.15f, 0.72f, 0.62f, 1}, {0.28f, 0.42f, 0.9f, 1}};
    const float cell = side / 4;
    for (int row = 0; row < 4; ++row) for (int column = 0; column < 4; ++column) {
        if ((row + column) % 3 != 1) {
            canvas->fillRect(GRect::XYWH(left + column * cell + 2, top + row * cell + 2,
                                         cell - 4, cell - 4), colors[(row + 2 * column) % 4]);
        }
    }
    canvas->hairLine({left, top + side}, {center.x, top}, GColor_white);
    canvas->hairLine({center.x, top}, {left + side, top + side}, GColor_white);
    return "Chromatic Crossings";
}