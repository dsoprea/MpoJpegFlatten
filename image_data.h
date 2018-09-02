#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <sstream>
#include <vector>

#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#include <jpeglib.h>
#include <jerror.h>

#include "exceptions.h"
#include "scanline_collector.h"

const unsigned char kMpFormatIdentifier[] = { 'M', 'P', 'F', 0 };


//// Error-management code required by libjpeg.

struct JpegErrorManager {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

typedef struct JpegErrorManager *JpegErrorPtr;

//// (end of error management code)


typedef struct {
    std::string error_message;
    uint8_t *data;

    uint32_t width;
    uint32_t height;

    uint32_t length;
} LrImage;


class ImageData {
public:
    ImageData(const std::string &filepath);
    ~ImageData();

    void Open();
    std::vector<ScanlineCollector *> *ExtractMpoImages();
    LrImage BuildLrImage(const ScanlineCollector *slc_left, const ScanlineCollector *slc_right);

private:
    ScanlineCollector *ParseNextMpoChildImage(jpeg_decompress_struct &cinfo);
    bool IsMpo(const jpeg_decompress_struct &cinfo);
    void SkipImageData(jpeg_decompress_struct &cinfo);
    bool HasImage(jpeg_decompress_struct &cinfo);

    std::string filepath;
    unsigned char *image_data = NULL;
    int image_length;
};
