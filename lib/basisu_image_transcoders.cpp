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
 * @file basisu_image_transcoder.cpp
 * @~English
 *
 * @brief Functions for transcoding a Basis Universal texture.
 *
 * Two worlds collide here too. More uglyness!
 *
 * @author Mark Callow, www.edgewise-consulting.com
 */

#include "basisu_image_transcoders.h"
#include "basisu_transcoder_config.h"
#include "basisu/basisu_comp.h"
#include "basisu/transcoder/basisu_file_headers.h"
#include "basisu/transcoder/basisu_transcoder.h"

using namespace basisu;
using namespace basist;

inline bool
isPow2(uint32_t x) { return x && ((x & (x - 1U)) == 0U); }

inline bool
isPow2(uint64_t x) { return x && ((x & (x - 1U)) == 0U); }

inline size_t
transcodedImageSize(transcoder_texture_format fmt,
                uint32_t bytes_per_block_or_pixel,
                uint32_t width, uint32_t height,
                uint32_t num_blocks_x, uint32_t num_blocks_y)
{
    if ((basis_transcoder_format_is_uncompressed(fmt))) {
        return width * height * bytes_per_block_or_pixel;
    } else if (fmt == transcoder_texture_format::cTFFXT1_RGB) {
        const uint32_t num_blocks_fxt1_x = (width + 7) / 8;
        const uint32_t num_blocks_fxt1_y = (height + 3) / 4;
        const uint32_t total_blocks_fxt1 = num_blocks_fxt1_x * num_blocks_fxt1_y;
        return total_blocks_fxt1 * bytes_per_block_or_pixel;
    } else {
        return num_blocks_x * num_blocks_y * bytes_per_block_or_pixel;
    }
}

/**
 * @internal
 * @~English
 * @brief Transcode a single Basis supercompressed image.
 *
 * @param[in] image     reference to the @c ktxBasisImageDesc of the image to be transcoded.
 *                   This comes from the @c supercompressionGlobalData area of a KTX2 file.
 * @param[in] targetFormat the format to which to transcode the image.
 * @param[in] dstBufferPtr pointer to the location to write the transcoded image.
 * @param[in] dstBufferByteLength the length of the buffer pointed to by @p dstBufferPtr.
 * @param[in] level the mip level of the image being transcoded.
 * @param[in] levelDataPtr pointer to the start of the supercompressed data for mip level @p level.
 * @param[in] width the pixel width of a level @p level image.
 * @param[in] height the pixel height of a level @p level image.
 * @param[in] num_blocks_x number of blocks in the x dimension of mip level @p level to be
 *                         transcoded. This is the number of blocks in base block-compressed
 *                         format used by Basis Universal. When the format is ETC1, as indicated
 *                         by @c eBuIsETC1S being set in @c globalFlags in the
 *                         supercompression global data, the block width to use for calculating
 *                         this from @p width is 4.
 * @param[in] num_blocks_y number of blocks in the y dimension of mip level @p level to be
 *                        transcoded. When @c eBuIsETC1S is set in @c globalFlags in the
 *                        supercompression global data the block height to use for calculating this
 *                        from @p height is 4.
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
                                         transcoder_texture_format targetFormat,
                                         uint8_t* dstBufferPtr,
                                         size_t dstBufferByteLength,
                                         uint32_t level,
                                         const uint8_t* levelDataPtr,
                                         uint32_t width, uint32_t height,
                                         uint32_t num_blocks_x,
                                         uint32_t num_blocks_y,
                                         bool isVideo,
                                         bool transcodeAlphaToOpaqueFormats,
                                         basisu_transcoder_state* pState)
{
    KTX_error_code result = KTX_SUCCESS;


    bool status = false;
    bool hasAlpha, isAlphaSlice;
    uint32_t sliceByteOffset, sliceByteLength;
    uint32_t bytes_per_block
              = basis_get_bytes_per_block_or_pixel((transcoder_texture_format)targetFormat);
    size_t requiredBufferSize
        = transcodedImageSize(targetFormat, bytes_per_block, width, height,
                              // Passing these is a slight cheat that works
                              // because all target block formats are 4x4
                              // like the input format.
                              num_blocks_x, num_blocks_y);
    if (requiredBufferSize > dstBufferByteLength) {
        return KTX_INVALID_VALUE;
    }

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
      case transcoder_texture_format::cTFETC1_RGB:
      {
        // No need to pass output_row_pitch_in_blocks. It defaults to
        // num_blocks_x.
        // transcoder state is only necessary for video.

        // level is used as an index, together with isAlphaSlice, to
        // retrieve an array of previous frame indices from a 2D table
        // maintained in the transcoder state when transcoding video.
        // This is used to match up images from the same mip level to
        // find the previous frame when the slice is not an IFrame.
        status = transcode_slice(dstBufferPtr,
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
      case transcoder_texture_format::cTFBC1_RGB:
      {
        if (!BASISD_SUPPORT_DXT1)
            return KTX_UNSUPPORTED_FEATURE;
        status = transcode_slice(dstBufferPtr,
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
      case transcoder_texture_format::cTFBC4_R:
      {
        if (!BASISD_SUPPORT_DXT5A)
            return KTX_UNSUPPORTED_FEATURE;
        status = transcode_slice(dstBufferPtr,
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
      case transcoder_texture_format::cTFPVRTC1_4_RGB:
      {
        if (!BASISD_SUPPORT_PVRTC1)
            return KTX_UNSUPPORTED_FEATURE;
        status = transcode_slice(dstBufferPtr,
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
      case transcoder_texture_format::cTFPVRTC2_4_RGB:
      {
        if (!BASISD_SUPPORT_PVRTC2)
            return KTX_UNSUPPORTED_FEATURE;
        status = transcode_slice(dstBufferPtr,
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
      case transcoder_texture_format::cTFPVRTC1_4_RGBA:
      {
        if (!BASISD_SUPPORT_PVRTC1)
            return KTX_UNSUPPORTED_FEATURE;

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
            status = transcode_slice(dstBufferPtr,
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
      case transcoder_texture_format::cTFBC7_RGBA:
      {
        if (!BASISD_SUPPORT_BC7_MODE5)
            return KTX_UNSUPPORTED_FEATURE;
        // Decode the color data
        status = transcode_slice(dstBufferPtr,
                num_blocks_x, num_blocks_y,
                levelDataPtr + image.rgbSliceByteOffset,
                image.rgbSliceByteLength,
                basist::block_format::cBC7_M5_COLOR, bytes_per_block,
                true,
                isVideo, false, level, width, height,
                0 /* row_pitch */, pState);

        if (status && hasAlpha) {
            status = transcode_slice(dstBufferPtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + image.alphaSliceByteOffset,
                    image.alphaSliceByteLength,
                    basist::block_format::cBC7_M5_ALPHA,
                    bytes_per_block,
                    true,
                    isVideo, true, level, width, height,
                    0 /* row_pitch */, pState);
        }

        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case transcoder_texture_format::cTFETC2_RGBA:
      {
        if (!BASISD_SUPPORT_ETC2_EAC_A8)
            return KTX_UNSUPPORTED_FEATURE;
        if (hasAlpha) {
            status = transcode_slice(dstBufferPtr,
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
                dstBufferPtr,
                (uint32_t)(dstBufferByteLength / bytes_per_block),
                basist::block_format::cETC2_EAC_A8, bytes_per_block,
                0);
            status = true;
        }
        if (status) {
            // Now decode the color data.
          status = transcode_slice(dstBufferPtr + 8,
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
      case transcoder_texture_format::cTFBC3_RGBA:
      {
        if (!BASISD_SUPPORT_DXT1 && !BASISD_SUPPORT_DXT5A)
            return KTX_UNSUPPORTED_FEATURE;

        // First decode the alpha data
        if (hasAlpha) {
            status = transcode_slice(dstBufferPtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + image.alphaSliceByteOffset,
                    image.alphaSliceByteLength,
                    basist::block_format::cBC4, bytes_per_block,
                    true,
                    isVideo, true, level, width, height,
                    0 /* row_pitch */, pState);
        } else {
            basisu_transcoder::write_opaque_alpha_blocks(
                num_blocks_x, num_blocks_y, dstBufferPtr,
                (uint32_t)(dstBufferByteLength/ bytes_per_block),
                basist::block_format::cBC4, bytes_per_block, 0);
            status = true;
        }

        if (status) {
            // Now decode the color data. Forbid BC1 3 color blocks,
            // which aren't supported in BC3.
            status = transcode_slice(dstBufferPtr + 8,
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
      case transcoder_texture_format::cTFBC5_RG:
      {
        if (!BASISD_SUPPORT_DXT5A)
            return KTX_UNSUPPORTED_FEATURE;

        // Decode the R data (actually the green channel of the color
        // data slice in the basis file)
        status = transcode_slice(dstBufferPtr,
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
                status = transcode_slice(dstBufferPtr + 8,
                        num_blocks_x, num_blocks_y,
                        levelDataPtr + image.alphaSliceByteOffset,
                        image.alphaSliceByteLength,
                        basist::block_format::cBC4, bytes_per_block,
                        true,
                        isVideo, true, level, width, height,
                        0 /* row_pitch */, pState);
            } else {
                basisu_transcoder::write_opaque_alpha_blocks(
                    num_blocks_x, num_blocks_y, dstBufferPtr + 8,
                    (uint32_t)((dstBufferByteLength - 8) / bytes_per_block),
                    basist::block_format::cBC4, bytes_per_block, 0);
                status = true;
            }
        }
        if (!status) {
             result = KTX_TRANSCODE_FAILED;
        }
        break;
      }
      case transcoder_texture_format::cTFASTC_4x4_RGBA:
      {
        if (!BASISD_SUPPORT_ASTC)
            return KTX_UNSUPPORTED_FEATURE;
        if (hasAlpha) {
            // First decode alpha to the output using the output texture
            // as a temporary buffer.
            status = transcode_slice(dstBufferPtr,
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
            status = transcode_slice(dstBufferPtr,
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
      case transcoder_texture_format::cTFPVRTC2_4_RGBA:
      {
        if (!BASISD_SUPPORT_PVRTC2)
            return KTX_UNSUPPORTED_FEATURE;
        if (hasAlpha) {
            // As with ASTC, use the output texture as a temporary
            // buffer for alpha.
            status = transcode_slice(dstBufferPtr,
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
            status = transcode_slice(dstBufferPtr,
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
      case transcoder_texture_format::cTFRGB565:
      {
        status = transcode_slice(dstBufferPtr,
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
      case transcoder_texture_format::cTFBGR565:
      {
        status = transcode_slice(dstBufferPtr,
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
      case transcoder_texture_format::cTFRGBA4444:
      {
        if(hasAlpha) {
            status = transcode_slice(dstBufferPtr,
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
            status = transcode_slice(dstBufferPtr,
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
      case transcoder_texture_format::cTFRGBA32:
      {
        if(hasAlpha) {
            status = transcode_slice(dstBufferPtr,
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
            status = transcode_slice(dstBufferPtr,
                    num_blocks_x, num_blocks_y,
                    levelDataPtr + image.rgbSliceByteOffset,
                    image.rgbSliceByteLength,
                    hasAlpha ? basist::block_format::cRGB32
                             : basist::block_format::cRGBA32,
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
        case transcoder_texture_format::cTFETC2_EAC_R11:
        {
          if (!BASISD_SUPPORT_ETC2_EAC_RG11)
              return KTX_UNSUPPORTED_FEATURE;
          status = transcode_slice(dstBufferPtr,
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
        case transcoder_texture_format::cTFETC2_EAC_RG11:
        {
          if (!BASISD_SUPPORT_ETC2_EAC_RG11)
              return KTX_UNSUPPORTED_FEATURE;
          if (hasAlpha) {
              // Decode the alpha data to G.
              status = transcode_slice(dstBufferPtr + 8,
                      num_blocks_x, num_blocks_y,
                      levelDataPtr + image.alphaSliceByteOffset,
                      image.alphaSliceByteLength,
                      basist::block_format::cETC2_EAC_R11, bytes_per_block,
                      true,
                      isVideo, true, level, width, height,
                      0 /* row_pitch */, pState);
          } else {
              basisu_transcoder::write_opaque_alpha_blocks(num_blocks_x,
                      num_blocks_y, dstBufferPtr + 8,
                      (uint32_t)(dstBufferByteLength / bytes_per_block),
                      basist::block_format::cETC2_EAC_R11,
                      bytes_per_block, 0);
              status = true;
          }
          if (status) {
              // Now decode the color data to R.
            status = transcode_slice(dstBufferPtr,
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

KTX_error_code
ktxUastcImageTranscoder::transcode_image(transcoder_texture_format targetFormat,
                                         uint8_t* dstBufferPtr,
                                         size_t dstBufferByteLength,
                                         uint32_t level,
                                         const uint8_t* inImagePtr,
                                         const uint32_t inImageByteLength,
                                         uint32_t width, uint32_t height,
                                         uint32_t num_blocks_x,
                                         uint32_t num_blocks_y,
                                         bool hasAlpha,
                                         uint32_t decodeFlags,
                                         basisu_transcoder_state* pState)
{
    bool status;
    KTX_error_code result = KTX_SUCCESS;

    uint32_t bytes_per_block_or_pixel
            = basis_get_bytes_per_block_or_pixel((transcoder_texture_format)targetFormat);

    size_t requiredBufferSize
        = transcodedImageSize(targetFormat, bytes_per_block_or_pixel,
                              width, height,
                              // Passing these is a slight cheat that works
                              // because all target block formats are 4x4
                              // like the input format.
                              num_blocks_x, num_blocks_y);
    if (requiredBufferSize > dstBufferByteLength) {
        return KTX_INVALID_VALUE;
    }

    const bool transcodeAlphaToOpaqueFormats
        = (hasAlpha && (decodeFlags & cDecodeFlagsTranscodeAlphaDataToOpaqueFormats));

    switch (targetFormat) {
      case transcoder_texture_format::cTFETC1_RGB:
        status = transcode_slice(dstBufferPtr, num_blocks_x, num_blocks_y,
                                 inImagePtr, inImageByteLength,
                                 block_format::cETC1,
                                 bytes_per_block_or_pixel,
                                 false, hasAlpha,
                                 width, height,
                                 0 /*output_row_pitch_in_blocks_or_pixels*/,
                                 pState,
                                 0 /*output_rows_in_pixels*/,
                                 -1, -1, decodeFlags);
        if (!status) {
            result = KTX_TRANSCODE_FAILED;
        }
        break;
      case transcoder_texture_format::cTFETC2_RGBA:
        status = transcode_slice(dstBufferPtr, num_blocks_x, num_blocks_y,
                                 inImagePtr, inImageByteLength,
                                 block_format::cETC2_RGBA,
                                 bytes_per_block_or_pixel,
                                 false, hasAlpha,
                                 width, height,
                                 0 /*output_row_pitch_in_blocks_or_pixels*/,
                                 pState,
                                 0 /*output_rows_in_pixels*/,
                                 -1, -1, decodeFlags);
        if (!status) {
            result = KTX_TRANSCODE_FAILED;
        }
        break;
      case transcoder_texture_format::cTFBC1_RGB:
        // TODO: ETC1S allows BC1 from alpha channel. That doesn't seem actually useful, though.
        status = transcode_slice(dstBufferPtr, num_blocks_x, num_blocks_y,
                                 inImagePtr, inImageByteLength,
                                 block_format::cBC1,
                                 bytes_per_block_or_pixel,
                                 false, hasAlpha,
                                 width, height,
                                 0 /*output_row_pitch_in_blocks_or_pixels*/,
                                 pState,
                                 0 /*output_rows_in_pixels*/,
                                 -1, -1, decodeFlags);
        if (!status) {
            result = KTX_TRANSCODE_FAILED;
        }
        break;
      case transcoder_texture_format::cTFBC3_RGBA:
        status = transcode_slice(dstBufferPtr, num_blocks_x, num_blocks_y,
                                 inImagePtr, inImageByteLength,
                                 block_format::cBC3,
                                 bytes_per_block_or_pixel,
                                 false, hasAlpha,
                                 width, height,
                                 0 /*output_row_pitch_in_blocks_or_pixels*/,
                                 pState,
                                 0 /*output_rows_in_pixels*/,
                                 -1, -1, decodeFlags);
        if (!status) {
            result = KTX_TRANSCODE_FAILED;
        }
        break;
      case transcoder_texture_format::cTFBC4_R:
        status = transcode_slice(dstBufferPtr, num_blocks_x, num_blocks_y,
                                 inImagePtr, inImageByteLength,
                                 block_format::cBC4,
                                 bytes_per_block_or_pixel,
                                 false, hasAlpha,
                                 width, height,
                                 0 /*output_row_pitch_in_blocks_or_pixels*/,
                                 pState,
                                 0 /*output_rows_in_pixels*/,
                                 transcodeAlphaToOpaqueFormats ? 3 : 0, -1,
                                 decodeFlags);
        if (!status) {
            result = KTX_TRANSCODE_FAILED;
        }
        break;
      case transcoder_texture_format::cTFBC5_RG:
        status = transcode_slice(dstBufferPtr, num_blocks_x, num_blocks_y,
                                 inImagePtr, inImageByteLength,
                                 block_format::cBC5,
                                 bytes_per_block_or_pixel,
                                 false, hasAlpha,
                                 width, height,
                                 0 /*output_row_pitch_in_blocks_or_pixels*/,
                                 pState,
                                 0 /*output_rows_in_pixels*/,
                                 0, 3,
                                 decodeFlags);
        if (!status) {
            result = KTX_TRANSCODE_FAILED;
        }
        break;
      case transcoder_texture_format::cTFBC7_RGBA:
        status = transcode_slice(dstBufferPtr, num_blocks_x, num_blocks_y,
                                 inImagePtr, inImageByteLength,
                                 block_format::cBC7,
                                 bytes_per_block_or_pixel,
                                 false, hasAlpha,
                                 width, height,
                                 0 /*output_row_pitch_in_blocks_or_pixels*/,
                                 pState,
                                 0 /*output_rows_in_pixels*/,
                                 -1, -1, decodeFlags);
        if (!status) {
            result = KTX_TRANSCODE_FAILED;
        }
        break;
      case transcoder_texture_format::cTFPVRTC1_4_RGB:
        status = transcode_slice(dstBufferPtr, num_blocks_x, num_blocks_y,
                                 inImagePtr, inImageByteLength,
                                 block_format::cPVRTC1_4_RGB,
                                 bytes_per_block_or_pixel,
                                 false, hasAlpha,
                                 width, height,
                                 0 /*output_row_pitch_in_blocks_or_pixels*/,
                                 pState,
                                 0 /*output_rows_in_pixels*/,
                                 -1, -1, decodeFlags);
        if (!status) {
            result = KTX_TRANSCODE_FAILED;
        }
        break;
      case transcoder_texture_format::cTFPVRTC1_4_RGBA:
        status = transcode_slice(dstBufferPtr, num_blocks_x, num_blocks_y,
                                 inImagePtr, inImageByteLength,
                                 block_format::cPVRTC1_4_RGBA,
                                 bytes_per_block_or_pixel,
                                 false, hasAlpha,
                                 width, height,
                                 0 /*output_row_pitch_in_blocks_or_pixels*/,
                                 pState,
                                 0 /*output_rows_in_pixels*/,
                                 -1, -1, decodeFlags);
        if (!status) {
            result = KTX_TRANSCODE_FAILED;
        }
        break;
      case transcoder_texture_format::cTFASTC_4x4_RGBA:
        status = transcode_slice(dstBufferPtr, num_blocks_x, num_blocks_y,
                                 inImagePtr, inImageByteLength,
                                 block_format::cASTC_4x4,
                                 bytes_per_block_or_pixel,
                                 false, hasAlpha,
                                 width, height,
                                 0 /*output_row_pitch_in_blocks_or_pixels*/,
                                 pState,
                                 0 /*output_rows_in_pixels*/,
                                 -1, -1, decodeFlags);
        if (!status) {
            result = KTX_TRANSCODE_FAILED;
        }
        break;
      case transcoder_texture_format::cTFPVRTC2_4_RGB:
        // UASTC->PVRTC2 currently unsupported.
        result = KTX_UNSUPPORTED_FEATURE;
        break;
      case transcoder_texture_format::cTFPVRTC2_4_RGBA:
        // UASTC->PVRTC2 currently unsupported.
        result = KTX_UNSUPPORTED_FEATURE;
        break;
      case transcoder_texture_format::cTFETC2_EAC_R11:
        status = transcode_slice(dstBufferPtr, num_blocks_x, num_blocks_y,
                                 inImagePtr, inImageByteLength,
                                 block_format::cETC2_EAC_R11,
                                 bytes_per_block_or_pixel,
                                 false, hasAlpha,
                                 width, height,
                                 0 /*output_row_pitch_in_blocks_or_pixels*/,
                                 pState,
                                 0 /*output_rows_in_pixels*/,
                                 transcodeAlphaToOpaqueFormats ? 3 : 0, -1,
                                 decodeFlags);
        if (!status) {
            result = KTX_TRANSCODE_FAILED;
        }
        break;
      case transcoder_texture_format::cTFETC2_EAC_RG11:
        status = transcode_slice(dstBufferPtr, num_blocks_x, num_blocks_y,
                                 inImagePtr, inImageByteLength,
                                 block_format::cETC2_EAC_RG11,
                                 bytes_per_block_or_pixel,
                                 false, hasAlpha,
                                 width, height,
                                 0 /*output_row_pitch_in_blocks_or_pixels*/,
                                 pState,
                                 0 /*output_rows_in_pixels*/,
                                 0, 3,
                                 decodeFlags);
        if (!status) {
            result = KTX_TRANSCODE_FAILED;
        }
        break;
      case transcoder_texture_format::cTFRGBA32:
        status = transcode_slice(dstBufferPtr, num_blocks_x, num_blocks_y,
                                 inImagePtr, inImageByteLength,
                                 block_format::cRGBA32,
                                 bytes_per_block_or_pixel,
                                 false, hasAlpha,
                                 width, height,
                                 0 /*output_row_pitch_in_blocks_or_pixels*/,
                                 pState,
                                 0 /*output_rows_in_pixels*/,
                                 -1, -1, decodeFlags);
        if (!status) {
            result = KTX_TRANSCODE_FAILED;
        }
        break;
      case transcoder_texture_format::cTFRGB565:
        status = transcode_slice(dstBufferPtr, num_blocks_x, num_blocks_y,
                                 inImagePtr, inImageByteLength,
                                 block_format::cRGB565,
                                 bytes_per_block_or_pixel,
                                 false, hasAlpha,
                                 width, height,
                                 0 /*output_row_pitch_in_blocks_or_pixels*/,
                                 pState,
                                 0 /*output_rows_in_pixels*/,
                                 -1, -1, decodeFlags);
        if (!status) {
            result = KTX_TRANSCODE_FAILED;
        }
        break;
      case transcoder_texture_format::cTFBGR565:
        status = transcode_slice(dstBufferPtr, num_blocks_x, num_blocks_y,
                                 inImagePtr, inImageByteLength,
                                 block_format::cBGR565,
                                 bytes_per_block_or_pixel,
                                 false, hasAlpha,
                                 width, height,
                                 0 /*output_row_pitch_in_blocks_or_pixels*/,
                                 pState,
                                 0 /*output_rows_in_pixels*/,
                                 -1, -1, decodeFlags);
        if (!status) {
            result = KTX_TRANSCODE_FAILED;
        }
        break;
      case transcoder_texture_format::cTFRGBA4444:
        status = transcode_slice(dstBufferPtr, num_blocks_x, num_blocks_y,
                                 inImagePtr, inImageByteLength,
                                 block_format::cRGBA4444,
                                 bytes_per_block_or_pixel,
                                 false, hasAlpha,
                                 width, height,
                                 0 /*output_row_pitch_in_blocks_or_pixels*/,
                                 pState,
                                 0 /*output_rows_in_pixels*/,
                                 -1, -1, decodeFlags);
        if (!status) {
            result = KTX_TRANSCODE_FAILED;
        }
        break;
      default:
        return KTX_INVALID_VALUE;
        break;
    }
    return result;
}
