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
 * @brief Functions for decoding a BCn-compressed texture. All BCn formats are
 * supported (BC1, BC2, BC3, BC4, BC5, BC6HU, BC6HS, and BC7).
 *
 * @author Walid Chtioui, independent contributor (walid.chtioui.main@gmail.com)
 */

#include "bcn_common.h"

#include "encoder/basisu_gpu_texture.h" /* for BC6HU/BC6HS decoders */
#include "rgbcx.h"
#include "transcoder/basisu_transcoder_internal.h"
#include "vkformat_enum.h" /* for VkFormat enum */
#include "ktxint.h"
#include "texture2.h"

#define DECLARE_PRIVATE_EX(n, t2) ktxTexture2_private& n = *(t2->_private)
#define DECLARE_PROTECTED_EX(n, t2) ktxTexture_protected& n = *(t2->_protected)

static inline bool
unpack_block_bc1(const void* pBlock, void* pPixels, rgbcx::bc1_approx_mode bc1_approx_mode) {
    const bool allow_3color_mode = true;           // hardcoded
    const bool use_3color_mode_for_black = false;  // hardcoded
    bool used_3color = rgbcx::unpack_bc1(pBlock, pPixels, true, bc1_approx_mode);
    // This check is copied from the original code at: rdo_bc_encoder.h
    if (used_3color) {
        if (!allow_3color_mode) return false;
        if (!use_3color_mode_for_black) {
            auto pBC1_block = static_cast<const rgbcx::bc1_block*>(pBlock);
            for (uint32_t y = 0; y < 4; ++y)
                for (uint32_t x = 0; x < 4; ++x)
                    if (pBC1_block->get_selector(x, y) == 3) return false;
        }
    }
    return true;
};

// Returns true on success, false otherwise.
static inline bool
unpack_block_bc3(const void* pBlock, void* pPixels, rgbcx::bc1_approx_mode bc1_approx_mode) {
    auto pDst_pixels = static_cast<uint8_t*>(pPixels);
    bool success = true;
    success = unpack_block_bc1((const uint8_t*)pBlock + BC4_BLOCK_SIZE, pPixels, bc1_approx_mode);
    rgbcx::unpack_bc4(pBlock, &pDst_pixels[3], BC3_NCHANNELS);
    return success;
}

/**
 * @ingroup reader
 * @brief Decode a ktx2 texture object, if it is BCn encoded.
 *
 * All BCn formats are supported (BC1, BC2, BC3, BC4, BC5, BC6HU, BC6HS, or
 * BC7).
 *
 * The decompressed format is determined from corresponding BCn format and the
 * transfer function in DFD:
 * - For BC1:
 *     VK_FORMAT_R8G8B8_[UNORM|SRGB]
 * - For BC2, BC3, and BC7:
 *     VK_FORMAT_R8G8B8A8_[UNORM|SRGB]
 * - For BC4:
 *     VK_FORMAT_R8_[UNORM|SNORM]
 * - For BC5:
 *     VK_FORMAT_R8G8_[UNORM|SNORM]
 * - For BC6HU, BC6HS:
 *     VK_FORMAT_R16G16B16_SFLOAT
 *
 * UNORM vs. SRGB is determined depending on the original transfer function
 * value in the DFD.
 *
 * UNORM vs. SNORM is determined based on the original VkFormat.
 *
 * The images are decompressed from BCn block-compressed format. The
 * decompressed images replace the original images and the texture's fields
 * including the DFD are modified to reflect the new state.
 *
 * Such textures can be directly uploaded to the GPU as raw (decompressed)
 * formats.
 *
 * Decoding into non-multiple-of-4 texture dimensions is also supported (decoded
 * blocks that fall out of the texture's dimensions are simply discarded).
 *
 * @param [in] This   pointer to the ktxTexture2 object of interest.
 *
 * @return            KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_DATA_ERROR
 *                    The texture's supercompression scheme is invalid.
 * @exception KTX_FILE_DATA_ERROR
 *                    @c This->vkFormat does not correspond to the set BCn color
 *                    model.
 * @exception KTX_INVALID_OPERATION
 *                    The texture is not compressed (i.e.,
 *                    @c This->isCompressed is @c false).
 * @exception KTX_INVALID_OPERATION
 *                    The texture's images are not in BCn format.
 * @exception KTX_INVALID_OPERATION
 *                    The texture does not contain any data (i.e.,
 *                    @c This->pData is @c NULL and there is no pending data
 *                    load).
 * @exception KTX_DECOMPRESS_FAILURE
 *                    Decoder/Unpacker returned an error exit code or a
 *                    non-success return flag. Only occurs for BC1, BC2, BC3,
 *                    and BC7 (BC2 and BC3 are based on BC1).
 * @exception KTX_OUT_OF_MEMORY
 *                    Not enough memory to carry out decoding.
 * @exception KTX_UNSUPPORTED_FEATURE
 *                    Unsupported supercompression scheme.
 */
extern "C" KTX_error_code
ktxTexture2_DecodeBCn(ktxTexture2* This) {
    uint32_t* BDB = This->pDfd + 1;
    uint32_t channelId = KHR_DFDSVAL(BDB, 0, CHANNELID);
    KTX_error_code result;

    if (This->supercompressionScheme == KTX_SS_BASIS_LZ ||
        This->supercompressionScheme == KTX_SS_UASTC_HDR_6x6_INTERMEDIATE)
        return KTX_FILE_DATA_ERROR;  // Not a valid file.
    // Safety check.
    if (This->supercompressionScheme > KTX_SS_END_RANGE)
        return KTX_UNSUPPORTED_FEATURE;  // Unsupported scheme.

    if (!This->isCompressed) return KTX_INVALID_OPERATION;

    const ktx_bcn_compression_e bcn =
        get_bcn_compression_kind(static_cast<VkFormat>(This->vkFormat));
    if (bcn == KTX_BCN_COMPRESSION_NONE) return KTX_INVALID_OPERATION;

    // Is the color model correctly set? (i.e., color model corresponds to
    // This->vkFormat). If not, then the given file doesn't align with the spec.
    auto expectedModel = get_bcn_colormodel(bcn);
    if ((khr_df_model_e)KHR_DFDVAL(BDB, MODEL) != expectedModel) return KTX_FILE_DATA_ERROR;

    const VkFormat decompressedVkFormat = get_bcn_decompressed_format(
        bcn, ktxTexture2_GetTransferFunction_e(This), static_cast<VkFormat>(This->vkFormat));
    if (decompressedVkFormat == VK_FORMAT_UNDEFINED) return KTX_INVALID_OPERATION;

    if ((bcn == KTX_BCN_COMPRESSION_BC1 || bcn == KTX_BCN_COMPRESSION_BC1A) &&
        !(channelId == KHR_DF_CHANNEL_BC1A_COLOR || channelId == KHR_DF_CHANNEL_BC1A_ALPHA))
        return KTX_FILE_DATA_ERROR;

    if (This->pData == NULL) {
        if (ktxTexture_isActiveStream((ktxTexture*)This)) {
            // Load pending. Complete it.
            result = ktxTexture2_LoadImageData(This, NULL, 0);
            if (result != KTX_SUCCESS) return result;
        } else {
            return KTX_INVALID_OPERATION;  // No data to decode.
        }
    }

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

    std::unique_ptr<ktxTexture2, decltype(ktxTexture2_Destroy)*> prototype_raii{
        nullptr, ktxTexture2_Destroy};
    ktxTexture2* prototype = nullptr;
    result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &prototype);
    prototype_raii.reset(prototype);

    if (result != KTX_SUCCESS) {
        assert(result == KTX_OUT_OF_MEMORY);
        return result;
    }

    for (uint32_t levelIndex = 0; levelIndex < This->numLevels; ++levelIndex) {
        const uint32_t width = std::max(This->baseWidth >> levelIndex, 1u);
        const uint32_t height = std::max(This->baseHeight >> levelIndex, 1u);
        const uint32_t depth = std::max(This->baseDepth >> levelIndex, 1u);

        const auto image_data_out_size = ktxTexture2_GetImageSize(prototype, levelIndex);

        for (uint32_t layerIndex = 0; layerIndex < This->numLayers; ++layerIndex) {
            for (uint32_t faceIndex = 0; faceIndex < This->numFaces; ++faceIndex) {
                for (uint32_t depthSliceIndex = 0; depthSliceIndex < depth; ++depthSliceIndex) {
                    ktx_size_t image_offset_in;
                    ktx_size_t image_offset_out;
                    ktxTexture2_GetImageOffset(This, levelIndex, layerIndex,
                                               faceIndex + depthSliceIndex, &image_offset_in);
                    ktxTexture2_GetImageOffset(prototype, levelIndex, layerIndex,
                                               faceIndex + depthSliceIndex, &image_offset_out);
                    const auto* image_data_in = This->pData + image_offset_in;
                    auto* image_data_out = prototype->pData + image_offset_out;
                    auto res = ktxUnpackBCn(image_data_in, image_data_out, image_data_out_size,
                                            width, height, bcn);
                    if (res != KTX_SUCCESS) return res;
                }  // depth slices
            }  // faces
        }  // layers
    }

    if (result != KTX_SUCCESS) return result;

    // Fix up the current texture
    DECLARE_PROTECTED_EX(thisPrtctd, This);
    DECLARE_PRIVATE_EX(protoPriv, prototype);
    DECLARE_PROTECTED_EX(protoPrtctd, prototype);
    memcpy(&thisPrtctd._formatSize, &protoPrtctd._formatSize, sizeof(ktxFormatSize));
    This->vkFormat = decompressedVkFormat;
    This->isCompressed = prototype->isCompressed;
    This->supercompressionScheme = KTX_SS_NONE;
    priv._requiredLevelAlignment = protoPriv._requiredLevelAlignment;

    // Copy typesize otherwise `ktx info` and `ktx validate` fails and
    // complains that block-compressed types should have a typesize of 1.
    This->_protected->_typeSize = prototype->_protected->_typeSize;

    // Copy the levelIndex from the prototype to This.
    memcpy(priv._levelIndex, protoPriv._levelIndex, This->numLevels * sizeof(ktxLevelIndexEntry));

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
    return result;
}

/**
 * @ingroup reader
 * @brief Decodes a provided BCn-encoded image. All BCn formats are supported
 *        (BC1, BC2, BC3, BC4, BC5, BC6HU, BC6HS, or BC7).
 *
 *        Decoding into non-multiple-of-4 texture dimensions is also supported
 *        (decoded blocks that fall out of the image's dimensions are simply
 *        discarded).
 *
 * @param [in] src_blocks       pointer to the BCn-encoded blocks.
 * @param [in] dst              pointer to where to write the decoded image.
 *                              Should be able to hold the size of the
 *                              corresponding decompressed vkFormat.
 * @param [in] dstByteLength    allocated destination byte length. Used for
 *                              safety check before attemping to write
 *                              to @p dst pointer.
 * @param [in] width            current image's width.
 * @param [in] height           current image's height.
 * @param [in] bcn              which BCn compression kind the provided image is
 *                              encoded in.
 *
 * @return                      KTX_SUCCESS on success, other KTX_* enum values
 *                              on error.
 *
 * @exception KTX_DECOMPRESS_FAILURE
 *                              Decoder/Unpacker returned an error exit code or a
 *                              non-success return flag. Only occurs for BC1, BC2,
 *                              BC3, and BC7 (BC2 and BC3 are based on BC1).
 * @exception KTX_INVALID_OPERATION
 *                              @p bcn is set to KTX_BCN_COMPRESSION_NONE.
 */
extern "C" KTX_error_code
ktxUnpackBCn(const ktx_uint8_t* src_blocks, void* dst, ktx_size_t dstByteLength, ktx_uint32_t width,
             ktx_uint32_t height, ktx_bcn_compression_e bcn) {
    // Create intermediate storage to store decoded blocks. Not all blocks
    // necessarily decode to 4x4x4 but this is enough to hold all possible
    // combinations (at least for LDR and not for BC6H formats).
    uint8_t rgba[4 * 4 * 4]; /* width * height * num channels = 64 bytes */

    // Number of elements to move from pixel x at some scanline to pixel at x on
    // the next scanline for an RGB HDR decoded block.
    const uint32_t rgbh_pitch = 4 * 3;
    uint16_t rgbh[4 * rgbh_pitch]; /* height * pitch = 48 uint16_t */

    uint8_t rgb[4 * 4 * 3]; /* only for BC1 */

    const uint32_t nchannels = get_bcn_nchannels(bcn);

    const bool is_hdr = (bcn == KTX_BCN_COMPRESSION_BC6HU) || (bcn == KTX_BCN_COMPRESSION_BC6HS);

    [[maybe_unused]] size_t nbr_written_bytes_total = 0;
    // Doing individual checks is a bit involved so just do an initial check
    size_t expectedDstByteLength =
        width * height * nchannels * (is_hdr ? sizeof(uint16_t) : sizeof(uint8_t));
    if (dstByteLength != expectedDstByteLength) return KTX_INVALID_VALUE;

    if (bcn == KTX_BCN_COMPRESSION_NONE) return KTX_INVALID_OPERATION;

    if (bcn == KTX_BCN_COMPRESSION_BC1 || bcn == KTX_BCN_COMPRESSION_BC1A ||
        bcn == KTX_BCN_COMPRESSION_BC2 || bcn == KTX_BCN_COMPRESSION_BC3 ||
        bcn == KTX_BCN_COMPRESSION_BC4 || bcn == KTX_BCN_COMPRESSION_BC5)
        rgbcx::init(rgbcx::bc1_approx_mode::cBC1Ideal);

    for (uint32_t y = 0; y < height; y += 4) {
        for (uint32_t x = 0; x < width; x += 4) {
            bool rv = true;
            switch (bcn) {
            case KTX_BCN_COMPRESSION_BC1:
            case KTX_BCN_COMPRESSION_BC1A:
                // BC1A: 8 bytes -> 4 x 4 x 4 = 64 bytes (alpha 1-bit encoded)
                rv = unpack_block_bc1(src_blocks, rgba, rgbcx::bc1_approx_mode::cBC1Ideal);
                src_blocks += BC1_BLOCK_SIZE;
                break;

            case KTX_BCN_COMPRESSION_BC2:
                // BC2: 16 bytes -> 4 x 4 x 4 = 64 bytes
                rv = rgbcx::unpack_bc2(src_blocks, rgba, rgbcx::bc1_approx_mode::cBC1Ideal);
                src_blocks += BC2_BLOCK_SIZE;
                break;

            case KTX_BCN_COMPRESSION_BC3:
                // BC3: 16 bytes -> 4 x 4 x 4 = 64 bytes
                rv = unpack_block_bc3(src_blocks, rgba, rgbcx::bc1_approx_mode::cBC1Ideal);
                src_blocks += BC3_BLOCK_SIZE;
                break;

            case KTX_BCN_COMPRESSION_BC4:
                // BC4: 8 bytes -> 4 x 4 x 1 = 16 bytes
                /* always succeeds */ rgbcx::unpack_bc4(src_blocks, rgba,
                                                        /* stride */ BC4_NCHANNELS);
                src_blocks += BC4_BLOCK_SIZE;
                break;

            case KTX_BCN_COMPRESSION_BC5:
                // BC5: 16 bytes -> 4 x 4 x 2 = 32 bytes
                /* always succeeds */ rgbcx::unpack_bc5(src_blocks, rgba, 0, 1,
                                                        /* stride */ BC5_NCHANNELS);
                src_blocks += BC5_BLOCK_SIZE;
                break;

            case KTX_BCN_COMPRESSION_BC6HU:
                // BC6HU: 16 bytes -> 4 x 4 x 3 x 2 = 96 bytes
                rv = basisu::unpack_bc6h(src_blocks, rgbh, false, rgbh_pitch);
                src_blocks += BC6H_BLOCK_SIZE;
                break;

            case KTX_BCN_COMPRESSION_BC6HS:
                // BC6HS: 16 bytes -> 4 x 4 x 3 x 2 = 96 bytes
                rv = basisu::unpack_bc6h(src_blocks, rgbh, true, rgbh_pitch);
                src_blocks += BC6H_BLOCK_SIZE;
                break;

            case KTX_BCN_COMPRESSION_BC7:
                // BC7: 16 bytes -> 4 x 4 x 4 = 64 bytes
                rv = basist::bc7u::unpack_bc7(src_blocks,
                                              reinterpret_cast<basist::color_rgba*>(rgba));
                src_blocks += BC7_BLOCK_SIZE;
                break;

            default:  // should never occur
                assert(false);
                rv = false;
                break;
            }

            // If any of the decoders/unpackers returned false
            // => something went wrong
            if (!rv) return KTX_DECOMPRESS_FAILURE;  // decoder failure

            [[maybe_unused]] size_t nbr_written_bytes = 0;

            // copy the decoded block into the actual texture image
            if (is_hdr) {
                nbr_written_bytes =
                    insert_block(static_cast<uint16_t*>(dst), rgbh, x, y, width, height, nchannels);
            } else if (bcn == KTX_BCN_COMPRESSION_BC1) {
                extract_rgb_from_rgba_block(rgb, rgba);
                nbr_written_bytes =
                    insert_block(static_cast<uint8_t*>(dst), rgb, x, y, width, height, nchannels);
            } else {
                nbr_written_bytes =
                    insert_block(static_cast<uint8_t*>(dst), rgba, x, y, width, height, nchannels);
            }
            nbr_written_bytes_total += nbr_written_bytes;
        }  // x blocks
    }  // y blocks
    assert(nbr_written_bytes_total == expectedDstByteLength);
    return KTX_SUCCESS;
}
