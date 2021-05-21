/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright (c) 2021, Arm Limited and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file astc_encode.cpp
 * @~English
 *
 * @brief Functions for compressing a texture to astc format.
 *
 * @author Wasim Abbas , www.arm.com
 */

#include <cstring>
#include <inttypes.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <zstd.h>
#include <KHR/khr_df.h>

#include "dfd.h"
#include "ktx.h"
#include "ktxint.h"
#include "texture2.h"
#include "vkformat_enum.h"
#include "vk_format.h"

#include "astcenc.h"
#include "../tools/toktx/image.hpp"

// Provide pthreads support on windows
#if defined(_WIN32) && !defined(__CYGWIN__)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef HANDLE pthread_t;
typedef int pthread_attr_t;

/* Public function, see header file for detailed documentation */
static int
pthread_create(pthread_t* thread, const pthread_attr_t* attribs,
               void* (*threadfunc)(void*), void* thread_arg) {
    (void)attribs;
    LPTHREAD_START_ROUTINE func = (LPTHREAD_START_ROUTINE)threadfunc;
    *thread = CreateThread(nullptr, 0, func, thread_arg, 0, nullptr);
    return 0;
}

/* Public function, see header file for detailed documentation */
static int
pthread_join(pthread_t thread, void** value) {
    (void)value;
    WaitForSingleObject(thread, INFINITE);
    return 0;
}

#endif

static astcenc_image*
imageAllocate(uint32_t bitness,
              uint32_t dim_x, uint32_t dim_y, uint32_t dim_z) {
    astcenc_image *img = new astcenc_image;
    assert(img);

    img->dim_x         = dim_x;
    img->dim_y         = dim_y;
    img->dim_z         = dim_z;

    if (bitness == 8) {
        void **data    = new void *[dim_z];
        img->data_type = ASTCENC_TYPE_U8;
        img->data      = data;

        for (uint32_t z = 0; z < dim_z; z++) {
            data[z] = new uint8_t[dim_x * dim_y * 4];
        }
    }
    else if (bitness == 16) {
        void **data    = new void *[dim_z];
        img->data_type = ASTCENC_TYPE_F16;
        img->data      = data;

        for (uint32_t z = 0; z < dim_z; z++) {
            data[z] = new uint16_t[dim_x * dim_y * 4];
        }
    }
    else {       // if (bitness == 32)
        assert(bitness == 32);
        void **data    = new void *[dim_z];
        img->data_type = ASTCENC_TYPE_F32;
        img->data      = data;

        for (uint32_t z = 0; z < dim_z; z++) {
            data[z] = new float[dim_x * dim_y * 4];
        }
    }

    return img;
}

static void
imageFree(astcenc_image *img) {
    if (img == nullptr) {
        return;
    }

    for (uint32_t z = 0; z < img->dim_z; z++) {
        delete[](char *) img->data[z];
    }

    delete[] img->data;
    delete img;
}

static astcenc_image*
unorm8x1ArrayToImage(const uint8_t *data, uint32_t dim_x, uint32_t dim_y) {
    astcenc_image *img = imageAllocate(8, dim_x, dim_y, 1);
    assert(img);

    for (uint32_t y = 0; y < dim_y; y++) {
        uint8_t *      data8 = static_cast<uint8_t *>(img->data[0]);
        const uint8_t *src   = data + dim_x * y;

        for (uint32_t x = 0; x < dim_x; x++) {
            data8[(4 * dim_x * y) + (4 * x)    ] = src[x];
            data8[(4 * dim_x * y) + (4 * x + 1)] = src[x];
            data8[(4 * dim_x * y) + (4 * x + 2)] = src[x];
            data8[(4 * dim_x * y) + (4 * x + 3)] = 255;
        }
    }

    return img;
}

static astcenc_image*
unorm8x2ArrayToImage(const uint8_t *data, uint32_t dim_x, uint32_t dim_y) {
    astcenc_image *img = imageAllocate(8, dim_x, dim_y, 1);
    assert(img);

    for (uint32_t y = 0; y < dim_y; y++) {
        uint8_t *      data8 = static_cast<uint8_t *>(img->data[0]);
        const uint8_t *src   = data + 2 * dim_x * y;

        for (uint32_t x = 0; x < dim_x; x++) {
            data8[(4 * dim_x * y) + (4 * x)    ] = src[2 * x    ];
            data8[(4 * dim_x * y) + (4 * x + 1)] = src[2 * x    ];
            data8[(4 * dim_x * y) + (4 * x + 2)] = src[2 * x    ];
            data8[(4 * dim_x * y) + (4 * x + 3)] = src[2 * x + 1];
        }
    }

    return img;
}

static astcenc_image*
unorm8x3ArrayToImage(const uint8_t *data, uint32_t dim_x, uint32_t dim_y) {
    astcenc_image *img = imageAllocate(8, dim_x, dim_y, 1);
    assert(img);

    for (uint32_t y = 0; y < dim_y; y++) {
        uint8_t *      data8 = static_cast<uint8_t *>(img->data[0]);
        const uint8_t *src   = data + 3 * dim_x * y;

        for (uint32_t x = 0; x < dim_x; x++) {
            data8[(4 * dim_x * y) + (4 * x)    ] = src[3 * x    ];
            data8[(4 * dim_x * y) + (4 * x + 1)] = src[3 * x + 1];
            data8[(4 * dim_x * y) + (4 * x + 2)] = src[3 * x + 2];
            data8[(4 * dim_x * y) + (4 * x + 3)] = 255;
        }
    }

    return img;
}

static astcenc_image*
unorm8x4ArrayToImage(const uint8_t *data, uint32_t dim_x, uint32_t dim_y) {
    astcenc_image *img = imageAllocate(8, dim_x, dim_y, 1);
    assert(img);

    for (uint32_t y = 0; y < dim_y; y++) {
        uint8_t *      data8 = static_cast<uint8_t *>(img->data[0]);
        const uint8_t *src   = data + 4 * dim_x * y;

        for (uint32_t x = 0; x < dim_x; x++) {
            data8[(4 * dim_x * y) + (4 * x)    ] = src[4 * x    ];
            data8[(4 * dim_x * y) + (4 * x + 1)] = src[4 * x + 1];
            data8[(4 * dim_x * y) + (4 * x + 2)] = src[4 * x + 2];
            data8[(4 * dim_x * y) + (4 * x + 3)] = src[4 * x + 3];
        }
    }

    return img;
}

static ktx_size_t
astcBufferSize(uint32_t width, uint32_t height, uint32_t depth,
               uint32_t block_x, uint32_t block_y, uint32_t block_z) {
    auto xblocs = (width  + block_x - 1) / block_x;
    auto yblocs = (height + block_y - 1) / block_y;
    auto zblocs = (depth  + block_z - 1) / block_z;

    return xblocs * yblocs * zblocs * 16;
}

/**
 * @memberof ktxTexture
 * @ingroup write
 * @~English
 * @brief       Creates default astc parameters
 *
 * @return      ktxAstcParams with default options for astc compressor
 */
static ktxAstcParams
astcDefaultOptions() {
    ktxAstcParams params{};
    params.structSize = sizeof(params);
    params.verbose = false;
    params.threadCount = 1;
    params.blockSize = KTX_PACK_ASTC_BLOCK_6x6;
    params.function = KTX_PACK_ASTC_ENCODER_FUNCTION_UNKNOWN;
    params.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
    params.qualityLevel = KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM;
    params.normalMap = false;

    return params;
}

/**
 * @memberof ktxTexture
 * @ingroup write
 * @~English
 * @brief Creates valid astc block size from string.
 *
 * @return      Valid ktx_pack_astc_block_size_e from string
 */
ktx_pack_astc_block_size_e
astcBlockSize(const char* block_size) {
  static std::unordered_map<std::string, ktx_pack_astc_block_size_e>
      astc_blocks_mapping{{"4x4", KTX_PACK_ASTC_BLOCK_4x4},
                          {"5x4", KTX_PACK_ASTC_BLOCK_5x4},
                          {"5x5", KTX_PACK_ASTC_BLOCK_5x5},
                          {"6x5", KTX_PACK_ASTC_BLOCK_6x5},
                          {"6x6", KTX_PACK_ASTC_BLOCK_6x6},
                          {"8x5", KTX_PACK_ASTC_BLOCK_8x5},
                          {"8x6", KTX_PACK_ASTC_BLOCK_8x6},
                          {"10x5", KTX_PACK_ASTC_BLOCK_10x5},
                          {"10x6", KTX_PACK_ASTC_BLOCK_10x6},
                          {"8x8", KTX_PACK_ASTC_BLOCK_8x8},
                          {"10x8", KTX_PACK_ASTC_BLOCK_10x8},
                          {"10x10", KTX_PACK_ASTC_BLOCK_10x10},
                          {"12x10", KTX_PACK_ASTC_BLOCK_12x10},
                          {"12x12", KTX_PACK_ASTC_BLOCK_12x12},
                          {"3x3x3", KTX_PACK_ASTC_BLOCK_3x3x3},
                          {"4x3x3", KTX_PACK_ASTC_BLOCK_4x3x3},
                          {"4x4x3", KTX_PACK_ASTC_BLOCK_4x4x3},
                          {"4x4x4", KTX_PACK_ASTC_BLOCK_4x4x4},
                          {"5x4x4", KTX_PACK_ASTC_BLOCK_5x4x4},
                          {"5x5x4", KTX_PACK_ASTC_BLOCK_5x5x4},
                          {"5x5x5", KTX_PACK_ASTC_BLOCK_5x5x5},
                          {"6x5x5", KTX_PACK_ASTC_BLOCK_6x5x5},
                          {"6x6x5", KTX_PACK_ASTC_BLOCK_6x6x5},
                          {"6x6x6", KTX_PACK_ASTC_BLOCK_6x6x6}};

  auto opt = astc_blocks_mapping.find(block_size);

  if (opt != astc_blocks_mapping.end())
      return opt->second;

  return KTX_PACK_ASTC_BLOCK_6x6;
}

/**
 * @memberof ktxTexture
 * @ingroup write
 * @~English
 * @brief Creates valid astc quality from string.
 *
 * @return      Valid ktx_pack_astc_quality_e from string
 */
ktx_pack_astc_quality_levels_e
astcQualityLevel(const char *quality) {

    static std::unordered_map<std::string,
                              ktx_pack_astc_quality_levels_e> astc_quality_mapping{
        {"fastest", KTX_PACK_ASTC_QUALITY_LEVEL_FASTEST},
        {"fast", KTX_PACK_ASTC_QUALITY_LEVEL_FAST},
        {"medium", KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM},
        {"thorough", KTX_PACK_ASTC_QUALITY_LEVEL_THOROUGH},
        {"exhaustive", KTX_PACK_ASTC_QUALITY_LEVEL_EXHAUSTIVE}
    };

  auto opt = astc_quality_mapping.find(quality);

  if (opt != astc_quality_mapping.end())
      return opt->second;

  return KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM;
}

/**
 * @memberof ktxTexture
 * @ingroup write
 * @~English
 * @brief       Creates valid astc function from string.
 *              Checks the input file color space to use as default.
 *
 * @return      Valid ktx_pack_astc_function_e from string
 */
ktx_pack_astc_encoder_function_e
astcEncoderFunction(const char* function) {
    if (std::strcmp(function, "srgb") == 0)
        return KTX_PACK_ASTC_ENCODER_FUNCTION_SRGB;
    else if (std::strcmp(function, "linear") == 0)
        return KTX_PACK_ASTC_ENCODER_FUNCTION_LINEAR;

    return KTX_PACK_ASTC_ENCODER_FUNCTION_SRGB;
}

/**
 * @memberof ktxTexture
 * @ingroup write
 * @~English
 * @brief Creates valid astc mode from string.
 *
 * @return      Valid ktx_pack_astc_mode_e from string
 */
ktx_pack_astc_encoder_mode_e
astcEncoderMode(const char* mode) {
    if (std::strcmp(mode, "ldr") == 0)
        return KTX_PACK_ASTC_ENCODER_MODE_LDR;
    else if (std::strcmp(mode, "hdr") == 0)
        return KTX_PACK_ASTC_ENCODER_MODE_HDR;

  return KTX_PACK_ASTC_ENCODER_MODE_LDR;
}

/**
 * @memberof ktxTexture
 * @ingroup write
 * @~English
 * @brief       Should be used to get VkFormat from astc block enum
 *
 * @return      VKFormat for a specific astc block size
 */
static ktx_uint32_t
astcVkFormat(ktx_uint32_t block_size, bool sRGB) {
    if (sRGB) {
        switch (block_size) {
        case KTX_PACK_ASTC_BLOCK_4x4: return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_5x4: return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_5x5: return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_6x5: return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_6x6: return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_8x5: return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_8x6: return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_8x8: return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_10x5: return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_10x6: return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_10x8: return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_10x10: return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_12x10: return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_12x12: return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_3x3x3: return VK_FORMAT_ASTC_3x3x3_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_4x3x3: return VK_FORMAT_ASTC_4x3x3_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_4x4x3: return VK_FORMAT_ASTC_4x4x3_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_4x4x4: return VK_FORMAT_ASTC_4x4x4_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_5x4x4: return VK_FORMAT_ASTC_5x4x4_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_5x5x4: return VK_FORMAT_ASTC_5x5x4_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_5x5x5: return VK_FORMAT_ASTC_5x5x5_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_6x5x5: return VK_FORMAT_ASTC_6x5x5_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_6x6x5: return VK_FORMAT_ASTC_6x6x5_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_6x6x6: return VK_FORMAT_ASTC_6x6x6_SRGB_BLOCK_EXT;
        }
    } else {
        switch (block_size) {
        case KTX_PACK_ASTC_BLOCK_4x4: return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_5x4: return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_5x5: return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_6x5: return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_6x6: return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_8x5: return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_8x6: return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_8x8: return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_10x5: return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_10x6: return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_10x8: return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_10x10: return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_12x10: return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_12x12: return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_3x3x3: return VK_FORMAT_ASTC_3x3x3_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_4x3x3: return VK_FORMAT_ASTC_4x3x3_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_4x4x3: return VK_FORMAT_ASTC_4x4x3_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_4x4x4: return VK_FORMAT_ASTC_4x4x4_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_5x4x4: return VK_FORMAT_ASTC_5x4x4_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_5x5x4: return VK_FORMAT_ASTC_5x5x4_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_5x5x5: return VK_FORMAT_ASTC_5x5x5_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_6x5x5: return VK_FORMAT_ASTC_6x5x5_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_6x6x5: return VK_FORMAT_ASTC_6x6x5_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_6x6x6: return VK_FORMAT_ASTC_6x6x6_UNORM_BLOCK_EXT;
        }
    }

    return VK_FORMAT_ASTC_6x6_SRGB_BLOCK; // Default is 6x6 sRGB image
}

/**
 * @memberof ktxTexture
 * @ingroup write
 * @~English
 * @brief Creates valid astc encoder action from string.
 *
 * @return      Valid astc_profile from string
 */
static astcenc_profile
astcEncoderAction(const ktxAstcParams &params, const uint32_t* bdb) {

    if (params.function == KTX_PACK_ASTC_ENCODER_FUNCTION_SRGB &&
        params.mode == KTX_PACK_ASTC_ENCODER_MODE_LDR) {
        return ASTCENC_PRF_LDR_SRGB;
    }
    else if (params.function == KTX_PACK_ASTC_ENCODER_FUNCTION_LINEAR &&
        params.mode == KTX_PACK_ASTC_ENCODER_MODE_LDR) {
        return ASTCENC_PRF_LDR;
    }
    else if (params.function == KTX_PACK_ASTC_ENCODER_FUNCTION_LINEAR &&
        params.mode == KTX_PACK_ASTC_ENCODER_MODE_HDR) {
        return ASTCENC_PRF_HDR;
    }
    else if (params.function == KTX_PACK_ASTC_ENCODER_FUNCTION_UNKNOWN) {
        // If no options provided assume the user wants to use
        // color space info provided from the file

        ktx_uint32_t transfer = KHR_DFDVAL(bdb, TRANSFER);
        if (transfer == KHR_DF_TRANSFER_SRGB &&
            params.mode == KTX_PACK_ASTC_ENCODER_MODE_LDR)
            return ASTCENC_PRF_LDR_SRGB;
        else if (transfer == KHR_DF_TRANSFER_LINEAR) {
            if (params.mode == KTX_PACK_ASTC_ENCODER_MODE_LDR)
                return ASTCENC_PRF_LDR;
            else
                return ASTCENC_PRF_HDR;
        }
    }
    // TODO: Add support for the following
    // KTX_PACK_ASTC_ENCODER_ACTION_COMP_HDR_RGB_LDR_ALPHA; not supported

  return ASTCENC_PRF_LDR_SRGB;
}


/**
 * @memberof ktxTexture
 * @ingroup write
 * @~English
 * @brief Creates valid astc encoder swizzle from string.
 *
 * @return      Valid astcenc_swizzle from string
 */
static astcenc_swizzle
astcSwizzle(const ktxAstcParams &params) {

    astcenc_swizzle swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};

    std::vector<astcenc_swz*> swizzle_array{&swizzle.r, &swizzle.g, &swizzle.b, &swizzle.a};

    for (int i = 0; i < 4; i++) {
        if (params.inputSwizzle[i] == 'r')
            *swizzle_array[i] = ASTCENC_SWZ_R;
        else if (params.inputSwizzle[i] == 'g')
            *swizzle_array[i] = ASTCENC_SWZ_G;
        else if (params.inputSwizzle[i] == 'b')
            *swizzle_array[i] = ASTCENC_SWZ_B;
        else if (params.inputSwizzle[i] == 'a')
            *swizzle_array[i] = ASTCENC_SWZ_A;
        else if (params.inputSwizzle[i] == '0')
            *swizzle_array[i] = ASTCENC_SWZ_0;
        else if (params.inputSwizzle[i] == '1')
            *swizzle_array[i] = ASTCENC_SWZ_1;
    }

    return swizzle;
}

static void
astcBlockSizes(ktx_uint32_t block_size,
               uint32_t& block_x, uint32_t& block_y, uint32_t& block_z) {
    switch (block_size) {
    case KTX_PACK_ASTC_BLOCK_4x4 : block_x = 4; block_y = 4; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_5x4 : block_x = 5; block_y = 4; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_5x5 : block_x = 5; block_y = 5; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_6x5 : block_x = 6; block_y = 5; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_6x6 : block_x = 6; block_y = 6; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_8x5 : block_x = 8; block_y = 5; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_8x6 : block_x = 8; block_y = 6; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_10x5 : block_x = 10; block_y = 5; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_10x6 : block_x = 10; block_y = 6; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_8x8 : block_x = 8; block_y = 8; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_10x8 : block_x = 10; block_y = 8; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_10x10 : block_x = 10; block_y = 10; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_12x10 : block_x = 12; block_y = 10; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_12x12 : block_x = 12; block_y = 12; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_3x3x3 : block_x = 3; block_y = 3; block_z = 3; break;
    case KTX_PACK_ASTC_BLOCK_4x3x3 : block_x = 4; block_y = 3; block_z = 3; break;
    case KTX_PACK_ASTC_BLOCK_4x4x3 : block_x = 4; block_y = 4; block_z = 3; break;
    case KTX_PACK_ASTC_BLOCK_4x4x4 : block_x = 4; block_y = 4; block_z = 4; break;
    case KTX_PACK_ASTC_BLOCK_5x4x4 : block_x = 5; block_y = 4; block_z = 4; break;
    case KTX_PACK_ASTC_BLOCK_5x5x4 : block_x = 5; block_y = 5; block_z = 4; break;
    case KTX_PACK_ASTC_BLOCK_5x5x5 : block_x = 5; block_y = 5; block_z = 5; break;
    case KTX_PACK_ASTC_BLOCK_6x5x5 : block_x = 6; block_y = 5; block_z = 5; break;
    case KTX_PACK_ASTC_BLOCK_6x6x5 : block_x = 6; block_y = 6; block_z = 5; break;
    case KTX_PACK_ASTC_BLOCK_6x6x6 : block_x = 6; block_y = 6; block_z = 6; break;
    default:
        block_x = 6; block_y = 6; block_z = 1; break;
    }
}

static float
astcQuality(ktx_uint32_t quality_level) {
    switch (quality_level) {
    case KTX_PACK_ASTC_QUALITY_LEVEL_FASTEST: return ASTCENC_PRE_FASTEST;
    case KTX_PACK_ASTC_QUALITY_LEVEL_FAST: return ASTCENC_PRE_FAST;
    case KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM: return ASTCENC_PRE_MEDIUM;
    case KTX_PACK_ASTC_QUALITY_LEVEL_THOROUGH: return ASTCENC_PRE_THOROUGH;
    case KTX_PACK_ASTC_QUALITY_LEVEL_EXHAUSTIVE: return ASTCENC_PRE_EXHAUSTIVE;
    }

    return ASTCENC_PRE_MEDIUM;
}

struct CompressionWorkload {
    astcenc_context* context;
    astcenc_image* image;
    astcenc_swizzle swizzle;
    uint8_t* data_out;
    size_t data_len;
    astcenc_error error;
};

static void
compressionWorkloadRunner(int threadCount, int threadId, void* payload) {
    (void)threadCount;

    CompressionWorkload* work = static_cast<CompressionWorkload*>(payload);
    astcenc_error error = astcenc_compress_image(
                           work->context, work->image, &work->swizzle,
                           work->data_out, work->data_len, threadId);

    // This is a racy update, so which error gets returned is a random, but it
    // will reliably report an error if an error occurs
    if (error != ASTCENC_SUCCESS) {
        work->error = error;
    }
}

/**
 * @brief Worker thread helper payload for launchThreads.
 */
struct LaunchDesc {
    /** The native thread handle. */
    pthread_t threadHandle;
    /** The total number of threads in the thread pool. */
    int threadCount;
    /** The thread index in the thread pool. */
    int threadId;
    /** The user thread function to execute. */
    void (*func)(int, int, void*);
    /** The user thread payload. */
    void* payload;
};

/**
 * @brief Helper function to translate thread entry points.
 *
 * Convert a (void*) thread entry to an (int, void*) thread entry, where the
 * integer contains the thread ID in the thread pool.
 *
 * @param p The thread launch helper payload.
 */
static void*
launchThreadsHelper(void *p) {
    LaunchDesc* ltd = (LaunchDesc*)p;
    ltd->func(ltd->threadCount, ltd->threadId, ltd->payload);
    return nullptr;
}

/* Public function, see header file for detailed documentation */
static void
launchThreads(int threadCount, void (*func)(int, int, void*), void *payload) {
    // Directly execute single threaded workloads on this thread
    if (threadCount <= 1) {
        func(1, 0, payload);
        return;
    }

    // Otherwise spawn worker threads
    LaunchDesc *threadDescs = new LaunchDesc[threadCount];
    for (int i = 0; i < threadCount; i++) {
        threadDescs[i].threadCount = threadCount;
        threadDescs[i].threadId = i;
        threadDescs[i].payload = payload;
        threadDescs[i].func = func;

        pthread_create(&(threadDescs[i].threadHandle), nullptr,
                       launchThreadsHelper, (void*)&(threadDescs[i]));
    }

    // ... and then wait for them to complete
    for (int i = 0; i < threadCount; i++) {
        pthread_join(threadDescs[i].threadHandle, nullptr);
    }

    delete[] threadDescs;
}

/**
 * @memberof ktxTexture
 * @ingroup writer
 * @~English
 * @brief Encode and compress a ktx texture with uncompressed images to astc.
 *
 * The images are encoded to astc block-compressed format. The encoded images
 * replace the original images and the texture's fields including the DFD are
 * modified to reflect the new state.
 *
 * Such textures can be directly uploaded to a GPU via a graphics API.
 *
 * @param[in]   This   pointer to the ktxTexture object of interest.
 * @param[in]   params pointer to astc params object.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_OPERATION
 *                              The texture is already supercompressed.
 * @exception KTX_INVALID_OPERATION
 *                              The texture's image are in a block compressed
 *                              format.
 * @exception KTX_INVALID_OPERATION
 *                              The texture image's format is a packed format
 *                              (e.g. RGB565).
 * @exception KTX_INVALID_OPERATION
 *                              The texture image format's component size is not
                                8-bits.
 * @exception KTX_INVALID_OPERATION
 *                              The texture's images are 1D. Only 2D images can
 *                              be supercompressed.
 * @exception KTX_INVALID_OPERATION
 *                              Astc compressor failed to compress image for any
                                reason.
 * @exception KTX_OUT_OF_MEMORY Not enough memory to carry out supercompression.
 */
extern "C" KTX_error_code
ktxTexture_CompressAstcEx(ktxTexture* _This, ktxAstcParams* params) {
    // FIXME: At the moment defaults to ktx2 textures only
    assert(_This->classId == ktxTexture2_c && "Only support ktx2 astc.");
    ktxTexture2* This = (ktxTexture2*)_This;

    KTX_error_code result;

    if (!params)
        return KTX_INVALID_VALUE;

    if (params->structSize != sizeof(struct ktxAstcParams))
        return KTX_INVALID_VALUE;

    if (This->supercompressionScheme != KTX_SS_NONE)
        return KTX_INVALID_OPERATION; // Can't apply multiple schemes.

    if (This->isCompressed)
        return KTX_INVALID_OPERATION;  // Only non-block compressed formats
                                       // can be encoded into a astc format.

    if (This->_protected->_formatSize.flags & KTX_FORMAT_SIZE_PACKED_BIT)
        return KTX_INVALID_OPERATION;

    // Basic descriptor block begins after the total size field.
    const uint32_t* BDB = This->pDfd+1;

    uint32_t num_components, component_size;
    getDFDComponentInfoUnpacked(This->pDfd, &num_components, &component_size);

    if (component_size != 1)
        return KTX_INVALID_OPERATION; // Can only deal with 8-bit components

    if (This->pData == NULL) {
        result = ktxTexture2_LoadImageData((ktxTexture2*)This, nullptr, 0);

        if (result != KTX_SUCCESS)
            return result;
    }

    ktx_uint32_t threadCount = params->threadCount;
    if (threadCount < 1)
        threadCount = 1;

    astcenc_profile profile{ASTCENC_PRF_LDR_SRGB};

    astcenc_swizzle swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};

    uint32_t        block_size_x{6};
    uint32_t        block_size_y{6};
    uint32_t        block_size_z{1};
    float           quality{ASTCENC_PRE_MEDIUM};
    uint32_t        flags{params->normalMap ? ASTCENC_FLG_MAP_NORMAL : 0};

    astcBlockSizes(params->blockSize,
                   block_size_x, block_size_y, block_size_z);
    quality = astcQuality(params->qualityLevel);
    profile = astcEncoderAction(*params, BDB);
    swizzle = astcSwizzle(*params);

    astcenc_config   astc_config;
    astcenc_context *astc_context;
    astcenc_error astc_error = astcenc_config_init(profile,
                                                   block_size_x, block_size_y, block_size_z,
                                                   quality, flags,
                                                   &astc_config);

    if (astc_error != ASTCENC_SUCCESS) {
        std::cout << "Astc config init failed\n";
        return KTX_INVALID_OPERATION;
    }

    astc_error  = astcenc_context_alloc(&astc_config, threadCount,
                                        &astc_context);

    if (astc_error != ASTCENC_SUCCESS) {
        std::cout << "Astc context alloc failed\n";
        return KTX_INVALID_OPERATION;
    }

    //
    // Copy images into compressor readable format.
    //
    std::vector<astcenc_image*> astc_input_images;
    std::vector<ktx_size_t> astc_image_sizes;

    // Conservative allocation, only used to resize data vectors
    uint32_t num_images = MAX(1, This->numLayers) * MAX(1, This->numFaces) *
        MAX(1, This->numLevels) * MAX(1, This->baseDepth);

    astc_input_images.reserve(num_images);
    astc_image_sizes.reserve(num_images);

    // Size of all astc compressed images in bytes
    ktx_size_t astc_images_size = 0;

    ktxTexture2_private& priv = *This->_private;

    ktx_size_t level_offset = 0;

    // Walk in reverse on levels so we don't have to do this later
    // This->numLevels = 0 not allowed for block compressed formats
    for (int32_t level = This->numLevels - 1; level >= 0; level--) {
        uint32_t width = MAX(1, This->baseWidth >> level);
        uint32_t height = MAX(1, This->baseHeight >> level);
        uint32_t depth = MAX(1, This->baseDepth >> level);
        uint32_t faceSlices = This->numFaces == 1 ? depth : This->numFaces;

        priv._levelIndex[level].byteLength             = 0;
        priv._levelIndex[level].uncompressedByteLength = 0;

        ktx_size_t size = astcBufferSize(width, height, depth,
                                         block_size_x, block_size_y, block_size_z);

        for (uint32_t layer = 0; layer < MAX(1, This->numLayers); layer++) {
            for (ktx_uint32_t slice = 0; slice < faceSlices; slice++) {
                ktx_size_t offset;
                ktxTexture2_GetImageOffset((ktxTexture2*)This, level, layer,
                                           slice, &offset);

                // TODO: Fix depth for 3D textures
                // Create compressor readable image from each image in the container
                astcenc_image *input_image = nullptr;
                if (num_components == 1)
                    input_image = unorm8x1ArrayToImage(This->pData + offset,
                                                       width, height);
                else if (num_components == 2)
                    input_image = unorm8x2ArrayToImage(This->pData + offset,
                                                       width, height);
                else if (num_components == 3)
                    input_image = unorm8x3ArrayToImage(This->pData + offset,
                                                       width, height);
                else // assume (num_components == 4)
                    input_image = unorm8x4ArrayToImage(This->pData + offset,
                                                       width, height);

                assert(input_image);

                astc_input_images.push_back(input_image);
                astc_image_sizes.push_back(size);

                astc_images_size += size;

                priv._levelIndex[level].byteLength             += size;
                priv._levelIndex[level].uncompressedByteLength += size;
            }
        }

        // Offset is from start of the astc compressed block
        priv._levelIndex[level].byteOffset = level_offset;
        priv._requiredLevelAlignment = 8; // For astc its always 8 bytes (128bits) irrespective of block sizes

        ktx_size_t prev_level_offset = level_offset;
        level_offset += _KTX_PADN(priv._requiredLevelAlignment,
                                  priv._levelIndex[level].byteLength);

        ktx_size_t diff = level_offset - prev_level_offset;

        assert(diff - priv._levelIndex[level].byteLength == 0
               && "\nSome format/blocks aren't alighned to 8 bytes, need to create offsets\n");
    }

    free(This->pData); // No longer needed. Reduce memory footprint.
    This->pData = NULL;
    This->dataSize = 0;

    // Allocate big enough buffer for all compressed images
    This->dataSize = astc_images_size;
    This->pData = reinterpret_cast<uint8_t *>(malloc(This->dataSize));
    if (!This->pData) {
        return KTX_OUT_OF_MEMORY;
    }

    uint8_t* buffer_out  = This->pData;
    uint32_t input_size = astc_input_images.size();

    for (uint32_t level = 0; level < input_size; level++) {
        if (params->verbose)
            std::cout << "Astc compressor: compressing image = " <<
                         level + 1 << " of " << input_size  << std::endl;

        // Lets compress to astc
        astcenc_image *input_image = astc_input_images[level];
        ktx_size_t     size        = astc_image_sizes[level];

        CompressionWorkload work;
        work.context = astc_context;
        work.image = input_image;
        work.swizzle = swizzle;
        work.data_out = buffer_out;
        work.data_len = size;
        work.error = ASTCENC_SUCCESS;

        launchThreads(threadCount, compressionWorkloadRunner, &work);

        if (work.error != ASTCENC_SUCCESS) {
            std::cout << "Astc compressor failed\n" <<
                         astcenc_get_error_string(work.error) << std::endl;

            for(auto& ii : astc_input_images)
                imageFree(ii);

            astcenc_context_free(astc_context);
            return KTX_INVALID_OPERATION;
        }

        buffer_out += size;

        // Free input image
        imageFree(input_image);

        // Reset astc context for next image
        astcenc_compress_reset(astc_context);
    }

    // We are done with astcencoder
    astcenc_context_free(astc_context);

    ktx_uint32_t transfer = KHR_DFDVAL(BDB, TRANSFER);
    bool sRGB = transfer == KHR_DF_TRANSFER_SRGB; // Is this right or I need "KHR_DF_PRIMARIES_SRGB"?

    This->vkFormat = astcVkFormat(params->blockSize, sRGB);

    free(This->pDfd);
    This->pDfd = vk2dfd(static_cast<VkFormat>(This->vkFormat));

    // reset pointer to new BDB
    BDB = This->pDfd+1;

    // Astc-related checks
    ktx_uint32_t model = KHR_DFDVAL(BDB, MODEL);
    ktx_uint32_t primaries = KHR_DFDVAL(BDB, PRIMARIES);
    This->supercompressionScheme = KTX_SS_ASTC;
    This->isCompressed = true;

    assert(model == KHR_DF_MODEL_ASTC && "Invalid dfd generated for astc image\n");
    if (transfer == KHR_DF_TRANSFER_SRGB) {
        assert(primaries == KHR_DF_PRIMARIES_SRGB && "Not a valid sRGB image\n");
    }

    // Since we only allow 8-bit components to be compressed
    This->_protected->_typeSize = 1;

    return KTX_SUCCESS;
}

/**
 * @memberof ktxTexture
 * @ingroup writer
 * @~English
 * @brief Encode and compress a ktx texture with uncompressed images to astc.
 *
 * The images are either encoded to astc block-compressed format. The encoded images
 * replace the original images and the texture's fields including the DFD are modified to reflect the new
 * state.
 *
 * Such textures can be directly uploaded to a GPU via a graphics API.
 *
 * @memberof ktxTexture
 * @ingroup writer
 * @~English
 *
 * @param[in]   This    pointer to the ktxTexture object of interest.
 * @param[in]   quality Compression quality, a value from 0 - 100.
                        Higher=higher quality/slower speed.
                        Lower=lower quality/faster speed.
                        Negative values for quality are considered > 100.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_OPERATION
 *                              The texture is already supercompressed.
 * @exception KTX_INVALID_OPERATION
 *                              The texture's image are in a block compressed
 *                              format.
 * @exception KTX_INVALID_OPERATION
 *                              The texture image's format is a packed format
 *                              (e.g. RGB565).
 * @exception KTX_INVALID_OPERATION
 *                              The texture image format's component size is not 8-bits.
 * @exception KTX_INVALID_OPERATION
 *                              The texture's images are 1D. Only 2D images can
 *                              be supercompressed.
 * @exception KTX_OUT_OF_MEMORY Not enough memory to carry out supercompression.
 */
extern "C" KTX_error_code
ktxTexture_CompressAstc(ktxTexture* This, ktx_uint32_t quality) {
    ktxAstcParams params = astcDefaultOptions();

    if (quality >= KTX_PACK_ASTC_QUALITY_LEVEL_FASTEST)
        params.qualityLevel = KTX_PACK_ASTC_QUALITY_LEVEL_FASTEST;

    if (quality >= KTX_PACK_ASTC_QUALITY_LEVEL_FAST)
        params.qualityLevel = KTX_PACK_ASTC_QUALITY_LEVEL_FAST;

    if (quality >= KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM)
        params.qualityLevel = KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM;

    if (quality >= KTX_PACK_ASTC_QUALITY_LEVEL_THOROUGH)
        params.qualityLevel = KTX_PACK_ASTC_QUALITY_LEVEL_THOROUGH;

    if (quality >= KTX_PACK_ASTC_QUALITY_LEVEL_EXHAUSTIVE)
        params.qualityLevel = KTX_PACK_ASTC_QUALITY_LEVEL_EXHAUSTIVE;

    return ktxTexture_CompressAstcEx(This, &params);
}
