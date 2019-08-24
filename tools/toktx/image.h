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
//! @file image.h
//!
//! @brief Internal interface for netpbm file reader
//!

#ifndef IMAGE_H
#define IMAGE_H

#include <math.h>

enum FileResult { SUCCESS, INVALID_FORMAT, INVALID_VALUE, INVALID_PAM_HEADER,
                  INVALID_TUPLETYPE, UNEXPECTED_EOF, IO_ERROR, OUT_OF_MEMORY };

FileResult readNPBM(FILE* src, unsigned int& width, unsigned int& height,
                    unsigned int& components, unsigned int& componentSize,
                    size_t& imageSize, unsigned char** pixels);

FileResult readPAM(FILE* src, unsigned int& width, unsigned int& height,
                    unsigned int& components, unsigned int& componentSize,
                    size_t& imageSize, unsigned char** pixels);

FileResult readPPM(FILE* src, unsigned int& width, unsigned int& height,
                    unsigned int& components, unsigned int& componentSize,
                    size_t& imageSize, unsigned char** pixels);

FileResult readPGM(FILE* src, unsigned int& width, unsigned int& height,
                    unsigned int& components, unsigned int& componentSize,
                    size_t& imageSize, unsigned char** pixels);

FileResult readImage(FILE* src, size_t imageSize, unsigned char*& pixels);

template <typename T> inline T clamp(T value, T low, T high) {
    return (value < low) ? low : ((value > high) ? high : value);
}

typedef float (*OETFFunc)(float const);

#if defined(_MSC_VER)
#define INLINE __inline
#else
#define INLINE __inline__
#endif

static INLINE float
encode709(float const intensity) {
    /* We're following what Netpbm does. This is their comment and code. */

    /* Here are parameters of the gamma transfer function for the Netpbm
       formats.  This is ITU-R Recommendation BT.709, FKA CIE Rec 709.  It is
       also ITU-R Recommendation BT.601, FKA CCIR 601.

       This transfer function is linear for sample values 0 .. .018
       and an exponential for larger sample values.
       The exponential is slightly stretched and translated, though,
       unlike the popular pure exponential gamma transfer function.

       The standard actually defines the linear expansion as 4.500, which
       means there is a discontinuity at linear intensity .018.  We instead
       use ~4.514 to make a continuous function.  This may have been simply
       a mistake when this code was written or based on an actual benefit
       to having a continuous function -- The history is not clear.

       Note that the discrepancy is below the precision of a maxval 255
       image.
    */
    float const gamma = 2.2f;
    float const oneOverGamma = 1.0f / gamma;
    float const linearCutoff = 0.018f;
    float const linearExpansion =
        (1.099f * pow(linearCutoff, oneOverGamma) - 0.099f) / linearCutoff;

    float brightness;

    if (intensity < linearCutoff)
        brightness = intensity * linearExpansion;
    else
        brightness = 1.099f * pow(intensity, oneOverGamma) - 0.099f;

    return brightness;
}

static INLINE float
decode709(float const brightness)
{
    float const gamma = 2.2f;
    float const oneOverGamma = 1.0f / gamma;
    float const linearCutoff = 0.018f;
    float const linearExpansion =
        (1.099f * pow(linearCutoff, oneOverGamma) - 0.099f) / linearCutoff;

    float intensity;

    if (brightness < linearCutoff * linearExpansion)
        intensity = brightness / linearExpansion;
    else
        intensity = pow((brightness + 0.099f) / 1.099f, gamma);

    return intensity;
}

static INLINE float
encode_sRGB(float const intensity)
{
    float brightness;
    if (intensity < 0.0031308f)
        brightness = 12.92f * intensity;
    else
        brightness = 1.055f * pow(intensity, 1.0f/2.4f) - 0.055f;

    return brightness;
}

void OETFtransform(size_t imageBytes, uint8_t* pixels,
                   uint32_t components, OETFFunc decode, OETFFunc encode);

void OETFtransform(size_t imageBytes, uint16_t* pixels,
                   uint32_t components, OETFFunc decode, OETFFunc encode);

#endif



