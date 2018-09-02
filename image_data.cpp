#include "image_data.h"

ImageData::ImageData(const std::string &filepath) : filepath(filepath) {
}

ImageData::~ImageData() {
    if (image_data != NULL) {
        delete image_data;
    }
}

void ImageData::Open() {
    std::ifstream image_file(filepath.c_str(), std::ios::in | std::ios::binary);
    if (image_file.is_open() == false) {
        throw new JpegLoadError("Could not open image.");
    }

    image_file.seekg(0, image_file.end);

    image_length = image_file.tellg();
    if (image_length == -1) {
        throw new JpegLoadError("Could not read file size.");
    }

    image_file.seekg(0, image_file.beg);

    image_data = new unsigned char[10000000];
    image_file.read(reinterpret_cast<char *>(image_data), image_length);

    image_file.close();
}


//// This is obligatory error-management code that will cause a segfault, even
//// if not present and even if there's no error.

METHODDEF(void) JpegErrorHandler(j_common_ptr cinfo)
{
    std::cout << "Error!" << std::endl;

    /* cinfo->err really points to a JpegErrorManager struct, so coerce pointer */
    JpegErrorPtr jse = (JpegErrorPtr)cinfo->err;

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

    // std::cout << std::endl;
    // std::cout << "Markers:" << std::endl;
    // std::cout << std::endl;

    jpeg_saved_marker_ptr current = cinfo.marker_list;
    while(current != NULL) {

        // std::cout
        //     << "MARKER=" << std::hex << int(current->marker) << " "
        //     << "OLEN=" << current->original_length << " "
        //     << "DLEN=" << current->data_length
        //     << std::endl;

        if (current->marker == JPEG_APP0 + 2) {
            // MPO-specific information.
            // std::cout << "> Identified APP2 data." << std::endl;

            char format_identifier[5];

            void *copyDest = memcpy(format_identifier, current->data, 4);
            if (copyDest == NULL) {
                throw new JpegLoadError("Could not read format identifier.");
            }

            format_identifier[4] = 0;
            // std::cout << "> FORMAT IDENTIFIER: [" << format_identifier << "]" << std::endl;

            if (memcmp(format_identifier, kMpFormatIdentifier, 4) == 0) {
                // std::cout << "> MPO! MPO!" << std::endl;
                return true;
            }
        }

        current = current->next;
    }

    // std::cout << std::endl;
    // std::cout << "(end of markers)" << std::endl;

    return false;
}


// Fast-forward past the current image's data.
void ImageData::SkipImageData(jpeg_decompress_struct &cinfo) {
    jpeg_start_decompress(&cinfo);

    uint32_t stride = cinfo.output_width * cinfo.output_components;
    uint8_t *line_buffer = new uint8_t[stride];

    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, &line_buffer, 1);
    }

    delete line_buffer;

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

    // Check if an SOI marker is at the front of the stream.
    if (byte1 != 0xff || byte2 != 0xd8) {
        return false;
    }

    return true;
}

// Parse the next JPEG image in the current stream. Return NULL if there's an
// image but not an MPO (should never happen; in this case, consume but ignore
// the data).
ScanlineCollector *ImageData::ParseNextMpoChildImage(jpeg_decompress_struct &cinfo) {
    int header_status = jpeg_read_header(&cinfo, TRUE);
    if (header_status != JPEG_HEADER_OK) {
        std::stringstream ss;
        ss << "Header read failed: (" << header_status << ")";

        throw new JpegLoadError(ss.str());
    }

    bool is_mpo = IsMpo(cinfo);

    if (is_mpo == false) {
        SkipImageData(cinfo);
        return NULL;
    }

    ScanlineCollector *slc = new ScanlineCollector();
    slc->Consume(cinfo);

    return slc;
}

bool ImageData::BuildLrImage(const ScanlineCollector *slc_left, const ScanlineCollector *slc_right) {
    // Make sure that our two images are exactly the same dimensions.
    if (slc_left->Components() != slc_right->Components()) {
        return false;
    } else if (slc_left->Width() != slc_right->Width()) {
        return false;
    } else if (slc_left->Height() != slc_right->Height()) {
        return false;
    }

// TODO(dustin): !! Do we want to support any other colorspaces?
    if (slc_left->Colorspace() != JCS_RGB || slc_right->Colorspace() != JCS_RGB) {
        return false;
    }

    std::cout << "Ready to build single LR image." << std::endl;

// TODO(dustin): !! Finish. Return actual image data.

    return true;
}

std::vector<ScanlineCollector *> *ImageData::ExtractMpoImages() {
    jpeg_decompress_struct cinfo;


    // Configure an error handler, or the program will segfault as soon as it
    // tries to read the header. This has memory-management complexities if we
    // put it somewhere else.

    struct JpegErrorManager jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = JpegErrorHandler;

    /* Establish the setjmp return context for JpegErrorHandler to use. */
    if (setjmp(jerr.setjmp_buffer)) {
        /* If we get here, the JPEG code has signaled an error.
         * We need to clean up the JPEG object, close the input file, and return.
         */

        throw new JpegLoadError("Could not set error handler.");
    }


    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, image_data, image_length);

    // JPEG uses APP1. JPEG MPO uses APP2, also.
    jpeg_save_markers(&cinfo, JPEG_APP0 + 1, 0xffff);
    jpeg_save_markers(&cinfo, JPEG_APP0 + 2, 0xffff);


    // Read all images from a single stream. Normal JPEGs will have one. The
    // MPOs we're interested in will have more than one.
    std::vector<ScanlineCollector *> *images = new std::vector<ScanlineCollector *>();
    while(true) {
        if (HasImage(cinfo) == false) {
            break;
        }

        ScanlineCollector *slc = ParseNextMpoChildImage(cinfo);
        if (slc == NULL) {
            break;
        }

        images->push_back(slc);
    }

    jpeg_destroy_decompress(&cinfo);

    return images;
}
