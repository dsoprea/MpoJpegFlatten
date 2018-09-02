#include <vector>
#include <string>

#include "image_data.h"
#include "scanline_collector.h"
#include "exceptions.h"

// Extracted mostly verbatim from the libjpeg project example.
void WriteJpegFile(std::string outputFilepath, uint8_t *data, uint32_t width, uint32_t height, int components) {
    /* This struct contains the JPEG compression parameters and pointers to
     * working space (which is allocated as needed by the JPEG library).
     * It is possible to have several such structures, representing multiple
     * compression/decompression processes, in existence at once.  We refer
     * to any one struct (and its associated working data) as a "JPEG object".
     */
    struct jpeg_compress_struct cinfo;
    /* This struct represents a JPEG error handler.  It is declared separately
     * because applications often want to supply a specialized error handler
     * (see the second half of this file for an example).  But here we just
     * take the easy way out and use the standard error handler, which will
     * print a message on stderr and call exit() if compression fails.
     * Note that this struct must live as long as the main JPEG parameter
     * struct, to avoid dangling-pointer problems.
     */
    struct jpeg_error_mgr jerr;
    /* More stuff */
    FILE * outfile;       /* target file */
    JSAMPROW row_pointer[1];  /* pointer to JSAMPLE row[s] */
    int row_stride;       /* physical row width in image buffer */

    /* Step 1: allocate and initialize JPEG compression object */

    /* We have to set up the error handler first, in case the initialization
     * step fails.  (Unlikely, but it could happen if you are out of memory.)
     * This routine fills in the contents of struct jerr, and returns jerr's
     * address which we place into the link field in cinfo.
     */
    cinfo.err = jpeg_std_error(&jerr);
    /* Now we can initialize the JPEG compression object. */
    jpeg_create_compress(&cinfo);

    /* Step 2: specify data destination (eg, a file) */
    /* Note: steps 2 and 3 can be done in either order. */

    /* Here we use the library-supplied code to send compressed data to a
     * stdio stream.  You can also write your own code to do something else.
     * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
     * requires it in order to write binary files.
     */
    if ((outfile = fopen(outputFilepath.c_str(), "wb")) == NULL) {
        throw new JpegLoadError("can't open output file");
    }

    jpeg_stdio_dest(&cinfo, outfile);

    /* Step 3: set parameters for compression */

    /* First we supply a description of the input image.
     * Four fields of the cinfo struct must be filled in:
     */
    cinfo.image_width = width;  /* image width and height, in pixels */
    cinfo.image_height = height;
    cinfo.input_components = components;       /* # of color components per pixel */
    cinfo.in_color_space = JCS_RGB;   /* colorspace of input image */
    /* Now use the library's routine to set default compression parameters.
     * (You must set at least cinfo.in_color_space before calling this,
     * since the defaults depend on the source color space.)
     */
    jpeg_set_defaults(&cinfo);
    /* Now you can set any non-default parameters you wish to.
     * Here we just illustrate the use of quality (quantization table) scaling:
     */
    jpeg_set_quality(&cinfo, 90, TRUE /* limit to baseline-JPEG values */);

    /* Step 4: Start compressor */

    /* TRUE ensures that we will write a complete interchange-JPEG file.
     * Pass TRUE unless you are very sure of what you're doing.
     */
    jpeg_start_compress(&cinfo, TRUE);

    /* Step 5: while (scan lines remain to be written) */
    /*           jpeg_write_scanlines(...); */

    /* Here we use the library's state variable cinfo.next_scanline as the
     * loop counter, so that we don't have to keep track ourselves.
     * To keep things simple, we pass one scanline per call; you can pass
     * more if you wish, though.
     */
    row_stride = cinfo.image_width * cinfo.input_components; /* JSAMPLEs per row in image_buffer */

    while (cinfo.next_scanline < cinfo.image_height) {
      /* jpeg_write_scanlines expects an array of pointers to scanlines.
       * Here the array is only one element long, but you could pass
       * more than one scanline at a time if that's more convenient.
       */
      row_pointer[0] = &data[cinfo.next_scanline * row_stride];
      (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    /* Step 6: Finish compression */

    jpeg_finish_compress(&cinfo);
    /* After finish_compress, we can close the output file. */
    fclose(outfile);

    /* Step 7: release JPEG compression object */

    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_compress(&cinfo);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Please provide a file-path to an MPO JPEG and another to the output JPEG file-path." << std::endl;
        return 1;
    }

    std::string const inputFilepath = argv[1];
    std::string const outputFilepath = argv[2];

    ImageData id(inputFilepath);
    id.Open();
    std::vector<ScanlineCollector *> *images = id.ExtractMpoImages();

    if (images->size() != 2) {
        for (auto it = images->begin() ; it != images->end(); ++it) {
            delete *it;
        }

        return 2;
    }

// NOTE(dustin): !! We have to just assume left, then right. Later, when we get into EXIF, we can revisit how to reliably determine this.
    ScanlineCollector *slc_left = (*images)[0];
    ScanlineCollector *slc_right = (*images)[1];

    LrImage result = id.BuildLrImage(slc_left, slc_right);
    if (result.error_message != "") {
        std::cout << "Could not build LR image: [" << result.error_message << "]" << std::endl;

        delete slc_left;
        delete slc_right;

        return 2;
    }

    WriteJpegFile(
        outputFilepath,
        result.data,
        result.width,
        result.height,
        slc_left->Components());

    delete result.data;
    delete images;

    return 0;
}
