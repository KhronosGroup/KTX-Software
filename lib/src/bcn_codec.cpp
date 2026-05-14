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
 * @brief Functions for compressing a texture to BCn format and decoding one in
 *        BCn format. Currently supported BCn formats are: BC1, BC3, BC4, BC5,
 *        and BC7.
 *
 * @author Walid Chtioui , individual contributor (walid.chtioui.main@gmail.com)
 */

#include "bcn_codec.h"

#include "bc7enc_rdo/bc7enc.h"    /* for BC7 encoder */
#include "bc7enc_rdo/bc7decomp.h" /* for BC7 decoder */
#include "bc7enc_rdo/ert.h"
#include "bc7enc_rdo/rgbcx.h" /* for BC1-BC5 encoders/decoders */

#include "vkformat_enum.h"
#include "ktxint.h"
#include "texture2.h"

//************************************************************************
//*                          Decoder functions                           *
//************************************************************************

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
 *
 * The decompressed format is calculated from corresponding BCn format.
 * For BC1, BC3, and BC7 the decompressed VkFormat is:
 * VK_FORMAT_R8G8B8A8_[UNORM|SRGB] depending on the original color space.
 * For BC4: VK_FORMAT_R8_UNORM
 * For BC5: VK_FORMAT_R8G8_UNORM
 *
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
 *                              The texture's images are not in BCn format
 *                              (i.e., either color model is not set to BCn or
 *                              This->vkFormat does not correspond to the set
 *                              BCn color model).
 * @exception KTX_INVALID_OPERATION
 *                              The texture object does not contain any data
 *                              (i.e., This->pData is NULL and there is no
 *                              pending data load).
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
    ktx_size_t nchannels;
    ktx_uint32_t decompressedVkFormat;

    switch (colorModel) {
    case KHR_DF_MODEL_BC1A:
        if (!(channelId == KHR_DF_CHANNEL_BC1A_COLOR || channelId == KHR_DF_CHANNEL_BC1A_ALPHA)) {
            return KTX_FILE_DATA_ERROR;
        }
        nchannels = BC1_NCHANNELS; /* 4 */
        // further sanity check on vkFormat
        switch (This->vkFormat) {
        // TODO: encode BC1 RGB (no Alpha) into RGB
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
            decompressedVkFormat = VK_FORMAT_R8G8B8A8_UNORM;
            break;
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
            decompressedVkFormat = VK_FORMAT_R8G8B8A8_SRGB;
            break;
        default:
            return KTX_INVALID_OPERATION;  // invalid vkFormat (should be BC1)
        }
        // TODO: expose as parameter
        rgbcx::init(rgbcx::bc1_approx_mode::cBC1Ideal);
        break;

    case KHR_DF_MODEL_BC3:
        nchannels = BC3_NCHANNELS; /* 4 */
        switch (This->vkFormat) {
        case VK_FORMAT_BC3_UNORM_BLOCK:
            decompressedVkFormat = VK_FORMAT_R8G8B8A8_UNORM;
            break;
        case VK_FORMAT_BC3_SRGB_BLOCK:
            decompressedVkFormat = VK_FORMAT_R8G8B8A8_SRGB;
            break;
        default:
            return KTX_INVALID_OPERATION;  // invalid vkFormat (should be BC3)
        }
        rgbcx::init(rgbcx::bc1_approx_mode::cBC1Ideal);
        break;

    case KHR_DF_MODEL_BC4:
        nchannels = BC4_NCHANNELS; /* 1 */
        switch (This->vkFormat) {
        case VK_FORMAT_BC4_UNORM_BLOCK:
            decompressedVkFormat = VK_FORMAT_R8_UNORM;
            break;
        case VK_FORMAT_BC4_SNORM_BLOCK:
            decompressedVkFormat = VK_FORMAT_R8_SNORM;
            break;
        default:
            return KTX_INVALID_OPERATION;  // invalid vkFormat (should be BC4)
        }
        rgbcx::init(rgbcx::bc1_approx_mode::cBC1Ideal);
        break;

    case KHR_DF_MODEL_BC5:
        nchannels = BC5_NCHANNELS; /* 2 */
        switch (This->vkFormat) {
        case VK_FORMAT_BC5_UNORM_BLOCK:
            decompressedVkFormat = VK_FORMAT_R8G8_UNORM;
            break;
        case VK_FORMAT_BC5_SNORM_BLOCK:
            decompressedVkFormat = VK_FORMAT_R8G8_SNORM;
            break;
        default:
            return KTX_INVALID_OPERATION;  // invalid vkFormat (should be BC5)
        }
        rgbcx::init(rgbcx::bc1_approx_mode::cBC1Ideal);
        break;

#if 0
    case KHR_DF_MODEL_BC6H:
        break;
#endif

    case KHR_DF_MODEL_BC7:
        nchannels = BC7_NCHANNELS; /* 4 */
        switch (This->vkFormat) {
        case VK_FORMAT_BC7_UNORM_BLOCK:
            decompressedVkFormat = VK_FORMAT_R8G8B8A8_UNORM;
            break;
        case VK_FORMAT_BC7_SRGB_BLOCK:
            decompressedVkFormat = VK_FORMAT_R8G8B8A8_SRGB;
            break;
        default:
            return KTX_INVALID_OPERATION;  // invalid vkFormat (should be BC7)
        }
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

    // Create a prototype texture to use for calculating sizes in the target
    // format and, as useful side effects, provide us with a properly sized
    // data allocation and the DFD for the target format.
    ktxTextureCreateInfo createInfo;
    createInfo.glInternalformat = 0;
    createInfo.vkFormat = decompressedVkFormat;
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

    // Create intermediate storage to store decoded blocks. Not all blocks
    // necessarily decode to 4x4x4 but this is enough to hold all possible
    // combinations (at least for LDR - i.e., not for BC6H formats).
    const ktx_size_t rgba_pitch = BCN_BLOCK_SIZE * BC1_NCHANNELS;
    ktx_uint8_t rgba[BCN_BLOCK_SIZE * rgba_pitch]; /* 64 bytes */

    for (uint32_t levelIndex = 0; levelIndex < This->numLevels; ++levelIndex) {
        const uint32_t imageWidth = std::max(This->baseWidth >> levelIndex, 1u);
        const uint32_t imageHeight = std::max(This->baseHeight >> levelIndex, 1u);
        const uint32_t imageDepths = std::max(This->baseDepth >> levelIndex, 1u);

        for (uint32_t layerIndex = 0; layerIndex < This->numLayers; ++layerIndex) {
            for (uint32_t faceIndex = 0; faceIndex < This->numFaces; ++faceIndex) {
                for (uint32_t depthSliceIndex = 0; depthSliceIndex < imageDepths;
                     ++depthSliceIndex) {
                    ktx_size_t imageOffsetIn;
                    ktx_size_t imageOffsetOut;

                    // TODO: are we sure these can't fail? (i.e., return != KTX_SUCCESS)
                    ktxTexture2_GetImageOffset(This, levelIndex, layerIndex,
                                               faceIndex + depthSliceIndex, &imageOffsetIn);
                    ktxTexture2_GetImageOffset(prototype, levelIndex, layerIndex,
                                               faceIndex + depthSliceIndex, &imageOffsetOut);

                    const ktx_uint8_t* imageDataIn = This->pData + imageOffsetIn;
                    ktx_uint8_t* imageDataOut = prototype->pData + imageOffsetOut;

                    // Probably very little benefit of using multithreading here

                    const ktx_size_t dst_pitch = imageWidth * nchannels;
                    const ktx_uint8_t* src_blocks = imageDataIn;

                    for (size_t y{0}; y < imageHeight; y += BCN_BLOCK_SIZE) {
                        for (size_t x{0}; x < imageWidth; x += BCN_BLOCK_SIZE) {
                            switch (colorModel) {
                            case KHR_DF_MODEL_BC1A:
                                // BC1: 8 bytes -> 4 x 4 x 4 = 64 bytes
                                // TODO: BC1 with punchthrough alpha should already be supported,
                                // right? (since we already write to 4x4x4 block and then to an RGBA
                                // texture)?
                                rgbcx::unpack_bc1(src_blocks, rgba, true);
                                src_blocks += BC1_BLOCK_SIZE;
                                break;

                            case KHR_DF_MODEL_BC3:
                                // BC3: 16 bytes -> 4 x 4 x 4 = 64 bytes
                                rgbcx::unpack_bc3(src_blocks, rgba);
                                src_blocks += BC3_BLOCK_SIZE;
                                break;

                            case KHR_DF_MODEL_BC4:
                                // BC4: 8 bytes -> 4 x 4 x 1 = 16 bytes
                                rgbcx::unpack_bc4(src_blocks, rgba,
                                                  /* stride */ BC4_NCHANNELS);
                                src_blocks += BC4_BLOCK_SIZE;
                                break;

                            case KHR_DF_MODEL_BC5:
                                // BC5: 16 bytes -> 4 x 4 x 2 = 32 bytes
                                rgbcx::unpack_bc5(src_blocks, rgba, 0, 1,
                                                  /* stride */ BC5_NCHANNELS);
                                src_blocks += BC5_BLOCK_SIZE;
                                break;

                            case KHR_DF_MODEL_BC7:
                                // BC7: 16 bytes -> 4 x 4 x 4 = 64 bytes
                                bc7decomp::unpack_bc7(
                                    src_blocks, reinterpret_cast<bc7decomp::color_rgba*>(rgba));
                                src_blocks += BC7_BLOCK_SIZE;
                                break;

                            default:
                                break;  // should never occur
                            }

                            // now we copy the decoded block into the actual
                            // texture image while taking into consideration
                            // that dimenions may not be a multiple of 4.
                            const ktx_uint8_t* pSrc = rgba;
                            ktx_uint8_t* pDst = imageDataOut + y * dst_pitch + nchannels * x;
                            ktx_size_t cols = fmin(BCN_BLOCK_SIZE, imageWidth - x);
                            for (ktx_size_t py{0}; py < BCN_BLOCK_SIZE && y + py < imageHeight;
                                 ++py) {
                                memcpy(pDst, pSrc, cols * nchannels);
                                pSrc += rgba_pitch;
                                pDst += dst_pitch;
                            }
                        }  // x blocks
                    }  // y blocks
                }  // depth slices
            }  // faces
        }  // layers
    }

    if (result == KTX_SUCCESS) {
        // Fix up the current texture
        DECLARE_PROTECTED_EX(thisPrtctd, This);
        DECLARE_PRIVATE_EX(protoPriv, prototype);
        DECLARE_PROTECTED_EX(protoPrtctd, prototype);
        memcpy(&thisPrtctd._formatSize, &protoPrtctd._formatSize, sizeof(ktxFormatSize));
        This->vkFormat = decompressedVkFormat;
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
 * @brief Encode and compress a ktx texture with uncompressed images to provided
 *        BCn format. Currently, only BC1, BC3, BC4, BC5, and BC7 target formats
 *        are supported.
 *
 * The images are encoded to BCn block-compressed format. The encoded images
 * replace the original images and the texture's fields including the DFD are
 * modified to reflect the new state.
 *
 * Such textures can be directly uploaded to a GPU via a graphics API.
 *
 * @param[in]   This   pointer to the ktxTexture2 object of interest.
 * @param[in]   params pointer to BCn params object.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_OPERATION
 *                              The texture's images are supercompressed.
 * @exception KTX_INVALID_OPERATION
 *                              The texture's images are already in a block
 *                              compressed format (i.e., This->isCompressed is
 *                              true).
 * @exception KTX_INVALID_OPERATION
 *                              The texture image's format is a packed format
 *                              (e.g. RGB565).
 * @exception KTX_INVALID_OPERATION
 *                              The texture image format's component size is not
 *                              8-bits.
 * @exception KTX_INVALID_OPERATION
 *                              The texture's images are 1D. Only 2D images can
 *                              be block compressed.
 * @exception  KTX_INVALID_OPERATION
 *                              Transfer function of @c This is not sRGB or
 *                              Linear.
 * @exception  KTX_INVALID_OPERATION
 *                              @c params->mode  is HDR but transfer function
 *                              of @c This is sRGB.
 * @exception KTX_INVALID_OPERATION
 *                              This->generateMipmaps is set.
 * @exception KTX_OUT_OF_MEMORY Not enough memory to carry out compression.
 * @exception KTX_UNSUPPORTED_FEATURE
 *                              @c params->mode is HDR mode which is not
 *                              yet implemented.
 */
extern "C" KTX_error_code
ktxTexture2_CompressBCnEx(ktxTexture2* This, ktxBCnParams* params) {
    assert(This->classId == ktxTexture2_c && "Only support ktx2 BCn.");

    ktx_error_code_e result;

    if (!params) return KTX_INVALID_VALUE;

    if (params->structSize != sizeof(struct ktxBCnParams)) return KTX_INVALID_VALUE;

    // TODO: why?
    if (This->generateMipmaps) return KTX_INVALID_OPERATION;

    if (This->supercompressionScheme != KTX_SS_NONE)
        return KTX_INVALID_OPERATION;  // Can't apply multiple schemes.

    if (This->isCompressed)
        return KTX_INVALID_OPERATION;  // Only non-block compressed formats
                                       // can be encoded into a BCn format.

    if (This->_protected->_formatSize.flags & KTX_FORMAT_SIZE_PACKED_BIT)
        return KTX_INVALID_OPERATION;

    // Basic descriptor block begins after the total size field.
    const uint32_t* BDB = This->pDfd + 1;
    ktx_uint8_t alphaMode = KHR_DFDVAL(BDB, FLAGS);
    size_t nchannels;
    size_t blocksize_in_bytes;
    VkFormat compressedVkFormat;
    bc7enc_compress_block_params bc7_cmp_params;

    switch (params->bcn) {
    case KHR_DF_MODEL_BC1A:
        // Currently we can only encode RGBA uncompressed textures into BC1
        // (TODO: I think this makes sense, if we have an R8 channel then using
        // BC1 makes no sense in the first place).
        switch (This->vkFormat) {
        case VK_FORMAT_R8G8B8_UNORM:
            compressedVkFormat = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
            break;
        case VK_FORMAT_R8G8B8_SRGB:
            compressedVkFormat = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
            break;
        case VK_FORMAT_R8G8B8A8_UNORM:
            compressedVkFormat = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
            break;
        case VK_FORMAT_R8G8B8A8_SRGB:
            compressedVkFormat = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
            break;
        default:
            return KTX_INVALID_OPERATION;  // Not a valid decompressed vkformat for BC1
        }
        nchannels = BC1_NCHANNELS;
        blocksize_in_bytes = BC1_BLOCK_SIZE;
        rgbcx::init(static_cast<rgbcx::bc1_approx_mode>(params->bc1ApproxMode));
        break;

    case KHR_DF_MODEL_BC3:
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
        rgbcx::init(static_cast<rgbcx::bc1_approx_mode>(params->bc1ApproxMode));
        break;

    case KHR_DF_MODEL_BC4:
        switch (This->vkFormat) {
        case VK_FORMAT_R8_UNORM:
            compressedVkFormat = VK_FORMAT_BC4_UNORM_BLOCK;
            break;
        case VK_FORMAT_R8_SNORM:
            compressedVkFormat = VK_FORMAT_BC4_SNORM_BLOCK;
            break;
        default:
            return KTX_INVALID_OPERATION;  // Not a valid decompressed vkformat for BC4
        }
        nchannels = BC4_NCHANNELS;
        blocksize_in_bytes = BC4_BLOCK_SIZE;
        rgbcx::init(static_cast<rgbcx::bc1_approx_mode>(params->bc1ApproxMode));
        break;

    case KHR_DF_MODEL_BC5:
        switch (This->vkFormat) {
        case VK_FORMAT_R8G8_UNORM:
            compressedVkFormat = VK_FORMAT_BC5_UNORM_BLOCK;
            break;
        case VK_FORMAT_R8G8_SNORM:
            compressedVkFormat = VK_FORMAT_BC5_SNORM_BLOCK;
            break;
        default:
            return KTX_INVALID_OPERATION;  // Not a valid decompressed vkformat for BC5
        }
        nchannels = BC5_NCHANNELS;
        blocksize_in_bytes = BC5_BLOCK_SIZE;
        rgbcx::init(static_cast<rgbcx::bc1_approx_mode>(params->bc1ApproxMode));
        break;

    case KHR_DF_MODEL_BC7:
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
        // MUST be called before calling bc7enc_compress_block() (or you'll get artifacts).
        bc7enc_compress_block_init();
        bc7enc_compress_block_params_init(&bc7_cmp_params);
        break;

    default:
        return KTX_INVALID_VALUE;  // Provided color model is not BCn
    }

    if (This->pData == NULL) {
        result = ktxTexture2_LoadImageData((ktxTexture2*)This, nullptr, 0);
        if (result != KTX_SUCCESS) return result;
    }

    ktx_uint32_t threadCount = params->threadCount;
    if (threadCount < 1) threadCount = 1;

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

    if (!prototype->pData) {
        return KTX_OUT_OF_MEMORY;
    }

    // This is the intermediate store for decoded LDR BCn block
    const size_t pixels_pitch = BCN_BLOCK_SIZE * 4;  // 4 x 4
    uint8_t pPixels[BCN_BLOCK_SIZE * pixels_pitch];  // 4 x 4 x 4

#if 0  // for BC6H blocks
    uint16_t pPixelsHdr [BCN_BLOCK_SIZE * BCN_BLOCK_SIZE * 4];  // 4 x 4 x 4
#endif

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
        // level compact or are there instances where a padding is added?
        assert(levelImageSizeIn == width * height * nchannels * sizeof(uint8_t) &&
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

        // TODO: add and profile multithreading (contrary to decoding, encoding
        // BC7 takes singnificantly much longer).

        for (uint32_t image = 0; image < levelImages; image++) {
            // Row-major loop over blocks
            for (size_t y{0}; y < height; y += BCN_BLOCK_SIZE) {
                for (size_t x{0}; x < width; x += BCN_BLOCK_SIZE) {
                    // Extract/Copy source block (non-multiple-of-4 texture dimensions are handled
                    // via clamping to edge).
                    extract_block(pPixels, pSrcLevelImage, x, y, width, height, nchannels);

                    const auto xBlock = x / BCN_BLOCK_SIZE;
                    const auto yBlock = y / BCN_BLOCK_SIZE;

                    switch (params->bcn) {
                    case KHR_DF_MODEL_BC1A:
                        // BC1: 4 x 4 x 4 = 64 bytes -> 8 bytes
                        rgbcx::encode_bc1(
                            params->bc1CompressionQuality,
                            pDstLevelImage + (yBlock * nbrBlocksX + xBlock) * BC1_BLOCK_SIZE,
                            reinterpret_cast<const uint8_t*>(pPixels), true, false);
                        break;

                    case KHR_DF_MODEL_BC3:
                        // BC3: 4 x 4 x 4 = 64 bytes -> 16 bytes
                        rgbcx::encode_bc3(
                            params->bc1CompressionQuality,
                            pDstLevelImage + (yBlock * nbrBlocksX + xBlock) * BC3_BLOCK_SIZE,
                            reinterpret_cast<const uint8_t*>(pPixels));
                        break;

                    case KHR_DF_MODEL_BC4:
                        // BC4: 4 x 4 x 1 = 16 bytes -> 8 bytes
                        rgbcx::encode_bc4(
                            pDstLevelImage + (yBlock * nbrBlocksX + xBlock) * BC4_BLOCK_SIZE,
                            reinterpret_cast<const uint8_t*>(pPixels),
                            /* stride */ BC4_NCHANNELS);
                        break;

                    case KHR_DF_MODEL_BC5:
                        // BC5: 4 x 4 x 2 = 32 bytes -> 16 bytes
                        rgbcx::encode_bc5(
                            pDstLevelImage + (yBlock * nbrBlocksX + xBlock) * BC5_BLOCK_SIZE,
                            reinterpret_cast<const uint8_t*>(pPixels), 0, 1,
                            /* stride */ BC5_NCHANNELS);
                        break;

                    case KHR_DF_MODEL_BC7:
                        // BC7: 4 x 4 x 4 = 64 bytes -> 16 bytes
                        bc7enc_compress_block(
                            pDstLevelImage + (yBlock * nbrBlocksX + xBlock) * BC7_BLOCK_SIZE,
                            reinterpret_cast<const uint8_t*>(pPixels), &bc7_cmp_params);
                        break;

                    default:
                        return KTX_INVALID_VALUE;  // should never occur
                    }
                }  // x blocks
            }  // y blocks

            // post process the encoded blocks using RDO (if enabled)
            if (params->rdo) {
                rdoParams rdo_p;
                rdo_p.ert_p.m_lambda = params->rdoQualityScalar;
                rdo_p.ert_p.m_lookback_window_size = params->rdoWindowLoopbackSize;
                rdo_p.ert_p.m_smooth_block_max_mse_scale = params->rdoMaxSmoothBlockMseScale;
                rdo_p.ert_p.m_max_smooth_block_std_dev = params->rdoMaxSmoothBlockStdDev;
                rdo_p.ert_p.m_debug_output = true;
                rdo_p.ert_p.m_try_two_matches = false;
                rdo_p.ert_p.m_allow_relative_movement = false;
                rdo_p.ert_p.m_skip_zero_mse_blocks = false;  // always set to false
                rdo_p.auto_smooth_block_max_mse_scale = params->rdoAutoSmoothBlockMaxMSEScale;
                rdo_p.bc1_approx_mode = params->bc1ApproxMode;
                rdo_p.allow_3color_mode = true;
                rdo_p.use_3color_mode_for_black = false;
                postprocess_rdo_bcn(pSrcLevelImage, width * height * nchannels, pDstLevelImage,
                                    nbrBlocksX * nbrBlocksY * blocksize_in_bytes, rdo_p,
                                    params->bcn, width, height);
            }

            pDstLevelImage += levelImageSizeOut;  // next destination image within this miplevel
            pSrcLevelImage += levelImageSizeIn;   // next source image within this miplevel
        }
    }

    assert(KHR_DFDVAL(prototype->pDfd + 1, MODEL) == params->bcn &&
           "Invalid dfd generated for BCn image\n");
    // assert((transfer == KHR_DF_TRANSFER_SRGB
    //             ? KHR_DFDVAL(prototype->pDfd + 1, TRANSFER) == KHR_DF_TRANSFER_SRGB &&
    //                   KHR_DFDVAL(prototype->pDfd + 1, PRIMARIES) == KHR_DF_PRIMARIES_SRGB
    //             : true) &&
    //        "Not a valid sRGB image\n");

// Fix up the current (This) texture (this is copied as is from ASTC encoder - see astc_codec.cpp)
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

/**
 * @~English
 * @brief Performs rate distorion optimization (RDO) on the provided BCn-encoded
 *        blocks to reduce entropy for a potential subsequent Deflate step.
 *        BC2 and BC6H formats are currently not supported.
 *
 *        Some values of the reduce_entropy_params struct may be adjusted before
 *        being passed to the underlying RDO subroutine. The reason for this is
 *        that, depending on the BCn compression, some values make no sense and
 *        may kill efficiency.
 *
 *
 *
 * @param[in]   unpacked_img pointer to the source unpacked data.
 * @param[in]   unpacked_img_size size of source unpacked data in bytes.
 *              Internal sanity checks are performed on this provided size.
 * @param[in,out]   packed_img pointer to the packed/encoded image data.
 *              These packed/encoded blocks are modified to reduce bit/texel
 *              rate which is measured via an LZ (Deflate) compression
 *              simulation while keeping distorion minimal (i.e., difference
 *              between decoded/unpacked and actual source image data pointed
 *              to by unpacked_img).
 * @param[in]   packed_img_size size of packed/encoded data in bytes.
 *              Internal sanity checks are performed on this provided size.
 * @param[in]   ert_p.
 *              No sanitation/checks are performed on the provided
 *              reduce_entropy_params. The called has to make sure that provided
 *              params are valid (i.e., in range for floats).
 * @param[in]   model used to determine the BCn compression of the provided
 *              compressed blocks.
 * @param[in]   width image width (in texels/pixels).
 * @param[in]   height image height (in texels/pixels).
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 */
KTX_error_code
postprocess_rdo_bcn(const ktx_uint8_t* unpacked_img, ktx_size_t unpacked_img_size,
                    ktx_uint8_t* packed_img, ktx_size_t packed_img_size, rdoParams params,
                    khr_df_model_e model, ktx_uint32_t width, ktx_uint32_t height) {
    const uint32_t nBlocksX = (width + BCN_BLOCK_SIZE - 1) / BCN_BLOCK_SIZE;
    const uint32_t nBlocksY = (height + BCN_BLOCK_SIZE - 1) / BCN_BLOCK_SIZE;
    const uint32_t nBlocksTotal = nBlocksX * nBlocksY;

    // Intermediate storage for extracted blocks. This is mainly for convenience
    // so that we do not have to repeat logic for extracting and potentially
    // padding a block of 4x4 RGBA pixels
    const size_t rgba_pitch = BCN_BLOCK_SIZE * 4;  // 4 x 4
    uint8_t rgba[BCN_BLOCK_SIZE * rgba_pitch];     // 4 x 4 x 4
    auto& ert_p = params.ert_p;

    auto unpack_block_bc1_func = [](const void* pBlock, ert::color_rgba* pPixels, uint32_t,
                                    void* pUser_data) -> bool {
        auto bc1_usr_data = reinterpret_cast<rdoParams*>(pUser_data);
        bool used_3color =
            rgbcx::unpack_bc1(pBlock, pPixels, true,
                              static_cast<rgbcx::bc1_approx_mode>(bc1_usr_data->bc1_approx_mode));
        if (used_3color) {
            if (!bc1_usr_data->allow_3color_mode) return false;
            if (!bc1_usr_data->use_3color_mode_for_black) {
                rgbcx::bc1_block* pBC1_block = (rgbcx::bc1_block*)pBlock;
                for (uint32_t y = 0; y < BCN_BLOCK_SIZE; y++)
                    for (uint32_t x = 0; x < BCN_BLOCK_SIZE; x++)
                        if (pBC1_block->get_selector(x, y) == 3) return false;
            }
        }
        return true;
    };

    auto unpack_block_bc4_func = [](const void* pBlock, ert::color_rgba* pPixels, uint32_t,
                                    void*) -> bool {
        memset(pPixels, 0, sizeof(ert::color_rgba) * 16);
        rgbcx::unpack_bc4(pBlock, reinterpret_cast<uint8_t*>(pPixels), 4);
        return true;
    };

    auto unpack_block_bc7_func = [](const void* pBlock, ert::color_rgba* pPixels, uint32_t,
                                    void*) -> bool {
        return bc7decomp::unpack_bc7(pBlock, reinterpret_cast<bc7decomp::color_rgba*>(pPixels));
    };

    if (ert_p.m_lambda <= 0.0f) return KTX_SUCCESS;

    // TODO: add RDO parallel-for multithreading
    switch (model) {
    case KHR_DF_MODEL_BC7: {
        // Some sanity checks
        assert(unpacked_img_size == width * height * BC7_NCHANNELS);
        assert(packed_img_size == nBlocksTotal * BC7_BLOCK_SIZE);

        // Attempt to compute a decent conservative smooth block MSE max scaling
        // factor. No single smooth block scale setting can work for all
        // textures (unless it's ridiuclously large, killing efficiency).
        if (params.auto_smooth_block_max_mse_scale) {
            ert_p.m_smooth_block_max_mse_scale =
                lerp(15.0f, 50.0f, std::min(1.0f, ert_p.m_lambda / 4.0f));
            // uncomment for debugging
            if (ert_p.m_debug_output)
                printf("Using an automatically computed smooth block error scale of %f\n",
                       ert_p.m_smooth_block_max_mse_scale);
        }

        // Source image data need to be laid out as blocks (i.e., 1st raw of 1s
        // block, then 2nd raw of 1st block, etc. - Not: 1st raw of 1st block
        // then 1st raw of 2nd block, etc.). This is what the main RDO function
        // ert::reduce_entropy expects.
        std::vector<ert::color_rgba> block_pixels(nBlocksTotal * BCN_BLOCK_SIZE * BCN_BLOCK_SIZE);
        for (uint32_t y = 0; y < height; y += BCN_BLOCK_SIZE) {
            for (uint32_t x = 0; x < width; x += BCN_BLOCK_SIZE) {
                // Extract block (non-multiple-of-4 texture dimensions are handled).
                extract_block(rgba, unpacked_img, x, y, width, height, BC7_NCHANNELS);
                // Now flatten the extracted block into block_pixels
                ert::color_rgba* p_dst =
                    block_pixels.data() + x * BCN_BLOCK_SIZE + y * BCN_BLOCK_SIZE * nBlocksX;
                for (size_t py{0}; py < BCN_BLOCK_SIZE; ++py) {
                    memcpy(p_dst + py * BCN_BLOCK_SIZE, rgba + py * rgba_pitch,
                           BCN_BLOCK_SIZE * BC7_NCHANNELS);
                }
            }
        }

        clock_t rdo_start_t = clock();
        uint32_t total_modified_local = 0;
        {
            ert::reduce_entropy(packed_img, nBlocksTotal, BC7_BLOCK_SIZE, BC7_BLOCK_SIZE,
                                BCN_BLOCK_SIZE, BCN_BLOCK_SIZE, 4, block_pixels.data(), ert_p,
                                total_modified_local, unpack_block_bc7_func, &params);
        }
        clock_t rdo_end_t = clock();

        // uncomment for debugging
        if (ert_p.m_debug_output) {
            printf("ERT params:\n");
            ert_p.print();
            printf("Total RDO time: %f secs\n", (double)(rdo_end_t - rdo_start_t) / CLOCKS_PER_SEC);
            printf("Total nbr modified blocks: %i\n", total_modified_local);
        }

        break;
    }  // BC7

    case KHR_DF_MODEL_BC1A: {
        assert(unpacked_img_size == width * height * BC1_NCHANNELS);
        assert(packed_img_size == nBlocksTotal * BC1_BLOCK_SIZE);

        ert_p.m_color_weights[3] = 0;

        if (params.auto_smooth_block_max_mse_scale) {
            ert_p.m_smooth_block_max_mse_scale =
                lerp(15.0f, 50.0f, std::min(1.0f, ert_p.m_lambda / 8.0f));
            // uncomment for debugging
            if (ert_p.m_debug_output)
                printf("Using an automatically computed smooth block error scale of %f\n",
                       ert_p.m_smooth_block_max_mse_scale);
        }

        std::vector<ert::color_rgba> block_pixels(nBlocksTotal * BCN_BLOCK_SIZE * BCN_BLOCK_SIZE);
        for (uint32_t y = 0; y < height; y += BCN_BLOCK_SIZE) {
            for (uint32_t x = 0; x < width; x += BCN_BLOCK_SIZE) {
                // Extract block (non-multiple-of-4 texture dimensions are handled).
                extract_block(rgba, unpacked_img, x, y, width, height, BC1_NCHANNELS);
                // Now flatten the extracted block into block_pixels
                ert::color_rgba* p_dst =
                    block_pixels.data() + x * BCN_BLOCK_SIZE + y * BCN_BLOCK_SIZE * nBlocksX;
                for (size_t py{0}; py < BCN_BLOCK_SIZE; ++py) {
                    memcpy(p_dst + py * BCN_BLOCK_SIZE, rgba + py * rgba_pitch,
                           BCN_BLOCK_SIZE * BC1_NCHANNELS);
                }
            }
        }

        clock_t rdo_start_t = clock();
        uint32_t total_modified_local = 0;
        {
            ert::reduce_entropy(packed_img, nBlocksTotal, BC1_BLOCK_SIZE, BC1_BLOCK_SIZE,
                                BCN_BLOCK_SIZE, BCN_BLOCK_SIZE, 3 /* alpha channel ignored */,
                                block_pixels.data(), ert_p, total_modified_local,
                                unpack_block_bc1_func, &params);
        }
        clock_t rdo_end_t = clock();

        // uncomment for debugging
        if (ert_p.m_debug_output) {
            printf("ERT params:\n");
            ert_p.print();
            printf("Total RDO time: %f secs\n", (double)(rdo_end_t - rdo_start_t) / CLOCKS_PER_SEC);
            printf("Total nbr modified blocks: %i\n", total_modified_local);
        }

        break;
    }  // BC1

    case KHR_DF_MODEL_BC3: {
        assert(unpacked_img_size == width * height * BC3_NCHANNELS);
        assert(packed_img_size == nBlocksTotal * BC3_BLOCK_SIZE);

        ert_p.m_color_weights[3] = 0;

        ert::reduce_entropy_params ert_alpha_p(ert_p);
        ert_alpha_p.m_color_weights[1] = 0;
        ert_alpha_p.m_color_weights[2] = 0;
        ert_alpha_p.m_color_weights[3] = 0;

        if (params.auto_smooth_block_max_mse_scale) {
            ert_p.m_smooth_block_max_mse_scale =
                lerp(15.0f, 50.0f, std::min(1.0f, ert_p.m_lambda / 8.0f));
            ert_alpha_p.m_smooth_block_max_mse_scale =
                lerp(10.0f, 30.0f, std::min(1.0f, ert_p.m_lambda / 4.0f));
            // uncomment for debugging
            if (ert_p.m_debug_output) {
                printf(
                    "Using an automatically computed smooth block error scale of %f for RGB "
                    "blocks\n",
                    ert_p.m_smooth_block_max_mse_scale);
                printf(
                    "Using an automatically computed smooth block error scale of %f for Alpha "
                    "blocks\n",
                    ert_alpha_p.m_smooth_block_max_mse_scale);
            }
        }

        std::vector<ert::color_rgba> block_pixels(nBlocksTotal * BCN_BLOCK_SIZE * BCN_BLOCK_SIZE);
        for (uint32_t y = 0; y < height; y += BCN_BLOCK_SIZE) {
            for (uint32_t x = 0; x < width; x += BCN_BLOCK_SIZE) {
                // Extract block (non-multiple-of-4 texture dimensions are handled).
                extract_block(rgba, unpacked_img, x, y, width, height, BC3_NCHANNELS);
                // Now flatten the extracted block into block_pixels
                ert::color_rgba* p_dst =
                    block_pixels.data() + x * BCN_BLOCK_SIZE + y * BCN_BLOCK_SIZE * nBlocksX;
                for (size_t py{0}; py < BCN_BLOCK_SIZE; ++py) {
                    memcpy(p_dst + py * BCN_BLOCK_SIZE, rgba + py * rgba_pitch,
                           BCN_BLOCK_SIZE * BC3_NCHANNELS);
                }
            }
        }

        // TODO: optimize this - currently we have to copy the whole array
        // because ert::reduce_entropy expects an rgba array even if we are only
        // using the alpha channel.
        std::vector<ert::color_rgba> block_pixels_a(block_pixels.size());
        for (size_t i{0}; i < block_pixels_a.size(); ++i) {
            block_pixels_a[i].m_c[0] = block_pixels[i].m_c[3];  // alpha
            block_pixels_a[i].m_c[1] = 0u;
            block_pixels_a[i].m_c[2] = 0u;
            block_pixels_a[i].m_c[3] = 0u;
        }

        clock_t rdo_start_t = clock();
        uint32_t total_modified_local_rgb = 0;
        uint32_t total_modified_local_alpha = 0;
        {
            // BC3 RDO - One BC4 block followed by one BC1 block
            // reduce entropy for the BC4 block ...
            ert::reduce_entropy(packed_img, nBlocksTotal, BC3_BLOCK_SIZE /* 2 x 8 */,
                                BC4_BLOCK_SIZE, BCN_BLOCK_SIZE, BCN_BLOCK_SIZE, 1,
                                block_pixels_a.data(), ert_alpha_p, total_modified_local_alpha,
                                unpack_block_bc4_func, &params);
            //  then the BC1 block ...
            ert::reduce_entropy(packed_img + BC4_BLOCK_SIZE, nBlocksTotal,
                                BC3_BLOCK_SIZE /* 2 x 8 */, BC1_BLOCK_SIZE, BCN_BLOCK_SIZE,
                                BCN_BLOCK_SIZE, 3 /* alpha channel ignored */, block_pixels.data(),
                                ert_p, total_modified_local_rgb, unpack_block_bc1_func, &params);
        }
        clock_t rdo_end_t = clock();

        // uncomment for debugging
        if (ert_p.m_debug_output) {
            printf("ERT RGB params:\n");
            ert_p.print();
            printf("ERT Alpha params:\n");
            ert_alpha_p.print();
            printf("Total RDO time: %f secs\n", (double)(rdo_end_t - rdo_start_t) / CLOCKS_PER_SEC);
            printf("Total nbr modified RGB blocks: %i\n", total_modified_local_rgb);
            printf("Total nbr modified Alpha blocks: %i\n", total_modified_local_alpha);
        }

        break;
    }  // BC3

    case KHR_DF_MODEL_BC4: {
        break;
    }  // BC4

    case KHR_DF_MODEL_BC5: {
        break;
    }  // BC5

    default:
        return KTX_INVALID_VALUE;  // not a supported/valid BCn color model
    }

    return KTX_SUCCESS;
}

/**
 * @memberof ktxTexture2
 * @ingroup writer
 * @~English
 * @brief Encode and compress a ktx texture with uncompressed images to astc.
 *
 * The images are either encoded to ASTC block-compressed format. The encoded images
 * replace the original images and the texture's fields including the DFD are modified to
 reflect the new
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
ktxTexture2_CompressBcnEx(ktxTexture2*, ktxBCnParams) {
    return KTX_INVALID_OPERATION;
}

extern "C" KTX_error_code
ktxTexture2_CompressBcn(ktxTexture2*, ktx_uint32_t) {
    return KTX_INVALID_OPERATION;
}
