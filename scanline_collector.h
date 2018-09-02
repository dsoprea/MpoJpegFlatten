#pragma once

#include <stdio.h>

#include <cstdint>
#include <cstddef>
#include <map>

#include <jpeglib.h>

const std::map<J_COLOR_SPACE, std::string> kJpegColorspaceNames = {
    { JCS_UNKNOWN, "JCS_UNKNOWN" },
    { JCS_GRAYSCALE, "JCS_GRAYSCALE" },
    { JCS_RGB, "JCS_RGB" },
    { JCS_YCbCr, "JCS_YCbCr" },
    { JCS_CMYK, "JCS_CMYK" },
    { JCS_YCCK, "JCS_YCCK" },
    { JCS_EXT_RGB, "JCS_EXT_RGB" },
    { JCS_EXT_RGBX, "JCS_EXT_RGBX" },
    { JCS_EXT_BGR, "JCS_EXT_BGR" },
    { JCS_EXT_BGRX, "JCS_EXT_BGRX" },
    { JCS_EXT_XBGR, "JCS_EXT_XBGR" },
    { JCS_EXT_XRGB, "JCS_EXT_XRGB" },
    { JCS_EXT_RGBA, "JCS_EXT_RGBA" },
    { JCS_EXT_BGRA, "JCS_EXT_BGRA" },
    { JCS_EXT_ABGR, "JCS_EXT_ABGR" },
    { JCS_EXT_ARGB, "JCS_EXT_ARGB" },
    { JCS_RGB565, "JCS_RGB565" }
};


class ScanlineCollector {
public:
    ~ScanlineCollector();

    void Consume(jpeg_decompress_struct &cinfo);

    inline uint8_t *ScanLine(int row) const { return &bitmap_data[row * stride]; }

    inline int Components() const { return components; }
    inline uint32_t Width() const { return width; }
    inline uint32_t Height() const { return height; }

    inline uint32_t Stride() const { return stride; }
    inline J_COLOR_SPACE Colorspace() const { return colorspace; }
    inline std::string ColorspaceName() const {
        // Use at() because [] will violate the const by automatically
        // generating a new entry for a non-existent key.
        return kJpegColorspaceNames.at(colorspace);
    }

private:
    int components;
    uint32_t width;
    uint32_t height;

    uint32_t stride;

    uint8_t *bitmap_data = NULL;

    J_COLOR_SPACE colorspace;
};
