#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <sstream>

#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#include <jpeglib.h>


class JpegLoadError : public std::exception {
private:
    std::string message;

public:
    JpegLoadError(std::string message) : message(message) {

    }

    const char* what() const throw()
    {
        return message.c_str();
    }
};


class ImageData {
public:
    ImageData(const std::string &filepath);
    ~ImageData();

    void Open();
    void ParseAll();
    void ParseOneImage(jpeg_decompress_struct &cinfo);

private:
    std::string filepath;
    unsigned char *imageData = NULL;
    unsigned long imageLength;
};

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

struct my_error_mgr {
  struct jpeg_error_mgr pub;    /* "public" fields */

  jmp_buf setjmp_buffer;    /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
    std::cout << "Error exit." << std::endl;

    /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
    my_error_ptr myerr = (my_error_ptr) cinfo->err;

    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    (*cinfo->err->output_message) (cinfo);

    /* Return control to the setjmp point */
    longjmp(myerr->setjmp_buffer, 1);
}

const unsigned char mpFormatIdentifier[] = { 'M', 'P', 'F', 0 };

void ImageData::ParseOneImage(jpeg_decompress_struct &cinfo) {
    int headerStatus = jpeg_read_header(&cinfo, TRUE);
    if (headerStatus != JPEG_HEADER_OK) {
        // fclose(imageFd);

        std::stringstream ss;
        ss << "Header read failed: (" << headerStatus << ")";

        throw new JpegLoadError(ss.str());
    }



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
            }
        }

        current = current->next;
    }

    std::cout << std::endl;
    std::cout << "(end of markers)" << std::endl;




    jpeg_start_decompress(&cinfo);


// NOTE(dustin): The library requires you to read the scanlines.
    uint32_t rowBytes = cinfo.output_width * cinfo.output_components;
    //format_ = format;
    uint32_t width_ = cinfo.output_width;
    uint32_t height_ = cinfo.output_height;

    uint8_t *bitmapData = new uint8_t[rowBytes * cinfo.output_height];

    while (cinfo.output_scanline < cinfo.output_height) {
        uint8_t *lineBuffer = bitmapData + cinfo.output_scanline * rowBytes;
        jpeg_read_scanlines(&cinfo, &lineBuffer, 1);
    }

// NOTE(dustin): We don't actually care about the bitmap just yet for what we're doing.
    delete bitmapData;


    jpeg_finish_decompress(&cinfo);
}

void ImageData::ParseAll() {
    // std::cout << "Length: ";
    // std::cout << sizeof(imageData) << std::endl;

    FILE *imageFd;

// TODO(dustin): !! Return to trying to work with memory.
    // if ((imageFd = fopen(filepath.c_str(), "rb")) == NULL) {
    //     // fprintf(stderr, "can't open %s\n", filename);
    //     throw new JpegLoadError("Couldn't open file.");
    // }



    jpeg_decompress_struct cinfo;


// NOTE(dustin): Totally necessary or the program will literally segfault.
    struct my_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    /* Establish the setjmp return context for my_error_exit to use. */
    if (setjmp(jerr.setjmp_buffer)) {
        /* If we get here, the JPEG code has signaled an error.
         * We need to clean up the JPEG object, close the input file, and return.
         */
        jpeg_destroy_decompress(&cinfo);

        fclose(imageFd);
        throw new JpegLoadError("Could not set error handler.");
    }



    jpeg_create_decompress(&cinfo);


// TODO(dustin): We need to jump to the offset where the previous read left-off at.
    jpeg_mem_src(&cinfo, imageData, imageLength);
    // jpeg_stdio_src(&cinfo, imageFd);

    // jpeg_mem_src(&cinfo, imageData, imageLength);
    // jpeg_stdio_src(&cinfo, imageFd);



// NOTE(dustin): JPEG uses APP1.
    jpeg_save_markers(&cinfo, JPEG_APP0 + 1, 0xffff);
// NOTE(dustin): JPEG MPO uses APP2, also.
    jpeg_save_markers(&cinfo, JPEG_APP0 + 2, 0xffff);





    ParseOneImage(cinfo);
    ParseOneImage(cinfo);




//     jpeg_create_decompress(&cinfo);



// // NOTE(dustin): JPEG uses APP1.
//     jpeg_save_markers(&cinfo, JPEG_APP0 + 1, 0xffff);
// // NOTE(dustin): JPEG MPO uses APP2, also.
//     jpeg_save_markers(&cinfo, JPEG_APP0 + 2, 0xffff);


//     ParseOneImage(cinfo);




//     jpeg_finish_decompress(&cinfo);



    jpeg_destroy_decompress(&cinfo);

    // fclose(imageFd);
}

int main() {
    std::string const filepath = "/home/local/MAGICLEAP/doprea/development/cpp/jpeg_mpo/resource/IMAG0238.mpo";

    ImageData id(filepath);
    id.Open();
    id.ParseAll();

    return 0;
}
