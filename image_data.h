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
    bool ParseOneImage(jpeg_decompress_struct &cinfo);
    bool IsMpo(const jpeg_decompress_struct &cinfo);
    void ConfigureErrorHandler(jpeg_decompress_struct &cinfo);

private:
    std::string filepath;
    unsigned char *imageData = NULL;
    int imageLength;
};
