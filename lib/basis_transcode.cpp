/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2019 Khronos Group, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @internal
 * @file basis_transcode.c
 * @~English
 *
 * @brief Functions for transcoding a Basis Universal texture.
 *
 * Two worlds collide here too. More uglyness!
 *
 * @author Mark Callow, www.edgewise-consulting.com
 */

#include <inttypes.h>
#include <KHR/khr_df.h>

#include "dfdutils/dfd.h"
#include "ktx.h"
#include "ktxint.h"
#include "texture2.h"
#include "vkformat_enum.h"
#include "vk_format.h"
#include "basis_sgd.h"
#include "basisu/basisu_comp.h"
#include "basisu/transcoder/basisu_file_headers.h"
#include "basisu/transcoder/basisu_transcoder.h"

using namespace basisu;
using namespace basist;

// WARNING: These need to match the same definitions in basisu_transcoder.cpp.
#ifndef BASISD_SUPPORT_DXT1
#define BASISD_SUPPORT_DXT1 1
#endif

#ifndef BASISD_SUPPORT_DXT5A
#define BASISD_SUPPORT_DXT5A 1
#endif

#ifndef BASISD_SUPPORT_BC7
#define BASISD_SUPPORT_BC7 1
#endif

#ifndef BASISD_SUPPORT_PVRTC1
#define BASISD_SUPPORT_PVRTC1 1
#endif

#ifndef BASISD_SUPPORT_ETC2_EAC_A8
#define BASISD_SUPPORT_ETC2_EAC_A8 1
#endif

// block size calculations
static inline uint32_t get_block_width(uint32_t w, uint32_t bw)
{
    return (w + (bw - 1)) / bw;
}

static inline uint32_t get_block_height(uint32_t h, uint32_t bh)
{
    return (h + (bh - 1)) / bh;
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Transcode a KTX2 texture with Basis supercompressed images.
 *
 * Inflates the images from Basis Universal supercompression back to ETC1S
 * then transcodes them to the specified block-compressed format. The
 * transcoded images replace the original images and the texture's fields
 * including the DFD are modified to reflect the new format.
 *
 * Basis supercompressed textures must be transcoded to a desired target
 * block-compressed format before they can be uploaded to a GPU via a graphics
 * API.
 *
 * The following transcode targets are available : KTX_TF_ETC1, KTX_TF_BC1,
 * KTX_TF_BC4, KTX_TF_PVRTC1_4_OPAQUE_ONLY, KTX_TF_BC7_M6_OPAQUE_ONLY,
 * KTX_TF_ETC2, KTX_TF_BC3 and KTX_TF_BC5.
 *
 * Note that KTX_TF_ETC2 will always transcode to an RGBA texture. If there
 * is no alpha channel in the suercompressed data, alpha will be set to 255
 * (opaque). If you know there is no alpha data then choose KTX_TF_ETC1. The
 * ETC2 texture will consist of an ETC2_EAC_A8 block followed by a ETC1 block.
 *
 * KTX_TF_BC3 has a BC4 alpha block follwed by a BC1 RGB block.
 *
 * KTX_TF_BC5 has two BC4 blocks, one  holding the R data, the other the G data.
 *
 * The following @p decodeFlags are available.
 *
 * @sa ktxtexture2_CompressBasis().
 *
 * @param[in]   This         pointer to the ktxTexture2 object of interest.
 * @param[in]   outputFormat a value from the ktx_texture_transcode_fmt_e enum
 *                           specifying the target format.
 * @param[in]   decodeFlags  bitfield of flags modifying the transcode
 *                           operation. @sa ktx_texture_decode_flags_e.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_DATA_ERROR
 *                              Supercompression global data is corrupted.
 * @exception KTX_INVALID_OPERATION
 *                              The texture is not supercompressed.
 * @exception KTX_INVALID_OPERATION
 *                              Supercompression global data is missing, i.e.,
 *                              the texture object is invalid.
 * @exception KTX_INVALID_OPERATION
 *                              Image data is missing, i.e., the texture object
 *                              is invalid.
 * @exception KTX_INVALID_OPERATION
 *                              @p outputFormat is PVRTC1 but the texture does
 *                              does not have power-of-two dimensions.
 * @exception KTX_INVALID_VALUE @p outputFormat is invalid.
 * @exception KTX_TRANSCODE_FAILED
 *                              Something went wrong during transcoding. The
 *                              texture object will be corrupted.
 * @exception KTX_UNSUPPORTED_FEATURE
 *                              KTX_DF_PVRTC_DECODE_TO_NEXT_POW2 was requested
 *                              or the specified transcode target has not been
 *                              included in the library being used.
 * @exception KTX_OUT_OF_MEMORY Not enough memory to carry out transcoding.
 */
KTX_error_code
ktxTexture2_TranscodeBasis(ktxTexture2* This, ktx_texture_transcode_fmt_e outputFormat,
                           ktx_uint32_t decodeFlags)
{
    ktxTexture2_private& priv = *This->_private;
    KTX_error_code result = KTX_SUCCESS;

    if (This->supercompressionScheme != KTX_SUPERCOMPRESSION_BASIS)
        return KTX_INVALID_OPERATION;

    if (!priv._supercompressionGlobalData || priv._sgdByteLength == 0)
        return KTX_INVALID_OPERATION;

    if (decodeFlags & KTX_DF_PVRTC_DECODE_TO_NEXT_POW2) {
        debug_printf("ktxTexture_TranscodeBasis: KTX_DF_PVRTC_DECODE_TO_NEXT_POW2 currently unsupported\n");
        return KTX_UNSUPPORTED_FEATURE;
    }

    if (outputFormat == KTX_TF_PVRTC1_4_OPAQUE_ONLY) {
        if ((!basisu::is_pow2(This->baseWidth)) || (!basisu::is_pow2(This->baseHeight))) {
            debug_printf("ktxTexture_TranscodeBasis: PVRTC1 only supports power of 2 dimensions\n");
            return KTX_INVALID_OPERATION;
       }
    }

    if (!This->pData) {
        if (ktxTexture_isActiveStream((ktxTexture*)This)) {
             // Load pending. Complete it.
            result = ktxTexture2_LoadImageData(This, NULL, 0);
            if (result != KTX_SUCCESS)
                return result;
        } else {
            // No data to transcode.
            return KTX_INVALID_OPERATION;
        }
    }

    uint8_t* bgd = priv._supercompressionGlobalData;
    ktxBasisGlobalHeader& bgdh = *reinterpret_cast<ktxBasisGlobalHeader*>(bgd);
    if (!(bgdh.endpointsByteLength && bgdh.selectorsByteLength && bgdh.tablesByteLength)) {
            debug_printf("ktxTexture_TranscodeBasis: missing endpoints, selectors or tables");
            return KTX_FILE_DATA_ERROR;
    }

    // Compute some helpful numbers.
    //
    // firstImages contains the indices of the first images for each level to
    // ease finding the correct slice description when iterating from smallest
    // level to largest or when randomly accessing them (t.b.c). The last array
    // entry contains the total number of images, for calculating the offsets
    // of the endpoints, etc.
    uint32_t* firstImages = new uint32_t[This->numLevels+1];

    // Temporary invariant value
    uint32_t layersFaces = This->numLayers * This->numFaces;
    firstImages[0] = 0;
    for (uint32_t level = 1; level <= This->numLevels; level++) {
        // NOTA BENE: numFaces * depth is only reasonable because they can't
        // both be > 1. I.e there are no 3d cubemaps.
        firstImages[level] = firstImages[level - 1]
                           + layersFaces * MAX(This->baseDepth >> (level - 1), 1);
    }
    uint32_t& imageCount = firstImages[This->numLevels];

    if (BGD_TABLES_ADDR(0, bgdh, imageCount) + bgdh.tablesByteLength > priv._sgdByteLength) {
        return KTX_FILE_DATA_ERROR;
    }
    // FIXME: Do more validation.

    // Prepare low-level transcoder for transcoding slices.

    basisu_transcoder_init();

    static basist::etc1_global_selector_codebook *global_codebook
        = new basist::etc1_global_selector_codebook(g_global_selector_cb_size,
                                                    g_global_selector_cb);
    basisu_lowlevel_transcoder llt(global_codebook);

    llt.decode_palettes(bgdh.endpointCount, BGD_ENDPOINTS_ADDR(bgd, imageCount),
                        bgdh.endpointsByteLength,
                        bgdh.selectorCount, BGD_SELECTORS_ADDR(bgd, bgdh, imageCount),
                        bgdh.selectorsByteLength);

    llt.decode_tables(BGD_TABLES_ADDR(bgd, bgdh, imageCount), bgdh.tablesByteLength);

    // Find matching VkFormat and calculate output sizes.

    const bool hasAlpha = (bgdh.globalFlags & cBASISHeaderFlagHasAlphaSlices) != 0;
    const bool transcodeAlphaToOpaqueFormats
     = (hasAlpha && (decodeFlags & KTX_DF_TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS));
    const bool isVideo = false; // FIXME when video is supported.

    uint32_t* BDB = This->pDfd + 1;
    const bool srgb = (KHR_DFDVAL(BDB, TRANSFER) == KHR_DF_TRANSFER_SRGB);

    VkFormat vkFormat;

    switch (outputFormat) {
      case KTX_TF_ETC1:
        // ETC2 is compatible & there are no ETC1 formats in Vulkan.
        vkFormat = srgb ? VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK
                        : VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
        break;
      case KTX_TF_ETC2:
        if (hasAlpha) {
            vkFormat = srgb ? VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK
                            : VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
        } else {
            // No point wasting a channel. Select ETC1.
            outputFormat = KTX_TF_ETC1;
            vkFormat = srgb ? VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK
                            : VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
        }
        break;
      case KTX_TF_BC1:
        // Transcoding doesn't support BC1 alpha.
        vkFormat = srgb ? VK_FORMAT_BC1_RGB_SRGB_BLOCK
                        : VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        break;
      case KTX_TF_BC3:
        if (hasAlpha) {
            vkFormat = srgb ? VK_FORMAT_BC3_SRGB_BLOCK
                            : VK_FORMAT_BC3_UNORM_BLOCK;
        } else {
            // No point wasting a channel. Select ETC1.
            outputFormat = KTX_TF_BC1;
            vkFormat = srgb ? VK_FORMAT_BC1_RGB_SRGB_BLOCK
                            : VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        }
        break;
      case KTX_TF_BC4:
        vkFormat = VK_FORMAT_BC4_UNORM_BLOCK;
        break;
      case KTX_TF_BC5:
        vkFormat = VK_FORMAT_BC5_UNORM_BLOCK;
        break;
      case KTX_TF_PVRTC1_4_OPAQUE_ONLY:
        vkFormat = srgb ? VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG
                        : VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
        break;
      case KTX_TF_BC7_M6_OPAQUE_ONLY:
        vkFormat = srgb ? VK_FORMAT_BC7_SRGB_BLOCK
                        : VK_FORMAT_BC7_UNORM_BLOCK;
        break;
      default:
        assert(0);
        return KTX_INVALID_VALUE;
    }

    // Set these so we can get the size needed for the output.
    // FIXME: Need to avoid modifying This until transcode is successful.
    This->vkFormat = vkFormat;
    vkGetFormatSize(vkFormat, &This->_protected->_formatSize);
    This->isCompressed = true;

    ktx_size_t transcodedDataSize = ktxTexture_calcDataSizeTexture(
                                            ktxTexture(This),
                                            KTX_FORMAT_VERSION_TWO);
    uint32_t bytes_per_block
                        = This->_protected->_formatSize.blockSizeInBits / 8;

    ktx_uint8_t* basisData = This->pData;
    This->pData = new uint8_t[transcodedDataSize];
    This->dataSize = transcodedDataSize;

    // Finally we're ready to transcode the slices.

    // FIXME: Iframe flag needs to be queryable by the application. In Basis
    // the app can query file_info and image_info from the transcoder which
    // returns a structure with lots of info about the image.

    ktxLevelIndexEntry* levelIndex = priv._levelIndex;
    uint64_t levelOffsetWrite = 0;
    for (int32_t level = This->numLevels - 1; level >= 0; level--) {
        uint64_t levelOffset = ktxTexture2_levelDataOffset(This, level);
        uint64_t writeOffset = levelOffsetWrite;
        const ktxBasisSliceDesc* sliceDescs = BGD_SLICE_DESCS(bgd);
        uint32_t width = MAX(1, This->baseWidth >> level);
        uint32_t height = MAX(1, This->baseHeight >> level);
        uint32_t depth = MAX(1, This->baseDepth >> level);
        //uint32_t faceSlices = This->numFaces == 1 ? depth : This->numFaces;
        uint32_t faceSlices = This->numFaces * depth;
        uint32_t numImages = This->numLayers * faceSlices;
        uint32_t image = firstImages[level];
        uint32_t endImage = image + numImages;

        // 4x4 is the ETC1S block size.
        const uint32_t num_blocks_x = get_block_width(width, 4);
        const uint32_t num_blocks_y = get_block_height(height, 4);

        for (; image < endImage; image++) {
            ktx_uint8_t* writePtr = This->pData + writeOffset;

            if (hasAlpha)
            {
                // The slice descriptions should have alpha information.
                if (sliceDescs[image].alphaSliceByteOffset == 0
                    || sliceDescs[image].alphaSliceByteLength == 0)
                    return KTX_FILE_DATA_ERROR;
            }

            bool status = false;
            uint32_t sliceByteOffset, sliceByteLength;
            // If the caller wants us to transcode the mip level's alpha data,
            // then use alpha slice.
            if (hasAlpha && transcodeAlphaToOpaqueFormats) {
                sliceByteOffset = sliceDescs[image].alphaSliceByteOffset;
                sliceByteLength = sliceDescs[image].alphaSliceByteLength;
            } else {
                sliceByteOffset = sliceDescs[image].sliceByteOffset;
                sliceByteLength = sliceDescs[image].sliceByteLength;
            }

            switch (outputFormat) {
              case KTX_TF_ETC1:
            {
                // No need to pass output_row_pitch_in_blocks. It defaults to
                // num_blocks_x.
                // No need to pass transcoder state. It will use default state.

                // Pass 0 for level_index. In Basis files, level_index is only
                // ever non-zero when the Basis encoder generated mipmaps, a
                // function we're not currently using. The method we're calling
                // only uses level_index for video textures.
                status = llt.transcode_slice(writePtr, num_blocks_x, num_blocks_y,
                        basisData + levelOffset + sliceByteOffset, sliceByteLength,
                        basist::cETC1, bytes_per_block,
                        (decodeFlags & KTX_DF_PVRTC_WRAP_ADDRESSING) != 0,
                        (decodeFlags & KTX_DF_BC1_FORBID_THREE_COLOR_BLOCKS) == 0,
                        isVideo, hasAlpha, 0/* level_index*/);
                if (!status) {
                     result = KTX_TRANSCODE_FAILED;
                     goto cleanup;
                }
                break;
            }
            case KTX_TF_BC1:
            {
#if !BASISD_SUPPORT_DXT1
                return KTX_UNSUPPORTED_FEATURE;
#endif
                status = llt.transcode_slice(writePtr, num_blocks_x, num_blocks_y,
                        basisData + levelOffset + sliceByteOffset, sliceByteLength,
                        basist::cBC1, bytes_per_block,
                        (decodeFlags & KTX_DF_PVRTC_WRAP_ADDRESSING) != 0,
                        (decodeFlags & KTX_DF_BC1_FORBID_THREE_COLOR_BLOCKS) == 0,
                        isVideo, hasAlpha, 0/* level_index*/);

                if (!status) {
                     result = KTX_TRANSCODE_FAILED;
                     goto cleanup;
                }
                break;
            }
            case KTX_TF_BC4:
            {
#if !BASISD_SUPPORT_DXT5A
                return KTX_UNSUPPORTED_FEATURE;
#endif
                status = llt.transcode_slice(writePtr, num_blocks_x, num_blocks_y,
                        basisData + levelOffset + sliceByteOffset, sliceByteLength,
                        basist::cBC4, bytes_per_block,
                        (decodeFlags & KTX_DF_PVRTC_WRAP_ADDRESSING) != 0,
                        (decodeFlags & KTX_DF_BC1_FORBID_THREE_COLOR_BLOCKS) == 0,
                        isVideo, hasAlpha, 0/* level_index*/);

                if (!status) {
                     return KTX_TRANSCODE_FAILED;
                }
                break;
            }
            case KTX_TF_PVRTC1_4_OPAQUE_ONLY:
            {
#if !BASISD_SUPPORT_PVRTC1
                return KTX_UNSUPPORTED_FEATURE;
#endif
                // Note that output_row_pitch_in_blocks is actually ignored because
                // we're transcoding to PVRTC1. Since we're using the default, 0,
                // this is not an issue at present.
                status = llt.transcode_slice(writePtr, num_blocks_x, num_blocks_y,
                        basisData + levelOffset + sliceByteOffset, sliceByteLength,
                        basist::cPVRTC1_4_OPAQUE_ONLY, bytes_per_block,
                        (decodeFlags & KTX_DF_PVRTC_WRAP_ADDRESSING) != 0,
                        (decodeFlags & KTX_DF_BC1_FORBID_THREE_COLOR_BLOCKS) == 0,
                        isVideo, hasAlpha, 0/* level_index*/);

                if (!status) {
                     result = KTX_TRANSCODE_FAILED;
                     goto cleanup;
                }
                break;
            }
            case KTX_TF_BC7_M6_OPAQUE_ONLY:
            {
#if !BASISD_SUPPORT_BC7
                return KTX_UNSUPPORTED_FEATURE;
#endif

                status = llt.transcode_slice(writePtr, num_blocks_x, num_blocks_y,
                        basisData + levelOffset + sliceByteOffset, sliceByteLength,
                        basist::cBC7_M6_OPAQUE_ONLY, bytes_per_block,
                        (decodeFlags & KTX_DF_PVRTC_WRAP_ADDRESSING) != 0,
                        (decodeFlags & KTX_DF_BC1_FORBID_THREE_COLOR_BLOCKS) == 0,
                        isVideo, hasAlpha, 0/* level_index*/);
                if (!status) {
                     result = KTX_TRANSCODE_FAILED;
                     goto cleanup;
                }
                break;
            }
            case KTX_TF_ETC2:
            {
#if !BASISD_SUPPORT_ETC2_EAC_A8
                return KTX_UNSUPPORTED_FEATURE;
#endif
                if (hasAlpha) {
                    // First decode the alpha data
                    status = llt.transcode_slice(writePtr, num_blocks_x, num_blocks_y,
                            basisData + levelOffset + sliceDescs[image].alphaSliceByteOffset,
                            sliceDescs[image].alphaSliceByteLength,
                            basist::cETC2_EAC_A8, bytes_per_block,
                            (decodeFlags & KTX_DF_PVRTC_WRAP_ADDRESSING) != 0,
                            (decodeFlags & KTX_DF_BC1_FORBID_THREE_COLOR_BLOCKS) == 0,
                            isVideo, hasAlpha, 0/* level_index*/);
                } else {
                    basisu_transcoder::write_opaque_alpha_blocks(num_blocks_x, num_blocks_y, writePtr,
                                 (uint32_t)((This->dataSize - writeOffset) / bytes_per_block),
                                 cETC2_EAC_A8, bytes_per_block, 0);
                    status = true;
                }

                if (status) {
                    // Now decode the color data
                    status = llt.transcode_slice(writePtr + 8, num_blocks_x, num_blocks_y,
                            basisData + levelOffset + sliceDescs[image].sliceByteOffset,
                            sliceDescs[image].sliceByteLength,
                            basist::cETC1, bytes_per_block,
                            (decodeFlags & KTX_DF_PVRTC_WRAP_ADDRESSING) != 0,
                            (decodeFlags & KTX_DF_BC1_FORBID_THREE_COLOR_BLOCKS) == 0,
                            isVideo, hasAlpha, 0/* level_index*/);
                }
                if (!status) {
                     result = KTX_TRANSCODE_FAILED;
                     goto cleanup;
                }
                break;
            }
            case KTX_TF_BC3:
            {
#if !BASISD_SUPPORT_DXT1
                return KTX_UNSUPPORTED_FEATURE;
#endif
#if !BASISD_SUPPORT_DXT5A
                return KTX_UNSUPPORTED_FEATURE;
#endif
                 // First decode the alpha data

                if (hasAlpha) {
                    status = llt.transcode_slice(writePtr, num_blocks_x, num_blocks_y,
                            basisData + levelOffset + sliceDescs[image].alphaSliceByteOffset,
                            sliceDescs[image].alphaSliceByteLength,
                            basist::cBC4, bytes_per_block,
                            (decodeFlags & KTX_DF_PVRTC_WRAP_ADDRESSING) != 0,
                            (decodeFlags & KTX_DF_BC1_FORBID_THREE_COLOR_BLOCKS) == 0,
                            isVideo, hasAlpha, 0/* level_index*/);
                } else {
                    basisu_transcoder::write_opaque_alpha_blocks(num_blocks_x, num_blocks_y, writePtr,
                                (uint32_t)((This->dataSize - writeOffset) / bytes_per_block),
                                basist::cBC4, bytes_per_block, 0);
                    status = true;
                }

                if (status) {
                    // Now decode the color data. Forbid 3 color blocks, which aren't allowed in BC3.
                    status = llt.transcode_slice(writePtr + 8, num_blocks_x, num_blocks_y,
                            basisData + levelOffset + sliceDescs[image].sliceByteOffset,
                            sliceDescs[image].sliceByteLength,
                            basist::cBC1, bytes_per_block,
                            (decodeFlags & KTX_DF_PVRTC_WRAP_ADDRESSING) != 0,
                            0, // Forbid 3 color blocks
                            isVideo, hasAlpha, 0/* level_index*/);
                }
                if (!status) {
                     result = KTX_TRANSCODE_FAILED;
                     goto cleanup;
                }
                break;
            }
            case KTX_TF_BC5:
            {
#if !BASISD_SUPPORT_DXT5A
                return KTX_UNSUPPORTED_FEATURE;
#endif
                // Decode the R data (actually the green channel of the color data slice in the basis file)
                status = llt.transcode_slice(writePtr, num_blocks_x, num_blocks_y,
                    basisData + levelOffset + sliceDescs[image].sliceByteOffset,
                    sliceDescs[image].sliceByteLength,
                    basist::cBC4, bytes_per_block,
                    (decodeFlags & KTX_DF_PVRTC_WRAP_ADDRESSING) != 0,
                    0, // Forbid 3 color blocks
                    isVideo, hasAlpha, 0/* level_index*/);

                if (status) {
                    if (hasAlpha) {
                        // Decode the G data (actually the green channel of the alpha data slice in the basis file)
                        status = llt.transcode_slice(writePtr + 8, num_blocks_x, num_blocks_y,
                                basisData + levelOffset + sliceDescs[image].alphaSliceByteOffset,
                                sliceDescs[image].alphaSliceByteLength,
                                basist::cBC4, bytes_per_block,
                                (decodeFlags & KTX_DF_PVRTC_WRAP_ADDRESSING) != 0,
                                (decodeFlags & KTX_DF_BC1_FORBID_THREE_COLOR_BLOCKS) == 0,
                                isVideo, hasAlpha, 0/* level_index*/);
                    } else {
                        basisu_transcoder::write_opaque_alpha_blocks(num_blocks_x, num_blocks_y, writePtr + 8,
                                (uint32_t)((This->dataSize - writeOffset - 8) / bytes_per_block),
                                basist::cBC4, bytes_per_block, 0);
                        status = true;
                    }
                }
                if (!status) {
                     result = KTX_TRANSCODE_FAILED;
                     goto cleanup;
                }
                break;
            }
            case KTX_TF_NONE_COMPATIBLE:
                result = KTX_INVALID_VALUE;
                goto cleanup;
        } // end outputFormat switch

        writeOffset += ktxTexture2_GetImageSize(This, level);
    } // end images loop
        // FIXME: Figure out a way to get the size out of the transcoder.
        uint64_t levelSize = ktxTexture_calcLevelSize(ktxTexture(This), level,
                                                      KTX_FORMAT_VERSION_TWO);
        levelIndex[level].byteOffset = levelOffsetWrite;
        levelIndex[level].byteLength = levelSize;
        levelIndex[level].uncompressedByteLength = levelSize;
        levelOffsetWrite += levelSize;
        assert(levelOffsetWrite == writeOffset);
    } // level loop

    delete This->pDfd;
    //This->isCompressed = true;
    This->pDfd = createDFD4VkFormat((enum VkFormat)This->vkFormat);
    delete basisData;
    delete[] firstImages;
    return KTX_SUCCESS;

cleanup: // FIXME when we stop modifying This until successful transcode.
    delete basisData;
    delete[] firstImages;
    delete This->pData;
    return result;
}
