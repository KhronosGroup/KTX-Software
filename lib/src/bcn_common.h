/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * Copyright 2026 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BCN_CODEC_H_
#define _BCN_CODEC_H_

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include "ktx.h"
#include "bc7enc_rdo/rgbcx.h"
#include "vulkan/vulkan_core.h"

#define BCN_BLOCK_SIZE 4

#define BC1_BLOCK_SIZE 8
#define BC2_BLOCK_SIZE 16
#define BC3_BLOCK_SIZE 16
#define BC4_BLOCK_SIZE 8
#define BC5_BLOCK_SIZE 16
#define BC6H_BLOCK_SIZE 16
#define BC7_BLOCK_SIZE 16

#define BC1_NCHANNELS 3
#define BC1A_NCHANNELS 4
#define BC2_NCHANNELS 4
#define BC3_NCHANNELS 4
#define BC4_NCHANNELS 1
#define BC5_NCHANNELS 2
#define BC6H_NCHANNELS 3
#define BC7_NCHANNELS 4

struct unpack_block_bc1_user_data {
    ktx_bool_t allow_3color_mode;
    ktx_bool_t use_3color_mode_for_black;
    rgbcx::bc1_approx_mode bc1_approx_mode;
};

/* Since this is used to pass parameters/data to thread runners, make sure all
 * pointers point to heap-allocated resources (and, obviously, make sure that
 * the lifetimes of spawned threads do not exceed that of these resources). */
struct bcn_compression_workload {
    uint32_t width;
    uint32_t height;
    uint32_t nchannels;
    ktxBCnParams params;
    const uint8_t* data_in;
    uint8_t* data_out;
};

template <typename F>
inline F
lerp(F a, F b, F s) {
    return a + (b - a) * s;
}

//
// Extracts/Copies a [4 x 4 x nchannels] unpacked/uncompressed block of data
// from provided pSrc to provided pDst. For each row of the source block,
// performs a memcpy to the destination block while accounting for potential
// non-multiple-of-block-size destination dimensions.
//
// Source and destination SHOULD have the same stride (i.e., nchannels).
// Texles/Pixels noted by 'x' are filled by clamp-to-edge method (i.e., for each
// raw, repeat the last pixel).
//
// This is the inverse (in terms of source/destination) of insert_block().
//
// Source: (width x height x nchannels)
//
//  <-------------------- width ----------------->
//  +--------------------------------------------+
//  | pSrc    | ... | pSrc + src_pitch - 1       | <-- row: 0
//  |                                            |
//  |                                            |
//  | pSrc + y * src_pitch + x * nchannels -> +--|---+ <- source block
//  |                                         |  |xxx|
//  |                                         |  |xxx|
//  |                                         +--|---+
//  |                                            |
//  |                                            |
//  |                                            |
//  |                                            | <-- row: height - 1
//  +--------------------------------------------+
//
// Destination: (4 x 4 x nchannels)
//
//  <-------------- block size ------------->
//  +---------------------------------------+
//  | pDst + 0 | ... | pDst + dst_pitch - 1 | <-- row: 0
//  |                   xxxxxxxxxxxxxxxxxxx |
//  |          | ... |  xxxxxxxxxxxxxxxxxxx |
//  |                   xxxxxxxxxxxxxxxxxxx |
//  |          | ... |  xxxxxxxxxxxxxxxxxxx |
//  |                   xxxxxxxxxxxxxxxxxxx |
//  |          | ... |  xxxxxxxxxxxxxxxxxxx |
//  |                   xxxxxxxxxxxxxxxxxxx |
//  |          | ... |  xxxxxxxxxxxxxxxxxxx |
//  |                   xxxxxxxxxxxxxxxxxxx |
//  |          | ... |  xxxxxxxxxxxxxxxxxxx | <-- row: block_size - 1
//  +---------------------------------------+
//
//
template <typename T>
inline void
extract_block(T* dst, const T* src, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
              uint32_t nchannels /* stride */) {
    // TODO: expose this as parameter for usage with other block sizes
    constexpr uint32_t kBlockSize = BCN_BLOCK_SIZE;
    const uint32_t src_pitch = width * nchannels;           // nbr bytes per raw of src
    const uint32_t dst_pitch = kBlockSize * nchannels;      // nbr bytes per raw of dst
    const uint32_t cols = std::min(kBlockSize, width - x);  // nbr columns to copy from src

    // nbr remaining columns that were not copied from src and should be
    // clamp-to-edge-x generated for dst
    const uint32_t remaining_cols = kBlockSize - cols;

    // nbr remaining raws that were not copied from src and should be
    // clamp-to-edge-y generated for src
    const uint32_t remaining_raws = (uint32_t)std::max<int>(0, (int)(y + kBlockSize) - (int)height);

    const T* pSrc = src + y * src_pitch + x * nchannels;
    T* pDst = dst;

    for (uint32_t py = 0; py < kBlockSize && y + py < height; ++py) {
        memcpy(pDst, pSrc, cols * nchannels * sizeof(T));
        // Add padding for this raw (it needed) - CLAMP_TO_EDGE_X
        for (uint32_t i = 0; i < remaining_cols; ++i) {
            memcpy(pDst + (cols + i) * nchannels, pSrc + (cols - 1) * nchannels,
                   nchannels * sizeof(T));
        }
        pSrc += src_pitch;
        pDst += dst_pitch;
    }

    // Add padding raws (if needed) CLAMP_TO_EDGE_Y
    const T* pDstLastRaw = dst + (kBlockSize - remaining_raws - 1) * dst_pitch;
    for (uint32_t py = 0; py < remaining_raws; ++py) {
        memcpy(pDst, pDstLastRaw, dst_pitch * sizeof(T));
        pDst += dst_pitch;
    }
}

//
// Inserts/Copies a [4 x 4 x nchannels] unpacked/uncompressed block from
// provided pSrc to provided pDst. For each row of the source block, performs a
// memcpy to the destination block while accounting for potential
// non-multiple-of-block-size destination dimensions.
//
// Source and destination SHOULD have the same stride (i.e., nchannels).
// Texles/Pixels noted by 'x' are discarded (i.e., not copied).
//
// This is the inverse (in terms of source/destination) of extract_block().
//
// Source: (4 x 4 x nchannels)
//
//  <-------------- block size ------------->
//  +---------------------------------------+
//  | pSrc + 0 | ... | pSrc + src_pitch - 1 | <-- row: 0
//  |                   xxxxxxxxxxxxxxxxxxx |
//  |          | ... |  xxxxxxxxxxxxxxxxxxx |
//  |                   xxxxxxxxxxxxxxxxxxx |
//  |          | ... |  xxxxxxxxxxxxxxxxxxx |
//  |                   xxxxxxxxxxxxxxxxxxx |
//  |          | ... |  xxxxxxxxxxxxxxxxxxx |
//  |                   xxxxxxxxxxxxxxxxxxx |
//  |          | ... |  xxxxxxxxxxxxxxxxxxx |
//  |                   xxxxxxxxxxxxxxxxxxx |
//  |          | ... |  xxxxxxxxxxxxxxxxxxx | <-- row: block_size - 1
//  +---------------------------------------+
//
// Destination: (width x height x nchannels)
//
//  <-------------------- width ----------------->
//  +--------------------------------------------+
//  | pDst    | ... | pDst + dst_pitch - 1       | <-- row: 0
//  |                                            |
//  |                                            |
//  | pDst + y * dst_pitch + x * nchannels -> +--|---+ <- source block
//  |                                         |  |xxx|
//  |                                         |  |xxx|
//  |                                         +--|---+
//  |                                            |
//  |                                            |
//  |                                            |
//  |                                            | <-- row: height - 1
//  +--------------------------------------------+
//
template <typename T>
inline uint32_t
insert_block(T* dst, const T* src, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
             uint32_t nchannels /* stride */) {
    constexpr uint32_t kBlockSize = BCN_BLOCK_SIZE;
    const uint32_t src_pitch = kBlockSize * nchannels;  // nbr bytes per raw of src
    const uint32_t dst_pitch = width * nchannels;       // nbr bytes per raw of dst
    uint32_t nbr_written_bytes = 0;
    const int cols = std::min(kBlockSize, width - x);  // nbr columns to copy from src
    const T* pSrc = src;
    T* pDst = dst + y * dst_pitch + nchannels * x;
    for (uint32_t py = 0; py < kBlockSize && y + py < height; ++py) {
        const uint32_t nbr_bytes_to_write = cols * nchannels * sizeof(T);
        memcpy(pDst, pSrc, nbr_bytes_to_write);
        nbr_written_bytes += nbr_bytes_to_write;
        pSrc += src_pitch;
        pDst += dst_pitch;
    }
    return nbr_written_bytes;
}

inline void
extract_rgb_from_rgba_block(uint8_t* rgb, const uint8_t* rgba) {
    const uint32_t src_pitch = BCN_BLOCK_SIZE * 4;
    const uint32_t dst_pitch = BCN_BLOCK_SIZE * 3;
    [[maybe_unused]] uint32_t nbr_written_bytes_total = 0;
    for (uint32_t py = 0; py < BCN_BLOCK_SIZE; ++py) {
        for (uint32_t px = 0; px < BCN_BLOCK_SIZE; ++px) {
            memcpy(rgb + px * 3 + py * dst_pitch, rgba + px * 4 + py * src_pitch, 3);
            nbr_written_bytes_total += 3;
        }
    }
    assert(nbr_written_bytes_total == (BCN_BLOCK_SIZE * BCN_BLOCK_SIZE * 3));
}

inline void
rgb_to_rgba_block(uint8_t* rgba, const uint8_t* rgb, uint8_t alpha = 255) {
    const uint32_t src_pitch = BCN_BLOCK_SIZE * 3; /* 4 x 3 */
    const uint32_t dst_pitch = BCN_BLOCK_SIZE * 4; /* because we add alpha */
    [[maybe_unused]] uint32_t nbr_written_bytes_total = 0;
    for (uint32_t py = 0; py < BCN_BLOCK_SIZE; ++py) {
        for (uint32_t px = 0; px < BCN_BLOCK_SIZE; ++px) {
            uint8_t* pDst = rgba + px * 4 + py * dst_pitch;
            memcpy(pDst, rgb + px * 3 + py * src_pitch, 3);
            pDst[3] = alpha;
            nbr_written_bytes_total += 4;
        }
    }
    assert(nbr_written_bytes_total == (BCN_BLOCK_SIZE * BCN_BLOCK_SIZE * 4));
}

inline ktx_bcn_compression_e
get_bcn_compression_kind(VkFormat vkformat, VkFormat& decompressed_vkformat, int& nchannels) {
    switch (vkformat) {
        /* BC1 */
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        decompressed_vkformat = VK_FORMAT_R8G8B8_UNORM;
        nchannels = BC1_NCHANNELS; /* 3 */
        return KTX_BCN_COMPRESSION_BC1;

    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        decompressed_vkformat = VK_FORMAT_R8G8B8_SRGB;
        nchannels = BC1_NCHANNELS; /* 3 */
        return KTX_BCN_COMPRESSION_BC1;

        /* BC1A */
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        decompressed_vkformat = VK_FORMAT_R8G8B8A8_UNORM;
        nchannels = BC1A_NCHANNELS; /* 4 */
        return KTX_BCN_COMPRESSION_BC1A;

    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        decompressed_vkformat = VK_FORMAT_R8G8B8A8_SRGB;
        nchannels = BC1A_NCHANNELS; /* 4 */
        return KTX_BCN_COMPRESSION_BC1A;

        /* BC2 */
    case VK_FORMAT_BC2_UNORM_BLOCK:
        decompressed_vkformat = VK_FORMAT_R8G8B8A8_UNORM;
        nchannels = BC2_NCHANNELS; /* 4 */
        return KTX_BCN_COMPRESSION_BC2;

    case VK_FORMAT_BC2_SRGB_BLOCK:
        decompressed_vkformat = VK_FORMAT_R8G8B8A8_SRGB;
        nchannels = BC2_NCHANNELS; /* 4 */
        return KTX_BCN_COMPRESSION_BC2;

        /* BC3 */
    case VK_FORMAT_BC3_UNORM_BLOCK:
        decompressed_vkformat = VK_FORMAT_R8G8B8A8_UNORM;
        nchannels = BC3_NCHANNELS; /* 4 */
        return KTX_BCN_COMPRESSION_BC3;

    case VK_FORMAT_BC3_SRGB_BLOCK:
        decompressed_vkformat = VK_FORMAT_R8G8B8A8_SRGB;
        nchannels = BC3_NCHANNELS; /* 4 */
        return KTX_BCN_COMPRESSION_BC3;

        /* BC4 */
    case VK_FORMAT_BC4_UNORM_BLOCK:
        decompressed_vkformat = VK_FORMAT_R8_UNORM;
        nchannels = BC4_NCHANNELS; /* 1 */
        return KTX_BCN_COMPRESSION_BC4;

    case VK_FORMAT_BC4_SNORM_BLOCK:
        decompressed_vkformat = VK_FORMAT_R8_SNORM;
        nchannels = BC4_NCHANNELS; /* 1 */
        return KTX_BCN_COMPRESSION_BC4;

        /* BC5 */
    case VK_FORMAT_BC5_UNORM_BLOCK:
        decompressed_vkformat = VK_FORMAT_R8G8_UNORM;
        nchannels = BC5_NCHANNELS; /* 2 */
        return KTX_BCN_COMPRESSION_BC5;

    case VK_FORMAT_BC5_SNORM_BLOCK:
        decompressed_vkformat = VK_FORMAT_R8G8_SNORM;
        nchannels = BC5_NCHANNELS; /* 2 */
        return KTX_BCN_COMPRESSION_BC5;

        /* BC6HU */
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        decompressed_vkformat = VK_FORMAT_R16G16B16_SFLOAT;
        nchannels = BC6H_NCHANNELS; /* 3 */
        return KTX_BCN_COMPRESSION_BC6HU;

        /* BC6HS */
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        decompressed_vkformat = VK_FORMAT_R16G16B16_SFLOAT;
        nchannels = BC6H_NCHANNELS; /* 3 */
        return KTX_BCN_COMPRESSION_BC6HS;

        /* BC7 */
    case VK_FORMAT_BC7_UNORM_BLOCK:
        decompressed_vkformat = VK_FORMAT_R8G8B8A8_UNORM;
        nchannels = BC7_NCHANNELS; /* 4 */
        return KTX_BCN_COMPRESSION_BC7;

    case VK_FORMAT_BC7_SRGB_BLOCK:
        decompressed_vkformat = VK_FORMAT_R8G8B8A8_SRGB;
        nchannels = BC7_NCHANNELS; /* 4 */
        return KTX_BCN_COMPRESSION_BC7;

    default:
        decompressed_vkformat = VK_FORMAT_UNDEFINED;
        nchannels = -1;
        return KTX_BCN_COMPRESSION_NONE;
    }
}

inline uint32_t
get_nchannels(ktx_bcn_compression_e bcn) {
    switch (bcn) {
    case KTX_BCN_COMPRESSION_BC4:
        return 1;
    case KTX_BCN_COMPRESSION_BC5:
        return 2;
    case KTX_BCN_COMPRESSION_BC1:
    case KTX_BCN_COMPRESSION_BC6HU:
    case KTX_BCN_COMPRESSION_BC6HS:
        return 3;
    case KTX_BCN_COMPRESSION_BC1A:
    case KTX_BCN_COMPRESSION_BC2:
    case KTX_BCN_COMPRESSION_BC3:
    case KTX_BCN_COMPRESSION_BC7:
        return 4;
    default:
        return 0;
    }
}

#endif
