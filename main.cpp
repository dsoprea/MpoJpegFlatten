#include <vector>
#include <string>

#include "image_data.h"
#include "scanline_collector.h"

int main() {
// TODO(dustin): !! Debugging.
    std::string const filepath = "/home/local/MAGICLEAP/doprea/development/cpp/jpeg_mpo/resource/IMAG0238.mpo";

    ImageData id(filepath);
    id.Open();
    std::vector<ScanlineCollector *> *images = id.ExtractMpoImages();

    if (images->size() != 2) {
        for (std::vector<ScanlineCollector *>::iterator it = images->begin() ; it != images->end(); ++it) {
            delete *it;
        }

        return 1;
    }

// NOTE(dustin): !! We have to just assume left, then right. Later, when we get into EXIF, we can revisit how to reliably determine this.
    ScanlineCollector *slc_left = (*images)[0];
    ScanlineCollector *slc_right = (*images)[1];

// TODO(dustin): !! Return actual data, later.
// TODO(dustin): !! Package the raw image data into a JPEG and write to a temporary file, for testing.
    if (id.BuildLrImage(slc_left, slc_right) == false) {
        delete slc_left;
        delete slc_right;

        return 2;
    }

    delete images;
    return 0;
}
