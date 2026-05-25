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
 * @brief Functions for encoding an uncompressed texture to a BCn format with an
 *        optional RDO post-processing step to significantly (circa 50% or more)
 *        reduce bit rate when supercompressed with Deflate (Zlib or ZSTD).
 *        Currently supported BCn formats are: BC1, BC3, BC4, BC5, BC6HU*, and
 *        BC7.
 *
 *        *: support for BC6HS is not yet implemented because basisu's encoder
 *        currently fails when given signed half float values.
 *
 *        BC2 (RGB+A) encoder is not implemented because it's rarely used in
 *        practice due to poor quality alpha encoding. BC3 is almost always used
 *        instead.
 *
 * @author Walid Chtioui , individual contributor (walid.chtioui.main@gmail.com)
 */

#include "ktx.h"

#if KTX_FEATURE_WRITE

    #include "bcn_common.h"

    #include "bc7enc/ert.h"                      /* for RDO */
    #include "bc7enc/rgbcx.h"                    /* for BC1-BC5 encoders/decoders */
    #include "transcoder/basisu_transcoder.h"    /* for BC7 encoder */
    #include "transcoder/basisu_astc_hdr_core.h" /* for BC6H encoder */
    #include "vkformat_enum.h"                   /* for VkFormat enum */
    #include "ktxint.h"
    #include "texture2.h"
    #include "multithreading.h"

    // #define DEBUG_PRINT_STATS 1               /* uncomment to print RDO stats */
    #ifdef DEBUG_PRINT_STATS
        #include "chrono"
        #include <atomic>
        #include <chrono>
        #include <iostream>
    #endif

    #define DECLARE_PRIVATE_EX(n, t2) ktxTexture2_private& n = *(t2->_private)
    #define DECLARE_PROTECTED_EX(n, t2) ktxTexture_protected& n = *(t2->_protected)

static inline bool
unpack_block_bc7(const void* pBlock, ert::color_rgba* pPixels, uint32_t, void*) {
    return basist::bc7u::unpack_bc7(pBlock, reinterpret_cast<basist::color_rgba*>(pPixels));
};

static inline bool
unpack_block_bc4(const void* pBlock, ert::color_rgba* pPixels, uint32_t, void*) {
    memset(pPixels, 0, sizeof(ert::color_rgba) * 16);
    rgbcx::unpack_bc4(pBlock, reinterpret_cast<uint8_t*>(pPixels), 4);
    return true;
};

static inline void
get_current_thread_blocks(uint32_t w, uint32_t h, int thread_id, int thread_count,
                          size_t& block_start_idx, size_t& num_blocks) {
    const size_t nbrBlocksX = (w + BCN_BLOCK_SIZE - 1) / BCN_BLOCK_SIZE;
    const size_t nbrBlocksY = (h + BCN_BLOCK_SIZE - 1) / BCN_BLOCK_SIZE;
    const size_t nbrBlocksTotal = nbrBlocksX * nbrBlocksY;
    // Each thread takes a set of contiguous blocks to encode
    const bool is_last_thread = thread_id == (thread_count - 1);
    const size_t num_blocks_per_thread = nbrBlocksTotal / thread_count;
    block_start_idx = thread_id * num_blocks_per_thread;
    num_blocks = is_last_thread ? nbrBlocksTotal - (thread_id * num_blocks_per_thread)
                                : num_blocks_per_thread;
    // debugging/sanity checks (should be obviously true)
    assert((block_start_idx + num_blocks) <= nbrBlocksTotal);
}

static void
compression_workload_runner(int thread_count, int thread_id, void* payload) {
    bcn_compression_workload* workload = static_cast<bcn_compression_workload*>(payload);
    const auto width = workload->width;
    const auto height = workload->height;
    const auto nchannels = workload->nchannels;
    const size_t nbrBlocksX = (width + BCN_BLOCK_SIZE - 1) / BCN_BLOCK_SIZE;

    // Each thread takes a set of contiguous blocks to encode
    size_t block_start_idx, block_end_idx, num_blocks;
    get_current_thread_blocks(workload->width, workload->height, thread_id, thread_count,
                              block_start_idx, num_blocks);
    block_end_idx = block_start_idx + num_blocks;

    // Intermediate store for decoded LDR BCn block
    uint8_t rgba[BCN_BLOCK_SIZE * BCN_BLOCK_SIZE * 4];  /* 4 x 4 x 4 */
    uint16_t rgbh[BCN_BLOCK_SIZE * BCN_BLOCK_SIZE * 3]; /* 4 x 4 x 3 x 2 */
    uint8_t rgbx[BCN_BLOCK_SIZE * BCN_BLOCK_SIZE * 4];  /* only for BC1 */

    basist::astc_6x6_hdr::fast_bc6h_params bc6h_params;
    bc6h_params.init();

    uint8_t* pDstLevelImage = workload->data_out;

    for (size_t block_idx = block_start_idx; block_idx < block_end_idx; ++block_idx) {
        const size_t xBlock = block_idx % nbrBlocksX;
        const size_t yBlock = block_idx / nbrBlocksX;
        // Extract/Copy source block (non-multiple-of-4 texture dimensions are handled
        // via clamping to edge).
        if (workload->params.bcn == KTX_BCN_COMPRESSION_BC6HU ||
            workload->params.bcn == KTX_BCN_COMPRESSION_BC6HS) {
            extract_block(rgbh, reinterpret_cast<const uint16_t*>(workload->data_in),
                          xBlock * BCN_BLOCK_SIZE, yBlock * BCN_BLOCK_SIZE, width, height,
                          nchannels);
        } else if (workload->params.bcn == KTX_BCN_COMPRESSION_BC1) {
            // BC1 is an edge case because encoder expects 4 while input is 3
            extract_block(rgba, workload->data_in, xBlock * BCN_BLOCK_SIZE, yBlock * BCN_BLOCK_SIZE,
                          width, height, nchannels);
            rgb_to_rgba_block(rgbx, rgba);
        } else {
            extract_block(rgba, workload->data_in, xBlock * BCN_BLOCK_SIZE, yBlock * BCN_BLOCK_SIZE,
                          width, height, nchannels);
        }

        switch (workload->params.bcn) {
    #if 0  // bc7enc BC1 encoder does not support punch-through alpha
        case KTX_BCN_COMPRESSION_BC1A:
    #endif
        case KTX_BCN_COMPRESSION_BC1:
            // BC1: 4 x 4 x 3 = 64 bytes -> 8 bytes
            rgbcx::encode_bc1(workload->params.bc1CompressionQuality,
                              pDstLevelImage + (yBlock * nbrBlocksX + xBlock) * BC1_BLOCK_SIZE,
                              rgbx, true, false);
            break;

            // BC1A: 4 x 4 x 4 = 64 bytes -> 8 bytes (alpha only 1 bit encoded)
            rgbcx::encode_bc1(workload->params.bc1CompressionQuality,
                              pDstLevelImage + (yBlock * nbrBlocksX + xBlock) * BC1_BLOCK_SIZE,
                              rgba, true, false);
            break;

        case KTX_BCN_COMPRESSION_BC3:
            // BC3: 4 x 4 x 4 = 64 bytes -> 16 bytes
            rgbcx::encode_bc3(workload->params.bc1CompressionQuality,
                              pDstLevelImage + (yBlock * nbrBlocksX + xBlock) * BC3_BLOCK_SIZE,
                              rgba);
            break;

        case KTX_BCN_COMPRESSION_BC4:
            // BC4: 4 x 4 x 1 = 16 bytes -> 8 bytes
            rgbcx::encode_bc4(pDstLevelImage + (yBlock * nbrBlocksX + xBlock) * BC4_BLOCK_SIZE,
                              rgba, /* stride */ BC4_NCHANNELS);
            break;

        case KTX_BCN_COMPRESSION_BC5:
            // BC5: 4 x 4 x 2 = 32 bytes -> 16 bytes
            rgbcx::encode_bc5(pDstLevelImage + (yBlock * nbrBlocksX + xBlock) * BC5_BLOCK_SIZE,
                              rgba, 0, 1,
                              /* stride */ BC5_NCHANNELS);
            break;

        case KTX_BCN_COMPRESSION_BC6HU:
            // TODO: this currently errors out (assert fails) on signed input.
            // Need to handle this the same way that UASTC HDR handles it or
            // just use another encoder?
            basist::astc_6x6_hdr::fast_encode_bc6h(
                rgbh,
                reinterpret_cast<basist::bc6h_block*>(
                    pDstLevelImage + (yBlock * nbrBlocksX + xBlock) * BC6H_BLOCK_SIZE),
                bc6h_params);
            break;

        case KTX_BCN_COMPRESSION_BC7:
            // BC7: 4 x 4 x 4 = 64 bytes -> 16 bytes
            basist::bc7f::fast_pack_bc7_auto_rgba(
                pDstLevelImage + (yBlock * nbrBlocksX + xBlock) * BC7_BLOCK_SIZE,
                reinterpret_cast<const basist::color_rgba*>(rgba),
                workload->params.bc7CompressionQuality);
            break;

        default:
            return;  // should never occur
        }
    }
}

static void
rdo_bc1_workload_runner(int thread_count, int thread_id, void* payload) {
    size_t block_start_idx, num_blocks;
    uint32_t total_modified_local = 0;
    ert::reduce_entropy_stats stats_local;
    auto workload = reinterpret_cast<rdo_bc1_workload*>(payload);
    get_current_thread_blocks(workload->m_width, workload->m_height, thread_id, thread_count,
                              block_start_idx, num_blocks);
    bool res = ert::reduce_entropy(
        workload->m_packed_img + block_start_idx * BC1_BLOCK_SIZE, num_blocks, BC1_BLOCK_SIZE,
        BC1_BLOCK_SIZE, BCN_BLOCK_SIZE, BCN_BLOCK_SIZE, 3 /* alpha channel ignored */,
        workload->m_unpacked_img_rgbx + block_start_idx * (BCN_BLOCK_SIZE * BCN_BLOCK_SIZE),
        workload->m_ert_p_rgb, total_modified_local, unpack_block_bc1, &workload->m_bc1_params,
        stats_local);

    if (res) {
        workload->m_total_modified_rgb += total_modified_local;
        workload->m_total_smooth_blocks_rgb += stats_local.total_smooth_blocks;
        workload->m_total_second_matches_rgb += stats_local.total_second_matches;
    } else {
        workload->m_success = false;
    }
}

static void
rdo_bc3_workload_runner(int thread_count, int thread_id, void* payload) {
    size_t block_start_idx, num_blocks;
    uint32_t total_modified_local_rgb = 0;
    uint32_t total_modified_local_a = 0;
    ert::reduce_entropy_stats local_stats;
    auto workload = reinterpret_cast<rdo_bc3_workload*>(payload);
    get_current_thread_blocks(workload->m_width, workload->m_height, thread_id, thread_count,
                              block_start_idx, num_blocks);
    // In bc7enc_rdo's code and after confirmed testing: BC4 A then BC1 RGB blocks.
    // First, RDO the BC4 A channel blocks ...
    bool res = ert::reduce_entropy(
        workload->m_packed_img + block_start_idx * BC3_BLOCK_SIZE, num_blocks, BC3_BLOCK_SIZE,
        BC4_BLOCK_SIZE, BCN_BLOCK_SIZE, BCN_BLOCK_SIZE, 1,
        workload->m_unpacked_img_axxx + block_start_idx * (BCN_BLOCK_SIZE * BCN_BLOCK_SIZE),
        workload->m_ert_p_a, total_modified_local_a, unpack_block_bc4, nullptr, local_stats);
    if (res) {
        workload->m_total_modified_a += total_modified_local_a;
        workload->m_total_smooth_blocks_a += local_stats.total_smooth_blocks;
        workload->m_total_second_matches_a += local_stats.total_second_matches;
    } else {
        workload->m_success = false;
    }
    // Then reduce entropy for the BC1 RGB block ...
    res = ert::reduce_entropy(
        (workload->m_packed_img + BC4_BLOCK_SIZE) + block_start_idx * BC3_BLOCK_SIZE, num_blocks,
        BC3_BLOCK_SIZE, BC1_BLOCK_SIZE, BCN_BLOCK_SIZE, BCN_BLOCK_SIZE, 3,
        workload->m_unpacked_img_rgbx + block_start_idx * (BCN_BLOCK_SIZE * BCN_BLOCK_SIZE),
        workload->m_ert_p_rgb, total_modified_local_rgb, unpack_block_bc1, &workload->m_bc1_params,
        local_stats);
    if (res) {
        workload->m_total_modified_rgb += total_modified_local_rgb;
        workload->m_total_smooth_blocks_rgb += local_stats.total_smooth_blocks;
        workload->m_total_second_matches_rgb += local_stats.total_second_matches;
    } else {
        workload->m_success = false;
        return;
    }
}

static void
rdo_bc4_workload_runner(int thread_count, int thread_id, void* payload) {
    uint32_t total_modified_local = 0;
    ert::reduce_entropy_stats local_stats;
    size_t block_start_idx, num_blocks;
    auto workload = reinterpret_cast<rdo_bc4_workload*>(payload);
    get_current_thread_blocks(workload->m_width, workload->m_height, thread_id, thread_count,
                              block_start_idx, num_blocks);
    bool res = ert::reduce_entropy(
        workload->m_packed_img + block_start_idx * BC4_BLOCK_SIZE, num_blocks, BC4_BLOCK_SIZE,
        BC4_BLOCK_SIZE, BCN_BLOCK_SIZE, BCN_BLOCK_SIZE, 1,
        workload->m_unpacked_img_rxxx + block_start_idx * (BCN_BLOCK_SIZE * BCN_BLOCK_SIZE),
        workload->m_ert_p_r, total_modified_local, unpack_block_bc4, nullptr, local_stats);
    if (res) {
        workload->m_total_modified_r += total_modified_local;
        workload->m_total_smooth_blocks_r += local_stats.total_smooth_blocks;
        workload->m_total_second_matches_r += local_stats.total_second_matches;
    } else {
        workload->m_success = false;
    }
}

static void
rdo_bc5_workload_runner(int thread_count, int thread_id, void* payload) {
    size_t block_start_idx, num_blocks;
    uint32_t total_modified_local_r = 0;
    uint32_t total_modified_local_g = 0;
    ert::reduce_entropy_stats local_stats;
    auto workload = reinterpret_cast<rdo_bc5_workload*>(payload);
    get_current_thread_blocks(workload->m_width, workload->m_height, thread_id, thread_count,
                              block_start_idx, num_blocks);
    // BC5: one BC4 block for R followed by one BC4 block for G
    // reduce entropy for the BC4 R block ...
    bool res = ert::reduce_entropy(
        workload->m_packed_img + block_start_idx * BC5_BLOCK_SIZE, num_blocks,
        BC5_BLOCK_SIZE /* 2 x BC4_BLOCK_SIZE */, BC4_BLOCK_SIZE, BCN_BLOCK_SIZE, BCN_BLOCK_SIZE, 1,
        workload->m_unpacked_img_rxxx + block_start_idx * (BCN_BLOCK_SIZE * BCN_BLOCK_SIZE),
        workload->m_ert_p_r, total_modified_local_r, unpack_block_bc4, nullptr, local_stats);
    if (res) {
        workload->m_total_modified_r += total_modified_local_r;
        workload->m_total_smooth_blocks_r += local_stats.total_smooth_blocks;
        workload->m_total_second_matches_r += local_stats.total_second_matches;
    } else {
        workload->m_success = false;
        return;
    }
    // then reduce entropy for the BC4 G block ...
    res = ert::reduce_entropy(
        (workload->m_packed_img + BC4_BLOCK_SIZE) + block_start_idx * BC5_BLOCK_SIZE, num_blocks,
        BC5_BLOCK_SIZE /* 2 x BC4_BLOCK_SIZE */, BC4_BLOCK_SIZE, BCN_BLOCK_SIZE, BCN_BLOCK_SIZE, 1,
        workload->m_unpacked_img_gxxx + block_start_idx * (BCN_BLOCK_SIZE * BCN_BLOCK_SIZE),
        workload->m_ert_p_g, total_modified_local_g, unpack_block_bc4, nullptr, local_stats);

    if (res) {
        workload->m_total_modified_g += total_modified_local_g;
        workload->m_total_smooth_blocks_g += local_stats.total_smooth_blocks;
        workload->m_total_second_matches_g += local_stats.total_second_matches;
    } else {
        workload->m_success = false;
    }
}

static void
rdo_bc7_workload_runner(int thread_count, int thread_id, void* payload) {
    auto workload = reinterpret_cast<rdo_bc7_workload*>(payload);

    size_t block_start_idx, num_blocks;
    get_current_thread_blocks(workload->m_width, workload->m_height, thread_id, thread_count,
                              block_start_idx, num_blocks);

    uint32_t total_modified_local = 0;
    ert::reduce_entropy_stats stats_local;
    bool res = ert::reduce_entropy(
        workload->m_packed_img + block_start_idx * BC7_BLOCK_SIZE, num_blocks, BC7_BLOCK_SIZE,
        BC7_BLOCK_SIZE, BCN_BLOCK_SIZE, BCN_BLOCK_SIZE, 4,
        workload->m_unpacked_img_rgba + block_start_idx * (BCN_BLOCK_SIZE * BCN_BLOCK_SIZE),
        workload->m_ert_p_rgba, total_modified_local, unpack_block_bc7, nullptr, stats_local);

    if (res) {
        workload->m_total_modified_rgba += total_modified_local;
        workload->m_total_smooth_blocks_rgba += stats_local.total_smooth_blocks;
        workload->m_total_second_matches_rgba += stats_local.total_second_matches;
    } else {
        workload->m_success = false;
    }
}

    #ifdef DEBUG_PRINT_STATS
[[maybe_unused]] static inline void
print_rdo_params(const rdo_params& params) {
    std::cout << "rdo lambda: " << params.ert_p.m_lambda << '\n';
    std::cout << "rdo loopback window size: " << params.ert_p.m_lookback_window_size << '\n';
    std::cout << "rdo auto smooth block max MSE scale: " << params.auto_smooth_block_max_mse_scale
              << '\n';
    std::cout << "rdo smooth block max MSE scale: " << params.ert_p.m_smooth_block_max_mse_scale
              << '\n';
    std::cout << "rdo smooth block max std dev: " << params.ert_p.m_max_smooth_block_std_dev
              << '\n';
    std::cout << "rdo max allowed RMS increase ratio: "
              << params.ert_p.m_max_allowed_rms_increase_ratio << '\n';
    std::cout << "rdo color weights: [" << params.ert_p.m_color_weights[0] << ", "
              << params.ert_p.m_color_weights[1] << ", " << params.ert_p.m_color_weights[2] << ", "
              << params.ert_p.m_color_weights[3] << "] \n";
    std::cout << "rdo try two matches: " << params.ert_p.m_try_two_matches << '\n';
    std::cout << "rdo allow relative movement: " << params.ert_p.m_allow_relative_movement << '\n';
    std::cout << "rdo skip zero MSE blocks: " << params.ert_p.m_skip_zero_mse_blocks << '\n';
};
    #endif

/**
 * @~English
 * @brief Performs rate distortion optimization (RDO) on the provided
 *        BCn-encoded blocks to reduce entropy for a potential subsequent
 *        Deflate step (i.e., significant bit rate reduction can be achieved
 *        when further supercompressed with Zlib/ZSTD). BC2 and BC6H formats are
 *        currently not supported.
 *
 *        Some values of the reduce_entropy_params struct may be adjusted before
 *        being passed to the underlying RDO subroutine. The reason for this is
 *        that, depending on the BCn compression, some values make no sense and
 *        may kill efficiency.
 *
 * @param [in] unpacked_img_0       pointer to the source unpacked data.
 * @param [in] unpacked_img_size    size of source unpacked data in bytes.
 *                                  Internal sanity checks are performed on this
 *                                  provided size.
 * @param [in,out] packed_img       pointer to the packed/encoded image data.
 *                                  These packed/encoded blocks are modified to
 *                                  reduce bit/texel rate which is measured via
 *                                  an LZ (Deflate) compression simulation while
 *                                  keeping distortion minimal (i.e., difference
 *                                  between decoded/unpacked and actual source
 *                                  image data pointed to by @p unpacked_img_0).
 * @param [in] packed_img_size      size of packed/encoded data in bytes.
 *                                  Internal sanity checks are performed on this
 *                                  provided size.
 * @param [in] params               RDO parameters. No sanitation/checks are
 *                                  performed on the provided reduce entropy
 *                                  params (ert). The caller has to make sure
 *                                  that provided params are valid (i.e., in
 *                                  range for floats).
 * @param [in] bcn                  used to determine the BCn compression of the
 *                                  provided compressed blocks.
 * @param [in] width                image width (in texels).
 * @param [in] height               image height (in texels).
 * @param [in] threads              number of threads to dispatch for RDO.
 *                                  Setting this to 1 disables multithreading
 *                                  and simply runs RDO within this main thread.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 */
KTX_error_code
postprocess_rdo_bcn(const ktx_uint8_t* unpacked_img_0, ktx_size_t unpacked_img_size,
                    ktx_uint8_t* packed_img, ktx_size_t packed_img_size, rdo_params params,
                    ktx_bcn_compression_e bcn, ktx_uint32_t width, ktx_uint32_t height,
                    ktx_uint32_t threads) {
    #define CHECK_SIZES(nchannels, block_size)                       \
        do {                                                         \
            if ((unpacked_img_size != width * height * nchannels) || \
                (packed_img_size == nBlocksTotal * block_size)) {    \
                return KTX_DECOMPRESS_LENGTH_ERROR;                  \
            }                                                        \
        } while (0)

    const uint32_t nBlocksX = (width + BCN_BLOCK_SIZE - 1) / BCN_BLOCK_SIZE;
    const uint32_t nBlocksY = (height + BCN_BLOCK_SIZE - 1) / BCN_BLOCK_SIZE;
    const uint32_t nBlocksTotal = nBlocksX * nBlocksY;
    bool success = true;

    // Intermediate storage for extracted blocks. This is mainly for convenience
    // so that we do not have to repeat logic for extracting and potentially
    // padding a block of 4x4 RGBA pixels
    const size_t rgba_pitch = BCN_BLOCK_SIZE * 4;  // 4 x 4
    uint8_t rgba[BCN_BLOCK_SIZE * rgba_pitch];     // 4 x 4 x 4
    auto& ert_p = params.ert_p;

    if (ert_p.m_lambda <= 0.0f) return KTX_SUCCESS;

    switch (bcn) {
    case KTX_BCN_COMPRESSION_BC1: {
        CHECK_SIZES(BC1_NCHANNELS, BC1_BLOCK_SIZE);

        ert_p.m_color_weights[3] = 0;

        if (params.auto_smooth_block_max_mse_scale) {
            ert_p.m_smooth_block_max_mse_scale =
                lerp(15.0f, 50.0f, std::min(1.0f, ert_p.m_lambda / 8.0f));
        }

        std::vector<ert::color_rgba> block_pixels(nBlocksTotal * BCN_BLOCK_SIZE * BCN_BLOCK_SIZE);
        for (uint32_t y = 0; y < height; y += BCN_BLOCK_SIZE) {
            for (uint32_t x = 0; x < width; x += BCN_BLOCK_SIZE) {
                // Extract block (non-multiple-of-4 texture dimensions are handled).
                extract_block(rgba, unpacked_img_0, x, y, width, height, BC1_NCHANNELS);
                // Now flatten the extracted block into block_pixels
                ert::color_rgba* p_dst =
                    block_pixels.data() + x * BCN_BLOCK_SIZE + y * BCN_BLOCK_SIZE * nBlocksX;
                for (size_t py{0}; py < BCN_BLOCK_SIZE; ++py) {
                    memcpy(p_dst + py * BCN_BLOCK_SIZE, rgba + py * rgba_pitch,
                           BCN_BLOCK_SIZE * BC1_NCHANNELS);
                }
            }
        }

    #ifdef DEBUG_PRINT_STATS
        auto start = std::chrono::high_resolution_clock::now();
    #endif

        rdo_bc1_workload workload(width, height, packed_img, block_pixels.data(), ert_p,
                                  params.bc1_params);
        launchThreads(threads, rdo_bc1_workload_runner, &workload);
        success = workload.m_success;

    #ifdef DEBUG_PRINT_STATS
        auto finish = std::chrono::high_resolution_clock::now();
        const auto total_rdo_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
        print_rdo_params(params);
        std::cout << "total rdo time (s): " << total_rdo_time / 1000.0f << '\n';
        std::cout << "total nbr modified blocks (%): "
                  << (100.0f * workload.m_total_modified_rgb.load()) / nBlocksTotal << '\n';
        std::cout << "total nbr smooth blocks (%): "
                  << (100.0f * workload.m_total_smooth_blocks_rgb) / nBlocksTotal << '\n';
        std::cout << "total second matches: " << workload.m_total_second_matches_rgb << '\n';
    #endif

        break;
    }  // BC1

    case KTX_BCN_COMPRESSION_BC3: {
        CHECK_SIZES(BC3_NCHANNELS, BC3_BLOCK_SIZE);

        ert_p.m_color_weights[3] = 0;

        ert::reduce_entropy_params ert_p_a(ert_p);
        ert_p_a.m_color_weights[1] = 0;
        ert_p_a.m_color_weights[2] = 0;
        ert_p_a.m_color_weights[3] = 0;

        if (params.auto_smooth_block_max_mse_scale) {
            ert_p.m_smooth_block_max_mse_scale =
                lerp(15.0f, 50.0f, std::min(1.0f, ert_p.m_lambda / 8.0f));
            ert_p_a.m_smooth_block_max_mse_scale =
                lerp(10.0f, 30.0f, std::min(1.0f, ert_p.m_lambda / 4.0f));
        }

        std::vector<ert::color_rgba> block_pixels_rgbx(nBlocksTotal * BCN_BLOCK_SIZE *
                                                       BCN_BLOCK_SIZE);
        std::vector<ert::color_rgba> block_pixels_axxx(block_pixels_rgbx.size());
        for (uint32_t y = 0; y < height; y += BCN_BLOCK_SIZE) {
            for (uint32_t x = 0; x < width; x += BCN_BLOCK_SIZE) {
                // Extract block (non-multiple-of-4 texture dimensions are handled).
                extract_block(rgba, unpacked_img_0, x, y, width, height, BC3_NCHANNELS);
                // Now flatten the extracted block into block_pixels
                const uint8_t* pSrc = rgba;
                ert::color_rgba* pDstRGB =
                    block_pixels_rgbx.data() + x * BCN_BLOCK_SIZE + y * BCN_BLOCK_SIZE * nBlocksX;
                ert::color_rgba* pDstA =
                    block_pixels_axxx.data() + x * BCN_BLOCK_SIZE + y * BCN_BLOCK_SIZE * nBlocksX;
                for (int i = 0; i < BCN_BLOCK_SIZE * BCN_BLOCK_SIZE; ++i) {
                    pDstRGB[0].m_c[0] = pSrc[0];
                    pDstRGB[0].m_c[1] = pSrc[1];
                    pDstRGB[0].m_c[2] = pSrc[2];
                    pDstRGB[0].m_c[3] = 0;
                    pDstA[0].m_c[0] = pSrc[3];  // alpha
                    pDstA[0].m_c[1] = 0;
                    pDstA[0].m_c[2] = 0;
                    pDstA[0].m_c[3] = 0;
                    pSrc += BC3_NCHANNELS; /* pSrc += 4 */
                    ++pDstRGB;
                    ++pDstA;
                }  // i
            }
        }

    #ifdef DEBUG_PRINT_STATS
        auto start = std::chrono::high_resolution_clock::now();
    #endif

        rdo_bc3_workload workload(width, height, packed_img, block_pixels_rgbx.data(),
                                  block_pixels_axxx.data(), ert_p, ert_p_a, params.bc1_params);
        launchThreads(threads, rdo_bc3_workload_runner, &workload);
        success = workload.m_success;

    #ifdef DEBUG_PRINT_STATS
        auto finish = std::chrono::high_resolution_clock::now();
        const auto total_rdo_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
        print_rdo_params(params);
        std::cout << "total rdo time (s): " << total_rdo_time / 1000.0f << '\n';
        std::cout << "total nbr modified BC1 RGB blocks (%): "
                  << (100.0f * workload.m_total_modified_rgb) / nBlocksTotal << '\n';
        std::cout << "total nbr rgb modified BC4 A blocks (%): "
                  << (100.0f * workload.m_total_modified_a) / nBlocksTotal << '\n';
        std::cout << "total nbr smooth BC1 RGB blocks (%): "
                  << (100.0f * workload.m_total_smooth_blocks_rgb) / nBlocksTotal << '\n';
        std::cout << "total nbr smooth BC4 A blocks (%): "
                  << (100.0f * workload.m_total_smooth_blocks_a) / nBlocksTotal << '\n';
        std::cout << "total second matches for BC1 RGB blocks: "
                  << workload.m_total_second_matches_rgb << '\n';
        std::cout << "total second matches for BC4 A blocks: " << workload.m_total_second_matches_a
                  << '\n';
    #endif

        break;
    }  // BC3

    case KTX_BCN_COMPRESSION_BC4: {
        CHECK_SIZES(BC4_NCHANNELS, BC4_BLOCK_SIZE);

        // only RDO R channel since this is BC4
        ert_p.m_color_weights[1] = 0;
        ert_p.m_color_weights[2] = 0;
        ert_p.m_color_weights[3] = 0;

        if (params.auto_smooth_block_max_mse_scale) {
            ert_p.m_smooth_block_max_mse_scale =
                lerp(10.0f, 30.0f, std::min(1.0f, ert_p.m_lambda / 4.0f));
        }

        const size_t r_pitch = BCN_BLOCK_SIZE * BC4_NCHANNELS;  // 4 x 1
        uint8_t r[BCN_BLOCK_SIZE * r_pitch];                    // 4 x 4 x 1

        std::vector<ert::color_rgba> block_pixels_rxxx(nBlocksTotal * BCN_BLOCK_SIZE *
                                                       BCN_BLOCK_SIZE);
        for (uint32_t y = 0; y < height; y += BCN_BLOCK_SIZE) {
            for (uint32_t x = 0; x < width; x += BCN_BLOCK_SIZE) {
                // Extract block (non-multiple-of-4 texture dimensions are handled).
                extract_block(r, unpacked_img_0, x, y, width, height, BC4_NCHANNELS);
                // Now flatten the extracted block into block_pixels
                ert::color_rgba* pDst =
                    block_pixels_rxxx.data() + x * BCN_BLOCK_SIZE + y * BCN_BLOCK_SIZE * nBlocksX;
                const uint8_t* pSrc = r;
                for (int i = 0; i < BCN_BLOCK_SIZE * BCN_BLOCK_SIZE; ++i) {
                    pDst[0].m_c[0] = pSrc[0];  // alpha
                    pDst[0].m_c[1] = 0;
                    pDst[0].m_c[2] = 0;
                    pDst[0].m_c[3] = 0;
                    pSrc += BC4_NCHANNELS; /* pSrc += 1 */
                    ++pDst;
                }  // i
            }
        }

    #ifdef DEBUG_PRINT_STATS
        auto start = std::chrono::high_resolution_clock::now();
    #endif

        rdo_bc4_workload workload(width, height, packed_img, block_pixels_rxxx.data(), ert_p);
        launchThreads(threads, rdo_bc4_workload_runner, &workload);
        success = workload.m_success;

    #ifdef DEBUG_PRINT_STATS
        auto finish = std::chrono::high_resolution_clock::now();
        const auto total_rdo_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
        print_rdo_params(params);
        std::cout << "total rdo time (s): " << total_rdo_time / 1000.0f << '\n';
        std::cout << "total nbr modified blocks (%): "
                  << (100.0f * workload.m_total_modified_r) / nBlocksTotal << '\n';
        std::cout << "total nbr smooth blocks (%): "
                  << (100.0f * workload.m_total_smooth_blocks_r) / nBlocksTotal << '\n';
        std::cout << "total second matches: " << workload.m_total_second_matches_r << '\n';
    #endif

        break;
    }  // BC4

    case KTX_BCN_COMPRESSION_BC5: {
        // One BC4 block for R followed by one BC4 block for G
        CHECK_SIZES(BC5_NCHANNELS, BC5_BLOCK_SIZE);

        ert_p.m_color_weights[1] = 0;
        ert_p.m_color_weights[2] = 0;
        ert_p.m_color_weights[3] = 0;

        if (params.auto_smooth_block_max_mse_scale) {
            ert_p.m_smooth_block_max_mse_scale =
                lerp(10.0f, 30.0f, std::min(1.0f, ert_p.m_lambda / 4.0f));
        }

        std::vector<ert::color_rgba> block_pixels_rxxx(nBlocksTotal * BCN_BLOCK_SIZE *
                                                       BCN_BLOCK_SIZE);
        std::vector<ert::color_rgba> block_pixels_gxxx(block_pixels_rxxx.size());
        for (uint32_t y = 0; y < height; y += BCN_BLOCK_SIZE) {
            for (uint32_t x = 0; x < width; x += BCN_BLOCK_SIZE) {
                // Extract block (non-multiple-of-4 texture dimensions are handled).
                extract_block(rgba, unpacked_img_0, x, y, width, height, BC5_NCHANNELS);
                // Now flatten the extracted block into block_pixels
                const uint8_t* pSrc = rgba;
                ert::color_rgba* pDstR =
                    block_pixels_rxxx.data() + x * BCN_BLOCK_SIZE + y * BCN_BLOCK_SIZE * nBlocksX;
                ert::color_rgba* pDstG =
                    block_pixels_gxxx.data() + x * BCN_BLOCK_SIZE + y * BCN_BLOCK_SIZE * nBlocksX;
                for (int i = 0; i < BCN_BLOCK_SIZE * BCN_BLOCK_SIZE; ++i) {
                    pDstR[0].m_c[0] = pSrc[0];  // R
                    pDstR[0].m_c[1] = 0;
                    pDstR[0].m_c[2] = 0;
                    pDstR[0].m_c[3] = 0;
                    pDstG[0].m_c[0] = pSrc[1];  // G
                    pDstG[0].m_c[1] = 0;
                    pDstG[0].m_c[2] = 0;
                    pDstG[0].m_c[3] = 0;
                    pSrc += BC5_NCHANNELS; /* pSrc += 2 */
                    ++pDstR;
                    ++pDstG;
                }  // i
            }
        }

    #ifdef DEBUG_PRINT_STATS
        auto start = std::chrono::high_resolution_clock::now();
    #endif

        rdo_bc5_workload workload(width, height, packed_img, block_pixels_rxxx.data(),
                                  block_pixels_gxxx.data(), ert_p, ert_p);
        launchThreads(threads, rdo_bc5_workload_runner, &workload);

        success = workload.m_success;

    #ifdef DEBUG_PRINT_STATS
        auto finish = std::chrono::high_resolution_clock::now();
        const auto total_rdo_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
        print_rdo_params(params);
        std::cout << "total rdo time (s): " << total_rdo_time / 1000.0f << '\n';
        std::cout << "total nbr modified BC4 R blocks (%): "
                  << (100.0f * workload.m_total_modified_r) / nBlocksTotal << '\n';
        std::cout << "total nbr modified BC4 G blocks (%): "
                  << (100.0f * workload.m_total_modified_g) / nBlocksTotal << '\n';
        std::cout << "total nbr smooth BC4 R blocks (%): "
                  << (100.0f * workload.m_total_smooth_blocks_r) / nBlocksTotal << '\n';
        std::cout << "total nbr smooth BC4 G blocks (%): "
                  << (100.0f * workload.m_total_smooth_blocks_g) / nBlocksTotal << '\n';
        std::cout << "total second matches for BC4 R blocks: " << workload.m_total_second_matches_r
                  << '\n';
        std::cout << "total second matches for BC4 G blocks: " << workload.m_total_second_matches_g
                  << '\n';
    #endif

        break;
    }  // BC5

        // TODO: add BC6H RDO support. Requires changes to ert::reduce_entropy to
        // handle half float channels (i.e., f16) instead of 1 byte per channel.

    case KTX_BCN_COMPRESSION_BC7: {
        CHECK_SIZES(BC7_NCHANNELS, BC7_BLOCK_SIZE);

        // Attempt to compute a decent conservative smooth block MSE max scaling
        // factor. No single smooth block scale setting can work for all
        // textures (unless it's ridiuclously large, killing efficiency).
        if (params.auto_smooth_block_max_mse_scale) {
            ert_p.m_smooth_block_max_mse_scale =
                lerp(15.0f, 50.0f, std::min(1.0f, ert_p.m_lambda / 4.0f));
        }

        // Source image data need to be laid out as blocks (i.e., 1st raw of 1s
        // block, then 2nd raw of 1st block, etc. - Not: 1st raw of 1st block
        // then 1st raw of 2nd block, etc.). This is what the main RDO function
        // ert::reduce_entropy expects.
        std::vector<ert::color_rgba> block_pixels_rgba(nBlocksTotal * BCN_BLOCK_SIZE *
                                                       BCN_BLOCK_SIZE);
        for (uint32_t y = 0; y < height; y += BCN_BLOCK_SIZE) {
            for (uint32_t x = 0; x < width; x += BCN_BLOCK_SIZE) {
                // Extract block (non-multiple-of-4 texture dimensions are handled).
                extract_block(rgba, unpacked_img_0, x, y, width, height, BC7_NCHANNELS);
                // Now flatten the extracted block into block_pixels
                ert::color_rgba* p_dst =
                    block_pixels_rgba.data() + x * BCN_BLOCK_SIZE + y * BCN_BLOCK_SIZE * nBlocksX;
                for (size_t py{0}; py < BCN_BLOCK_SIZE; ++py) {
                    memcpy(p_dst + py * BCN_BLOCK_SIZE, rgba + py * rgba_pitch,
                           BCN_BLOCK_SIZE * BC7_NCHANNELS);
                }
            }
        }

    #ifdef DEBUG_PRINT_STATS
        auto start = std::chrono::high_resolution_clock::now();
    #endif

        rdo_bc7_workload workload(width, height, packed_img, block_pixels_rgba.data(), ert_p);
        launchThreads(threads, rdo_bc7_workload_runner, &workload);
        success = workload.m_success;

    #ifdef DEBUG_PRINT_STATS
        auto finish = std::chrono::high_resolution_clock::now();
        const auto total_rdo_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
        print_rdo_params(params);
        std::cout << "total rdo time (s): " << total_rdo_time / 1000.0f << '\n';
        std::cout << "total nbr modified blocks (%): "
                  << (100.0f * workload.m_total_modified_rgba.load()) / nBlocksTotal << '\n';
        std::cout << "total nbr smooth blocks (%): "
                  << (100.0f * workload.m_total_smooth_blocks_rgba) / nBlocksTotal << '\n';
        std::cout << "total second matches: " << workload.m_total_second_matches_rgba << '\n';
    #endif

        break;
    }  // BC7

    default:
        return KTX_INVALID_VALUE;  // not a supported/valid BCn color bcn
    }

    return success ? KTX_SUCCESS : KTX_INVALID_OPERATION;
}

/**
 * @memberof ktxTexture2
 * @ingroup writer
 * @~English
 * @brief Compress a ktx texture with uncompressed images to provided BCn
 *        format. Currently, only BC1, BC3, BC4, BC5, BC6HU*, and BC7 target
 *        formats are supported. Punch-through alpha for BC1 is not supported.
 *
 *        The images are encoded to provided BCn block-compressed format. The
 *        encoded images replace the original images and the texture's fields
 *        including the DFD are modified to reflect the new state.
 *
 *        Such textures can be directly uploaded to a GPU via a graphics API.
 *
 *        Encoding non-multiple-of-4 texture dimensions is supported. Block
 *        pixels/texels that are out of the texture's dimensions are
 *        simply filled via a clamp-to-edge strategy. This strategy is easier
 *        for the encoder to handle since no abrupt changes are introduced in
 *        image boundaries.
 *
 * @param [in] This     pointer to the ktxTexture2 object of interest.
 * @param [in] params   pointer to BCn parameters object. These
 *                      parameters, among other things, control the
 *                      compression quality and whether to apply an RDO
 *                      post-processing step to significantly reduce
 *                      bit rate when @p This texture gets supercompressed
 *                      with Deflate (i.e., Zlib/ZSTD).
 *
 * @return              KTX_SUCCESS on success, other KTX_* enum values on
 *                      error.
 *
 * @exception KTX_INVALID_VALUE
 *                      @p params is @c NULL or used with incompatible
 *                      @p params.
 * @exception KTX_INVALID_OPERATION
 *                      The texture's images are supercompressed.
 * @exception KTX_INVALID_OPERATION
 *                      The texture's images are already in a block compressed
 *                      format (i.e., @c This->isCompressed is true).
 * @exception KTX_INVALID_OPERATION
 *                      The texture image's format is a packed format (e.g.,
 *                      RGB565).
 * @exception KTX_INVALID_OPERATION
 *                      The texture's images are not 2D. Only 2D images can be
 *                      block compressed.
 * @exception  KTX_INVALID_OPERATION
 *                      VkFormat of @p This texture is not supported or does
 *                      not match that of the set BCn color model.
 * @exception KTX_INVALID_OPERATION
 *                      This->generateMipmaps is set.
 * @exception KTX_OUT_OF_MEMORY
 *                      Not enough memory to carry out compression.
 */
extern "C" KTX_error_code
ktxTexture2_CompressBCnEx(ktxTexture2* This, ktxBCnParams* params) {
    assert(This->classId == ktxTexture2_c && "Only support ktx2 BCn.");

    if (!params) return KTX_INVALID_VALUE;

    if (params->structSize != sizeof(struct ktxBCnParams)) return KTX_INVALID_VALUE;

    // TODO: why?
    if (This->generateMipmaps) return KTX_INVALID_OPERATION;

    if (This->numDimensions != 2) return KTX_INVALID_OPERATION;

    if (This->supercompressionScheme != KTX_SS_NONE)
        return KTX_INVALID_OPERATION;  // Can't apply multiple schemes.

    // Only non-block compressed formats can be encoded into a BCn format.
    if (This->isCompressed) return KTX_INVALID_OPERATION;

    if (This->_protected->_formatSize.flags & KTX_FORMAT_SIZE_PACKED_BIT)
        return KTX_INVALID_OPERATION;

    // Basic descriptor block begins after the total size field.
    const uint32_t* BDB = This->pDfd + 1;
    ktx_uint8_t alphaMode = KHR_DFDVAL(BDB, FLAGS);
    ktx_uint32_t transfer = KHR_DFDVAL(BDB, TRANSFER);
    khr_df_model_e expected_color_model;
    size_t nchannels;
    size_t blocksize_in_bytes;
    VkFormat compressedVkFormat;
    ktx_error_code_e result;

    switch (params->bcn) {
        // TODO: Currently we can only encode RGBA uncompressed textures into BC1
    case KTX_BCN_COMPRESSION_BC1:
        switch (This->vkFormat) {
        case VK_FORMAT_R8G8B8_UNORM:
            compressedVkFormat = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
            break;
        case VK_FORMAT_R8G8B8_SRGB:
            compressedVkFormat = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
            break;
        default:
            return KTX_INVALID_OPERATION;  // Not a valid decompressed vkformat for BC1
        }
        nchannels = BC1_NCHANNELS;
        blocksize_in_bytes = BC1_BLOCK_SIZE;
        expected_color_model = khr_df_model_e::KHR_DF_MODEL_BC1A;
        rgbcx::init(static_cast<rgbcx::bc1_approx_mode>(params->bc1ApproxMode));
        break;

    #if 0  // bc7enc BC1 encoder does not support punch-through alpha
    case KTX_BCN_COMPRESSION_BC1A:
        switch (This->vkFormat) {
        case VK_FORMAT_R8G8B8A8_UNORM:
            compressedVkFormat = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
            break;
        case VK_FORMAT_R8G8B8A8_SRGB:
            compressedVkFormat = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
            break;
        default:
            return KTX_INVALID_OPERATION;  // Not a valid decompressed vkformat for BC1A
        }
        nchannels = BC1_NCHANNELS;
        blocksize_in_bytes = BC1_BLOCK_SIZE;
        expected_color_model = khr_df_model_e::KHR_DF_MODEL_BC1A;
        rgbcx::init(static_cast<rgbcx::bc1_approx_mode>(params->bc1ApproxMode));
        break;
    #endif

    case KTX_BCN_COMPRESSION_BC3:
        switch (This->vkFormat) {
        case VK_FORMAT_R8G8B8A8_UNORM:
            compressedVkFormat = VK_FORMAT_BC3_UNORM_BLOCK;
            break;
        case VK_FORMAT_R8G8B8A8_SRGB:
            compressedVkFormat = VK_FORMAT_BC3_SRGB_BLOCK;
            break;
        default:
            return KTX_INVALID_OPERATION;  // Not a valid decompressed vkformat for BC3
        }
        nchannels = BC3_NCHANNELS;
        blocksize_in_bytes = BC3_BLOCK_SIZE;
        expected_color_model = khr_df_model_e::KHR_DF_MODEL_BC3;
        rgbcx::init(static_cast<rgbcx::bc1_approx_mode>(params->bc1ApproxMode));
        break;

    case KTX_BCN_COMPRESSION_BC4:
        if (This->vkFormat != VK_FORMAT_R8_UNORM)
            return KTX_INVALID_OPERATION;  // Not a valid decompressed vkformat for BC4
        compressedVkFormat = VK_FORMAT_BC4_UNORM_BLOCK;
        nchannels = BC4_NCHANNELS;
        blocksize_in_bytes = BC4_BLOCK_SIZE;
        expected_color_model = khr_df_model_e::KHR_DF_MODEL_BC4;
        rgbcx::init(static_cast<rgbcx::bc1_approx_mode>(params->bc1ApproxMode));
        break;

    case KTX_BCN_COMPRESSION_BC5:
        if (This->vkFormat != VK_FORMAT_R8G8_UNORM)
            return KTX_INVALID_OPERATION;  // Not a valid decompressed vkformat for BC5
        compressedVkFormat = VK_FORMAT_BC5_UNORM_BLOCK;
        nchannels = BC5_NCHANNELS;
        blocksize_in_bytes = BC5_BLOCK_SIZE;
        expected_color_model = khr_df_model_e::KHR_DF_MODEL_BC5;
        rgbcx::init(static_cast<rgbcx::bc1_approx_mode>(params->bc1ApproxMode));
        break;

    case KTX_BCN_COMPRESSION_BC6HU:
        switch (This->vkFormat) {
        case VK_FORMAT_R16G16B16_SFLOAT:
            compressedVkFormat = VK_FORMAT_BC6H_UFLOAT_BLOCK;
            break;
        default:
            return KTX_INVALID_OPERATION;  // Not a valid decompressed vkformat for BC6H
        }
        nchannels = BC6H_NCHANNELS;
        blocksize_in_bytes = BC6H_BLOCK_SIZE;
        expected_color_model = khr_df_model_e::KHR_DF_MODEL_BC6H;
        basist::basisu_transcoder_init();
        break;

    case KTX_BCN_COMPRESSION_BC7:
        switch (This->vkFormat) {
        case VK_FORMAT_R8G8B8A8_UNORM:
            compressedVkFormat = VK_FORMAT_BC7_UNORM_BLOCK;
            break;
        case VK_FORMAT_R8G8B8A8_SRGB:
            compressedVkFormat = VK_FORMAT_BC7_SRGB_BLOCK;
            break;
        default:
            return KTX_INVALID_OPERATION;  // Not a valid decompressed vkformat for BC7
        }
        nchannels = BC7_NCHANNELS;
        blocksize_in_bytes = BC7_BLOCK_SIZE;
        expected_color_model = khr_df_model_e::KHR_DF_MODEL_BC7;
        basist::basisu_transcoder_init();
        break;

    default:
        return KTX_INVALID_VALUE;  // Provided color bcn is not BCn
    }

    if (This->pData == NULL) {
        result = ktxTexture2_LoadImageData((ktxTexture2*)This, nullptr, 0);
        if (result != KTX_SUCCESS) return result;
    }

    ktx_uint32_t thread_count = params->threadCount;
    if (thread_count < 1) thread_count = 1;
    ktx_uint32_t rdoThreadCount = thread_count;

    // This->numLevels = 0 not allowed for block compressed formats
    // But just in case make sure it's not zero
    This->numLevels = MAX(1, This->numLevels);

    // Create a prototype texture to use for calculating sizes in the target
    // format and, as useful side effects, provide us with a properly sized
    // data allocation and the DFD for the target format.
    ktxTextureCreateInfo createInfo;
    createInfo.glInternalformat = 0;
    createInfo.vkFormat = compressedVkFormat;
    createInfo.baseWidth = This->baseWidth;
    createInfo.baseHeight = This->baseHeight;
    createInfo.baseDepth = This->baseDepth;
    createInfo.generateMipmaps = This->generateMipmaps;
    createInfo.isArray = This->isArray;
    createInfo.numDimensions = This->numDimensions;
    createInfo.numFaces = This->numFaces;
    createInfo.numLayers = This->numLayers;
    createInfo.numLevels = This->numLevels;
    createInfo.pDfd = nullptr;

    ktxTexture2* prototype;
    result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &prototype);
    if (result != KTX_SUCCESS) {
        assert(result == KTX_OUT_OF_MEMORY && "Out of memory allocating texture.");
        return result;
    }
    assert(prototype->dataSize && "Prototype texture size not initialized.\n");
    if (!prototype->pData) return KTX_OUT_OF_MEMORY;

    bcn_compression_workload work;
    work.nchannels = nchannels;
    work.params = *params;

    // ASTC encoder does this loop in reverse to (probably) avoid having to add
    // levelDataOffsetOut (which has no additional cost whatsoever)
    for (ktx_uint32_t level = 0; level < This->numLevels; ++level) {
        // dims for current level + nbr blocks
        const uint32_t width = MAX(1, This->baseWidth >> level);
        const uint32_t height = MAX(1, This->baseHeight >> level);
        const uint32_t depth = MAX(1, This->baseDepth >> level);
        const size_t nbrBlocksX = (width + BCN_BLOCK_SIZE - 1) / BCN_BLOCK_SIZE;
        const size_t nbrBlocksY = (height + BCN_BLOCK_SIZE - 1) / BCN_BLOCK_SIZE;

        ktx_size_t levelImageSizeIn = 0;
        ktx_size_t levelImageSizeOut = 0;
        const ktx_uint32_t levelImages = This->numLayers * This->numFaces * depth;

        levelImageSizeIn =
            ktxTexture_calcImageSize(ktxTexture(This), level, KTX_FORMAT_VERSION_TWO);
        levelImageSizeOut =
            ktxTexture_calcImageSize(ktxTexture(prototype), level, KTX_FORMAT_VERSION_TWO);

        // TODO: all of this needs robust verification. Is the data whithin a given
        // level compact or are there instances where a padding is added (e.g.,
        // because graphics API has a different data layout requirements)?
        assert(levelImageSizeIn == width * height * nchannels *
                                       ((params->bcn == KTX_BCN_COMPRESSION_BC6HU ||
                                         params->bcn == KTX_BCN_COMPRESSION_BC6HS)
                                            ? 2
                                            : 1) &&
               "Probably non-compact data (i.e., some padding)");
        assert(levelImageSizeOut == nbrBlocksX * nbrBlocksY * blocksize_in_bytes &&
               "Probably non-compact data (i.e., some padding) for BCn compressed texture");

        const size_t levelDataOffsetIn = ktxTexture2_levelDataOffset(This, level);
        const size_t levelDataOffsetOut = ktxTexture2_levelDataOffset(prototype, level);

        // Points to start of raw source/destination compressed blocks image
        // data within this miplevl (e.g., for 3D texture and for miplevel 0,
        // this initially points to start of first depth slice image data, then
        // this gets update to point to next slice and so on until we exit this
        // loop and this gets initialized again to point to start of depth slice
        // 0 withing next mip level)
        const ktx_uint8_t* pSrcLevelImage = This->pData + levelDataOffsetIn;

        // points to start of destination image within this miplevel
        ktx_uint8_t* pDstLevelImage = prototype->pData + levelDataOffsetOut;

        work.width = width;
        work.height = height;

        for (uint32_t image = 0; image < levelImages; image++) {
            work.data_in = pSrcLevelImage;
            work.data_out = pDstLevelImage;

            launchThreads(thread_count, compression_workload_runner, &work);

            // post process the encoded blocks using RDO (if enabled)
            if (params->rdo) {
                rdo_params rdo_p;
                rdo_p.ert_p.m_lambda = params->rdoQualityScalar;
                rdo_p.ert_p.m_lookback_window_size = params->rdoWindowLoopbackSize;
                rdo_p.ert_p.m_smooth_block_max_mse_scale = params->rdoMaxSmoothBlockMseScale;
                rdo_p.ert_p.m_max_smooth_block_std_dev = params->rdoMaxSmoothBlockStdDev;
                rdo_p.ert_p.m_try_two_matches = params->rdoTry2Matches;
                rdo_p.ert_p.m_allow_relative_movement = params->rdoAllowRelativeMovement;
                rdo_p.ert_p.m_skip_zero_mse_blocks = false;
                rdo_p.auto_smooth_block_max_mse_scale = params->rdoAutoSmoothBlockMaxMSEScale;
                rdo_p.bc1_params.bc1_approx_mode =
                    static_cast<rgbcx::bc1_approx_mode>(params->bc1ApproxMode);
                rdo_p.bc1_params.allow_3color_mode = true;           // hardcoded
                rdo_p.bc1_params.use_3color_mode_for_black = false;  // hardcoded
                auto res =
                    postprocess_rdo_bcn(pSrcLevelImage, width * height * nchannels, pDstLevelImage,
                                        nbrBlocksX * nbrBlocksY * blocksize_in_bytes, rdo_p,
                                        params->bcn, width, height, rdoThreadCount);
                if (res != KTX_SUCCESS) {
                    ktxTexture2_Destroy(prototype);
                    return res;
                }
            }

            pDstLevelImage += levelImageSizeOut;  // next destination image within this miplevel
            pSrcLevelImage += levelImageSizeIn;   // next source image within this miplevel
        }
    }
    if (KHR_DFDVAL(prototype->pDfd + 1, MODEL) != expected_color_model) {
        ktxTexture2_Destroy(prototype);
        return KTX_INVALID_OPERATION;
    }

    if (transfer == KHR_DF_TRANSFER_SRGB &&
        (KHR_DFDVAL(prototype->pDfd + 1, TRANSFER) != KHR_DF_TRANSFER_SRGB ||
         KHR_DFDVAL(prototype->pDfd + 1, PRIMARIES) != KHR_DF_PRIMARIES_SRGB)) {
        ktxTexture2_Destroy(prototype);
        return KTX_INVALID_OPERATION;
    };

    // Fix up the current (This) texture (this is copied from ASTC encoder - see
    // astc_codec.cpp)
    #undef DECLARE_PRIVATE
    #undef DECLARE_PROTECTED
    #define DECLARE_PRIVATE(n, t2) ktxTexture2_private& n = *(t2->_private)
    #define DECLARE_PROTECTED(n, t2) ktxTexture_protected& n = *(t2->_protected)

    DECLARE_PROTECTED(thisPrtctd, This);
    DECLARE_PRIVATE(protoPriv, prototype);
    DECLARE_PROTECTED(protoPrtctd, prototype);
    memcpy(&thisPrtctd._formatSize, &protoPrtctd._formatSize, sizeof(ktxFormatSize));
    This->vkFormat = prototype->vkFormat;
    This->isCompressed = prototype->isCompressed;
    This->supercompressionScheme = KTX_SS_NONE;
    This->_private->_requiredLevelAlignment = protoPriv._requiredLevelAlignment;

    // Copy typesize otherwise `ktx info` and `ktx validate` fails and
    // complains that block-compressed types should have a typesize of 1.
    This->_protected->_typeSize = prototype->_protected->_typeSize;

    // Copy the levelIndex from the prototype to This.
    memcpy(This->_private->_levelIndex, protoPriv._levelIndex,
           This->numLevels * sizeof(ktxLevelIndexEntry));

    // Move the DFD and data from the prototype to This.
    free(This->pDfd);
    This->pDfd = prototype->pDfd;
    prototype->pDfd = 0;
    free(This->pData);
    This->pData = prototype->pData;
    This->dataSize = prototype->dataSize;
    prototype->pData = 0;
    prototype->dataSize = 0;

    ktxTexture2_Destroy(prototype);

    KHR_DFDSETVAL(This->pDfd + 1, FLAGS, alphaMode);  // Restore alphaMode flags
    return KTX_SUCCESS;
}

#else  // !KTX_FEATURE_WRITE

extern "C" KTX_error_code
ktxTexture2_CompressBCnEx(ktxTexture2*, ktxBCnParams*) {
    return KTX_INVALID_OPERATION;
}

// extern "C" KTX_error_code
// ktxTexture2_CompressBCn(ktxTexture2*, ktx_uint32_t) {
//     return KTX_INVALID_OPERATION;
// }

#endif  // KTX_FEATURE_WRITE
