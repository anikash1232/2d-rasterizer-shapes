/**
 *  Copyright 2018 Mike Reed
 */

#include "../include/GBitmap.h"
#include "lodepng.h"

static void convertToPNG(const GPixel src[], int width, uint8_t dst[]) {
    for (int i = 0; i < width; i++) {
        GPixel c = *src++;
        int a = GPixel_GetA(c);
        int r = GPixel_GetR(c);
        int g = GPixel_GetG(c);
        int b = GPixel_GetB(c);
        
        // PNG requires unpremultiplied, but GPixel is premultiplied
        if (0 != a && 255 != a) {
            r = (r * 255 + a/2) / a;
            g = (g * 255 + a/2) / a;
            b = (b * 255 + a/2) / a;
        }
        *dst++ = r;
        *dst++ = g;
        *dst++ = b;
        *dst++ = a;
    }
}

static std::vector<uint8_t> unpremul_and_compact_with_malloc(const GBitmap& bm) {
    size_t rb = bm.width() * 4;
    std::vector<uint8_t> pix(bm.height() * rb);

    const GPixel* src = bm.pixels();
    uint8_t* dst = pix.data();
    for (int y = 0; y < bm.height(); ++y) {
        convertToPNG(src, bm.width(), dst);
        src += bm.rowBytes() / 4;
        dst += rb;
    }
    return pix;
}

bool GBitmap::writeToFile(const char path[]) const {
    auto pix = unpremul_and_compact_with_malloc(*this);
    return lodepng_encode32_file(path, pix.data(), this->width(), this->height()) == 0;
}

std::shared_ptr<GData> GBitmap::asPNGData() const {
    auto pix = unpremul_and_compact_with_malloc(*this);
    uint8_t* outBuffer = nullptr;
    size_t outSize = 0;
    int err = lodepng_encode32(&outBuffer, &outSize, pix.data(), this->width(), this->height());
    return err ? nullptr : GData::Copy(outBuffer, outSize);
}

///////////////////////////////////////////////////////////////////////////////

static int alpha_mul(unsigned a, unsigned c) {
    return (a * c + 127) / 255;
}

static void swizzle_rgba_row(GPixel dst[], const uint8_t src[], int count) {
    for (int i = 0; i < count; ++i) {
        unsigned a = src[3];
        dst[i] = GPixel_PackARGB(a,
                                 alpha_mul(a, src[0]),
                                 alpha_mul(a, src[1]),
                                 alpha_mul(a, src[2]));
        src += 4;
    }
}

std::optional<GBitmap> GBitmap::ReadFromFile(const char path[]) {
    unsigned w, h;
    unsigned char* pix = nullptr;
    if (lodepng_decode32_file(&pix, &w, &h, path)) {
        free(pix);
        return {};
    }

    GBitmap bm({w, h});

    GPixel* dst = bm.pixels();
    const uint8_t* src = pix;
    size_t rb = w * 4;
    for (unsigned y = 0; y < h; ++y) {
        swizzle_rgba_row(dst, src, w);
        src += rb;
        dst += bm.rowBytes() / 4;
    }
    free(pix);

    bm.computeIsOpaque();
    return bm;
}
