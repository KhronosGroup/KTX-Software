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
#include <KHR/khr_df.h>

#include "dfdutils/dfd.h"
#include "ktx.h"
#include "ktxint.h"
#include "texture2.h"
#include "vkformat_enum.h"

#include "bc7enc_rdo/bc7decomp.h" /* for BC7 decoder */
#include "bc7enc_rdo/rgbcx.h"     /* for BC1-BC5 encoders/decoders */

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
pthread_create(pthread_t* thread, const pthread_attr_t* attribs, void* (*threadfunc)(void*),
               void* thread_arg) {
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
launchThreadsHelper(void* p) {
    LaunchDesc* ltd = (LaunchDesc*)p;
    ltd->func(ltd->threadCount, ltd->threadId, ltd->payload);
    return nullptr;
}

static void
launchThreads(int threadCount, void (*func)(int, int, void*), void* payload) {
    // Directly execute single threaded workloads on this thread
    if (threadCount <= 1) {
        func(1, 0, payload);
        return;
    }

    // Otherwise spawn worker threads
    LaunchDesc* threadDescs = new LaunchDesc[threadCount];
    for (int i = 0; i < threadCount; i++) {
        threadDescs[i].threadCount = threadCount;
        threadDescs[i].threadId = i;
        threadDescs[i].payload = payload;
        threadDescs[i].func = func;

        pthread_create(&(threadDescs[i].threadHandle), nullptr, launchThreadsHelper,
                       (void*)&(threadDescs[i]));
    }

    // ... and then wait for them to complete
    for (int i = 0; i < threadCount; i++) {
        pthread_join(threadDescs[i].threadHandle, nullptr);
    }

    delete[] threadDescs;
}

//************************************************************************
//*                          Decoder functions                           *
//************************************************************************

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
getUncompressedFormat(ktxTexture2* This) noexcept {
    switch (This->vkFormat) {
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

/*
 * Cannot use DECLARE_PRIVATE macro declared in texture.h because it calls the
 * variable `private` which is obviously a no-no in c++. TODO: consider changing.
 * Declare our own similar macros. Cognizant that the using functions handle both
 * This and a prototype object, pass the object as a parameter.
 */
#define DECLARE_PRIVATE_EX(n, t2) ktxTexture2_private& n = *(t2->_private)
#define DECLARE_PROTECTED_EX(n, t2) ktxTexture_protected& n = *(t2->_protected)

/**
 * @ingroup reader
 * @brief Decodes a ktx2 texture object, if it is BCn encoded (i.e., BC1, BC3,
 * BC4, BC5, or BC7 encoded).

 * The decompressed format is calculated from corresponding BCn format. There
 * are only 3 possible options currently supported: RGBA8, SRGBA8 and RGBA32.
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
 *                              sample's channelId do not match BCn colorModel.
 * @exception KTX_INVALID_OPERATION
 *                              The texture's images are not in BCn format.
 * @exception KTX_INVALID_OPERATION
 *                              The texture object does not contain any data.
 * @exception KTX_OUT_OF_MEMORY
 *                              Not enough memory to carry out decoding.
 * @exception KTX_UNSUPPORTED_FEATURE
 *                              The texture's images are supercompressed with an
 *                              unsupported scheme.
 */
KTX_error_code
ktxTexture2_DecodeBCn(ktxTexture2* This) {
    // Decompress This using bc7enc_rdo
    uint32_t* BDB = This->pDfd + 1;
    khr_df_model_e colorModel = (khr_df_model_e)KHR_DFDVAL(BDB, MODEL);
    uint32_t channelId = KHR_DFDSVAL(BDB, 0, CHANNELID);
    switch (colorModel) {
    case KHR_DF_MODEL_BC1A:
        if (!(channelId == KHR_DF_CHANNEL_BC1A_COLOR || channelId == KHR_DF_CHANNEL_BC1A_ALPHA)) {
            return KTX_FILE_DATA_ERROR;
        }
        break;
    case KHR_DF_MODEL_BC3:
        break;
    case KHR_DF_MODEL_BC4:
        break;
    case KHR_DF_MODEL_BC5:
        break;
    case KHR_DF_MODEL_BC7:
        break;
    default:
        return KTX_INVALID_OPERATION;  // Not in BCn decodable format
    }

    if (This->supercompressionScheme == KTX_SS_BASIS_LZ ||
        This->supercompressionScheme == KTX_SS_UASTC_HDR_6x6_INTERMEDIATE) {
        return KTX_FILE_DATA_ERROR;  // Not a valid file.
    }
    // Safety check.
    if (This->supercompressionScheme > KTX_SS_END_RANGE) {
        return KTX_UNSUPPORTED_FEATURE;  // Unsupported scheme.
    }
    // Other schemes are decoded in ktxTexture2_LoadImageData.

    DECLARE_PRIVATE_EX(priv, This);

    ktx_uint32_t vkformat = (ktx_uint32_t)getUncompressedFormat(This);
    if (vkformat == VK_FORMAT_UNDEFINED) {
        return KTX_INVALID_OPERATION;  // Not in BCn decodable format
    }

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
        assert(result == KTX_OUT_OF_MEMORY);  // The only run time error
        return result;
    }

    if (!This->pData) {
        if (ktxTexture_isActiveStream((ktxTexture*)This)) {
            // Load pending. Complete it.
            result = ktxTexture2_LoadImageData(This, NULL, 0);
            if (result != KTX_SUCCESS) {
                ktxTexture2_Destroy(prototype);
                return result;
            }
        } else {
            // No data to decode.
            ktxTexture2_Destroy(prototype);
            return KTX_INVALID_OPERATION;
        }
    }

    const ktx_size_t rgba_pitch = BCN_BLOCK_SIZE * BC1_OUTPUT_NCHANNELS;
    ktx_uint8_t rgba[BCN_BLOCK_SIZE * rgba_pitch]; /* 64 bytes */

    for (uint32_t levelIndex = 0; levelIndex < This->numLevels; ++levelIndex) {
        const uint32_t imageWidth = std::max(This->baseWidth >> levelIndex, 1u);
        const uint32_t imageHeight = std::max(This->baseHeight >> levelIndex, 1u);
        const uint32_t imageDepths = std::max(This->baseDepth >> levelIndex, 1u);

        for (uint32_t layerIndex = 0; layerIndex < This->numLayers; ++layerIndex) {
            for (uint32_t faceIndex = 0; faceIndex < This->numFaces; ++faceIndex) {
                for (uint32_t depthSliceIndex = 0; depthSliceIndex < imageDepths;
                     ++depthSliceIndex) {
                    ktx_size_t levelImageSizeIn = ktxTexture_calcImageSize(
                        ktxTexture(This), levelIndex, KTX_FORMAT_VERSION_TWO);

                    ktx_size_t imageOffsetIn;
                    ktx_size_t imageOffsetOut;

                    // TODO: are we sure these can't fail? (i.e., return != KTX_SUCCESS)
                    ktxTexture2_GetImageOffset(This, levelIndex, layerIndex,
                                               faceIndex + depthSliceIndex, &imageOffsetIn);
                    ktxTexture2_GetImageOffset(prototype, levelIndex, layerIndex,
                                               faceIndex + depthSliceIndex, &imageOffsetOut);

                    auto* imageDataIn = This->pData + imageOffsetIn;
                    auto* imageDataOut = prototype->pData + imageOffsetOut;

                    const ktx_size_t dst_pitch = imageWidth * BC1_OUTPUT_NCHANNELS;

                    const ktx_uint8_t* src_blocks = imageDataIn;

                    for (size_t y{0}; y < imageHeight; y += BCN_BLOCK_SIZE) {
                        for (size_t x{0}; x < imageWidth; x += BCN_BLOCK_SIZE) {
                            switch (colorModel) {
                            // BC1: 8 bytes -> 4 x 4 x 4 = 64 bytes
                            case KHR_DF_MODEL_BC1A:
                                rgbcx::unpack_bc1(src_blocks, rgba, true);
                                src_blocks += BC1_BLOCK_SIZE;
                                break;

                            case KHR_DF_MODEL_BC3:
                                break;

                            case KHR_DF_MODEL_BC4:
                                break;

                            case KHR_DF_MODEL_BC5:
                                break;

                            case KHR_DF_MODEL_BC7:
                                break;

                            default:
                                break;  // should never occur
                            }

                            // now we copy the decoded block into the actual
                            // texture image while taking into consideration
                            // that dimenions may not be a multiple of 4.
                            const ktx_uint8_t* pSrc = rgba;
                            ktx_uint8_t* pDst =
                                imageDataOut + y * dst_pitch + BC1_OUTPUT_NCHANNELS * x;
                            int cols = fmin(BCN_BLOCK_SIZE, imageWidth - x);
                            for (ktx_size_t py{0}; py < BCN_BLOCK_SIZE && y + py < imageHeight;
                                 ++py) {
                                memcpy(pDst, pSrc, cols * BC1_OUTPUT_NCHANNELS);
                                pSrc += rgba_pitch;
                                pDst += dst_pitch;
                            }
                        }
                    }

                    // astcenc_decompress_image(work.context, work.data, work.data_len;
                }
            }
        }
    }

    // We are done with astcdecoder
    // astcenc_context_free(astc_context);

    if (result == KTX_SUCCESS) {
        // Fix up the current texture
        DECLARE_PROTECTED_EX(thisPrtctd, This);
        DECLARE_PRIVATE_EX(protoPriv, prototype);
        DECLARE_PROTECTED_EX(protoPrtctd, prototype);
        memcpy(&thisPrtctd._formatSize, &protoPrtctd._formatSize, sizeof(ktxFormatSize));
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
// extern "C" KTX_error_code
// ktxTexture2_CompressAstcEx(ktxTexture2* This, ktxAstcParams* params) {
//     assert(This->classId == ktxTexture2_c && "Only support ktx2 ASTC.");
//
//     ktx_error_code_e result;
//     ktx_pack_astc_encoder_mode_e mode;
//
//     if (!params) return KTX_INVALID_VALUE;
//
//     if (params->structSize != sizeof(struct ktxAstcParams)) return KTX_INVALID_VALUE;
//
//     if (This->generateMipmaps) return KTX_INVALID_OPERATION;
//
//     if (This->supercompressionScheme != KTX_SS_NONE)
//         return KTX_INVALID_OPERATION;  // Can't apply multiple schemes.
//
//     if (This->isCompressed)
//         return KTX_INVALID_OPERATION;  // Only non-block compressed formats
//                                        // can be encoded into an ASTC format.
//
//     if (This->_protected->_formatSize.flags & KTX_FORMAT_SIZE_PACKED_BIT)
//         return KTX_INVALID_OPERATION;
//
//     // Basic descriptor block begins after the total size field.
//     const uint32_t* BDB = This->pDfd + 1;
//
//     uint32_t num_components, component_size;
//     getDFDComponentInfoUnpacked(This->pDfd, &num_components, &component_size);
//     ktx_uint32_t transfer = KHR_DFDVAL(BDB, TRANSFER);
//     bool sRGB = transfer == KHR_DF_TRANSFER_SRGB;
//     ktx_uint8_t alphaMode = KHR_DFDVAL(BDB, FLAGS);
//
//     if (component_size != 1)
//         return KTX_UNSUPPORTED_FEATURE;  // Can only deal with 8-bit components at the moment
//     if (params->mode == KTX_PACK_ASTC_ENCODER_MODE_DEFAULT) {
//         if (component_size == 1 || sRGB)
//             mode = KTX_PACK_ASTC_ENCODER_MODE_LDR;
//         else
//             mode = KTX_PACK_ASTC_ENCODER_MODE_HDR;
//     } else {
//         mode = static_cast<ktx_pack_astc_encoder_mode_e>(params->mode);
//     }
//
//     if (mode == KTX_PACK_ASTC_ENCODER_MODE_HDR && sRGB) return KTX_INVALID_OPERATION;
//
//     if (!(sRGB || transfer == KHR_DF_TRANSFER_LINEAR)) return KTX_INVALID_OPERATION;
//
//     if (This->pData == NULL) {
//         result = ktxTexture2_LoadImageData((ktxTexture2*)This, nullptr, 0);
//
//         if (result != KTX_SUCCESS) return result;
//     }
//
//     ktx_uint32_t threadCount = params->threadCount;
//     if (threadCount < 1) threadCount = 1;
//
//     // VkFormat vkFormat = astcVkFormat(params->blockDimension, sRGB);
//
//     // This->numLevels = 0 not allowed for block compressed formats
//     // But just in case make sure its not zero
//     This->numLevels = MAX(1, This->numLevels);
//
//     // Create a prototype texture to use for calculating sizes in the target
//     // format and, as useful side effects, provide us with a properly sized
//     // data allocation and the DFD for the target format.
//     ktxTextureCreateInfo createInfo;
//     createInfo.glInternalformat = 0;
//     // createInfo.vkFormat = vkFormat;
//     createInfo.baseWidth = This->baseWidth;
//     createInfo.baseHeight = This->baseHeight;
//     createInfo.baseDepth = This->baseDepth;
//     createInfo.generateMipmaps = This->generateMipmaps;
//     createInfo.isArray = This->isArray;
//     createInfo.numDimensions = This->numDimensions;
//     createInfo.numFaces = This->numFaces;
//     createInfo.numLayers = This->numLayers;
//     createInfo.numLevels = This->numLevels;
//     createInfo.pDfd = nullptr;
//
//     ktxTexture2* prototype;
//     result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &prototype);
//
//     if (result != KTX_SUCCESS) {
//         assert(result == KTX_OUT_OF_MEMORY && "Out of memory allocating texture.");
//         return result;
//     }
//
//     uint32_t block_size_x{6};
//     uint32_t block_size_y{6};
//     uint32_t block_size_z{1};
//     float quality{0};
//     // uint32_t flags{params->normalMap ? ASTCENC_FLG_MAP_NORMAL : 0};
//
//     // Walk in reverse on levels so we don't have to do this later
//     assert(prototype->dataSize && "Prototype texture size not initialized.\n");
//
//     if (!prototype->pData) {
//         return KTX_OUT_OF_MEMORY;
//     }
//
//     uint8_t* buffer_out = prototype->pData;
//
//     for (int32_t level = This->numLevels - 1; level >= 0; level--) {
//         uint32_t width = MAX(1, This->baseWidth >> level);
//         uint32_t height = MAX(1, This->baseHeight >> level);
//         uint32_t depth = MAX(1, This->baseDepth >> level);
//         ktx_size_t levelImageSizeIn = 0;
//         ktx_size_t levelImageSizeOut = 0;
//         ktx_uint32_t levelImages = 0;
//
//         levelImages = This->numLayers * This->numFaces * depth;
//         levelImageSizeIn =
//             ktxTexture_calcImageSize(ktxTexture(This), level, KTX_FORMAT_VERSION_TWO);
//         levelImageSizeOut =
//             ktxTexture_calcImageSize(ktxTexture(prototype), level, KTX_FORMAT_VERSION_TWO);
//         ktx_size_t offset = ktxTexture2_levelDataOffset(This, level);
//
//         for (uint32_t image = 0; image < levelImages; image++) {
//         }
//     }
//
//     // We are done with astcencoder
//
//     assert(KHR_DFDVAL(prototype->pDfd + 1, MODEL) == KHR_DF_MODEL_ASTC &&
//            "Invalid dfd generated for ASTC image\n");
//     assert((transfer == KHR_DF_TRANSFER_SRGB
//                 ? KHR_DFDVAL(prototype->pDfd + 1, TRANSFER) == KHR_DF_TRANSFER_SRGB &&
//                       KHR_DFDVAL(prototype->pDfd + 1, PRIMARIES) == KHR_DF_PRIMARIES_SRGB
//                 : true) &&
//            "Not a valid sRGB image\n");
//
// // Fix up the current (This) texture
// #undef DECLARE_PRIVATE
// #undef DECLARE_PROTECTED
// #define DECLARE_PRIVATE(n, t2) ktxTexture2_private& n = *(t2->_private)
// #define DECLARE_PROTECTED(n, t2) ktxTexture_protected& n = *(t2->_protected)
//
//     DECLARE_PROTECTED(thisPrtctd, This);
//     DECLARE_PRIVATE(protoPriv, prototype);
//     DECLARE_PROTECTED(protoPrtctd, prototype);
//     memcpy(&thisPrtctd._formatSize, &protoPrtctd._formatSize, sizeof(ktxFormatSize));
//     // This->vkFormat = vkFormat;
//     This->isCompressed = prototype->isCompressed;
//     This->supercompressionScheme = KTX_SS_NONE;
//     This->_private->_requiredLevelAlignment = protoPriv._requiredLevelAlignment;
//     // Copy the levelIndex from the prototype to This.
//     memcpy(This->_private->_levelIndex, protoPriv._levelIndex,
//            This->numLevels * sizeof(ktxLevelIndexEntry));
//     // Move the DFD and data from the prototype to This.
//     free(This->pDfd);
//     This->pDfd = prototype->pDfd;
//     prototype->pDfd = 0;
//     free(This->pData);
//     This->pData = prototype->pData;
//     This->dataSize = prototype->dataSize;
//     prototype->pData = 0;
//     prototype->dataSize = 0;
//
//     ktxTexture2_Destroy(prototype);
//
//     KHR_DFDSETVAL(This->pDfd + 1, FLAGS, alphaMode);  // Restore alphaMode flags
//     return KTX_SUCCESS;
// }

/**
 * @memberof ktxTexture2
 * @ingroup writer
 * @~English
 * @brief Encode and compress a ktx texture with uncompressed images to astc.
 *
 * The images are either encoded to ASTC block-compressed format. The encoded images
 * replace the original images and the texture's fields including the DFD are modified to reflect
 the new
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
// extern "C" KTX_error_code
// ktxTexture2_CompressAstc(ktxTexture2* This, ktx_uint32_t quality) {
//     return ktxTexture2_CompressAstcEx(This, NULL);
// }

// extern "C" KTX_error_code
// ktxTexture2_CompressBcnEx(ktxTexture2*) {
//     return KTX_INVALID_OPERATION;
// }
//
// extern "C" KTX_error_code
// ktxTexture2_CompressBcn(ktxTexture2*, ktx_uint32_t) {
//     return KTX_INVALID_OPERATION;
// }
