#pragma once

#include <stdio.h>

#include <cstdint>
#include <cstddef>

#include <jpeglib.h>

class ScanLineCollector {
public:
    ~ScanLineCollector();

    void Consume(jpeg_decompress_struct &cinfo);

    inline uint8_t *ScanLine(int row) { return &bitmap_data[row * row_bytes]; }
    inline uint32_t Rows() { return height; }
    inline uint32_t RowBytes() { return row_bytes; }

private:
    int output_components;
    uint32_t width;
    uint32_t height;
    uint32_t row_bytes;

    uint8_t *bitmap_data = NULL;
};
