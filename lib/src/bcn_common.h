/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * Copyright 2026 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file
 * @~English
 *
 * @brief Utilities (structs, functions, macros, etc.) common to both; BCn
 * encoder and decoder.
 *
 * @author Walid Chtioui, independent contributor (walid.chtioui.main@gmail.com)
 */

#ifndef _BCN_CODEC_H_
#define _BCN_CODEC_H_

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include "ktx.h"
#include "transcoder/basisu_transcoder_internal.h"
#include "vulkan/vulkan_core.h"
#include <algorithm>

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

/* Since this is used to pass parameters/data to thread runners, make sure all
 * pointers point to heap-allocated resources (and, obviously, make sure that
 * the lifetimes of spawned threads do not exceed that of these resources). */
struct bcn_compression_workload {
    uint32_t width;
    uint32_t height;
    uint32_t nchannels;
    ktxBCnParams params;
    union {
        const uint8_t* ldr;
        const uint16_t* hdr;
    } data_in;
    uint8_t* data_out;
};

template <typename F>
inline F
lerp(F a, F b, F s) {
    return a + (b - a) * s;
}

inline uint32_t
get_bc1_compression_quality(ktx_pack_bcn_quality_levels bcn_quality) {
    switch (bcn_quality) {
    case ktx_pack_bcn_quality_levels_e::KTX_PACK_BCN_QUALITY_LEVEL_FASTEST:
        return 0u;
    case ktx_pack_bcn_quality_levels_e::KTX_PACK_BCN_QUALITY_LEVEL_FASTER:
        return 2U;
    case ktx_pack_bcn_quality_levels_e::KTX_PACK_BCN_QUALITY_LEVEL_FAST:
        return 5U;
    case ktx_pack_bcn_quality_levels_e::KTX_PACK_BCN_QUALITY_LEVEL_MEDIUM:
        return 10U;
    case ktx_pack_bcn_quality_levels_e::KTX_PACK_BCN_QUALITY_LEVEL_THOROUGH:
        return 15U;
    case ktx_pack_bcn_quality_levels_e::KTX_PACK_BCN_QUALITY_LEVEL_EXHAUSTIVE:
        return 19U;
    default:  // should never occur
        assert(false);
        return 15U;
    }
}

inline uint32_t
get_bc7_compression_quality(ktx_pack_bcn_quality_levels bcn_quality) {
    switch (bcn_quality) {
    case ktx_pack_bcn_quality_levels_e::KTX_PACK_BCN_QUALITY_LEVEL_FASTEST:
        return basist::bc7f::cPackBC7FlagDefaultFastest;
    case ktx_pack_bcn_quality_levels_e::KTX_PACK_BCN_QUALITY_LEVEL_FASTER:
        return basist::bc7f::cPackBC7FlagDefaultFaster;
    case ktx_pack_bcn_quality_levels_e::KTX_PACK_BCN_QUALITY_LEVEL_FAST:
        return basist::bc7f::cPackBC7FlagDefaultFast;
    case ktx_pack_bcn_quality_levels_e::KTX_PACK_BCN_QUALITY_LEVEL_MEDIUM:
        return basist::bc7f::cPackBC7FlagDefault;
    case ktx_pack_bcn_quality_levels_e::KTX_PACK_BCN_QUALITY_LEVEL_THOROUGH:
        return basist::bc7f::cPackBC7FlagDefaultPartiallyAnalytical;
    case ktx_pack_bcn_quality_levels_e::KTX_PACK_BCN_QUALITY_LEVEL_EXHAUSTIVE:
        return basist::bc7f::cPackBC7FlagDefaultNonAnalytical;
    default:  // should never occur
        assert(false);
        return basist::bc7f::cPackBC7FlagDefaultPartiallyAnalytical;
    }
}

//
// Extracts/Copies a [4 x 4 x nchannels] unpacked/uncompressed block of data
// from provided pSrc to provided pDst. For each row of the source block,
// performs a memcpy to the destination block while accounting for potential
// non-multiple-of-block-size destination dimensions.
//
// Source and destination SHOULD have the same stride (i.e., nchannels).
//
// This is the inverse (in terms of source/destination) of insert_block().
//
// In the following diagrams:
//   - '.': denotes input texture/image's texels/pixels
//   - '=': denotes texels/pixels that are copied from source texture/image
//   - 'x': denotes texels/pixels that are filled by clamp-to-edge method
//        (i.e., for each raw, repeat the last pixel).
//
// Source texture and block: (width x height x nchannels)
//
//    <----------- width ----------->
//    +-----------------------------+
//    | . . . . . . . . . . . . . . | <-- row: 0
//    | . . . . . . . . . . . . . . |
//    | . . . . . . . . . . . +-----|-----+ <- source block
//    | . . . . . . . . . . . | = = | x x |
//    | . . . . . . . . . . . | = = | x x |
//    | . . . . . . . . . . . | = = | x x |
//    | . . . . . . . . . . . | = = | x x |
//    | . . . . . . . . . . . +-----|-----+
//    | . . . . . . . . . . . . . . |
//    | . . . . . . . . . . . . . . | <-- row: height - 1
//    +-----------------------------+
//
// Destination block: (4 x 4 x nchannels)
//
//     block_size
//    +---------+
//    | = = x x | <-- row: 0
//    | = = x x |
//    | = = x x |
//    | = = x x | <-- row: block_size - 1
//    +---------+
//
template <typename T>
inline void
extract_block(T* dst, const T* src, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
              uint32_t nchannels /* stride */) {
    constexpr uint32_t kBlockSize = 4;
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
//
// This is the inverse (in terms of source/destination) of extract_block().
//
// In the following diagrams:
//   - '.': denotes untouched destination texture/image's texels/pixels.
//   - '=': denotes texels/pixels that are copied from source block and written
//          into destination texture.
//   - 'x': denotes block texels/pixels that are discarded/ignored.
//
// Source block: (4 x 4 x nchannels)
//
//     block_size
//    +---------+
//    | = = x x | <-- row: 0
//    | = = x x |
//    | x x x x |
//    | x x x x | <-- row: block_size - 1
//    +---------+
//
// Destination: (width x height x nchannels)
//
//    <----------- width ----------->
//    +-----------------------------+
//    | . . . . . . . . . . . . . . | <-- row: 0
//    | . . . . . . . . . . . . . . |
//    | . . . . . . . . . . . . . . |
//    | . . . . . . . . . . . . . . |
//    | . . . . . . . . . . . . . . |
//    | . . . . . . . . . . . +-----|-----+
//    | . . . . . . . . . . . | = = | x x |
//    | . . . . . . . . . . . | = = | x x | <-- row: height - 1
//    +-----------------------------+
//                            | x x | x x |
//                            | x x | x x |
//                            +-----+-----+
//
template <typename T>
inline uint32_t
insert_block(T* dst, const T* src, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
             uint32_t nchannels /* stride */) {
    constexpr uint32_t kBlockSize = 4;
    const uint32_t src_pitch = kBlockSize * nchannels;  // nbr bytes per raw of src
    const uint32_t dst_pitch = width * nchannels;       // nbr bytes per raw of dst
    uint32_t nbr_written_bytes = 0;
    const int cols = std::min(kBlockSize, width - x);  // nbr columns to copy from src
    const uint32_t nbr_bytes_to_write = cols * nchannels * sizeof(T);
    const T* pSrc = src;
    T* pDst = dst + y * dst_pitch + nchannels * x;
    for (uint32_t py = 0; py < kBlockSize && y + py < height; ++py) {
        memcpy(pDst, pSrc, nbr_bytes_to_write);
        nbr_written_bytes += nbr_bytes_to_write;
        pSrc += src_pitch;
        pDst += dst_pitch;
    }
    return nbr_written_bytes;
}

template <typename T>
inline void
extract_rgb_from_rgba_block(T* rgb, const T* rgba) {
    const uint32_t src_pitch = 4 * 4;
    const uint32_t dst_pitch = 4 * 3;
    [[maybe_unused]] uint32_t nbr_written_bytes_total = 0;
    for (uint32_t py = 0; py < 4; ++py) {
        for (uint32_t px = 0; px < 4; ++px) {
            memcpy(rgb + px * 3 + py * dst_pitch, rgba + px * 4 + py * src_pitch, 3 * sizeof(T));
            nbr_written_bytes_total += 3 * sizeof(T);
        }
    }
    assert(nbr_written_bytes_total == 4 * 4 * 3 * sizeof(T));
}

inline void
rgb_to_rgba_block(uint8_t* rgba, const uint8_t* rgb, uint8_t alpha = 255) {
    const uint32_t src_pitch = 4 * 3; /* 4 x 3 */
    const uint32_t dst_pitch = 4 * 4; /* because we add alpha */
    [[maybe_unused]] uint32_t nbr_written_bytes_total = 0;
    for (uint32_t py = 0; py < 4; ++py) {
        for (uint32_t px = 0; px < 4; ++px) {
            uint8_t* pDst = rgba + px * 4 + py * dst_pitch;
            memcpy(pDst, rgb + px * 3 + py * src_pitch, 3);
            pDst[3] = alpha;
            nbr_written_bytes_total += 4;
        }
    }
    assert(nbr_written_bytes_total == 4 * 4 * 4);
}

// Returns BCn compression kind from given VkFormat or KTX_BCN_COMPRESSION_NONE
// in case given VkFormat is not a BCn format.
inline ktx_bcn_compression_e
get_bcn_compression_kind(VkFormat vkformat) {
    switch (vkformat) {
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        return KTX_BCN_COMPRESSION_BC1;
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        return KTX_BCN_COMPRESSION_BC1A;
    case VK_FORMAT_BC2_UNORM_BLOCK:
    case VK_FORMAT_BC2_SRGB_BLOCK:
        return KTX_BCN_COMPRESSION_BC2;
    case VK_FORMAT_BC3_UNORM_BLOCK:
    case VK_FORMAT_BC3_SRGB_BLOCK:
        return KTX_BCN_COMPRESSION_BC3;
    case VK_FORMAT_BC4_UNORM_BLOCK:
    case VK_FORMAT_BC4_SNORM_BLOCK:
        return KTX_BCN_COMPRESSION_BC4;
    case VK_FORMAT_BC5_UNORM_BLOCK:
    case VK_FORMAT_BC5_SNORM_BLOCK:
        return KTX_BCN_COMPRESSION_BC5;
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
        return KTX_BCN_COMPRESSION_BC6HU;
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        return KTX_BCN_COMPRESSION_BC6HS;
    case VK_FORMAT_BC7_UNORM_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK:
        return KTX_BCN_COMPRESSION_BC7;
    default:
        return KTX_BCN_COMPRESSION_NONE;
    }
}

inline VkFormat
get_bcn_decompressed_format(ktx_bcn_compression_e bcn, khr_df_transfer_e tf, VkFormat vkformat) {
    const bool is_srgb = tf == KHR_DF_TRANSFER_SRGB;
    switch (bcn) {
    case KTX_BCN_COMPRESSION_BC4:
        return vkformat == VK_FORMAT_BC4_UNORM_BLOCK ? VK_FORMAT_R8_UNORM : VK_FORMAT_R8_SNORM;
    case KTX_BCN_COMPRESSION_BC5:
        return vkformat == VK_FORMAT_BC5_UNORM_BLOCK ? VK_FORMAT_R8G8_UNORM : VK_FORMAT_R8G8_SNORM;
    case KTX_BCN_COMPRESSION_BC1:
        return is_srgb ? VK_FORMAT_R8G8B8_SRGB : VK_FORMAT_R8G8B8_UNORM;
    case KTX_BCN_COMPRESSION_BC1A:
    case KTX_BCN_COMPRESSION_BC2:
    case KTX_BCN_COMPRESSION_BC3:
    case KTX_BCN_COMPRESSION_BC7:
        return is_srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
    case KTX_BCN_COMPRESSION_BC6HU:
    case KTX_BCN_COMPRESSION_BC6HS:
        return VK_FORMAT_R16G16B16_SFLOAT;
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

// Returns number of channels for given BCn compression kind or 0 in case of
// KTX_BCN_COMPRESSION_NONE.
inline uint32_t
get_bcn_nchannels(ktx_bcn_compression_e bcn) {
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

// Returns corresponding KHR color model for the given BCn compression
// kind if BCn is not KTX_BCN_COMPRESSION_NONE otherwise returns
// KHR_DF_MODEL_UNSPECIFIED.
inline khr_df_model_e
get_bcn_colormodel(ktx_bcn_compression_e bcn) {
    switch (bcn) {
    case KTX_BCN_COMPRESSION_BC1:
    case KTX_BCN_COMPRESSION_BC1A:
        return KHR_DF_MODEL_BC1A;
    case KTX_BCN_COMPRESSION_BC2:
        return KHR_DF_MODEL_BC2;
    case KTX_BCN_COMPRESSION_BC3:
        return KHR_DF_MODEL_BC3;
    case KTX_BCN_COMPRESSION_BC4:
        return KHR_DF_MODEL_BC4;
    case KTX_BCN_COMPRESSION_BC5:
        return KHR_DF_MODEL_BC5;
    case KTX_BCN_COMPRESSION_BC6HU:
    case KTX_BCN_COMPRESSION_BC6HS:
        return KHR_DF_MODEL_BC6H;
    case KTX_BCN_COMPRESSION_BC7:
        return KHR_DF_MODEL_BC7;
    default:
        return KHR_DF_MODEL_UNSPECIFIED;
    }
}

inline void
set_default_bcn_params_fields(ktxBCnParams& params) {
    if (params.threadCount == 0) params.threadCount = 1;
    if (params.bcnRDOQualityScalar == 0) params.bcnRDOQualityScalar = 1.0f;
    if (params.bcnRDODictSize == 0) params.bcnRDODictSize = 4096;
    if (params.bcnRDOMaxSmoothBlockStdDev == 0) params.bcnRDOMaxSmoothBlockStdDev = 18.0f;
    if (params.bcnRDOMaxAllowedRMSIncreaseRatio == 0)
        params.bcnRDOMaxAllowedRMSIncreaseRatio = 10.0f;
}

#endif
