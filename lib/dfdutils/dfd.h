/*! \file */

/*
 * Copyright (c) 2019 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Author: Andrew Garrard
 */

#ifndef _DFD_H_
#define _DFD_H_

#ifdef __cplusplus
extern "C" {
#endif

/*! Qualifier suffix to the format, in Vulkan terms. */
enum VkSuffix {
    s_UNORM,   /*!< Unsigned normalized format. */
    s_SNORM,   /*!< Signed normalized format. */
    s_USCALED, /*!< Unsigned scaled format. */
    s_SSCALED, /*!< Signed scaled format. */
    s_UINT,    /*!< Unsigned integer format. */
    s_SINT,    /*!< Signed integer format. */
    s_SFLOAT,  /*!< Signed float format. */
    s_UFLOAT,  /*!< Unsigned float format. */
    s_SRGB     /*!< sRGB normalized format. */
};

/*! Compression scheme, in Vulkan terms. */
enum VkCompScheme {
    c_BC1_RGB,       /*!< BC1, aka DXT1, no alpha. */
    c_BC1_RGBA,      /*!< BC1, aka DXT1, punch-through alpha. */
    c_BC2,           /*!< BC2, aka DXT2 and DXT3. */
    c_BC3,           /*!< BC3, aka DXT4 and DXT5. */
    c_BC4,           /*!< BC4. */
    c_BC5,           /*!< BC5. */
    c_BC6H,          /*!< BC6h HDR format. */
    c_BC7,           /*!< BC7. */
    c_ETC2_R8G8B8,   /*!< ETC2 no alpha. */
    c_ETC2_R8G8B8A1, /*!< ETC2 punch-through alpha. */
    c_ETC2_R8G8B8A8, /*!< ETC2 independent alpha. */
    c_EAC_R11,       /*!< R11 ETC2 single-channel. */
    c_EAC_R11G11,    /*!< R11G11 ETC2 dual-channel. */
    c_ASTC           /*!< ASTC. */
};

typedef unsigned int uint32_t;

#include "../vkformat_enum.h"

uint32_t* createDFD4VkFormat(enum VkFormat format);

/* Create a Data Format Descriptor for an unpacked format. */
uint32_t *createDFDUnpacked(int bigEndian, int numChannels, int bytes,
                            int redBlueSwap, enum VkSuffix suffix);

/* Create a Data Format Descriptor for a packed format. */
uint32_t *createDFDPacked(int bigEndian, int numChannels,
                          int bits[], int channels[],
                          enum VkSuffix suffix);

/* Create a Data Format Descriptor for a compressed format. */
    uint32_t *createDFDCompressed(enum VkCompScheme compScheme, int bwidth, int bheight, enum VkSuffix suffix);

/* Print a human-readable interpretation of a data format descriptor. */
void printDFD(uint32_t *DFD);

#ifdef __cplusplus
}
#endif

#endif /* _DFD_H_ */