#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <sstream>

#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#include <jpeglib.h>

#include "main.h"
#include "exceptions.h"
#include "image_data.h"

int main() {
    std::string const filepath = "/home/local/MAGICLEAP/doprea/development/cpp/jpeg_mpo/resource/IMAG0238.mpo";

    ImageData id(filepath);
    id.Open();
    id.ParseAll();

    return 0;
}
