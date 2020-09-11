// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

// Copyright 2010-2020 The Khronos Group Inc.
// SPDX-License-Identifier: Apache-2.0

//!
//! @internal
//! @~English
//! @file
//!
//! @brief Create Images from PNG format files.
//!
//! @author Mark Callow, HI Corporation.
//! @author Jacob Str&ouml;m, Ericsson AB.
//!

#include "stdafx.h"

#include <sstream>
#include <stdexcept>

#include "image.hpp"
#include "lodepng.h"
#include <KHR/khr_df.h>
#include "dfd.h"

void warning(const char *pFmt, ...);

Image*
Image::CreateFromPNG(FILE* src, bool transformOETF, bool rescaleTo8Bits)
{
    // Unfortunately LoadPNG doesn't believe in stdio plus
    // the function we need only reads from memory. To avoid
    // a potentially unnecessary read of the whole file check the
    // signature ourselves.
    uint8_t pngsig[8] = {
       0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a
    };
    uint8_t filesig[sizeof(pngsig)];
    if (fseek(src, 0L, SEEK_SET) < 0) {
       std::stringstream message;
       message << "Could not seek. " << strerror(errno);
       throw std::runtime_error(message.str());
    }
    if (fread(filesig, sizeof(pngsig), 1, src) != 1) {
       std::stringstream message;
       message << "Could not read. " << strerror(errno);
       throw std::runtime_error(message.str());
    }
    if (memcmp(filesig, pngsig, sizeof(pngsig))) {
       throw Image::different_format();
    }

    // It's a PNG file.

    // Find out the size.
    size_t fsz;
    fseek(src, 0L, SEEK_END);
    fsz = ftell(src);
    fseek(src, 0L, SEEK_SET);

    // Slurp it into memory so we can use lodepng_inspect to determine
    // the data type and look at the ancilliary chunks.
    std::vector<uint8_t> png;
    png.resize(fsz);
    if (fread(png.data(), 1L, png.size(), src) != png.size()) {
       if (feof(src)) {
           throw std::runtime_error("Unexpected end of file.");
       } else {
           std::stringstream message;
           message << "Could not read. " << strerror(ferror(src));
           throw std::runtime_error(message.str());
       }
    }

    lodepng::State state;
    uint32_t componentCount, componentBits;
    uint32_t w, h;
    // Find out the color type so we can request that type when
    // decoding and avoid conversions. Oh for an option to decode
    // to file's colortype. What a palaver! Sigh!
    lodepng_inspect(&w, &h, &state, &png[0], png.size());
    // Tell the decoder we want the same color type as the file
    state.info_raw = state.info_png.color;
    switch (state.info_png.color.colortype) {
      case LCT_GREY:
        componentCount = 1;
        // TODO: Create 4-bit color type and rescale 1- & 2-bpp to that.
        rescaleTo8Bits = true;
        break;
      case LCT_RGB:
        if (state.info_png.color.key_defined) {
            state.info_raw.colortype = LCT_RGBA;
            componentCount = 4;
        } else {
            state.info_raw.colortype = LCT_RGB;
            componentCount = 3;
        }
        break;
      case LCT_PALETTE:
        if (state.info_png.color.key_defined) {
            state.info_raw.colortype = LCT_RGBA;
            componentCount = 4;
        } else {
            state.info_raw.colortype = LCT_RGB;
            componentCount = 3;
        }
        state.info_raw.bitdepth = 8; // Palette values are 8 bit RGB
        warning("Expanding paletted image to %s 8-bit",
                state.info_raw.colortype == LCT_RGBA ? "RGBA" : "RGB");
        break;
      case LCT_GREY_ALPHA:
        componentCount = 2;
        break;
      case LCT_RGBA:
        componentCount = 4;
        break;
    }
    if (rescaleTo8Bits) {
        state.info_raw.bitdepth = 8;
        if (state.info_png.color.bitdepth != 8) {
            warning("Rescaling %d-bit image to 8 bits.",
                    state.info_png.color.bitdepth);
        }
        componentBits = 8;
    } else {
        componentBits = state.info_png.color.bitdepth;
    }

    uint8_t* imageData;
    size_t imageByteCount;
    uint32_t error = lodepng_decode(&imageData, &w, &h, &state,
                                   png.data(), png.size());
    if (imageData && !error) {
        imageByteCount = lodepng_get_raw_size(w, h, &state.info_raw);
    } else {
        free(imageData);
        std::stringstream message;
        message << "PNG decoder error. " << lodepng_error_text(error);
        throw std::runtime_error(message.str());
    }

    Image* image;
    if (componentBits == 16 ) {
        switch (componentCount) {
          case 1: {
            using Color = color<uint16_t, 1>;
            image = new ImageT< Color >(w, h, (Color*)imageData);
            break;
          } case 2: {
            using Color = color<uint16_t, 2>;
            image = new ImageT< Color >(w, h, (Color*)imageData);
            break;
          } case 3: {
            using Color = color<uint16_t, 3>;
            image = new ImageT< Color >(w, h, (Color*)imageData);
            break;
          } case 4: {
            using Color = color<uint16_t, 4>;
            image = new ImageT< Color >(w, h, (Color*)imageData);
            break;
          }
        }
    } else {
        switch (componentCount) {
          case 1: {
            using Color = color<uint8_t, 1>;
            image = new ImageT< Color >(w, h, (Color*)imageData);
            break;
          } case 2: {
            using Color = color<uint8_t, 2>;
            image = new ImageT< Color >(w, h, (Color*)imageData);
            break;
          } case 3: {
            using Color = color<uint8_t, 3>;
            image = new ImageT< Color >(w, h, (Color*)imageData);
            break;
          } case 4: {
            using Color = color<uint8_t, 4>;
            image = new ImageT< Color >(w, h, (Color*)imageData);
            break;
          }
        }
    }

    if (!transformOETF) {
        // User is overriding color space info from file.
        return image;
    }

    // state will have been updated with the rest of the file info.

    // Here is the priority of the color space info in PNG:
    //
    // 1. No color-info chunks: assume sRGB default or 2.2 gamma
    //    (up to the implementation).
    // 2. sRGB chunk: use sRGB intent specified in the chunk, ignore
    //    all other color space information.
    // 3. iCCP chunk: use the provided ICC profile, ignore gamma and
    //    primaries.
    // 4. gAMA and/or cHRM chunks: use provided gamma and primaries.
    //
    // A PNG image could signal linear transfer function with one
    // of these two options:
    //
    // 1. Provide an ICC profile in iCCP chunk.
    // 2. Use a gAMA chunk with a value that yields linear
    //    function (100000).
    //
    // Using no. 1 above or setting transfer func & primaries from
    // the ICC profile would require parsing the ICC payload.

    if (state.info_png.srgb_defined) {
        // intent is a matter for the user when a color transform
        // is needed during rendering, especially when gamut
        // mapping. It does not affect the meaning or value of the
        // image pixels so there is nothing to do here.
        image->setOetf(KHR_DF_TRANSFER_SRGB);
    } else if (state.info_png.iccp_defined) {
        delete image;
        throw std::runtime_error("PNG file has an ICC profile chunk. "
                                 "These are not supported");
    } else if (state.info_png.gama_defined) {
        if (state.info_png.gama_gamma == 100000)
            image->setOetf(KHR_DF_TRANSFER_LINEAR);
        else if (state.info_png.gama_gamma == 45455)
            image->setOetf(KHR_DF_TRANSFER_SRGB);
        else {
            if (state.info_png.gama_gamma < 0) {
                delete image;
                throw std::runtime_error("PNG file has gAMA of 0.");
            }
            // What PNG calls gamma is the power to use for encoding. Elsewhere
            // gamma is commonly used for the power to use for decoding.
            // For example by spec. the value in the PNG file is
            // gamma * 100000 so gamma of 45455 is .45455. The power for
            // decoding is the inverse, i.e  1 / .45455 which is 2.2.
            // The variable gamma below is for decoding and is 1 / gAMA.
            float gamma = (float) 100000 / state.info_png.gama_gamma;
            // 1.6667 is a very arbitrary cutoff.
            if (componentBits == 8 && gamma > 1.6667f) {
                image->transformOETF(decode_gamma, encode_sRGB, gamma);
                image->setOetf(KHR_DF_TRANSFER_SRGB);
                if (gamma > 3.3333f) {
                    warning("Transformed PNG image with gamma of %f to sRGB"
                            " gamma (~2.2)", gamma);
                }
            } else {
                image->transformOETF(decode_gamma, encode_linear, gamma);
                image->setOetf(KHR_DF_TRANSFER_LINEAR);
                if (gamma > 1.3) {
                    warning("Transformed PNG image with gamma of %f to"
                            " linear", gamma);
                }
            }
        }
    } else {
        image->setOetf(KHR_DF_TRANSFER_SRGB);
    }

    if (state.info_png.chrm_defined
        && !state.info_png.srgb_defined && !state.info_png.iccp_defined) {
        Primaries primaries;
        primaries.Rx = (float)state.info_png.chrm_red_x / 100000;
        primaries.Ry = (float)state.info_png.chrm_red_y / 100000;
        primaries.Gx = (float)state.info_png.chrm_green_x / 100000;
        primaries.Gy = (float)state.info_png.chrm_green_y / 100000;
        primaries.Bx = (float)state.info_png.chrm_blue_x / 100000;
        primaries.By = (float)state.info_png.chrm_blue_y / 100000;
        primaries.Wx = (float)state.info_png.chrm_white_x / 100000;
        primaries.Wy = (float)state.info_png.chrm_white_y / 100000;
        image->setPrimaries(findMapping(&primaries, 0.002f));
    }

    return image;
}

