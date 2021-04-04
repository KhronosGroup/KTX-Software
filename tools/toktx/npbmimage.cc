// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

// Copyright 2010-2020 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

//!
//! @internal
//! @~English
//! @file
//!
//! @brief Create Images from netpbm format (.pam, .pbm or .pgm) files.
//!
//! The loader transforms the image from its bt.709 transfer function to
//! something supported by by 3D APIs. The .ppm specification
//! (http://netpbm.sourceforge.net/doc/ppm.html)
//! does not indicate that BT.709 only applies when maxval <= 255
//! so transformation is always performed. Since 3D APIs, quite
//! sensibly, only provide sRGB support for 8-bit formats, larger formats
//! are transformed to a linear OETF.
//!
//! The specification also says that both sRGB and linear encoding are
//! often used with the netpbm formats. Since there is no metadata
//! to indicate a differing transform, this loader always assumes bt.709.
//!
//! @author Mark Callow.
//! @author Jacob Str&ouml;m, Ericsson AB.
//!

#include "stdafx.h"
#include <inttypes.h>
#include <stdlib.h>
#include "image.hpp"


static int tupleSize(const char* tupleType);

// Skips over comments in a netpbm file
// (i.e., lines starting with #)
//
// Written by Jacob Strom
//
static
void skipComments(FILE *src)
{
    int c;

    while((c = getc(src)) == '#')
    {
        char line[1024];
                // This is to silence -Wunused-result from GCC 4.8+.
        char* retval;
        retval = fgets(line, 1024, src);
    }
    ungetc(c, src);
}

// Skips over white spaces in a netpbm file
//
// Written by Jacob Strom
//
static
void skipSpaces(FILE *src)
{
    int c;

    c = getc(src);
    while(c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r')
    {
        c = getc(src);
    }
    ungetc(c, src);
}


// Skips over intervening non-data elements in a netpbm file
static
void skipNonData(FILE *src)
{
    skipSpaces(src);
    skipComments(src);
    skipSpaces(src);
}

static Image* createFromPPM(FILE*, bool transformOETF, bool rescaleTo8Bits);
static Image* createFromPGM(FILE*, bool transformOETF, bool rescaleTo8Bits);
static Image* createFromPAM(FILE*, bool transformOETF, bool rescaleTo8Bits);
static void parseHeader(FILE* src, uint32_t& width, uint32_t& height,
                        int32_t& maxval);
static void readImage(FILE* src, Image& image, int32_t maxval);

//!
//! @internal
//! @~English
//! @brief Create an Image from a netpbm file, either PAM, PGM or PPM
//!
//! The file type is determined from the magic number.
//! P5 is a PGM file. P6 is a PPM binary file, P7 is a PAM file.
//!
//! @param [in] src           pointer to FILE stream to read
//! @param [in] transformOETF transform the image from the
//!                           bt.709 OETF of the input.
//!
//! @return A newly constructed Image with the type matching the file contents
//!         and containing the file contents.
//!
//! @exception different_format the file is not in .pam, .pgm or .ppm format
//!
//! @author Mark Callow
//!
Image*
Image::CreateFromNPBM(FILE* src, bool transformOETF, bool rescaleTo8Bits)
{
    char line[255];
    int numvals;

    skipNonData(src);

    numvals = fscanf(src, "%3s", line);
    if (numvals != 0) {
        if (strcmp(line, "P6") == 0) {
            return createFromPPM(src, transformOETF, rescaleTo8Bits);
        } else if (strcmp(line, "P5") == 0) {
            return createFromPGM(src, transformOETF, rescaleTo8Bits);
        } else if (strcmp(line, "P7") == 0) {
            return createFromPAM(src, transformOETF, rescaleTo8Bits);
        } else if (strcmp(line, "P3") == 0) {
            throw std::runtime_error("Plain PPM format is not supported.");
        }
    }
    throw different_format();
}


//!
//! @internal
//! @~English
//! @brief Create an Image from a PPM file with a P6 header.
//!
//! P6 indicates binary, as opposed to P5, which is ASCII format. The
//! header must look like this:
//!
//! P6
//! # Comments (not necessary)
//! width height
//! 255
//!
//! after that follows RGBRGBRGB...
//!
//! @param [in] src           pointer to FILE stream to read
//! @param [in] transformOETF transform the image from the
//!                           bt.709 OETF of the input.
//!
//! @return A newly constructed Image with the type matching the file contents
//!         and containing the file contents.
//!
//! @exception invalid_file @sa parseHeader for details.
//!
//! @author Mark Callow
//!
Image*
createFromPPM(FILE* src, bool transformOETF, bool rescaleTo8Bits)
{
    int32_t maxval;
    uint32_t numvals, width, height;
    Image* image;

    skipNonData(src);

    parseHeader(src, width, height, maxval);
    //fprintf(stderr, "maxval is %d\n",maxval);

    // PPM is 3 components
    if (maxval > 255 && !rescaleTo8Bits)
        image = new rgb16image(width, height);
    else
        image = new rgb8image(width, height);
    image->setColortype(Image::eRGB);

    // We need to remove the newline.
    char c = 0;
    while(c != '\n')
        numvals = fscanf(src, "%c", &c);

    readImage(src, *image, maxval);
    if (transformOETF) {
        if (maxval <= 255) {
            image->transformOETF(decode_bt709, encode_sRGB);
            image->setOetf(KHR_DF_TRANSFER_SRGB);
        } else {
            image->transformOETF(decode_bt709, encode_linear);
            image->setOetf(KHR_DF_TRANSFER_LINEAR);
        }
    }
    return image;
}


//!
//! @internal
//! @~English
//! @brief Create an Image from a PGM file with a P5 header.
//!
//! P6 indicates binary, as opposed to P3, which is ASCII format. The
//! header must look like this:
//!
//! P5
//! # Comments (not necessary)
//! width height
//! 255
//!
//! after that follows RGBRGBRGB...
//!
//! @param [in] src           pointer to FILE stream to read
//! @param [in] transformOETF transform the image from the
//!                           bt.709 OETF of the input.
//!
//! @return A newly constructed Image with the type matching the file contents
//!         and containing the file contents.
//!
//! @exception invalid_file @sa parseHeader for details.
//!
//! @author Mark Callow
//!
Image*
createFromPGM(FILE* src, bool transformOETF, bool rescaleTo8Bits)
{
    int maxval;
    int numvals;
    uint32_t width, height;
    Image* image;

    skipNonData(src);

    parseHeader(src, width, height, maxval);

    // PGM is 1 component. Treat as luminance for consistency with .png & .jpg.
    if (maxval > 255 && !rescaleTo8Bits)
        image = new r16image(width, height);
    else
        image = new r8image(width, height);
    image->setColortype(Image::eLuminance);

    /* gotta eat the newline too */
    char ch=0;
    while(ch!='\n') numvals = fscanf(src,"%c",&ch);

    readImage(src, *image, maxval);
    if (transformOETF) {
         if (maxval <= 255) {
            image->transformOETF(decode_bt709, encode_sRGB);
            image->setOetf(KHR_DF_TRANSFER_SRGB);
        } else {
            image->transformOETF(decode_bt709, encode_linear);
            image->setOetf(KHR_DF_TRANSFER_LINEAR);
        }
    }
    return image;
}


//!
//! @internal
//! @~English
//! @brief Create an Image from a PAM file with a P7 header.
//!
//! The header must look like this:
//!
//! P7
//! # Comments if you want to
//! WIDTH nnn
//! HEIGHT nnn
//! DEPTH n
//! MAXVAL nnn
//! TUPLTYPE nnn
//! ENDHDR
//!
//! then follows TUPLETUPLETUPLETUPLE...
//!
//! @param [in] src           pointer to FILE stream to read
//! @param [in] transformOETF transform the image from the
//!                           bt.709 OETF of the input.
//!
//! @return A newly constructed Image with the type matching the file contents
//!         and containing the file contents.
//!
//! @exception invalid_file if the PAM header or TUPLETYPE is invalid, the
//!            TUPLTYPE does not match DEPTH or MAXVAL is out of range.
//!
//! @author Mark Callow
//!
Image*
createFromPAM(FILE* src, bool transformOETF, bool rescaleTo8Bits)
{
    char line[255];
#define MAX_TUPLETYPE_SIZE 20
#define xtupletype_sscanf_fmt(ms) tupletype_sscanf_fmt(ms)
#define tupletype_sscanf_fmt(ms) "TUPLTYPE %"#ms"s"
    char tupleType[MAX_TUPLETYPE_SIZE+1];   // +1 for terminating NUL.
    unsigned int width, height;
    unsigned int maxval, depth;
    unsigned int numFieldsFound = 0;
    unsigned int components;
    Image* image;

    for (;;) {
        skipNonData(src);
        if (!fgets(line, sizeof(line), src)) {
            if (feof(src))
                throw std::runtime_error("Unexpected end of file.");
            else
                throw std::runtime_error("IO error.");
        }
        if (strcmp(line, "ENDHDR\n") == 0)
            break;

        if (sscanf(line, "HEIGHT %u", &height))
            numFieldsFound++;
        else if (sscanf(line, "WIDTH %u", &width))
            numFieldsFound++;
        else if (sscanf(line, "DEPTH %u", &depth))
            numFieldsFound++;
        else if (sscanf(line, "MAXVAL %u", &maxval))
            numFieldsFound++;
        else if (sscanf(line, xtupletype_sscanf_fmt(MAX_TUPLETYPE_SIZE),
                        tupleType))
            numFieldsFound++;
    };

    if (numFieldsFound < 5)
        throw Image::invalid_file("Invalid PAM header.");

    if ((components = tupleSize(tupleType)) < 1)
        throw Image::invalid_file("Invalid TUPLTYPE.");

    if (components != depth)
        throw Image::invalid_file("Mismatched TUPLTYPE and DEPTH.");

    if (maxval <= 0 || maxval >= (1<<16)) {
        throw Image::invalid_file("Max color component value must be > 0 && < 65536.");
    }
    if (maxval > 255 && !rescaleTo8Bits) {
        switch (depth) {
          case 1:
            image = new r16image(width, height);
            break;
          case 2:
            image = new rg16image(width, height);
            break;
          case 3:
            image = new rgb16image(width, height);
            break;
          case 4:
            image = new rgba16image(width, height);
            break;
        }
    } else {
        switch (depth) {
          case 1:
            image = new r8image(width, height);
            break;
          case 2:
            image = new rg8image(width, height);
            break;
          case 3:
            image = new rgb8image(width, height);
            break;
          case 4:
            image = new rgba8image(width, height);
            break;
        }
    }
    switch (depth) {
      case 1:
        // NPBM specs do spec. what a depth 1 image is.
        // We choose to treat is luminance, for consistency with PNG.
        image->setColortype(Image::eLuminance);
        break;
      case 2:
        // As with depth=1, handle consistently with PNG.
        image->setColortype(Image::eLuminanceAlpha);
        break;
      case 3:
        image->setColortype(Image::eRGB);
        break;
      case 4:
        image->setColortype(Image::eRGBA);
        break;
    }


    readImage(src, *image, maxval);
    if (transformOETF) {
        if (maxval <= 255) {
            image->transformOETF(decode_bt709, encode_sRGB);
            image->setOetf(KHR_DF_TRANSFER_SRGB);
        } else {
            image->transformOETF(decode_bt709, encode_linear);
            image->setOetf(KHR_DF_TRANSFER_LINEAR);
        }
    }

    return image;
}


static int
tupleSize(const char* tupleType)
{
    if (strcmp(tupleType, "BLACKANDWHITE") == 0)
        return -1;
    else if (strcmp(tupleType, "GRAYSCALE") == 0)
        return 1;
    else if (strcmp(tupleType, "GRAYSCALE_ALPHA") == 0)
        return 2;
    else if (strcmp(tupleType, "RGB") == 0)
        return 3;
    else if (strcmp(tupleType, "RGB_ALPHA") == 0)
        return 4;
    else
        return -1;
}

static void
rescale16to8(uint8_t* dst, uint16_t* src, uint32_t maxval, uint32_t count)
{
    float multiplier = 255.0f / maxval;
    for (uint32_t i = 0; i < count; ++i) {
        *dst++ = (uint8_t)roundf(*src++ * multiplier);
    }
}

static void
rescale8(uint8_t* dst, uint8_t* src, uint32_t maxval, uint32_t count)
{
    float multiplier = 255.0f / maxval;
    for (uint32_t i = 0; i < count; ++i) {
        *dst++ = (uint8_t)roundf(*src++ * multiplier);
    }
}

static void
rescale16(uint16_t* dst, uint16_t* src, uint32_t maxval, uint32_t count)
{
    float multiplier = 65535.0f / maxval;
    for (uint32_t i = 0; i < count; ++i) {
        *dst++ = (uint16_t)roundf(*src++ * multiplier);
    }
}

/*
 * SwapEndian16: Swaps endianness in an array of 16-bit values
 */
static void
swapEndian16(uint16_t* pData16, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        uint16_t x = *pData16;
        *pData16++ = (x << 8) | (x >> 8);
    }
}

//#define IS_LITTLE_ENDIAN (((union foo { uint16_t x; uint8_t c; }){1}).c)

union foo { uint16_t x; uint8_t c; } bar{1};
#define IS_LITTLE_ENDIAN (bar.c)

void
readImage(FILE* src, Image& image, int32_t maxval)
{
    // Image have either 2 bytes/pixel if maxval > 255 or 1 byte. If 2, MSB is
    // first.
    char* buffer16 = 0;
    void* pBuffer;
    uint32_t nitems;

    if (maxval > 255 && image.getComponentSize() == 1) {
        // Need to rescale so read the image into a temporary buffer.
        nitems = 2;
        buffer16 = new char[image.getByteCount()*nitems];
        pBuffer = buffer16;
    } else {
        pBuffer = image;
        nitems = 1;
    }
    if (fread(pBuffer, image.getByteCount(), nitems, src) != 1)
    {
        std::stringstream message;
        message << "unexpected end of file. Could not read "
                << image.getByteCount() << " bytes of pixel data.";
        throw std::runtime_error(message.str());
    }
    if (IS_LITTLE_ENDIAN && maxval > 255) {
        swapEndian16((uint16_t*)pBuffer,
                     image.getPixelCount() * image.getComponentCount());
    }

    if (buffer16 != nullptr) {
        rescale16to8(image, (uint16_t*)pBuffer, maxval,
                     image.getPixelCount() * image.getComponentCount());
        delete[] buffer16;
    }
    // Maxval is whitepoint. Rescale needed if white is not uint MAX.
    else if (maxval < 255) {
        rescale8((uint8_t*)pBuffer, (uint8_t*)pBuffer, maxval,
                 image.getPixelCount() * image.getComponentCount());
    }
    else if (maxval > 255 && maxval < 65535) {
        rescale16((uint16_t*)pBuffer, (uint16_t*)pBuffer, maxval,
                  image.getPixelCount() * image.getComponentCount());
    }

    image.setOetf(KHR_DF_TRANSFER_ITU);
}


//!
//! @internal
//! @~English
//! @brief parse the header of a PGM or PPM file.
//!
//! @param [in]  src    pointer to FILE stream to read
//! @param [out] width  reference to a var in which to write the image width.
//! @param [out] height reference to a var in which to write the image height
//! @param [out] maxval reference to a var in which to write the maxval.
//!
//! @exception invalid_file if there is no width or height, if maxval is not
//!                         an integer or if maxval is out of range.
//!
void parseHeader(FILE* src, uint32_t& width, uint32_t& height, int32_t& maxval)
{
    uint32_t numvals;

    numvals = fscanf(src, "%u %u", &width, &height);
    if (numvals != 2) {
        fclose(src);
        throw Image::invalid_file("width or height is missing.");
    }
    if (width <= 0 || height <= 0) {
        fclose(src);
        throw Image::invalid_file("width or height is negative.");
    }

    skipNonData(src);

    numvals = fscanf(src, "%d", &maxval);
    if (numvals == 0) {
        fclose(src);
        throw Image::invalid_file("maxval must be an integer.");
    }
    if (maxval <= 0 || maxval >= (1<<16)) {
        fclose(src);
        throw Image::invalid_file("Max color component value must be > 0 && < 65536.");
    }
}
