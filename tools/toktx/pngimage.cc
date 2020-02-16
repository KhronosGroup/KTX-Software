// -*- tab-width: 4; -*-
// vi: set sw=2 ts=4 expandtab:

//
// ©2010 The Khronos Group, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

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

#include <sstream>
#include <stdexcept>

#include "image.hpp"
#include "lodepng.h"

Image*
Image::CreateFromPNG(FILE* src, bool transformOETF)
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
    uint32_t componentCount, componentSize;
    uint32_t w, h;
    // Find out the color type so we can request that type when
    // decoding and avoid conversions. Oh for an option to decode
    // to file's colortype. What a palaver! Sigh!
    lodepng_inspect(&w, &h, &state, &png[0], png.size());
    switch (state.info_png.color.colortype) {
      case LCT_GREY:
        componentCount = 1;
        break;
      case LCT_RGB:
        componentCount = 3;
        break;
      case LCT_PALETTE:
        throw std::runtime_error("Paletted images not supported.");
        break;
      case LCT_GREY_ALPHA:
        componentCount = 2;
        break;
      case LCT_RGBA:
        componentCount = 4;
        break;
    }
    componentSize = state.info_png.color.bitdepth / 8;


     // Tell the decoder we want the same color type as the file
     uint8_t* imageData;
     size_t imageByteCount;
     size_t pixelCount;
     state.info_raw = state.info_png.color;
     uint32_t error = lodepng_decode(&imageData, &w, &h, &state,
                                     png.data(), png.size());
     if (imageData && !error) {
         imageByteCount = lodepng_get_raw_size(w, h, &state.info_raw);
         pixelCount = w * h;
     } else {
         free(imageData);
         std::stringstream message;
         message << "PNG decoder error. " << lodepng_error_text(error);
         throw std::runtime_error(message.str());
     }

    Image* image;
    if (state.info_png.color.bitdepth == 16) {
        switch (componentCount) {
          case 1: {
            using Color = color<uint16_t, 1>;
            image = new ImageT< Color, PreAllocator<Color> >(w, h, (Color*)imageData, pixelCount);
            break;
          } case 2: {
            using Color = color<uint16_t, 2>;
            image = new ImageT< Color, PreAllocator<Color> >(w, h, (Color*)imageData, pixelCount);
            break;
          } case 3: {
            using Color = color<uint16_t, 3>;
            image = new ImageT< Color, PreAllocator<Color> >(w, h, (Color*)imageData, pixelCount);
            break;
          } case 4: {
            using Color = color<uint16_t, 4>;
            image = new ImageT< Color, PreAllocator<Color> >(w, h, (Color*)imageData, pixelCount);            break;
          }
        }
    } else {
        switch (componentCount) {
          case 1: {
            using Color = color<uint8_t, 1>;
            image = new ImageT< Color, PreAllocator<Color> >(w, h, (Color*)imageData, pixelCount);
            break;
          } case 2: {
            using Color = color<uint8_t, 2>;
            image = new ImageT< Color, PreAllocator<Color> >(w, h, (Color*)imageData, pixelCount);
            break;
          } case 3: {
            using Color = color<uint8_t, 3>;
            image = new ImageT< Color, PreAllocator<Color> >(w, h, (Color*)imageData, pixelCount);
            break;
          } case 4: {
            using Color = color<uint8_t, 4>;
            image = new ImageT< Color, PreAllocator<Color> >(w, h, (Color*)imageData, pixelCount);            break;
          }
        }
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
    //
    // Using cHRM to get the primaries would require matching a set
    // of primary values to a DFD primaries id.

    if (state.info_png.srgb_defined) {
       // intent is a matter for the user when a color transform
       // is needed during rendering, especially when gamut
       // mapping. It does not affect the meaning or value of the
       // image pixels so there is nothing to do here.
       image->setOetf(Image::eOETFsRGB);
    } else {
       if (state.info_png.iccp_defined) {
           //delete image;
           throw std::runtime_error("PNG file has an ICC profile chunk. "
                                    "These are not supported");
       } else if (state.info_png.gama_defined) {
           if (state.info_png.gama_gamma == 100000)
               image->setOetf(Image::eOETFLinear);
           else if (state.info_png.gama_gamma == 45455)
               image->setOetf(Image::eOETFsRGB);
           else {
               //delete image;
               std::stringstream message;
               message << "PNG image has gamma of "
                       << (float)100000 / state.info_png.gama_gamma
                       << ". This is currently unsupported.";
               throw std::runtime_error(message.str());
           }
       } else {
           image->setOetf(Image::eOETFsRGB);
       }
    }

    return image;
}

