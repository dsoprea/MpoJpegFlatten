## Overview

MPO images are JPEG files that have multiple contiguous images, where each of these images have an APP2 marker in addition to a APP1 marker, and the APP2 has an image index (just the first image has this) and a block of EXIF tags.

This is a tool that validates that a MPO file has exactly two images that each have a APP2 IFD and the MPO format-string inside of it, and then combines those two images into a single normal JPEG. The first image is taken as the left and the second as the right.


## Dependencies

- libturbojpeg


## Building

```
$ make
```


## Testing

This will convert the MPO in the resources/ path to a JPEG in the build/ path.

```
$ make test
```


## NOTES

- Conformant to C++11.
- The original subimages have to have identical sizes, depths, and colorspaces.
- The output image has the same colorspace as the input.
- We don't currently process EXIF information. We assume the first image is the left image and the second is the right.
