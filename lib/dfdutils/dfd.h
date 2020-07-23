/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

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

/**
 * @file
 * @~English
 * @brief Header file defining the data format descriptor utilities API.
 */

/*
 * Author: Andrew Garrard
 */

#ifndef _DFD_H_
#define _DFD_H_

#include <KHR/khr_df.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Qualifier suffix to the format, in Vulkan terms. */
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

/** Compression scheme, in Vulkan terms. */
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
    c_ASTC,          /*!< ASTC. */
    c_ETC1S,         /*!< ETC1S. */
    c_PVRTC,         /*!< PVRTC(1). */
    c_PVRTC2         /*!< PVRTC2. */
};

typedef unsigned int uint32_t;

#if !defined(LIBKTX)
#include <vulkan/vulkan_core.h>
#else
#include "../vkformat_enum.h"
#endif

uint32_t* vk2dfd(enum VkFormat format);

/* Create a Data Format Descriptor for an unpacked format. */
uint32_t *createDFDUnpacked(int bigEndian, int numChannels, int bytes,
                            int redBlueSwap, enum VkSuffix suffix);

/* Create a Data Format Descriptor for a packed format. */
uint32_t *createDFDPacked(int bigEndian, int numChannels,
                          int bits[], int channels[],
                          enum VkSuffix suffix);

/* Create a Data Format Descriptor for a compressed format. */
uint32_t *createDFDCompressed(enum VkCompScheme compScheme,
                              int bwidth, int bheight, int bdepth,
                              enum VkSuffix suffix);

/* Create a Data Format Descriptor for a depth/stencil format. */
uint32_t *createDFDDepthStencil(int depthBits,
                                int stencilBits,
                                int sizeBytes);

/** Result of interpreting the data format descriptor. */
enum InterpretDFDResult {
    i_LITTLE_ENDIAN_FORMAT_BIT = 0, /*!< Confirmed little-endian (default for 8bpc). */
    i_BIG_ENDIAN_FORMAT_BIT = 1,    /*!< Confirmed big-endian. */
    i_PACKED_FORMAT_BIT = 2,        /*!< Packed format. */
    i_SRGB_FORMAT_BIT = 4,          /*!< sRGB transfer function. */
    i_NORMALIZED_FORMAT_BIT = 8,    /*!< Normalized (UNORM or SNORM). */
    i_SIGNED_FORMAT_BIT = 16,       /*!< Format is signed. */
    i_FLOAT_FORMAT_BIT = 32,        /*!< Format is floating point. */
    i_UNSUPPORTED_ERROR_BIT = 64,   /*!< Format not successfully interpreted. */
    /** "NONTRIVIAL_ENDIANNESS" means not big-endian, not little-endian
     * (a channel has bits that are not consecutive in either order). **/
    i_UNSUPPORTED_NONTRIVIAL_ENDIANNESS     = i_UNSUPPORTED_ERROR_BIT,
    /** "MULTIPLE_SAMPLE_LOCATIONS" is an error because only single-sample
     * texel blocks (with coordinates 0,0,0,0 for all samples) are supported. **/
    i_UNSUPPORTED_MULTIPLE_SAMPLE_LOCATIONS = i_UNSUPPORTED_ERROR_BIT + 1,
    /** "MULTIPLE_PLANES" is an error because only contiguous data is supported. */
    i_UNSUPPORTED_MULTIPLE_PLANES           = i_UNSUPPORTED_ERROR_BIT + 2,
    /** Only channels R, G, B and A are supported. */
    i_UNSUPPORTED_CHANNEL_TYPES             = i_UNSUPPORTED_ERROR_BIT + 3,
    /** Only channels with the same flags are supported
     * (e.g. we don't support float red with integer green). */
    i_UNSUPPORTED_MIXED_CHANNELS            = i_UNSUPPORTED_ERROR_BIT + 4
};

/** Interpretation of a channel from the data format descriptor. */
typedef struct _InterpretedDFDChannel {
    uint32_t offset; /*!< Offset in bits for packed, bytes for unpacked. */
    uint32_t size;   /*!< Size in bits for packed, bytes for unpacked. */
} InterpretedDFDChannel;

/* Interpret a Data Format Descriptor. */
enum InterpretDFDResult interpretDFD(const uint32_t *DFD,
                                     InterpretedDFDChannel *R,
                                     InterpretedDFDChannel *G,
                                     InterpretedDFDChannel *B,
                                     InterpretedDFDChannel *A,
                                     uint32_t *wordBytes);

/* Print a human-readable interpretation of a data format descriptor. */
void printDFD(uint32_t *DFD);

/* Get the number of components & component size from a DFD for an
 * unpacked format.
 */
void
getDFDComponentInfoUnpacked(const uint32_t* DFD, uint32_t* numComponents,
                            uint32_t* componentByteLength);

/* Return the number of components described by a DFD. */
uint32_t getDFDNumComponents(const uint32_t* DFD);

typedef struct _Primaries {
    float Rx;
    float Ry;
    float Gx;
    float Gy;
    float Bx;
    float By;
    float Wx;
    float Wy;
} Primaries;

khr_df_primaries_e findMapping(Primaries *p, float latitude);

#ifdef __cplusplus
}
#endif

#endif /* _DFD_H_ */
