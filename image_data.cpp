#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <sstream>

#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#include <jpeglib.h>

#include "image_data.h"
#include "exceptions.h"

ImageData::ImageData(const std::string &filepath) : filepath(filepath) {
}

ImageData::~ImageData() {
    if (imageData != NULL) {
        delete imageData;
    }
}

void ImageData::Open() {
    std::ifstream imageFile(filepath.c_str(), std::ios::in | std::ios::binary);
    if (imageFile.is_open() == false) {
        throw new JpegLoadError("Could not open image.");
    }

    imageFile.seekg(0, imageFile.end);

    imageLength = imageFile.tellg();
    if (imageLength == -1) {
        throw new JpegLoadError("Could not read file size.");
    }

    imageFile.seekg(0, imageFile.beg);

    imageData = new unsigned char[10000000];
    imageFile.read(reinterpret_cast<char *>(imageData), imageLength);

    imageFile.close();
}


//// This is obligatory error-management code that will cause a segfault, even
//// if not present and even if there's no error.

METHODDEF(void) jpegErrorHandler(j_common_ptr cinfo)
{
    /* cinfo->err really points to a jpegErrorManager struct, so coerce pointer */
    jpegErrorPtr jse = (jpegErrorPtr)cinfo->err;

    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    (*cinfo->err->output_message) (cinfo);

    /* Return control to the setjmp point */
    longjmp(jse->setjmp_buffer, 1);
}

//// (end of error management)


bool ImageData::IsMpo(const jpeg_decompress_struct &cinfo) {
    // Print marker list.

    // struct jpeg_marker_struct {
    //     jpeg_saved_marker_ptr next;   /* next in list, or NULL */
    //     UINT8 marker;                 /* marker code: JPEG_COM, or JPEG_APP0+n */
    //     unsigned int original_length;  # bytes of data in the file
    //     unsigned int data_length;     /* # bytes of data saved at data[] */
    //     JOCTET * data;                /* the data contained in the marker */
    //     /* the marker length word is not counted in data_length or original_length */
    // };

    std::cout << std::endl;
    std::cout << "Markers:" << std::endl;
    std::cout << std::endl;

    jpeg_saved_marker_ptr current = cinfo.marker_list;
    while(current != NULL) {

        std::cout
            << "MARKER=" << std::hex << int(current->marker) << " "
            << "OLEN=" << current->original_length << " "
            << "DLEN=" << current->data_length
            << std::endl;

        if (current->marker == JPEG_APP0 + 2) {
            // MPO-specific information.
            std::cout << "> Identified APP2 data." << std::endl;

            char formatIdentifier[5];

            void *copyDest = memcpy(formatIdentifier, current->data, 4);
            if (copyDest == NULL) {
                throw new JpegLoadError("Could not read format identifier.");
            }

            formatIdentifier[4] = 0;
            std::cout << "> FORMAT IDENTIFIER: [" << formatIdentifier << "]" << std::endl;

            if (memcmp(formatIdentifier, mpFormatIdentifier, 4) == 0) {
                std::cout << "> MPO! MPO!" << std::endl;
                return true;
            }
        }

        current = current->next;
    }

    std::cout << std::endl;
    std::cout << "(end of markers)" << std::endl;

    return false;
}

bool ImageData::ParseOneImage(jpeg_decompress_struct &cinfo) {

    // Parse header of next image in stream.

    int headerStatus = jpeg_read_header(&cinfo, TRUE);
    if (headerStatus != JPEG_HEADER_OK) {
        std::stringstream ss;
        ss << "Header read failed: (" << headerStatus << ")";

        throw new JpegLoadError(ss.str());
    }

    bool isMpo = IsMpo(cinfo);

    jpeg_start_decompress(&cinfo);


// NOTE(dustin): The library requires you to read the scanlines.
    uint32_t rowBytes = cinfo.output_width * cinfo.output_components;
    //format_ = format;
    // uint32_t width_ = cinfo.output_width;
    // uint32_t height_ = cinfo.output_height;

    uint8_t *bitmapData = new uint8_t[rowBytes * cinfo.output_height];

    while (cinfo.output_scanline < cinfo.output_height) {
        uint8_t *lineBuffer = bitmapData + cinfo.output_scanline * rowBytes;
        jpeg_read_scanlines(&cinfo, &lineBuffer, 1);
    }

// NOTE(dustin): We don't actually care about the bitmap just yet for what we're doing.
    delete bitmapData;


    jpeg_finish_decompress(&cinfo);

    if (isMpo == false) {
        std::cout << "Skipping non-MPO image." << std::endl;
        return false;
    }

// TODO(dustin): !! Finish.

    return true;
}

void ImageData::ParseAll() {
    jpeg_decompress_struct cinfo;


    // Configure an error handler, or the program will segfault as soon as it
    // tries to read the header. This has memory-management complexities if we
    // put it somewhere else.

    struct jpegErrorManager jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpegErrorHandler;

    /* Establish the setjmp return context for jpegErrorHandler to use. */
    if (setjmp(jerr.setjmp_buffer)) {
        /* If we get here, the JPEG code has signaled an error.
         * We need to clean up the JPEG object, close the input file, and return.
         */
        jpeg_destroy_decompress(&cinfo);

        throw new JpegLoadError("Could not set error handler.");
    }


    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, imageData, imageLength);

    // JPEG uses APP1. JPEG MPO uses APP2, also.
    jpeg_save_markers(&cinfo, JPEG_APP0 + 1, 0xffff);
    jpeg_save_markers(&cinfo, JPEG_APP0 + 2, 0xffff);

    bool isMpo;

    // Read two inline images.

    isMpo = ParseOneImage(cinfo);
    if (isMpo == false) {
        std::cout << "First image is not an MPO." << std::endl;
    }

    isMpo = ParseOneImage(cinfo);
    if (isMpo == false) {
        std::cout << "Second image is not an MPO." << std::endl;
    }

    jpeg_destroy_decompress(&cinfo);
}
