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

#include "bcn_common.h"

#include "bc7enc_rdo/bc7decomp.h"  /* for BC7 decoder */
#include "bc7enc_rdo/bc6hdecomp.h" /* for BC6H decoder */
#include "bc7enc_rdo/rgbcx.h"      /* for BC1-BC5 encoders/decoders */
#include "vkformat_enum.h"         /* for VkFormat enum */
#include "ktxint.h"
#include "texture2.h"

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
 * Updates @p This with the decoded image.
 *
 * @param This     The texture to decode
 *
 * @return    KTX_SUCCESS on success, other KTX_* enum values on error.
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
    bool is_signed_bc6h = false;

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

    case KHR_DF_MODEL_BC6H:
        nchannels = BC6H_NCHANNELS; /* 3 */
        switch (This->vkFormat) {
            // TODO: UFLOAT is also mapped to VK_FORMAT_R16G16B16_SFLOAT?
        case VK_FORMAT_BC6H_SFLOAT_BLOCK:
            decompressedVkFormat = VK_FORMAT_R16G16B16_SFLOAT;
            is_signed_bc6h = true;
            break;
        case VK_FORMAT_BC6H_UFLOAT_BLOCK:
            decompressedVkFormat = VK_FORMAT_R16G16B16_SFLOAT;
            is_signed_bc6h = false;
            break;
        default:
            return KTX_INVALID_OPERATION;  // invalid vkFormat (should be BC6H)
        }
        break;

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
    const ktx_size_t rgba_pitch = BCN_BLOCK_SIZE * 4;
    ktx_uint8_t rgba[BCN_BLOCK_SIZE * rgba_pitch]; /* 64 bytes */

    const ktx_size_t rgbh_pitch = BCN_BLOCK_SIZE * 3;
    ktx_uint16_t rgbh[BCN_BLOCK_SIZE * BCN_BLOCK_SIZE * rgbh_pitch];

    for (uint32_t levelIndex = 0; levelIndex < This->numLevels; ++levelIndex) {
        const uint32_t width = std::max(This->baseWidth >> levelIndex, 1u);
        const uint32_t height = std::max(This->baseHeight >> levelIndex, 1u);
        const uint32_t depth = std::max(This->baseDepth >> levelIndex, 1u);

        for (uint32_t layerIndex = 0; layerIndex < This->numLayers; ++layerIndex) {
            for (uint32_t faceIndex = 0; faceIndex < This->numFaces; ++faceIndex) {
                for (uint32_t depthSliceIndex = 0; depthSliceIndex < depth; ++depthSliceIndex) {
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

                    const ktx_uint8_t* src_blocks = imageDataIn;

                    for (size_t y{0}; y < height; y += BCN_BLOCK_SIZE) {
                        for (size_t x{0}; x < width; x += BCN_BLOCK_SIZE) {
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

                            case KHR_DF_MODEL_BC6H:
                                // BC6H: 16 bytes -> 4 x 4 x 3 x 2 = 96 bytes
                                bc6hdecomp::bcdec_bc6h_half(src_blocks, rgbh, rgbh_pitch, is_signed_bc6h);
                                src_blocks += BC6H_BLOCK_SIZE;
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
                            // copy the decoded block into the actual texture image
                            colorModel == KHR_DF_MODEL_BC6H
                                ? insert_block<uint16_t>(reinterpret_cast<uint16_t*>(imageDataOut),
                                                         rgbh, x, y, width, height, nchannels)
                                : insert_block<uint8_t>(imageDataOut, rgba, x, y, width, height,
                                                        nchannels);
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
