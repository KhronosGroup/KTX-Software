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
#include "basis_image_transcoder.h"
#include "basis_transcoder_config.h"
#include "basisu/basisu_comp.h"
#include "basisu/transcoder/basisu_file_headers.h"
#include "basisu/transcoder/basisu_transcoder.h"

using namespace basisu;
using namespace basist;

// block size calculations
static inline uint32_t getBlockWidth(uint32_t w, uint32_t bw)
{
    return (w + (bw - 1)) / bw;
}

static inline uint32_t getBlockHeight(uint32_t h, uint32_t bh)
{
    return (h + (bh - 1)) / bh;
}

inline bool isPow2(uint32_t x) { return x && ((x & (x - 1U)) == 0U); }

inline bool isPow2(uint64_t x) { return x && ((x & (x - 1U)) == 0U); }

/**
 * @memberof ktxTexture2
 * @ingroup reader
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
 * The following block compressed transcode targets are available: @c KTX_TTF_ETC1_RGB,
 * @c KTX_TTF_ETC2_RGBA, @c KTX_TTF_BC1_RGB, @c KTX_TTF_BC3_RGBA,
 * @c KTX_TTF_BC4_R, @c KTX_TTF_BC5_RG, @c KTX_TTF_BC7_M6_RGB,
 * @c KTX_TTF_BC7_M5_RGBA, @c KTX_TTF_PVRTC1_4_RGB, @c KTX_TTF_PVRTC1_4_RGBA,
 * @c KTX_TTF_PVRTC2_4_RGB, @c KTX_TTF_PVRTC2_4_RGBA, @c KTX_TTF_ASTC_4x4_RGBA,
 * @c KTX_TTF_ETC2_EAC_R11, @c KTX_TTF_ETC2_EAC_RG11, @c KTX_TTF_ETC and
 * @c KTX_TTF_BC1_OR_3.
 *
 * @c KTX_TTF_ETC automatically selects between @c KTX_TTF_ETC1_RGB and
 * @c KTX_TTF_ETC2_RGBA according to whether an alpha channel is available. @c KTX_TTF_BC1_OR_3
 * does likewise between @c KTX_TTF_BC1_RGB and @c KTX_TTF_BC3_RGBA. Note that if
 * @c KTX_TTF_PVRTC1_4_RGBA or @c KTX_TTF_PVRTC2_4_RGBA is specified and there is no alpha
 * channel @c KTX_TTF_PVRTC1_4_RGB or @c KTX_TTF_PVRTC2_4_RGB respectively will be selected.
 *
 * ATC & FXT1 formats are not supported by KTX2 as there are no equivalent Vulkan formats.
 *
 * The following uncompressed transcode targets are also available: @c KTX_TTF_RGBA32,
 * @c KTX_TTF_RGB565, KTX_TTF_BGR565 and KTX_TTF_RGBA4444.
 *
 * The following @p transcodeFlags are available.
 *
 * @sa ktxtexture2_CompressBasis().
 *
 * @param[in]   This         pointer to the ktxTexture2 object of interest.
 * @param[in]   outputFormat a value from the ktx_texture_transcode_fmt_e enum
 *                           specifying the target format.
 * @param[in]   transcodeFlags  bitfield of flags modifying the transcode
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
 *                              KTX_TF_PVRTC_DECODE_TO_NEXT_POW2 was requested
 *                              or the specified transcode target has not been
 *                              included in the library being used.
 * @exception KTX_OUT_OF_MEMORY Not enough memory to carry out transcoding.
 */
KTX_error_code
ktxTexture2_TranscodeBasis(ktxTexture2* This,
                           ktx_transcode_fmt_e outputFormat,
                           ktx_transcode_flags transcodeFlags)
{
    ktxTexture2_private& priv = *This->_private;
    KTX_error_code result = KTX_SUCCESS;

    if (This->supercompressionScheme != KTX_SUPERCOMPRESSION_BASIS)
        return KTX_INVALID_OPERATION;

    if (!priv._supercompressionGlobalData || priv._sgdByteLength == 0)
        return KTX_INVALID_OPERATION;

    if (transcodeFlags & KTX_TF_PVRTC_DECODE_TO_NEXT_POW2) {
        debug_printf("ktxTexture_TranscodeBasis: KTX_TF_PVRTC_DECODE_TO_NEXT_POW2 currently unsupported\n");
        return KTX_UNSUPPORTED_FEATURE;
    }

    if (outputFormat == KTX_TTF_PVRTC1_4_RGB
        || outputFormat == KTX_TTF_PVRTC1_4_RGBA) {
        if ((!isPow2(This->baseWidth)) || (!isPow2(This->baseHeight))) {
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
    ktxBasisImageTranscoder bit(global_codebook);

    bit.decode_palettes(bgdh.endpointCount, BGD_ENDPOINTS_ADDR(bgd, imageCount),
                        bgdh.endpointsByteLength,
                        bgdh.selectorCount, BGD_SELECTORS_ADDR(bgd, bgdh, imageCount),
                        bgdh.selectorsByteLength);

    bit.decode_tables(BGD_TABLES_ADDR(bgd, bgdh, imageCount), bgdh.tablesByteLength);

    // Find matching VkFormat and calculate output sizes.

    const bool hasAlpha = (bgdh.globalFlags & cBASISHeaderFlagHasAlphaSlices) != 0;
    const bool transcodeAlphaToOpaqueFormats
     = (hasAlpha && (transcodeFlags & KTX_TF_TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS));
    const bool isVideo = This->isVideo;

    uint32_t* BDB = This->pDfd + 1;
    const bool srgb = (KHR_DFDVAL(BDB, TRANSFER) == KHR_DF_TRANSFER_SRGB);

    VkFormat vkFormat;

    // Do some format mapping.
    switch (outputFormat) {
      case KTX_TTF_BC1_OR_3:
        outputFormat = hasAlpha ? KTX_TTF_BC3_RGBA : KTX_TTF_BC1_RGB;
        break;
      case KTX_TTF_ETC:
        outputFormat = hasAlpha ? KTX_TTF_ETC2_RGBA : KTX_TTF_ETC1_RGB;
        break;
      case KTX_TTF_PVRTC1_4_RGBA:
        // This transcoder does not write opaque alpha blocks.
        outputFormat = hasAlpha ? KTX_TTF_PVRTC1_4_RGBA : KTX_TTF_PVRTC1_4_RGB;
        break;
      case KTX_TTF_PVRTC2_4_RGBA:
        // This transcoder does not write opaque alpha blocks.
        outputFormat = hasAlpha ? KTX_TTF_PVRTC2_4_RGBA : KTX_TTF_PVRTC2_4_RGB;
        break;
      default:
        /*NOP*/;
    }

    switch (outputFormat) {
      case KTX_TTF_ETC1_RGB:
        vkFormat = srgb ? VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK
                        : VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
        break;
      case KTX_TTF_ETC2_RGBA:
        vkFormat = srgb ? VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK
                        : VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
        break;
      case KTX_TTF_ETC2_EAC_R11:
        vkFormat = VK_FORMAT_EAC_R11_UNORM_BLOCK;
        break;
      case KTX_TTF_ETC2_EAC_RG11:
        vkFormat = VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
        break;
      case KTX_TTF_BC1_RGB:
        // Transcoding doesn't support BC1 alpha.
        vkFormat = srgb ? VK_FORMAT_BC1_RGB_SRGB_BLOCK
                        : VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        break;
      case KTX_TTF_BC3_RGBA:
        vkFormat = srgb ? VK_FORMAT_BC3_SRGB_BLOCK
                        : VK_FORMAT_BC3_UNORM_BLOCK;
        break;
      case KTX_TTF_BC4_R:
        vkFormat = VK_FORMAT_BC4_UNORM_BLOCK;
        break;
      case KTX_TTF_BC5_RG:
        vkFormat = VK_FORMAT_BC5_UNORM_BLOCK;
        break;
      case KTX_TTF_PVRTC1_4_RGB:
      case KTX_TTF_PVRTC1_4_RGBA:
        vkFormat = srgb ? VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG
                        : VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
        break;
      case KTX_TTF_PVRTC2_4_RGB:
      case KTX_TTF_PVRTC2_4_RGBA:
        vkFormat = srgb ? VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG
                        : VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG;
        break;
      case KTX_TTF_BC7_M6_RGB:
        vkFormat = srgb ? VK_FORMAT_BC7_SRGB_BLOCK
                        : VK_FORMAT_BC7_UNORM_BLOCK;
        break;
      case KTX_TTF_BC7_M5_RGBA:
        vkFormat = srgb ? VK_FORMAT_BC7_SRGB_BLOCK
                        : VK_FORMAT_BC7_UNORM_BLOCK;
        break;
      case KTX_TTF_ASTC_4x4_RGBA:
        vkFormat = srgb ? VK_FORMAT_ASTC_4x4_SRGB_BLOCK
                        : VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        break;
      case KTX_TTF_RGB565:
        vkFormat = VK_FORMAT_R5G6B5_UNORM_PACK16;
        break;
      case KTX_TTF_BGR565:
        vkFormat = VK_FORMAT_B5G6R5_UNORM_PACK16;
        break;
      case KTX_TTF_RGBA4444:
        vkFormat = VK_FORMAT_R4G4B4A4_UNORM_PACK16;
        break;
      case KTX_TTF_RGBA32:
        vkFormat = srgb ? VK_FORMAT_R8G8B8A8_SRGB
                        : VK_FORMAT_R8G8B8A8_UNORM;
        break;
      default:
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

    ktx_uint8_t* basisData = This->pData;
    This->pData = (uint8_t*) malloc(transcodedDataSize);
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
        const ktxBasisImageDesc* imageDescs = BGD_IMAGE_DESCS(bgd);
        uint32_t width = MAX(1, This->baseWidth >> level);
        uint32_t height = MAX(1, This->baseHeight >> level);
        uint32_t depth = MAX(1, This->baseDepth >> level);
        //uint32_t faceSlices = This->numFaces == 1 ? depth : This->numFaces;
        uint32_t faceSlices = This->numFaces * depth;
        uint32_t numImages = This->numLayers * faceSlices;
        uint32_t image = firstImages[level];
        uint32_t endImage = image + numImages;

        // 4x4 is the ETC1S block size.
        const uint32_t num_blocks_x = getBlockWidth(width, 4);
        const uint32_t num_blocks_y = getBlockHeight(height, 4);

        for (; image < endImage; image++) {
            ktx_uint8_t* writePtr = This->pData + writeOffset;
            ktx_size_t bufferByteLength = This->dataSize - writeOffset;

            if (hasAlpha)
            {
                // The slice descriptions should have alpha information.
                if (imageDescs[image].alphaSliceByteOffset == 0
                    || imageDescs[image].alphaSliceByteLength == 0)
                    return KTX_FILE_DATA_ERROR;
            }

            result = bit.transcode_image(imageDescs[image], outputFormat,
                                writePtr, bufferByteLength,
                                level,
                                basisData + levelOffset,
                                width, height, num_blocks_x, num_blocks_y,
                                isVideo, transcodeAlphaToOpaqueFormats);
            if (result != KTX_SUCCESS)
                goto cleanup;

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
    This->pDfd = vk2dfd((enum VkFormat)This->vkFormat);
    delete basisData;
    delete[] firstImages;
    return KTX_SUCCESS;

cleanup: // FIXME when we stop modifying This until successful transcode.
    delete basisData;
    delete[] firstImages;
    delete This->pData;
    return result;
}

/**
 * @internal
 * @~English
 * @brief Transcode a single Basis supercompressed image.
 *
 * @param[in] image     reference to the @c ktxBasisImageDesc of the image to be transcoded.
 *                   This comes from the @c supercompressionGlobalData area of a KTX2 file.
 * @param[in] targetFormat the format to which to transcode the image. Only real formats are
 *                        accepted, i.e., any format accepted by ktxTexture2_TranscodeBasis
 *                        except @c KTX_TTF_ETC and @c KTX_TTF_BC1_OR_3.
 * @param[in] writePtr pointer to the location to write the transcoded image.
 * @param[in] bufferByteLength the length of the buffer pointed to by @p writePtr.
 * @param[in] level the mip level of the image being transcoded.
 * @param[in] levelDataPtr pointer to the start of the supercompressed data for mip level @p level.
 * @param[in] width the pixel width of a level @p level image.
 * @param[in] height the pixel height of a level @p level image.
 * @param[in] num_blocks_x number of blocks in the x dimension for mip level @p level in the
 *            pre-deflation input. When @c eBuIsETC1S is set in @c globalFlags in the
 *            supercompression global data, the block width to use for calculating this from
 *            @p width is 4.
 * @param[in] num_blocks_y number of blocks in the y dimension for mip level @p level in the
 *            pre-deflation input. When @c eBuIsETC1S is set in @c globalFlags in the
 *            supercompression global data, the block height to use for calculating this from
 *            @p height is 4.
 * @param[in] isVideo @c true if the image comes from a file containing an animation sequence,
 *                   @c false otherwise.
 * @param[in] transcodeAlphaToOpaqueFormats if @p targetFormat is a format lacking an
 *                                         alpha component, transcode the alpha slice into
 *                                         the RGB components of the destination.
 * @param[in,out] pState pointer to a transcoder_state object. Only needed when transcoding
 *                      multiple mip levels in parallel on different threads.
 * @return    KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE  A non-real format was specified as @p targetFormat.
 * @exception KTX_TRANSCODE_FAILED   Something went wrong during transcoding. The
 *                                 image will be corrupted.
 */
KTX_error_code
ktxBasisImageTranscoder::transcode_image(const ktxBasisImageDesc& image,
                                         ktx_transcode_fmt_e targetFormat,
                                         ktx_uint8_t* writePtr,
                                         ktx_size_t bufferByteLength,
                                         uint32_t level,
                                         ktx_uint8_t* levelDataPtr,
                                         uint32_t width, uint32_t height,
                                         uint32_t num_blocks_x,
                                         uint32_t num_blocks_y,
                                         bool isVideo,
                                         bool transcodeAlphaToOpaqueFormats,
                                         basisu_transcoder_state* pState)
{
    KTX_error_code result = KTX_SUCCESS;

    if (targetFormat > KTX_TTF_ETC) {
        // Only real format values can be accepted here.
        return KTX_INVALID_VALUE;
    }

    bool status = false;
    bool hasAlpha, isAlphaSlice;
    uint32_t sliceByteOffset, sliceByteLength;
    uint32_t bytes_per_block
          = basis_get_bytes_per_block((transcoder_texture_format)targetFormat);

    hasAlpha = image.alphaSliceByteLength > 0;
    // If the caller wants us to transcode the mip level's alpha data,
    // for opaque formats then use alpha slice.
    if (hasAlpha && transcodeAlphaToOpaqueFormats) {
        sliceByteOffset = image.alphaSliceByteOffset;
        sliceByteLength = image.alphaSliceByteLength;
        isAlphaSlice = true;
    } else {
        sliceByteOffset = image.rgbSliceByteOffset;
        sliceByteLength = image.rgbSliceByteLength;
        isAlphaSlice = false;
    }
    switch (targetFormat) {
      case KTX_TTF_ETC1_RGB:
      {
        // No need to pass output_row_pitch_in_blocks. It defaults to
        // num_blocks_x.
        // transcoder state is only necessary for video.

        // level is used as an index, together with isAlphaSlice, into
        // to retrieve an array of previous frame indices from a 2D
        // table maintained in the transcoder state when transcoding
        // video.
        // TODO: Find out what if any ordering of the levels is
        // necessary for transcoding video frames with mip levels.
        status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + sliceByteOffset,
                    sliceByteLength,
                    basist::block_format::cETC1, bytes_per_block,
                    true,
                    isVideo, isAlphaSlice, level, width, height,
                    0 /* row_pitch */, pState);
        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case KTX_TTF_BC1_RGB:
      {
#if !BASISD_SUPPORT_DXT1
        return KTX_UNSUPPORTED_FEATURE;
#endif
        status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + sliceByteOffset,
                    sliceByteLength,
                    basist::block_format::cBC1, bytes_per_block,
                    true,
                    isVideo, isAlphaSlice, level, width, height,
                    0 /* row_pitch */, pState);

        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case KTX_TTF_BC4_R:
      {
#if !BASISD_SUPPORT_DXT5A
        return KTX_UNSUPPORTED_FEATURE;
#endif
        status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + sliceByteOffset,
                    sliceByteLength,
                    basist::block_format::cBC4, bytes_per_block,
                    true,
                    isVideo, isAlphaSlice, level, width, height,
                    0 /* row_pitch */, pState);

        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case KTX_TTF_PVRTC1_4_RGB:
      {
#if !BASISD_SUPPORT_PVRTC1
        return KTX_UNSUPPORTED_FEATURE;
#endif
        status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + sliceByteOffset,
                    sliceByteLength,
                    basist::block_format::cPVRTC1_4_RGB,
                    bytes_per_block,
                    true,
                    isVideo, isAlphaSlice, level, width, height,
                    0 /* row_pitch */, pState);

        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case KTX_TTF_PVRTC2_4_RGB:
      {
#if !BASISD_SUPPORT_PVRTC2
        return KTX_UNSUPPORTED_FEATURE;
#endif
        status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + sliceByteOffset,
                    sliceByteLength,
                    basist::block_format::cPVRTC2_4_RGB,
                    bytes_per_block,
                    true,
                    isVideo, isAlphaSlice, level, width, height,
                    0 /* row_pitch */, pState);

        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case KTX_TTF_PVRTC1_4_RGBA:
      {
#if !BASISD_SUPPORT_PVRTC1
        return KTX_UNSUPPORTED_FEATURE;
#endif
        assert(hasAlpha);

        std::vector<uint32_t> temp_block_indices(num_blocks_x * num_blocks_y);

        // First decode alpha to temp buffer
        status = transcode_slice(temp_block_indices.data(),
                num_blocks_x, num_blocks_y,
                levelDataPtr + image.alphaSliceByteOffset,
                image.alphaSliceByteLength,
                basist::block_format::cIndices, sizeof(uint32_t),
                true,
                isVideo, true, level, width, height,
                0 /* row_pitch */, pState);

        if (status) {
            // Now decode the color data.
            // Note that output_row_pitch_in_blocks is actually ignored
            // when transcoding to PVRTC1. Since we're using the
            // the default, 0, this is not an issue.
            status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + image.rgbSliceByteOffset,
                    image.rgbSliceByteLength,
                    basist::block_format::cPVRTC1_4_RGBA,
                    bytes_per_block,
                    true,
                    isVideo, false, level, width, height,
                    0 /* row_pitch */, pState,
                    hasAlpha, temp_block_indices.data());
        }
        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case KTX_TTF_BC7_M6_RGB:
      {
#if !BASISD_SUPPORT_BC7
        return KTX_UNSUPPORTED_FEATURE;
#endif

        status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + sliceByteOffset,
                    sliceByteLength,
                    basist::block_format::cBC7_M6_OPAQUE_ONLY,
                    bytes_per_block,
                    true,
                    isVideo, isAlphaSlice, level, width, height,
                    0 /* row_pitch */, pState);
        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case KTX_TTF_BC7_M5_RGBA:
      {
#if !BASISD_SUPPORT_BC7
        return KTX_UNSUPPORTED_FEATURE;
#endif
        // Decode the color data
        status = transcode_slice(writePtr,
                num_blocks_x, num_blocks_y,
                levelDataPtr + image.rgbSliceByteOffset,
                image.rgbSliceByteLength,
                basist::block_format::cBC7_M5_COLOR, bytes_per_block,
                true,
                isVideo, false, level, width, height,
                0 /* row_pitch */, pState);

        if (status) {
            if (hasAlpha) {
                status = transcode_slice(writePtr,
                        num_blocks_x, num_blocks_y,
                        levelDataPtr + image.alphaSliceByteOffset,
                        image.alphaSliceByteLength,
                        basist::block_format::cBC7_M5_ALPHA,
                        bytes_per_block,
                        true,
                        isVideo, true, level, width, height,
                        0 /* row_pitch */, pState);
            } else {
                basisu_transcoder::write_opaque_alpha_blocks(
                    num_blocks_x, num_blocks_y, writePtr,
                    (uint32_t)(bufferByteLength / bytes_per_block),
                    basist::block_format::cBC7_M5_ALPHA, bytes_per_block,
                    0);
                status = true;
            }
        }

        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case KTX_TTF_ETC2_RGBA:
      {
#if !BASISD_SUPPORT_ETC2_EAC_A8
        return KTX_UNSUPPORTED_FEATURE;
#endif
        if (hasAlpha) {
            status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + image.alphaSliceByteOffset,
                    image.alphaSliceByteLength,
                    basist::block_format::cETC2_EAC_A8, bytes_per_block,
                    true,
                    isVideo, true, level, width, height,
                    0 /* row_pitch */, pState);
        } else {
            basisu_transcoder::write_opaque_alpha_blocks(
                num_blocks_x, num_blocks_y,
                writePtr,
                (uint32_t)(bufferByteLength / bytes_per_block),
                basist::block_format::cETC2_EAC_A8, bytes_per_block,
                0);
            status = true;
        }
        if (status) {
            // Now decode the color data.
          status = transcode_slice(writePtr + 8,
                  num_blocks_x, num_blocks_y,
                  levelDataPtr + image.rgbSliceByteOffset,
                  image.rgbSliceByteLength,
                  basist::block_format::cETC1, bytes_per_block,
                  true,
                  isVideo, false, level, width, height,
                  0 /* row_pitch */, pState);
        }
        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case KTX_TTF_BC3_RGBA:
      {
#if !BASISD_SUPPORT_DXT1 && !BASISD_SUPPORT_DXT5A
        return KTX_UNSUPPORTED_FEATURE;
#endif
         // First decode the alpha data

        if (hasAlpha) {
            status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + image.alphaSliceByteOffset,
                    image.alphaSliceByteLength,
                    basist::block_format::cBC4, bytes_per_block,
                    true,
                    isVideo, true, level, width, height,
                    0 /* row_pitch */, pState);
        } else {
            basisu_transcoder::write_opaque_alpha_blocks(
                num_blocks_x, num_blocks_y, writePtr,
                (uint32_t)(bufferByteLength/ bytes_per_block),
                basist::block_format::cBC4, bytes_per_block, 0);
            status = true;
        }

        if (status) {
            // Now decode the color data. Forbid BC1 3 color blocks,
            // which aren't supported in BC3.
            status = transcode_slice(writePtr + 8,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + image.rgbSliceByteOffset,
                    image.rgbSliceByteLength,
                    basist::block_format::cBC1, bytes_per_block,
                    false, // Forbid 3 color blocks
                    isVideo, false, level, width, height,
                    0 /* row_pitch */, pState);
        }
        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case KTX_TTF_BC5_RG:
      {
#if !BASISD_SUPPORT_DXT5A
        return KTX_UNSUPPORTED_FEATURE;
#endif
        // Decode the R data (actually the green channel of the color
        // data slice in the basis file)
        status = transcode_slice(writePtr,
                num_blocks_x, num_blocks_y,
                levelDataPtr + image.rgbSliceByteOffset,
                image.rgbSliceByteLength,
                basist::block_format::cBC4, bytes_per_block,
                0, // Forbid 3 color blocks
                isVideo, false, level, width, height,
                0 /* row_pitch */, pState);

        if (status) {
            if (hasAlpha) {
                // Decode the G data (actually the green channel of the
                // alpha data slice in the basis file)
                status = transcode_slice(writePtr + 8,
                        num_blocks_x, num_blocks_y,
                        levelDataPtr + image.alphaSliceByteOffset,
                        image.alphaSliceByteLength,
                        basist::block_format::cBC4, bytes_per_block,
                        true,
                        isVideo, true, level, width, height,
                        0 /* row_pitch */, pState);
            } else {
                basisu_transcoder::write_opaque_alpha_blocks(
                    num_blocks_x, num_blocks_y, writePtr + 8,
                    (uint32_t)((bufferByteLength - 8) / bytes_per_block),
                    basist::block_format::cBC4, bytes_per_block, 0);
                status = true;
            }
        }
        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case KTX_TTF_ASTC_4x4_RGBA:
      {
#if !BASISD_SUPPORT_ASTC
        return KTX_UNSUPPORTED_FEATURE;
#endif
        if (hasAlpha) {
            // First decode alpha to the output using the output texture
            // as a temporary buffer.
            status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + image.alphaSliceByteOffset,
                    image.alphaSliceByteLength,
                    basist::block_format::cIndices, bytes_per_block,
                    true,
                    isVideo, true, level, width, height,
                    0 /* row_pitch */, pState);
        } else {
            status = true;
        }
        if (status) {
            // Now decode the color data and transcode to ASTC. The
            // transcoder function will read the alpha selector data
            // from the output texture as it converts and transcode
            // both the alpha and color data at the same time to
            // ASTC. hasAlpha tells the transcoder alpha is present.
            status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + image.rgbSliceByteOffset,
                    image.rgbSliceByteLength,
                    basist::block_format::cASTC_4x4, bytes_per_block,
                    true,
                    isVideo, false, level, width, height,
                    0 /* row_pitch */, pState, hasAlpha);
        }
        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case KTX_TTF_PVRTC2_4_RGBA:
      {
#if !BASISD_SUPPORT_PVRTC2
        return KTX_UNSUPPORTED_FEATURE;
#endif
        if (hasAlpha) {
            // As with ASTC, use the output texture as a temporary
            // buffer for alpha.
            status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + image.alphaSliceByteOffset,
                    image.alphaSliceByteLength,
                    basist::block_format::cIndices, bytes_per_block,
                    true,
                    isVideo, true, level, width, height,
                    0 /* row_pitch */, pState);
        } else {
            status = true;
        }
        if (status) {
            // Now decode the color data and transcode to PVRTC.
            status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + image.rgbSliceByteOffset,
                    image.rgbSliceByteLength,
                    basist::block_format::cPVRTC2_4_RGBA,
                    bytes_per_block,
                    true,
                    isVideo, false, level, width, height,
                    0 /* row_pitch */, pState, hasAlpha);
        }
        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case KTX_TTF_RGB565:
      {
        status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + sliceByteOffset,
                    sliceByteLength,
                    basist::block_format::cRGB565, sizeof(uint16_t),
                    true,
                    isVideo, isAlphaSlice, level, width, height,
                    0 /* row_pitch */, pState);
        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case KTX_TTF_BGR565:
      {
        status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + sliceByteOffset,
                    sliceByteLength,
                    basist::block_format::cBGR565, sizeof(uint16_t),
                    true,
                    isVideo, isAlphaSlice, level, width, height,
                    0 /* row_pitch */, pState);
        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case KTX_TTF_RGBA4444:
      {
        if(hasAlpha) {
            status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + image.alphaSliceByteOffset,
                    image.alphaSliceByteLength,
                    basist::block_format::cRGBA4444_ALPHA,
                    sizeof(uint16_t),
                    true,
                    isVideo, true, level, width, height,
                    0 /* row_pitch */, pState);
        } else {
            status = true;
        }

        if (status) {
            status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + image.rgbSliceByteOffset,
                    image.rgbSliceByteLength,
                    hasAlpha ? basist::block_format::cRGBA4444_COLOR : basist::block_format::cRGBA4444_COLOR_OPAQUE,
                    sizeof(uint16_t),
                    true,
                    isVideo, false, level, width, height,
                    0 /* row_pitch */, pState);
        }
        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case KTX_TTF_RGBA32:
      {
        if(hasAlpha) {
            status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + image.alphaSliceByteOffset,
                    image.alphaSliceByteLength,
                    basist::block_format::cA32, sizeof(uint32_t),
                    true,
                    isVideo, true, level, width, height,
                    0 /* row_pitch */, pState);
        } else {
            status = true;
        }

        if (status) {
            status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + image.rgbSliceByteOffset,
                    image.rgbSliceByteLength,
                    hasAlpha ? basist::block_format::cRGB32 : basist::block_format::cRGBA32,
                    sizeof(uint32_t),
                    true,
                    isVideo, false, level, width, height,
                    0 /* row_pitch */, pState);
        }

        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
        }
        case KTX_TTF_ETC2_EAC_R11:
        {
#if !BASISD_SUPPORT_ETC2_EAC_RG11
          return KTX_UNSUPPORTED_FEATURE;
#endif
          status = transcode_slice(writePtr,
                      num_blocks_x, num_blocks_y,
                      levelDataPtr + sliceByteOffset,
                      sliceByteLength,
                      basist::block_format::cETC2_EAC_R11, bytes_per_block,
                      true,
                      isVideo, isAlphaSlice, level, width, height,
                      0 /* row_pitch */, pState);

          if (!status) {
               result = KTX_TRANSCODE_FAILED;
          }
          break;
        }
        case KTX_TTF_ETC2_EAC_RG11:
        {
#if !BASISD_SUPPORT_ETC2_EAC_RG11
          return KTX_UNSUPPORTED_FEATURE;
#endif
          if (hasAlpha) {
              // Decode the alpha data to G.
              status = transcode_slice(writePtr + 8,
                      num_blocks_x, num_blocks_y,
                      levelDataPtr + image.alphaSliceByteOffset,
                      image.alphaSliceByteLength,
                      basist::block_format::cETC2_EAC_R11, bytes_per_block,
                      true,
                      isVideo, true, level, width, height,
                      0 /* row_pitch */, pState);
          } else {
              basisu_transcoder::write_opaque_alpha_blocks(num_blocks_x,
                      num_blocks_y, writePtr + 8,
                      (uint32_t)(bufferByteLength / bytes_per_block),
                      basist::block_format::cETC2_EAC_R11,
                      bytes_per_block, 0);
              status = true;
          }
          if (status) {
              // Now decode the color data to R.
            status = transcode_slice(writePtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + image.rgbSliceByteOffset,
                    image.rgbSliceByteLength,
                    basist::block_format::cETC2_EAC_R11, bytes_per_block,
                    true,
                    isVideo, false, level, width, height,
                    0 /* row_pitch */, pState);
          }
          if (!status) {
               result = KTX_TRANSCODE_FAILED;
          }
          break;
        }
        default:
          return KTX_INVALID_VALUE;
    } // end targetFormat switch
    return result;
}
