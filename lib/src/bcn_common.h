/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * Copyright 2026 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BCN_CODEC_H_
#define _BCN_CODEC_H_

#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include "KHR/khr_df.h"
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

struct unpack_block_bc1_user_data {
    ktx_bool_t allow_3color_mode;
    ktx_bool_t use_3color_mode_for_black;
    ktx_bc1_approx_mode_e bc1_approx_mode;
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

struct rdo_params {
    ert::reduce_entropy_params ert_p;
    /* Whether to automatically compute the maximum MSE scale factor for
     * smooth/flat blocks. */
    ktx_bool_t auto_smooth_block_max_mse_scale;
    unpack_block_bc1_user_data bc1_params;
};

/* Since this is used to pass parameters/data to thread runners, make sure all
 * pointers point to heap-allocated resources (and, obviously, make sure that
 * the lifetimes of spawned threads do not exceed that of these resources). */
struct rdo_bc1_workload {
    uint32_t m_width;
    uint32_t m_height;
    /* [in,out] packed/encoded image blocks */
    uint8_t* m_packed_img;
    /* [in,out] BC1 RGBX unpacked/decoded blocks (x == ignored) */
    const ert::color_rgba* m_unpacked_img_rgbx;
    /* [in] BC1 RGB blocks entropy reduction params */
    ert::reduce_entropy_params m_ert_p_rgb;
    /* [out] BC1 total number smooth RGB blocks  */
    std::atomic_int32_t m_total_smooth_blocks_rgb;
    /* [out] BC1 total number second matches for RGB blocks */
    std::atomic_int32_t m_total_second_matches_rgb;
    /* [out] BC1 total number total number modified packed/encoded RGB blocks */
    std::atomic_int32_t m_total_modified_rgb;
    /* [out] false if any thread fails */
    bool m_success;
    unpack_block_bc1_user_data m_bc1_params;

    rdo_bc1_workload() = delete;
    rdo_bc1_workload(uint32_t width, uint32_t height, uint8_t* packed_img,
                     const ert::color_rgba* unpacked_img_rgbx, ert::reduce_entropy_params ert_p_rgb,
                     unpack_block_bc1_user_data bc1_params)
        : m_width{width},
          m_height{height},
          m_packed_img{packed_img},
          m_unpacked_img_rgbx{unpacked_img_rgbx},
          m_ert_p_rgb{ert_p_rgb},
          m_bc1_params{bc1_params} {
        assert(m_packed_img != nullptr);
        assert(m_unpacked_img_rgbx != nullptr);
        assert(m_ert_p_rgb.m_color_weights[3] == 0);
        m_total_smooth_blocks_rgb = 0;
        m_total_second_matches_rgb = 0;
        m_total_modified_rgb = 0;
        m_success = true;
    }
};

struct rdo_bc3_workload {
    uint32_t m_width;
    uint32_t m_height;
    /* [in,out] packed/encoded image blocks */
    uint8_t* m_packed_img;
    /* [in,out] BC1 RGBX unpacked/decoded blocks (x == ignored) */
    const ert::color_rgba* m_unpacked_img_rgbx;
    /* [in,out] BC4 AXXX unpacked/decoded blocks (x == ignored) */
    const ert::color_rgba* m_unpacked_img_axxx;
    /* [in] BC1 RGB blocks entropy reduction params */
    ert::reduce_entropy_params m_ert_p_rgb;
    /* [in] BC4 A blocks entropy reduction params */
    ert::reduce_entropy_params m_ert_p_a;
    /* [out] BC1 total number smooth RGB blocks */
    std::atomic_int32_t m_total_smooth_blocks_rgb;
    /* [out] BC4 total number smooth A blocks */
    std::atomic_int32_t m_total_smooth_blocks_a;
    /* [out] BC1 total number second matches for RGB blocks */
    std::atomic_int32_t m_total_second_matches_rgb;
    /* [out] BC4 total number second matches for A blocks */
    std::atomic_int32_t m_total_second_matches_a;
    /* [out] BC1 total number total number modified packed/encoded RGB blocks */
    std::atomic_int32_t m_total_modified_rgb;
    /* [out] BC4 total number total number modified packed/encoded A blocks */
    std::atomic_int32_t m_total_modified_a;
    /* [out] false if any thread fails */
    bool m_success;
    unpack_block_bc1_user_data m_bc1_params;

    rdo_bc3_workload() = delete;
    rdo_bc3_workload(uint32_t width, uint32_t height, uint8_t* packed_img,
                     const ert::color_rgba* unpacked_img_rgbx,
                     const ert::color_rgba* unpacked_img_axxx, ert::reduce_entropy_params ert_p_rgb,
                     ert::reduce_entropy_params ert_p_a, unpack_block_bc1_user_data bc1_params)
        : m_width{width},
          m_height{height},
          m_packed_img{packed_img},
          m_unpacked_img_rgbx{unpacked_img_rgbx},
          m_unpacked_img_axxx{unpacked_img_axxx},
          m_ert_p_rgb{ert_p_rgb},
          m_ert_p_a{ert_p_a},
          m_bc1_params{bc1_params} {
        assert(m_packed_img != nullptr);
        assert(m_unpacked_img_rgbx != nullptr);
        assert(m_unpacked_img_axxx != nullptr);
        assert(m_ert_p_rgb.m_color_weights[3] == 0);
        assert(m_ert_p_a.m_color_weights[1] == 0);
        assert(m_ert_p_a.m_color_weights[2] == 0);
        assert(m_ert_p_a.m_color_weights[3] == 0);
        m_total_smooth_blocks_rgb = 0;
        m_total_smooth_blocks_a = 0;
        m_total_second_matches_rgb = 0;
        m_total_second_matches_a = 0;
        m_total_modified_rgb = 0;
        m_total_modified_a = 0;
        m_success = true;
    }
};

struct rdo_bc4_workload {
    uint32_t m_width;
    uint32_t m_height;
    /* [in,out] BC4 packed/encoded image blocks */
    uint8_t* m_packed_img;
    /* [in,out] BC4 RXXX unpacked/decoded blocks (x == ignored) */
    const ert::color_rgba* m_unpacked_img_rxxx;
    /* [in] BC4 R blocks entropy reduction params */
    ert::reduce_entropy_params m_ert_p_r;
    /* [out] BC4 total number smooth R blocks */
    std::atomic_int32_t m_total_smooth_blocks_r;
    /* [out] BC4 total number smooth G blocks */
    std::atomic_int32_t m_total_second_matches_r;
    /* [out] BC4 total number second matches for G blocks */
    std::atomic_int32_t m_total_modified_r;
    /* [out] false if any thread fails */
    bool m_success;

    rdo_bc4_workload() = delete;
    rdo_bc4_workload(uint32_t width, uint32_t height, uint8_t* packed_img,
                     const ert::color_rgba* unpacked_img_rxxx,
                     const ert::reduce_entropy_params ert_p_r)
        : m_width{width},
          m_height{height},
          m_packed_img{packed_img},
          m_unpacked_img_rxxx{unpacked_img_rxxx},
          m_ert_p_r{ert_p_r} {
        assert(m_packed_img != nullptr);
        assert(m_unpacked_img_rxxx != nullptr);
        assert(m_ert_p_r.m_color_weights[1] == 0);
        assert(m_ert_p_r.m_color_weights[2] == 0);
        assert(m_ert_p_r.m_color_weights[3] == 0);
        m_total_smooth_blocks_r = 0;
        m_total_second_matches_r = 0;
        m_total_modified_r = 0;
        m_success = true;
    }
};

struct rdo_bc5_workload {
    uint32_t m_width;
    uint32_t m_height;
    /* [in,out] BC5 packed/encoded image blocks */
    uint8_t* m_packed_img;
    /* [in,out] BC4 RXXX unpacked/decoded blocks (x == ignored) */
    const ert::color_rgba* m_unpacked_img_rxxx;
    /* [in,out] BC4 GXXX unpacked/decoded blocks (x == ignored) */
    const ert::color_rgba* m_unpacked_img_gxxx;
    /* [in] BC4 R blocks entropy reduction params */
    ert::reduce_entropy_params m_ert_p_r;
    /* [in] BC4 G blocks entropy reduction params */
    ert::reduce_entropy_params m_ert_p_g;
    /* [out] BC4 total number smooth R blocks  */
    std::atomic_int32_t m_total_smooth_blocks_r;
    /* [out] BC4 total number smooth G blocks  */
    std::atomic_int32_t m_total_smooth_blocks_g;
    /* [out] BC4 total number second matches for R blocks */
    std::atomic_int32_t m_total_second_matches_r;
    /* [out] BC4 total number second matches for G blocks */
    std::atomic_int32_t m_total_second_matches_g;
    /* [out] BC4 total number total number modified packed/encoded R blocks */
    std::atomic_int32_t m_total_modified_r;
    /* [out] BC4 total number total number modified packed/encoded G blocks */
    std::atomic_int32_t m_total_modified_g;
    /* [out] false if any thread fails */
    bool m_success;

    rdo_bc5_workload() = delete;
    rdo_bc5_workload(uint32_t width, uint32_t height, uint8_t* packed_img,
                     const ert::color_rgba* unpacked_img_rxxx,
                     const ert::color_rgba* unpacked_img_gxxx, ert::reduce_entropy_params ert_p_r,
                     ert::reduce_entropy_params ert_p_g)
        : m_width{width},
          m_height{height},
          m_packed_img{packed_img},
          m_unpacked_img_rxxx{unpacked_img_rxxx},
          m_unpacked_img_gxxx{unpacked_img_gxxx},
          m_ert_p_r{ert_p_r},
          m_ert_p_g{ert_p_g} {
        assert(m_packed_img != nullptr);
        assert(m_unpacked_img_rxxx != nullptr);
        assert(m_unpacked_img_gxxx != nullptr);
        assert(m_ert_p_r.m_color_weights[3] == 0);
        assert(m_ert_p_g.m_color_weights[1] == 0);
        assert(m_ert_p_g.m_color_weights[2] == 0);
        assert(m_ert_p_g.m_color_weights[3] == 0);
        m_total_smooth_blocks_r = 0;
        m_total_smooth_blocks_g = 0;
        m_total_second_matches_r = 0;
        m_total_second_matches_g = 0;
        m_total_modified_r = 0;
        m_total_modified_g = 0;
        m_success = true;
    }
};

struct rdo_bc7_workload {
    uint32_t m_width;
    uint32_t m_height;
    /* [in,out] packed/encoded image blocks */
    uint8_t* m_packed_img;
    /* [in,out] BC7 RGBA unpacked/decoded blocks */
    const ert::color_rgba* m_unpacked_img_rgba;
    /* [in] BC7 RGBA blocks entropy reduction params  */
    ert::reduce_entropy_params m_ert_p_rgba;
    /* [out] BC7 total number smooth RGBA blocks  */
    std::atomic_int32_t m_total_smooth_blocks_rgba;
    /* [out] BC7 total number second matches for RGBA blocks */
    std::atomic_int32_t m_total_second_matches_rgba;
    /* [out] BC7 total number total number modified packed/encoded RGBA blocks */
    std::atomic_int32_t m_total_modified_rgba;
    /* [out] false if any thread fails */
    bool m_success;

    rdo_bc7_workload() = delete;
    rdo_bc7_workload(uint32_t width, uint32_t height, uint8_t* packed_img,
                     const ert::color_rgba* unpacked_img_rgba, ert::reduce_entropy_params ert_p)
        : m_width{width},
          m_height{height},
          m_packed_img{packed_img},
          m_unpacked_img_rgba{unpacked_img_rgba},
          m_ert_p_rgba{ert_p} {
        assert(m_packed_img != nullptr);
        assert(m_unpacked_img_rgba != nullptr);
        m_total_smooth_blocks_rgba = 0;
        m_total_second_matches_rgba = 0;
        m_total_modified_rgba = 0;
        m_success = true;
    }
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
inline void
insert_block(uint8_t* dst, const uint8_t* src, size_t x, size_t y, size_t width, size_t height,
             size_t nchannels /* stride */) {
    // TODO: expose this as parameter for usage with other block sizes
    constexpr size_t kBlockSize = BCN_BLOCK_SIZE;
    const size_t src_pitch = kBlockSize * nchannels;   // nbr bytes per raw of src
    const size_t dst_pitch = width * nchannels;        // nbr bytes per raw of dst
    const int cols = std::min(kBlockSize, width - x);  // nbr columns to copy from src
    const uint8_t* pSrc = src;
    uint8_t* pDst = dst + y * dst_pitch + nchannels * x;
    for (ktx_size_t py{0}; py < kBlockSize && y + py < height; ++py) {
        memcpy(pDst, pSrc, cols * nchannels);
        pSrc += src_pitch;
        pDst += dst_pitch;
    }
}

// TODO: this is meant to be called within a C++ context only (no C support) so
// maybe improve this? (use spans?)
KTX_error_code postprocess_rdo_bcn(const ktx_uint8_t* unpacked_img, ktx_size_t unpacked_img_size,
                                   ktx_uint8_t* packed_img, ktx_size_t packed_img_size,
                                   rdo_params params, khr_df_model_e model, ktx_uint32_t width,
                                   ktx_uint32_t height);

#endif
