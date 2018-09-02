#pragma once

#include <string>
#include <vector>

#include <jpeglib.h>

const unsigned char kMpFormatIdentifier[] = { 'M', 'P', 'F', 0 };


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


//// Error-management code required by libjpeg.

struct JpegErrorManager {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

typedef struct JpegErrorManager *JpegErrorPtr;

//// (end of error management code)


class ImageData {
public:
    ImageData(const std::string &filepath);
    ~ImageData();

    void Open();
    void ParseAll();

private:
    ScanLineCollector *ParseNextMpoChildImage(jpeg_decompress_struct &cinfo);
    void BuildLrImage(std::vector<ScanLineCollector *> &images);
    bool IsMpo(const jpeg_decompress_struct &cinfo);
    void DrainImage(jpeg_decompress_struct &cinfo);
    bool HasImage(jpeg_decompress_struct &cinfo);

    std::string filepath;
    unsigned char *image_data = NULL;
    int image_length;
};
