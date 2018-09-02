#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <sstream>

#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#include <jpeglib.h>

const unsigned char mpFormatIdentifier[] = { 'M', 'P', 'F', 0 };


class ScanLineCollector {
public:
    ~ScanLineCollector();

    void Consume(jpeg_decompress_struct &cinfo);

    inline uint8_t *ScanLine(int row) { return &bitmapData[row * rowBytes]; }
    inline uint32_t Rows() { return height; }
    inline uint32_t RowBytes() { return rowBytes; }

private:
    int output_components;
    uint32_t width;
    uint32_t height;
    uint32_t rowBytes;

    uint8_t *bitmapData = NULL;
};


struct jpegErrorManager {
  struct jpeg_error_mgr pub;    /* "public" fields */

  jmp_buf setjmp_buffer;    /* for return to caller */
};

typedef struct jpegErrorManager * jpegErrorPtr;


class ImageData {
public:
    ImageData(const std::string &filepath);
    ~ImageData();

    void Open();
    void ParseAll();

private:
    ScanLineCollector *ParseNextMpoChildImage(jpeg_decompress_struct &cinfo);
    bool IsMpo(const jpeg_decompress_struct &cinfo);
    void DrainImage(jpeg_decompress_struct &cinfo);
    bool HasImage(jpeg_decompress_struct &cinfo);

    std::string filepath;
    unsigned char *imageData = NULL;
    int imageLength;
};
