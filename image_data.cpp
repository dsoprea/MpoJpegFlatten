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
    std::cout << "Error!" << std::endl;

    /* cinfo->err really points to a jpegErrorManager struct, so coerce pointer */
    jpegErrorPtr jse = (jpegErrorPtr)cinfo->err;

    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    (*cinfo->err->output_message) (cinfo);

    // char error_message[JMSG_LENGTH_MAX];
    // cinfo->err->format_message(cinfo, error_message);

    // std::cout << "Message: [" << error_message << "]" << std::endl;

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


ScanLineCollector::~ScanLineCollector() {
    if (bitmapData != NULL) {
        delete bitmapData;
    }
}

// Store the current image.
void ScanLineCollector::Consume(jpeg_decompress_struct &cinfo) {
    jpeg_start_decompress(&cinfo);

    output_components = cinfo.output_components;
    width = cinfo.output_width;
    height = cinfo.output_height;

    rowBytes = width * output_components;
    //format_ = format;

    bitmapData = new uint8_t[rowBytes * height];

    while (cinfo.output_scanline < cinfo.output_height) {
        uint8_t *lineBuffer = bitmapData + cinfo.output_scanline * rowBytes;
        jpeg_read_scanlines(&cinfo, &lineBuffer, 1);
    }

    jpeg_finish_decompress(&cinfo);
}


// Fast-forward past the current image.
void ImageData::DrainImage(jpeg_decompress_struct &cinfo) {
    jpeg_start_decompress(&cinfo);

    uint32_t rowBytes = cinfo.output_width * cinfo.output_components;
    uint8_t *lineBuffer = new uint8_t[rowBytes];

    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, &lineBuffer, 1);
    }

    delete lineBuffer;

    jpeg_finish_decompress(&cinfo);
}

// Check for another image in the stream.
//
// We'll using a derivation of what the internal logic does in order to check
// for the beginning of another JPEG image but without consuming bytes from it.
bool ImageData::HasImage(jpeg_decompress_struct &cinfo) {

    // Make sure we have at least two bytes in the buffer. Since we used
    // jpeg_mem_src() with an existing buffer, we have an guarantee from the
    // libjpeg internals that we already had all data, upfront, and, when we
    // don't have enough data, there's truly no more data available. See
    // fill_mem_input_buffer() for more information.
    if (cinfo.src->bytes_in_buffer < 2) {
        return false;
    }

    int byte1 = GETJOCTET(*(cinfo.src->next_input_byte + 0));
    int byte2 = GETJOCTET(*(cinfo.src->next_input_byte + 1));

    if (byte1 != 0xff || byte2 != 0xd8) {
        return false;
    }

    return true;
}

// Parse the next JPEG image in the current stream. Return NULL if there's an
// image but not an MPO (should never happen; in this case, consume but ignore
// the data).
ScanLineCollector *ImageData::ParseNextMpoChildImage(jpeg_decompress_struct &cinfo) {
    int headerStatus = jpeg_read_header(&cinfo, TRUE);
    if (headerStatus != JPEG_HEADER_OK) {
        std::stringstream ss;
        ss << "Header read failed: (" << headerStatus << ")";

        throw new JpegLoadError(ss.str());
    }

    bool isMpo = IsMpo(cinfo);

    if (isMpo == false) {
        DrainImage(cinfo);
        return NULL;
    }

    ScanLineCollector *slc = new ScanLineCollector();
    slc->Consume(cinfo);

    return slc;
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

        throw new JpegLoadError("Could not set error handler.");
    }


    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, imageData, imageLength);

    // JPEG uses APP1. JPEG MPO uses APP2, also.
    jpeg_save_markers(&cinfo, JPEG_APP0 + 1, 0xffff);
    jpeg_save_markers(&cinfo, JPEG_APP0 + 2, 0xffff);


    // Read all images from a single stream. Normal JPEGs will have one. The
    // MPOs we're interested in will have more than one.
    std::vector<ScanLineCollector *> images;
    while(true) {
        if (HasImage(cinfo) == false) {
            break;
        }

        ScanLineCollector *slc = ParseNextMpoChildImage(cinfo);
        if (slc == NULL) {
            break;
        }

        images.push_back(slc);
    }

    for (std::vector<ScanLineCollector *>::iterator it = images.begin() ; it != images.end(); ++it) {
        // std::cout << ' ' << (*it)->Rows() << std::endl;
        delete *it;
    }

    jpeg_destroy_decompress(&cinfo);
}
