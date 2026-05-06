/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab textwidth=70: */

/*
 * Copyright 2019-2020 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BCN_CODEC_H_
#define _BCN_CODEC_H_

#include "vkformat_enum.h"
#include "ktx.h"

#define BCN_BLOCK_SIZE 4

#define BC1_BLOCK_SIZE 8
#define BC2_BLOCK_SIZE 16
#define BC3_BLOCK_SIZE 16
#define BC4_BLOCK_SIZE 8
#define BC5_BLOCK_SIZE 16
#define BC6H_BLOCK_SIZE 16
#define BC7_BLOCK_SIZE 16

#define BC1_OUTPUT_NCHANNELS 4
#define BC3_OUTPUT_NCHANNELS 4
#define BC4_OUTPUT_NCHANNELS 1
#define BC5_OUTPUT_NCHANNELS 2
#define BC6H_OUTPUT_NCHANNELS 4
#define BC7_OUTPUT_NCHANNELS 4

/**
 * @memberof ktxTexture
 * @internal
 * @ingroup reader
 * @~English
 * @brief       Should be used to get uncompressed version of BCn VkFormat
 *
 * The decompressed format is calculated from corresponding BCn format. There are
 * only 3 possible options currently supported. RGBA8, SRGBA8 and RGBA16.
 *
 * @return      Uncompressed version of VKFormat for a specific BCn VkFormat or
 *              VK_FORMAT_UNDEFINED in case the ktxTexture2's vkFormat does not
 *              reflect a BCn compressed texture.
 */
inline VkFormat
getBCnUncompressedFormat(ktx_uint32_t vkFormat) noexcept {
    switch (vkFormat) {
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
    case VK_FORMAT_BC3_SRGB_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case VK_FORMAT_BC4_UNORM_BLOCK:
        return VK_FORMAT_R8_UNORM;
    case VK_FORMAT_BC5_UNORM_BLOCK:
        return VK_FORMAT_R8G8_UNORM;
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

#endif
