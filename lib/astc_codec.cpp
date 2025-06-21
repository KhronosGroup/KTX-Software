/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright (c) 2021, Arm Limited and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file
 * @~English
 *
 * @brief Functions for compressing a texture to ASTC format and decoding one in ASTC format..
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

//************************************************************************
//*              Functions common to decoder and encoder                 *
//************************************************************************

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
#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wcast-function-type-mismatch"
#endif
    LPTHREAD_START_ROUTINE func = (LPTHREAD_START_ROUTINE)(threadfunc);
#ifdef __clang__
    #pragma clang diagnostic pop
#endif
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

/**
 * @internal
 * @~English
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
 * @internal
 * @~English
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
 * @internal
 * @~English
 * @brief Map astcenc error code to KTX error code
 *
 * Asserts are fired on errors reflecting bad parameters passed by libktx
 * or astcenc compilation settings that do not permit correct operation.
 *
 * @param astc_error The error code to be mapped.
 * @return An equivalent KTX error code.
 */
static ktx_error_code_e
mapAstcError(astcenc_error astc_error) {
    switch (astc_error) {
    case ASTCENC_SUCCESS:
        return KTX_SUCCESS;
    case ASTCENC_ERR_OUT_OF_MEM:
        return KTX_OUT_OF_MEMORY;
    case ASTCENC_ERR_BAD_BLOCK_SIZE: //[[fallthrough]];
    case ASTCENC_ERR_BAD_DECODE_MODE: //[[fallthrough]];
    case ASTCENC_ERR_BAD_FLAGS: //[[fallthrough]];
    case ASTCENC_ERR_BAD_PARAM: //[[fallthrough]];
    case ASTCENC_ERR_BAD_PROFILE: //[[fallthrough]];
    case ASTCENC_ERR_BAD_QUALITY: //[[fallthrough]];
    case ASTCENC_ERR_BAD_SWIZZLE:
        assert(false && "libktx passing bad parameter to astcenc");
        return KTX_INVALID_VALUE;
    case ASTCENC_ERR_BAD_CONTEXT:
        assert(false && "libktx has set up astcenc context incorrectly");
        return KTX_INVALID_OPERATION;
    case ASTCENC_ERR_BAD_CPU_FLOAT:
        assert(false && "Code compiled such that float operations do not meet codec's assumptions.");
        // Most likely compiled with fast math enabled.
        return KTX_INVALID_OPERATION;
    case ASTCENC_ERR_NOT_IMPLEMENTED:
        assert(false && "ASTCENC_BLOCK_MAX_TEXELS not enough for specified block size");
        return KTX_UNSUPPORTED_FEATURE;
    // gcc fails to detect that the switch handles all astcenc_error
    // enumerators and raises a return-type error, "control reaches end of
    // non-void function", hence this
    default:
        assert(false && "Unhandled astcenc error");
        return KTX_INVALID_OPERATION;
    }
}

/**
 * @memberof ktxTexture
 * @internal
 * @ingroup reader writer
 * @~English
 * @brief       Creates valid ASTC decoder profile from VkFormat
 *
 * @return      Valid astc_profile from VkFormat
 */
static astcenc_profile
astcProfile(bool sRGB, bool ldr) {

    if (sRGB && ldr)
        return ASTCENC_PRF_LDR_SRGB;
    else if (!sRGB) {
        if (ldr)
            return ASTCENC_PRF_LDR;
        else
            return ASTCENC_PRF_HDR;
    }
    // TODO: Add support for the following
    // KTX_PACK_ASTC_ENCODER_ACTION_COMP_HDR_RGB_LDR_ALPHA; currently not supported

    assert(ldr && "HDR sRGB profile not supported");

  return ASTCENC_PRF_LDR_SRGB;
}

//************************************************************************
//*                          Decoder functions                           *
//************************************************************************

/**
 * @memberof ktxTexture
 * @internal
 * @ingroup reader
 * @~English
 * @brief       Used to check if an ASTC encoded texture is LDR format or not.
 *
 * @return      true if the VkFormat is an ASTC LDR format.
 */
inline bool isFormatAstcLDR(ktxTexture2* This) noexcept {
    return (KHR_DFDSVAL(This->pDfd + 1, 0, QUALIFIERS) & KHR_DF_SAMPLE_DATATYPE_FLOAT) == 0;
}

/**
 * @memberof ktxTexture
 * @internal
 * @ingroup reader
 * @~English
 * @brief       Should be used to get uncompressed version of ASTC VkFormat
 *
 * The decompressed format is calculated from corresponding ASTC format. There are
 * only 3 possible options currently supported. RGBA8, SRGBA8 and RGBA32.
 *
 * @return      Uncompressed version of VKFormat for a specific ASTC VkFormat
 */
inline VkFormat getUncompressedFormat(ktxTexture2* This) noexcept {
    uint32_t* BDB = This->pDfd + 1;

    if (KHR_DFDSVAL(BDB, 0, QUALIFIERS) & KHR_DF_SAMPLE_DATATYPE_FLOAT) {
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    } else {
        if (khr_df_transfer_e(KHR_DFDVAL(BDB, TRANSFER) == KHR_DF_TRANSFER_SRGB))
            return VK_FORMAT_R8G8B8A8_SRGB;
        else
            return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

struct decompression_workload
{
    astcenc_context* context;
    uint8_t* data;
    size_t data_len;
    astcenc_image* image_out;
    astcenc_swizzle swizzle;
    astcenc_error error;
};

/**
 * @internal
 * @ingroup reader
 * @brief Runner callback function for a decompression worker thread.
 *
 * @param thread_count   The number of threads in the worker pool.
 * @param thread_id      The index of this thread in the worker pool.
 * @param payload        The parameters for this thread.
 */
static void decompression_workload_runner(int thread_count, int thread_id, void* payload) {
    (void)thread_count;

    decompression_workload* work = static_cast<decompression_workload*>(payload);
    astcenc_error error = astcenc_decompress_image(work->context, work->data, work->data_len,
                                                   work->image_out, &work->swizzle, thread_id);
    // This is a racy update, so which error gets returned is a random, but it
    // will reliably report an error if an error occurs
    if (error != ASTCENC_SUCCESS)
    {
        work->error = error;
    }
}

/*
 * Cannot use DECLARE_PRIVATE macro declared in texture.h because it calls the
 * variable `private` which is obviously a no-no in c++. TODO: consider changing.
 * Declare our own similar macros. Cognizant that the using functions handle both
 * This and a prototype object, pass the object as a parameter.
 */
#define DECLARE_PRIVATE_EX(n,t2) ktxTexture2_private& n = *(t2->_private)
#define DECLARE_PROTECTED_EX(n,t2) ktxTexture_protected& n = *(t2->_protected)

/**
 * @ingroup reader
 * @brief Decodes a ktx2 texture object, if it is ASTC encoded.

 * The decompressed format is calculated from corresponding ASTC format. There are
 * only 3 possible options currently supported. RGBA8, SRGBA8 and RGBA32.
 * @note 3d textures are decoded to a multi-slice 3d texture.
 *
 * Updates @p This with the decoded image.
 *
 * @param This     The texture to decode
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_DATA_ERROR
 *                              DFD is incorrect: supercompression scheme or
 *                              sample's channelId do not match ASTC colorModel.
 * @exception KTX_INVALID_OPERATION
 *                              The texture's images are not in ASTC format.
 * @exception KTX_INVALID_OPERATION
 *                              The texture object does not contain any data.
 * @exception KTX_INVALID_OPERATION
 *                              ASTC decoder failed to decompress image.
 *                              Possibly due to incorrect floating point
 *                              compilation settings. Should not happen
 *                              in release package.
 * @exception KTX_OUT_OF_MEMORY Not enough memory to carry out decoding.
 * @exception KTX_UNSUPPORTED_FEATURE
 *                              The texture's images are supercompressed with an
 *                              unsupported scheme.
 * @exception KTX_UNSUPPORTED_FEATURE
 *                              ASTC encoder not compiled with enough
 *                              capacity for requested block size. Should
 *                              not happen in release package.
 */
KTX_error_code
ktxTexture2_DecodeAstc(ktxTexture2 *This) {
    // Decompress This using astc-decoder
    uint32_t* BDB = This->pDfd + 1;
    khr_df_model_e colorModel = (khr_df_model_e)KHR_DFDVAL(BDB, MODEL);
    if (colorModel != KHR_DF_MODEL_ASTC) {
        return KTX_INVALID_OPERATION; // Not in astc decodable format
    }
    if (This->supercompressionScheme == KTX_SS_BASIS_LZ) {
        return KTX_FILE_DATA_ERROR; // Not a valid file.
    }
    // Safety check.
    if (This->supercompressionScheme > KTX_SS_END_RANGE) {
        return KTX_UNSUPPORTED_FEATURE; // Unsupported scheme.
    }
    // Other schemes are decoded in ktxTexture2_LoadImageData.

    DECLARE_PRIVATE_EX(priv, This);

    uint32_t channelId = KHR_DFDSVAL(BDB, 0, CHANNELID);
    if (channelId != KHR_DF_CHANNEL_ASTC_DATA) {
        return KTX_FILE_DATA_ERROR;
    }

    ktx_uint32_t vkformat = (ktx_uint32_t)getUncompressedFormat(This);

    // Create a prototype texture to use for calculating sizes in the target
    // format and, as useful side effects, provide us with a properly sized
    // data allocation and the DFD for the target format.
    ktxTextureCreateInfo createInfo;
    createInfo.glInternalformat = 0;
    createInfo.vkFormat = vkformat;
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

    KTX_error_code result;
    ktxTexture2* prototype;
    result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &prototype);

    if (result != KTX_SUCCESS) {
        assert(result == KTX_OUT_OF_MEMORY); // The only run time error
        return result;
    }

    if (!This->pData) {
        if (ktxTexture_isActiveStream((ktxTexture*)This)) {
             // Load pending. Complete it.
            result = ktxTexture2_LoadImageData(This, NULL, 0);
            if (result != KTX_SUCCESS)
            {
                ktxTexture2_Destroy(prototype);
                return result;
            }
        } else {
            // No data to decode.
            ktxTexture2_Destroy(prototype);
            return KTX_INVALID_OPERATION;
        }
    }

    // This is where I do the decompression from "This" to prototype target
    astcenc_swizzle swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};
    float           quality{ASTCENC_PRE_MEDIUM};
    uint32_t        flags{0}; // TODO: Use normals mode to reconstruct normals params->normalMap ? ASTCENC_FLG_MAP_NORMAL : 0};

    uint32_t        block_size_x = KHR_DFDVAL(BDB, TEXELBLOCKDIMENSION0) + 1;
    uint32_t        block_size_y = KHR_DFDVAL(BDB, TEXELBLOCKDIMENSION1) + 1;
    uint32_t        block_size_z = KHR_DFDVAL(BDB, TEXELBLOCKDIMENSION2) + 1;

    // quality = astcQuality(params->qualityLevel);
    // swizzle = astcSwizzle(*params);

    // if(params->perceptual) flags |= ASTCENC_FLG_USE_PERCEPTUAL;

    ktx_uint32_t transfer = KHR_DFDVAL(BDB, TRANSFER);
    bool ldr = isFormatAstcLDR(This);
    astcenc_profile profile = astcProfile(transfer == KHR_DF_TRANSFER_SRGB, ldr);

    uint32_t threadCount{1}; // Decompression isn't the bottleneck and only used when checking for psnr and ssim
    astcenc_config   astc_config;
    astcenc_context *astc_context;
    astcenc_error astc_error = astcenc_config_init(profile,
                                                   block_size_x, block_size_y, block_size_z,
                                                   quality, flags,
                                                   &astc_config);

    if (astc_error != ASTCENC_SUCCESS)
        return mapAstcError(astc_error);

    astc_error  = astcenc_context_alloc(&astc_config, threadCount, &astc_context);

    if (astc_error != ASTCENC_SUCCESS)
        return mapAstcError(astc_error);

    decompression_workload work;
    work.context = astc_context;
    work.swizzle = swizzle;
    work.error = ASTCENC_SUCCESS;

    for (uint32_t levelIndex = 0; levelIndex < This->numLevels; ++levelIndex) {
        const uint32_t imageWidth = std::max(This->baseWidth >> levelIndex, 1u);
        const uint32_t imageHeight = std::max(This->baseHeight >> levelIndex, 1u);
        const uint32_t imageDepths = std::max(This->baseDepth >> levelIndex, 1u);

        for (uint32_t layerIndex = 0; layerIndex < This->numLayers; ++layerIndex) {
            for (uint32_t faceIndex = 0; faceIndex < This->numFaces; ++faceIndex) {
                for (uint32_t depthSliceIndex = 0; depthSliceIndex < imageDepths; ++depthSliceIndex) {

                    ktx_size_t levelImageSizeIn = ktxTexture_calcImageSize(ktxTexture(This), levelIndex, KTX_FORMAT_VERSION_TWO);

                    ktx_size_t imageOffsetIn;
                    ktx_size_t imageOffsetOut;

                    ktxTexture2_GetImageOffset(This, levelIndex, layerIndex, faceIndex + depthSliceIndex, &imageOffsetIn);
                    ktxTexture2_GetImageOffset(prototype, levelIndex, layerIndex, faceIndex + depthSliceIndex, &imageOffsetOut);

                    auto* imageDataIn = This->pData + imageOffsetIn;
                    auto* imageDataOut = prototype->pData + imageOffsetOut;

                    astcenc_image imageOut;
                    imageOut.dim_x = imageWidth;
                    imageOut.dim_y = imageHeight;
                    imageOut.dim_z = imageDepths;
                    imageOut.data_type = ASTCENC_TYPE_U8; // TODO: Fix for HDR types
                    imageOut.data = (void**)&imageDataOut; // TODO: Fix for HDR types

                    work.data = imageDataIn;
                    work.data_len = levelImageSizeIn;
                    work.image_out = &imageOut;

                    // Only launch worker threads for multi-threaded use - it makes basic
                    // single-threaded profiling and debugging a little less convoluted
                    if (threadCount > 1) {
                        launchThreads(threadCount, decompression_workload_runner, &work);
                    } else {
                        work.error = astcenc_decompress_image(work.context, work.data, work.data_len,
                                                              work.image_out, &work.swizzle, 0);
                    }

                    // Reset ASTC context for next image
                    astcenc_decompress_reset(astc_context);

                    if (work.error != ASTCENC_SUCCESS) {
                        //std::cout << "ASTC decompressor failed\n" << astcenc_get_error_string(work.error) << std::endl;

                        astcenc_context_free(astc_context);
                        return mapAstcError(work.error);
                    }
                }
            }
        }
    }

    // We are done with astcdecoder
    astcenc_context_free(astc_context);

    if (result == KTX_SUCCESS) {
        // Fix up the current texture
        DECLARE_PROTECTED_EX(thisPrtctd, This);
        DECLARE_PRIVATE_EX(protoPriv, prototype);
        DECLARE_PROTECTED_EX(protoPrtctd, prototype);
        memcpy(&thisPrtctd._formatSize, &protoPrtctd._formatSize,
               sizeof(ktxFormatSize));
        This->vkFormat = vkformat;
        This->isCompressed = prototype->isCompressed;
        This->supercompressionScheme = KTX_SS_NONE;
        priv._requiredLevelAlignment = protoPriv._requiredLevelAlignment;
        // Copy the levelIndex from the prototype to This.
        memcpy(priv._levelIndex, protoPriv._levelIndex,
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
        // Free SGD data
        This->_private->_sgdByteLength = 0;
        if (This->_private->_supercompressionGlobalData) {
            free(This->_private->_supercompressionGlobalData);
            This->_private->_supercompressionGlobalData = NULL;
        }
    }
    ktxTexture2_Destroy(prototype);
    return result;
}

//************************************************************************
//*                          Encoder functions                           *
//************************************************************************

#if KTX_FEATURE_WRITE
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
 * @internal
 * @ingroup writer
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
 * @internal
 * @ingroup writer
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
 * @internal
 * @ingroup writer
 * @~English
 * @brief       Creates valid ASTC encoder swizzle from string.
 *
 * @return      Valid astcenc_swizzle from string
 */
static astcenc_swizzle
astcSwizzle(const ktxAstcParams &params) {

    astcenc_swizzle swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};

    std::vector<astcenc_swz*> swizzle_array{&swizzle.r, &swizzle.g, &swizzle.b, &swizzle.a};
    // For historical reasons params.inputSwizzle[0] == '\0' is interpreted to mean no
    // swizzle. The docs says it must match the regular expression /^[rgba01]{4}$/.
    if (params.inputSwizzle[0] != '\0') {
        std::string inputSwizzle(params.inputSwizzle, sizeof(params.inputSwizzle));
        // TODO: Check for RE match.
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
 * @exception  KTX_INVALID_OPERATION
 *                              Transfer function of @c This is not sRGB or Linear.
 * @exception  KTX_INVALID_OPERATION
 *                              @c params->mode  is HDR but transfer function
 *                              of @c This is sRGB.
 * @exception KTX_INVALID_OPERATION
 *                              ASTC encoder failed to compress image.
 *                              Possibly due to incorrect floating point
 *                              compilation settings. Should not happen
 *                              in release package.
 * @exception KTX_INVALID_OPERATION
 *                              This->generateMipmaps is set.
 * @exception KTX_OUT_OF_MEMORY Not enough memory to carry out compression.
 * @exception KTX_UNSUPPORTED_FEATURE
 *                              @c params->mode is HDR mode which is not
 *                              yet implemented.
 * @exception KTX_UNSUPPORTED_FEATURE
 *                              ASTC encoder not compiled with enough
 *                              capacity for requested block size. Should
 *                              not happen in release package.
 */
extern "C" KTX_error_code
ktxTexture2_CompressAstcEx(ktxTexture2* This, ktxAstcParams* params) {
    assert(This->classId == ktxTexture2_c && "Only support ktx2 ASTC.");

    ktx_error_code_e result;
    ktx_pack_astc_encoder_mode_e mode;

    if (!params)
        return KTX_INVALID_VALUE;

    if (params->structSize != sizeof(struct ktxAstcParams))
        return KTX_INVALID_VALUE;

    if (This->generateMipmaps)
        return KTX_INVALID_OPERATION;

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
    ktx_uint32_t transfer = KHR_DFDVAL(BDB, TRANSFER);
    bool sRGB = transfer == KHR_DF_TRANSFER_SRGB;

    if (component_size != 1)
        return KTX_UNSUPPORTED_FEATURE; // Can only deal with 8-bit components at the moment
    if (params->mode == KTX_PACK_ASTC_ENCODER_MODE_DEFAULT) {
        if (component_size == 1 || sRGB)
            mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
        else
            mode = KTX_PACK_ASTC_ENCODER_MODE_HDR;
    } else {
        mode = static_cast<ktx_pack_astc_encoder_mode_e>(params->mode);
    }

    if (mode == KTX_PACK_ASTC_ENCODER_MODE_HDR && sRGB)
        return KTX_INVALID_OPERATION;

    if (!(sRGB || transfer == KHR_DF_TRANSFER_LINEAR))
        return KTX_INVALID_OPERATION;

    if (This->pData == NULL) {
        result = ktxTexture2_LoadImageData((ktxTexture2*)This, nullptr, 0);

        if (result != KTX_SUCCESS)
            return result;
    }

    ktx_uint32_t threadCount = params->threadCount;
    if (threadCount < 1)
        threadCount = 1;

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
    profile = astcProfile(sRGB, mode);
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
        return mapAstcError(astc_error);

    astc_error  = astcenc_context_alloc(&astc_config, threadCount,
                                        &astc_context);

    if (astc_error != ASTCENC_SUCCESS)
        return mapAstcError(astc_error);

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
                //std::cout << "ASTC compressor failed\n" <<
                //             astcenc_get_error_string(work.error) << std::endl;

                astcenc_context_free(astc_context);
                return mapAstcError(work.error);
            }
        }
    }

    // We are done with astcencoder
    astcenc_context_free(astc_context);

    assert(KHR_DFDVAL(prototype->pDfd+1, MODEL) == KHR_DF_MODEL_ASTC
           && "Invalid dfd generated for ASTC image\n");
    assert((transfer == KHR_DF_TRANSFER_SRGB
           ? KHR_DFDVAL(prototype->pDfd+1, TRANSFER) == KHR_DF_TRANSFER_SRGB &&
             KHR_DFDVAL(prototype->pDfd+1, PRIMARIES) == KHR_DF_PRIMARIES_SRGB
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
 *                              The texture's images are supercompressed.
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
#else
extern "C" KTX_error_code
ktxTexture2_CompressAstcEx(ktxTexture2*, ktxAstcParams*) {
    return KTX_INVALID_OPERATION;
}

extern "C" KTX_error_code
ktxTexture2_CompressAstc(ktxTexture2*, ktx_uint32_t) {
    return KTX_INVALID_OPERATION;
}
#endif /* KTX_FEATURE_WRITE */


