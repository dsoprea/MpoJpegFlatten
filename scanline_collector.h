#pragma once

#include <stdio.h>

#include <cstdint>
#include <cstddef>

#include <jpeglib.h>

class ScanlineCollector {
public:
    ~ScanlineCollector();

    void Consume(jpeg_decompress_struct &cinfo);

    inline uint8_t *ScanLine(int row) const { return &bitmap_data[row * row_bytes]; }

    inline int Components() const { return components; }
    inline uint32_t Width() const { return width; }
    inline uint32_t Height() const { return height; }

    inline uint32_t RowBytes() const { return row_bytes; }

private:
    int components;
    uint32_t width;
    uint32_t height;

    uint32_t row_bytes;
    uint8_t *bitmap_data = NULL;
};
