#include "scanline_collector.h"

ScanLineCollector::~ScanLineCollector() {
    if (bitmap_data != NULL) {
        delete bitmap_data;
    }
}

// Store the current image.
void ScanLineCollector::Consume(jpeg_decompress_struct &cinfo) {
    jpeg_start_decompress(&cinfo);

    output_components = cinfo.output_components;
    width = cinfo.output_width;
    height = cinfo.output_height;
    row_bytes = width * output_components;

    bitmap_data = new uint8_t[row_bytes * height];

    while (cinfo.output_scanline < cinfo.output_height) {
        uint8_t *line_buffer = bitmap_data + cinfo.output_scanline * row_bytes;
        jpeg_read_scanlines(&cinfo, &line_buffer, 1);
    }

    jpeg_finish_decompress(&cinfo);
}