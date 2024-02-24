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
 * @brief Functions for compressing a texture to ASTC format.
 *
 * @author Wasim Abbas , www.arm.com
 */

#include <assert.h>
#include <cstring>
#include <inttypes.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <zstd.h>
#include <KHR/khr_df.h>

#include "dfdutils/dfd.h"
#include "ktx.h"
#include "ktxint.h"
#include "texture2.h"
#include "vkformat_enum.h"

#include "astc-encoder/Source/astcenc.h"

#if !defined(_WIN32) || defined(WIN32_HAS_PTHREADS)
#include <pthread.h>
#else
// Provide pthreads support on windows
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

/**
 * @memberof ktxTexture
 * @ingroup write
 * @~English
 * @brief       Creates default ASTC parameters
 *
 * @return      ktxAstcParams with default options for ASTC compressor
 */
static ktxAstcParams
astcDefaultOptions() {
    ktxAstcParams params{};
    params.structSize = sizeof(params);
    params.threadCount = 1;
    params.blockDimension = KTX_PACK_ASTC_BLOCK_DIMENSION_6x6;
    params.mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
    params.qualityLevel = KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM;
    params.normalMap = false;

    return params;
}

/**
 * @memberof ktxTexture
 * @ingroup write
 * @~English
 * @brief       Should be used to get VkFormat from ASTC block enum
 *
 * @return      VKFormat for a specific ASTC block size
 */
static VkFormat
astcVkFormat(ktx_uint32_t block_size, bool sRGB) {
    if (sRGB) {
        switch (block_size) {
        case KTX_PACK_ASTC_BLOCK_DIMENSION_4x4: return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_5x4: return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_5x5: return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_6x5: return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_6x6: return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_8x5: return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_8x6: return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_8x8: return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_10x5: return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_10x6: return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_10x8: return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_10x10: return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_12x10: return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_12x12: return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_3x3x3: return VK_FORMAT_ASTC_3x3x3_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_4x3x3: return VK_FORMAT_ASTC_4x3x3_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_4x4x3: return VK_FORMAT_ASTC_4x4x3_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_4x4x4: return VK_FORMAT_ASTC_4x4x4_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_5x4x4: return VK_FORMAT_ASTC_5x4x4_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_5x5x4: return VK_FORMAT_ASTC_5x5x4_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_5x5x5: return VK_FORMAT_ASTC_5x5x5_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_6x5x5: return VK_FORMAT_ASTC_6x5x5_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_6x6x5: return VK_FORMAT_ASTC_6x6x5_SRGB_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_6x6x6: return VK_FORMAT_ASTC_6x6x6_SRGB_BLOCK_EXT;
        }
    } else {
        switch (block_size) {
        case KTX_PACK_ASTC_BLOCK_DIMENSION_4x4: return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_5x4: return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_5x5: return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_6x5: return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_6x6: return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_8x5: return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_8x6: return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_8x8: return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_10x5: return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_10x6: return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_10x8: return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_10x10: return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_12x10: return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_12x12: return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_3x3x3: return VK_FORMAT_ASTC_3x3x3_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_4x3x3: return VK_FORMAT_ASTC_4x3x3_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_4x4x3: return VK_FORMAT_ASTC_4x4x3_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_4x4x4: return VK_FORMAT_ASTC_4x4x4_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_5x4x4: return VK_FORMAT_ASTC_5x4x4_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_5x5x4: return VK_FORMAT_ASTC_5x5x4_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_5x5x5: return VK_FORMAT_ASTC_5x5x5_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_6x5x5: return VK_FORMAT_ASTC_6x5x5_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_6x6x5: return VK_FORMAT_ASTC_6x6x5_UNORM_BLOCK_EXT;
        case KTX_PACK_ASTC_BLOCK_DIMENSION_6x6x6: return VK_FORMAT_ASTC_6x6x6_UNORM_BLOCK_EXT;
        }
    }

    return VK_FORMAT_ASTC_6x6_SRGB_BLOCK; // Default is 6x6 sRGB image
}

/**
 * @memberof ktxTexture
 * @ingroup write
 * @~English
 * @brief Creates valid ASTC encoder action from string.
 *
 * @return      Valid astc_profile from string
 */
static astcenc_profile
astcEncoderAction(const ktxAstcParams &params, const uint32_t* bdb) {

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
    // TODO: Add support for the following
    // KTX_PACK_ASTC_ENCODER_ACTION_COMP_HDR_RGB_LDR_ALPHA; currently not supported

  return ASTCENC_PRF_LDR_SRGB;
}


/**
 * @memberof ktxTexture
 * @ingroup write
 * @~English
 * @brief Creates valid ASTC encoder swizzle from string.
 *
 * @return      Valid astcenc_swizzle from string
 */
static astcenc_swizzle
astcSwizzle(const ktxAstcParams &params) {

    astcenc_swizzle swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};

    std::vector<astcenc_swz*> swizzle_array{&swizzle.r, &swizzle.g, &swizzle.b, &swizzle.a};
    std::string inputSwizzle = params.inputSwizzle;

    if (inputSwizzle.size() > 0) {
        assert(inputSwizzle.size() == 4 && "InputSwizzle is invalid.");

        for (int i = 0; i < 4; i++) {
            if (inputSwizzle[i] == 'r')
                *swizzle_array[i] = ASTCENC_SWZ_R;
            else if (inputSwizzle[i] == 'g')
                *swizzle_array[i] = ASTCENC_SWZ_G;
            else if (inputSwizzle[i] == 'b')
                *swizzle_array[i] = ASTCENC_SWZ_B;
            else if (inputSwizzle[i] == 'a')
                *swizzle_array[i] = ASTCENC_SWZ_A;
            else if (inputSwizzle[i] == '0')
                *swizzle_array[i] = ASTCENC_SWZ_0;
            else if (inputSwizzle[i] == '1')
                *swizzle_array[i] = ASTCENC_SWZ_1;
        }
    } else if (params.normalMap) {
        return {ASTCENC_SWZ_R, ASTCENC_SWZ_R, ASTCENC_SWZ_R, ASTCENC_SWZ_G};
    }

    return swizzle;
}

static void
astcBlockDimensions(ktx_uint32_t block_size,
               uint32_t& block_x, uint32_t& block_y, uint32_t& block_z) {
    switch (block_size) {
    case KTX_PACK_ASTC_BLOCK_DIMENSION_4x4 : block_x = 4; block_y = 4; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_5x4 : block_x = 5; block_y = 4; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_5x5 : block_x = 5; block_y = 5; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_6x5 : block_x = 6; block_y = 5; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_6x6 : block_x = 6; block_y = 6; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_8x5 : block_x = 8; block_y = 5; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_8x6 : block_x = 8; block_y = 6; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_10x5 : block_x = 10; block_y = 5; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_10x6 : block_x = 10; block_y = 6; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_8x8 : block_x = 8; block_y = 8; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_10x8 : block_x = 10; block_y = 8; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_10x10 : block_x = 10; block_y = 10; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_12x10 : block_x = 12; block_y = 10; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_12x12 : block_x = 12; block_y = 12; block_z = 1; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_3x3x3 : block_x = 3; block_y = 3; block_z = 3; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_4x3x3 : block_x = 4; block_y = 3; block_z = 3; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_4x4x3 : block_x = 4; block_y = 4; block_z = 3; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_4x4x4 : block_x = 4; block_y = 4; block_z = 4; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_5x4x4 : block_x = 5; block_y = 4; block_z = 4; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_5x5x4 : block_x = 5; block_y = 5; block_z = 4; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_5x5x5 : block_x = 5; block_y = 5; block_z = 5; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_6x5x5 : block_x = 6; block_y = 5; block_z = 5; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_6x6x5 : block_x = 6; block_y = 6; block_z = 5; break;
    case KTX_PACK_ASTC_BLOCK_DIMENSION_6x6x6 : block_x = 6; block_y = 6; block_z = 6; break;
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
 * @memberof ktxTexture2
 * @ingroup writer
 * @~English
 * @brief Encode and compress a ktx texture with uncompressed images to astc.
 *
 * The images are encoded to ASTC block-compressed format. The encoded images
 * replace the original images and the texture's fields including the DFD are
 * modified to reflect the new state.
 *
 * Such textures can be directly uploaded to a GPU via a graphics API.
 *
 * @param[in]   This   pointer to the ktxTexture2 object of interest.
 * @param[in]   params pointer to ASTC params object.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_OPERATION
 *                              The texture's images are supercompressed.
 * @exception KTX_INVALID_OPERATION
 *                              The texture's images are in a block compressed
 *                              format.
 * @exception KTX_INVALID_OPERATION
 *                              The texture image's format is a packed format
 *                              (e.g. RGB565).
 * @exception KTX_INVALID_OPERATION
 *                              The texture image format's component size is not
 *                              8-bits.
 * @exception KTX_INVALID_OPERATION
 *                              The texture's images are 1D. Only 2D images can
 *                              be supercompressed.
 * @exception KTX_INVALID_OPERATION
 *                              ASTC  compressor failed to compress image for any
                                reason.
 * @exception KTX_OUT_OF_MEMORY Not enough memory to carry out compression.
 */
extern "C" KTX_error_code
ktxTexture2_CompressAstcEx(ktxTexture2* This, ktxAstcParams* params) {
    assert(This->classId == ktxTexture2_c && "Only support ktx2 ASTC.");

    KTX_error_code result;

    if (!params)
        return KTX_INVALID_VALUE;

    if (params->structSize != sizeof(struct ktxAstcParams))
        return KTX_INVALID_VALUE;

    if (This->supercompressionScheme != KTX_SS_NONE)
        return KTX_INVALID_OPERATION; // Can't apply multiple schemes.

    if (This->isCompressed)
        return KTX_INVALID_OPERATION;  // Only non-block compressed formats
                                       // can be encoded into an ASTC format.

    if (This->_protected->_formatSize.flags & KTX_FORMAT_SIZE_PACKED_BIT)
        return KTX_INVALID_OPERATION;

    // Basic descriptor block begins after the total size field.
    const uint32_t* BDB = This->pDfd+1;

    uint32_t num_components, component_size;
    getDFDComponentInfoUnpacked(This->pDfd, &num_components, &component_size);

    if (component_size != 1)
        return KTX_INVALID_OPERATION; // Can only deal with 8-bit components at the moment

    if (This->pData == NULL) {
        result = ktxTexture2_LoadImageData((ktxTexture2*)This, nullptr, 0);

        if (result != KTX_SUCCESS)
            return result;
    }

    ktx_uint32_t threadCount = params->threadCount;
    if (threadCount < 1)
        threadCount = 1;

    ktx_uint32_t transfer = KHR_DFDVAL(BDB, TRANSFER);
    bool sRGB = transfer == KHR_DF_TRANSFER_SRGB;

    VkFormat vkFormat = astcVkFormat(params->blockDimension, sRGB);

    // This->numLevels = 0 not allowed for block compressed formats
    // But just in case make sure its not zero
    This->numLevels = MAX(1, This->numLevels);

    // Create a prototype texture to use for calculating sizes in the target
    // format and, as useful side effects, provide us with a properly sized
    // data allocation and the DFD for the target format.
    ktxTextureCreateInfo createInfo;
    createInfo.glInternalformat = 0;
    createInfo.vkFormat = vkFormat;
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
    result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE,
                                &prototype);

    if (result != KTX_SUCCESS) {
        assert(result == KTX_OUT_OF_MEMORY && "Out of memory allocating texture.");
        return result;
    }

    astcenc_profile profile{ASTCENC_PRF_LDR_SRGB};

    astcenc_swizzle swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};

    uint32_t        block_size_x{6};
    uint32_t        block_size_y{6};
    uint32_t        block_size_z{1};
    float           quality{ASTCENC_PRE_MEDIUM};
    uint32_t        flags{params->normalMap ? ASTCENC_FLG_MAP_NORMAL : 0};

    astcBlockDimensions(params->blockDimension,
                        block_size_x, block_size_y, block_size_z);
    quality = astcQuality(params->qualityLevel);
    profile = astcEncoderAction(*params, BDB);
    swizzle = astcSwizzle(*params);

    if(params->perceptual)
        flags |= ASTCENC_FLG_USE_PERCEPTUAL;

    astcenc_config   astc_config;
    astcenc_context *astc_context;
    astcenc_error astc_error = astcenc_config_init(profile,
                                                   block_size_x, block_size_y, block_size_z,
                                                   quality, flags,
                                                   &astc_config);

    if (astc_error != ASTCENC_SUCCESS)
        return KTX_INVALID_OPERATION;

    astc_error  = astcenc_context_alloc(&astc_config, threadCount,
                                        &astc_context);

    if (astc_error != ASTCENC_SUCCESS)
        return KTX_INVALID_OPERATION;

    // Walk in reverse on levels so we don't have to do this later
    assert(prototype->dataSize && "Prototype texture size not initialized.\n");

    if (!prototype->pData) {
        return KTX_OUT_OF_MEMORY;
    }

    uint8_t* buffer_out  = prototype->pData;

    for (int32_t level = This->numLevels - 1; level >= 0; level--) {
        uint32_t width = MAX(1, This->baseWidth >> level);
        uint32_t height = MAX(1, This->baseHeight >> level);
        uint32_t depth = MAX(1, This->baseDepth >> level);
        ktx_size_t levelImageSizeIn = 0;
        ktx_size_t levelImageSizeOut = 0;
        ktx_uint32_t levelImages = 0;

        levelImages = This->numLayers * This->numFaces * depth;
        levelImageSizeIn = ktxTexture_calcImageSize(ktxTexture(This), level,
                                                    KTX_FORMAT_VERSION_TWO);
        levelImageSizeOut = ktxTexture_calcImageSize(ktxTexture(prototype), level,
                                                     KTX_FORMAT_VERSION_TWO);
        ktx_size_t offset = ktxTexture2_levelDataOffset(This, level);

        for (uint32_t image = 0; image < levelImages; image++) {
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

            CompressionWorkload work;
            work.context = astc_context;
            work.image = input_image;
            work.swizzle = swizzle;
            work.data_out = buffer_out;
            work.data_len = levelImageSizeOut;
            work.error = ASTCENC_SUCCESS;

            launchThreads(threadCount, compressionWorkloadRunner, &work);

            buffer_out += levelImageSizeOut;

            // Reset ASTC context for next image
            astcenc_compress_reset(astc_context);
            offset += levelImageSizeIn;

            imageFree(input_image);

            if (work.error != ASTCENC_SUCCESS) {
                std::cout << "ASTC compressor failed\n" <<
                             astcenc_get_error_string(work.error) << std::endl;

                astcenc_context_free(astc_context);
                return KTX_INVALID_OPERATION;
            }
        }
    }

    // We are done with astcencoder
    astcenc_context_free(astc_context);

    assert(KHR_DFDVAL(prototype->pDfd+1, MODEL) == KHR_DF_MODEL_ASTC
           && "Invalid dfd generated for ASTC image\n");
    assert((transfer == KHR_DF_TRANSFER_SRGB
           ? KHR_DFDVAL(prototype->pDfd+1, PRIMARIES) == KHR_DF_PRIMARIES_SRGB
           : true) && "Not a valid sRGB image\n");

    // Fix up the current (This) texture
    #undef DECLARE_PRIVATE
    #undef DECLARE_PROTECTED
    #define DECLARE_PRIVATE(n,t2) ktxTexture2_private& n = *(t2->_private)
    #define DECLARE_PROTECTED(n,t2) ktxTexture_protected& n = *(t2->_protected)

    DECLARE_PROTECTED(thisPrtctd, This);
    DECLARE_PRIVATE(protoPriv, prototype);
    DECLARE_PROTECTED(protoPrtctd, prototype);
    memcpy(&thisPrtctd._formatSize, &protoPrtctd._formatSize,
           sizeof(ktxFormatSize));
    This->vkFormat = vkFormat;
    This->isCompressed = prototype->isCompressed;
    This->supercompressionScheme = KTX_SS_NONE;
    This->_private->_requiredLevelAlignment = protoPriv._requiredLevelAlignment;
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
    return KTX_SUCCESS;
}

/**
 * @memberof ktxTexture2
 * @ingroup writer
 * @~English
 * @brief Encode and compress a ktx texture with uncompressed images to astc.
 *
 * The images are either encoded to ASTC block-compressed format. The encoded images
 * replace the original images and the texture's fields including the DFD are modified to reflect the new
 * state.
 *
 * Such textures can be directly uploaded to a GPU via a graphics API.
 *
 * @param[in]   This    pointer to the ktxTexture2 object of interest.
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
ktxTexture2_CompressAstc(ktxTexture2* This, ktx_uint32_t quality) {
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

    return ktxTexture2_CompressAstcEx(This, &params);
}
