/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * Copyright 2026 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BCN_CODEC_H_
#define _BCN_CODEC_H_

#include <cmath>
#include <cstdint>
#include <cstring>
#include "bc7enc_rdo/ert.h"
#include "ktx.h"
#define BCN_BLOCK_SIZE 4

#define BC1_BLOCK_SIZE 8
#define BC2_BLOCK_SIZE 16
#define BC3_BLOCK_SIZE 16
#define BC4_BLOCK_SIZE 8
#define BC5_BLOCK_SIZE 16
#define BC6H_BLOCK_SIZE 16
#define BC7_BLOCK_SIZE 16

#define BC1_NCHANNELS 4
#define BC3_NCHANNELS 4
#define BC4_NCHANNELS 1
#define BC5_NCHANNELS 2
#define BC6H_NCHANNELS 4
#define BC7_NCHANNELS 4

struct rdoParams {
    ert::reduce_entropy_params ert_p;
    // Whether to automatically compute the maximum MSE scale factor for
    // smooth/flat blocks.

    ktx_bool_t auto_smooth_block_max_mse_scale;
    ktx_bool_t allow_3color_mode;
    ktx_bool_t use_3color_mode_for_black;
    ktx_bc1_approx_mode_e bc1_approx_mode;
};

template <typename F>
inline F
lerp(F a, F b, F s) {
    return a + (b - a) * s;
}

//
// Extracts/Copies a [4 x 4 x nchannels] unpacked/uncompressed block of data from
// provided pSrc to provided pDst. For each row of the source block, performs a
// memcpy to the destination block while accounting for potential
// non-multiple-of-block-size destination dimensions.
//
// Source and destination SHOULD have the same stride (i.e., nchannels).
//
// Source: (width x height x nchannels)
//
//  <-------------------- width ----------------->
//  +--------------------------------------------+
//  | pSrc    | ... | pSrc + src_pitch - 1       | <-- row: 0
//  |                                            |
//  |                                            |
//  | pSrc + y * src_pitch + x * nchannels -> +--|---+ <- source block
//  |                                         |  |XXX|
//  |                                         |  |XXX|
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
//  |                   XXXXXXXXXXXXXXXXXXX |
//  |          | ... |  XXXXXXXXXXXXXXXXXXX |
//  |                   XXXXXXXXXXXXXXXXXXX |
//  |          | ... |  XXXXXXXXXXXXXXXXXXX |
//  |                   XXXXXXXXXXXXXXXXXXX |
//  |          | ... |  XXXXXXXXXXXXXXXXXXX |
//  |                   XXXXXXXXXXXXXXXXXXX |
//  |          | ... |  XXXXXXXXXXXXXXXXXXX |
//  |                   XXXXXXXXXXXXXXXXXXX |
//  |          | ... |  XXXXXXXXXXXXXXXXXXX | <-- row: block_size - 1
//  +---------------------------------------+
//
//
inline void
extract_block(uint8_t* dst, const uint8_t* src, size_t x, size_t y, size_t width, size_t height,
              size_t nchannels /* stride */) {
    // TODO: expose this as parameter for usage with other block sizes
    constexpr size_t kBlockSize = BCN_BLOCK_SIZE;
    const size_t src_pitch = width * nchannels;        // nbr bytes per raw of src
    const size_t dst_pitch = kBlockSize * nchannels;   // nbr bytes per raw of dst
    const int cols = std::min(kBlockSize, width - x);  // nbr columns to copy from src

    // nbr remaining columns that were not copied from src and should be
    // clamp-to-edge-x generated for dst
    const int remaining_cols = kBlockSize - cols;

    // nbr remaining raws that were not copied from src and should be
    // clamp-to-edge-y generated for src
    const int remaining_raws = std::max((size_t)0, (y + kBlockSize) - height);

    const uint8_t* pSrc = src + y * src_pitch + x * nchannels;
    uint8_t* pDst = dst;

    for (size_t py{0}; py < kBlockSize && y + py < height; ++py) {
        memcpy(pDst, pSrc, cols * nchannels);
        // Add padding for this raw (it needed) - CLAMP_TO_EDGE_X
        for (int i = 0; i < remaining_cols; ++i) {
            memcpy(pDst + (cols + i) * nchannels, pSrc + (cols - 1) * nchannels, nchannels);
        }
        pSrc += src_pitch;
        pDst += dst_pitch;
    }

    // Add padding raws (if needed) CLAMP_TO_EDGE_Y
    for (int py{0}; py < remaining_raws; ++py) {
        const uint8_t* pDstLastRaw = dst + (kBlockSize - remaining_raws) * dst_pitch;
        memcpy(pDst, pDstLastRaw, dst_pitch);
    }
}

// TODO: this is meant to be called within a C++ context only (no C support) so
// maybe improve this? (use spans?)
KTX_error_code postprocess_rdo_bcn(const ktx_uint8_t* unpacked_img, ktx_size_t unpacked_img_size,
                                   ktx_uint8_t* packed_img, ktx_size_t packed_img_size,
                                   rdoParams params, khr_df_model_e model, ktx_uint32_t width,
                                   ktx_uint32_t height);

#endif
