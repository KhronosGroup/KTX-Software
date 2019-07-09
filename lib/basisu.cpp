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
 * @file basisu_sgd.c
 * @~English
 *
 * @brief Functions for supercompressing a texture with Basis Universal.
 *
 * This is where two worlds collide. Ugly!
 *
 * @author Mark Callow, www.edgewise-consulting.com
 */

#include <inttypes.h>
#include <KHR/khr_df.h>
#include "ktx.h"
#include "ktxint.h"
#include "texture2.h"
#include "vkformat_enum.h"
#include "vk_format.h"
#include "basisu_sgd.h"
#include "basisu/basisu_comp.h"
#include "basisu/transcoder/basisu_file_headers.h"
#include "basisu/transcoder/basisu_transcoder.h"

using namespace basisu;
using namespace basist;

// Copy rgb to rgba. Source images are expected to have no row padding.
static void
copy_rgb_to_rgba(uint8_t* rgbadst, uint8_t* rgbsrc, uint32_t w, uint32_t h)
{
    for (uint32_t row = 0; row < h; row++) {
        for (uint32_t pixel = 0; pixel < w; pixel++) {
            memcpy(rgbadst, rgbsrc, 3);
            rgbadst[3] = 255; // Convince Basis there is no alpha.
            rgbadst += 4; rgbsrc += 3;
        }
    }
}

// Rewrite DFD without sample information and with unspecified color model.
static KTX_error_code
ktxTexture2_rewriteDfd(ktxTexture2* This)
{
    uint32_t* cdfd = This->pDfd;
    uint32_t* ndfd = (uint32_t *)malloc(sizeof(uint32_t) *
                                        (1 + KHR_DF_WORD_SAMPLESTART));
    if (!ndfd)
        return KTX_OUT_OF_MEMORY;
    uint32_t* cbdfd = cdfd + 1; // Point to basic format descriptor.
    uint32_t* nbdfd = ndfd + 1;
    ndfd[0] = sizeof(uint32_t) * (1 + KHR_DF_WORD_SAMPLESTART);
    nbdfd[KHR_DF_WORD_VENDORID] =
        (KHR_DF_VENDORID_KHRONOS << KHR_DF_SHIFT_VENDORID) |
        (KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT << KHR_DF_SHIFT_DESCRIPTORTYPE);
    nbdfd[KHR_DF_WORD_VERSIONNUMBER] =
        (KHR_DF_VERSIONNUMBER_LATEST << KHR_DF_SHIFT_VERSIONNUMBER) |
        (((uint32_t)sizeof(uint32_t) * KHR_DF_WORD_SAMPLESTART)
          << KHR_DF_SHIFT_DESCRIPTORBLOCKSIZE);
    nbdfd[KHR_DF_WORD_MODEL] = cbdfd[KHR_DF_WORD_MODEL] & ~KHR_DF_MASK_MODEL;
    nbdfd[KHR_DF_WORD_MODEL] |= KHR_DF_MODEL_UNSPECIFIED << KHR_DF_SHIFT_MODEL;
    nbdfd[KHR_DF_WORD_TRANSFER] = cbdfd[KHR_DF_WORD_TRANSFER];
    nbdfd[KHR_DF_WORD_TEXELBLOCKDIMENSION0] = 0;
    nbdfd[KHR_DF_WORD_BYTESPLANE0] = 0;
    nbdfd[KHR_DF_WORD_BYTESPLANE4] = 0;
    This->pDfd = ndfd;
    free(cdfd);
    return KTX_SUCCESS;
}

extern "C" KTX_error_code
ktxTexture2_CompressBasis(ktxTexture2* This)
{
    KTX_error_code result;

    if (This->supercompressionScheme != KTX_SUPERCOMPRESSION_NONE)
        return KTX_INVALID_OPERATION; // Can't apply multiple schemes.

    if (This->isCompressed)
        return KTX_INVALID_OPERATION;  // Basis can't be applied to compression
                                       // types other than ETC1S and underlying
                                       // Basis software does ETC1S encoding &
                                       // Basis supercompression together.

    if (This->pData == NULL) {
        result = ktxTexture2_LoadImageData(This, NULL, 0);
        if (result != KTX_SUCCESS)
            return result;
    }

    basis_compressor_params cparams;
    cparams.m_read_source_images = false; // Don't read from source files.
    cparams.m_write_output_basis_files = false; // Don't write output files.

    // Basic descriptor block begins after the total size field.
    const uint32_t* BDB = This->pDfd+1;
    const uint32_t num_components = KHR_DFDSAMPLECOUNT(BDB);

    assert(num_components == 3 || num_components == 4);
    assert(This->_protected->_formatSize.blockSizeInBits == 3 * 8
           || This->_protected->_formatSize.blockSizeInBits == 4 * 8);

    //
    // Calculate number of images
    //
    uint32_t num_images;
    if (This->numDimensions < 3) {
        num_images = This->numLevels * This->numLayers * This->numFaces;
    } else {
        num_images = 0;
        // How find to find out how many images total without these loops?
        for (uint32_t level = 0; level < This->numLevels; level++) {
            uint32_t depth = MAX(1, This->baseDepth >> level);
            for (uint32_t layer = 0; layer < This->numLayers; layer++) {
                num_images += depth;
            }
        }
    }

    //
    // Copy images into compressor parameters.
    //
    // Darn it! m_source_images is a vector of an internal image class which
    // has its own array of RGBA-only pixels. Pending modifications to the
    // basisu code we'll have to copy in the images.
    cparams.m_source_images.resize(num_images);
    std::vector<image>::iterator iit = cparams.m_source_images.begin();

    // NOTA BENE: Mipmap levels are ordered from largest to smallest in .basis.
    for (uint32_t level = 0; level < This->numLevels; level++) {
        uint32_t width = MAX(1, This->baseWidth >> level);
        uint32_t height = MAX(1, This->baseHeight >> level);
        uint32_t depth = MAX(1, This->baseDepth >> level);
        ktx_size_t image_size = ktxTexture2_GetImageSize(This, level);
        for (uint32_t layer = 0; layer < This->numLayers; layer++) {
            uint32_t faceSlices = This->numFaces == 1 ? depth : This->numFaces;
            for (ktx_uint32_t slice = 0; slice < faceSlices; slice++) {
                ktx_size_t offset;
                ktxTexture2_GetImageOffset(This, level, layer, slice, &offset);
                iit->resize(width, height);
                if (num_components == 4) {
                    memcpy(iit->get_ptr(), This->pData + offset, image_size);
                } else {
                    copy_rgb_to_rgba((uint8_t*)iit->get_ptr(),
                                     This->pData + offset, width, height);
                }
                ++iit;
            }
        }
    }

    //
    // Setup rest of compressor parameters
    //
    ktx_uint32_t transfer = KHR_DFDVAL(BDB, TRANSFER);
    if (transfer == KHR_DF_TRANSFER_SRGB)
        cparams.m_perceptual = true;
    else
        cparams.m_perceptual = false;

    cparams.m_mip_gen = false; // We provide the mip levels.

    // There's not default for this. Either set this or the max number of
    // endpoint and selector clusters.
    cparams.m_quality_level = 128;
    // Why's there no default for this? I have no idea.
    basist::etc1_global_selector_codebook sel_codebook(basist::g_global_selector_cb_size, basist::g_global_selector_cb);
    cparams.m_pSel_codebook = &sel_codebook;
    // Or for this;
    job_pool jpool(1);
    cparams.m_pJob_pool = &jpool;

    // Flip images across Y axis
    // cparams.m_y_flip = false; // Let tool, e.g. toktx do its own yflip so
    // ktxTexture is consistent.

    // Output debug information during compression
    //cparams.m_debug = false;

    // m_debug_images is pretty slow
    //cparams.m_debug_images = false;

    // Defaults to BASISU_DEFAULT_COMPRESSION_LEVEL
    //cparams.m_compression_level;

    // Split the R channel to RGB and the G channel to alpha, then write a basis file with alpha channels
    //bool_param<false> m_seperate_rg_to_color_alpha;

    // m_tex_type, m_userdata0, m_userdata1, m_framerate - These fields go directly into the Basis file header.
    // FIXME: Not sure we need to set these for encoding.
    if (This->isCubemap)
        cparams.m_tex_type = cBASISTexTypeCubemapArray;
    else if (This->isArray && This->baseHeight > 1)
         cparams.m_tex_type = cBASISTexType2DArray;
    else if (This->baseDepth > 1)
         cparams.m_tex_type = cBASISTexTypeVolume;
    else if (This->baseHeight > 1)
         cparams.m_tex_type = cBASISTexType2D;
    else
        return KTX_INVALID_OPERATION;

    // TODO When video support is added set m_tex_type to this if video.
    //cBASISTexTypeVideoFrames
    // and set cparams.m_us_per_frame;

    basis_compressor c;

    // init() only returns false if told to read source image files and the
    // list of files is empty.
    (void)c.init(cparams);
    enable_debug_printf(true);
    basis_compressor::error_code ec = c.process();

    if (ec != basis_compressor::cECSuccess) {
        // We should be sending valid 2d arrays, cubemaps or video ...
        assert(ec != basis_compressor::cECFailedValidating);
        // Do something sensible with other errors
        return KTX_INVALID_OPERATION;
#if 0
        switch (ec) {
                case basis_compressor::cECFailedReadingSourceImages:
                {
                    error_printf("Compressor failed reading a source image!\n");

                    if (opts.m_individual)
                        exit_flag = false;

                    break;
                }
                case basis_compressor::cECFailedFrontEnd:
                    error_printf("Compressor frontend stage failed!\n");
                    break;
                case basis_compressor::cECFailedFontendExtract:
                    error_printf("Compressor frontend data extraction failed!\n");
                    break;
                case basis_compressor::cECFailedBackend:
                    error_printf("Compressor backend stage failed!\n");
                    break;
                case basis_compressor::cECFailedCreateBasisFile:
                    error_printf("Compressor failed creating Basis file data!\n");
                    break;
                case basis_compressor::cECFailedWritingOutput:
                    error_printf("Compressor failed writing to output Basis file!\n");
                    break;
                default:
                    error_printf("basis_compress::process() failed!\n");
                    break;
            }
        }
        return KTX_WHAT_ERROR?;
#endif
    }

    //
    // Compression successful. Now we have to unpick the basis output and
    // copy the info and images to This texture.
    //

    const uint8_vec& bf = c.get_output_basis_file();
    const basis_file_header& bfh = *reinterpret_cast<const basis_file_header*>(bf.data());
    uint8_t* bgd;
    uint32_t bgd_size;
    uint32_t slice_desc_size;

    assert(bfh.m_total_images == num_images);

    //
    // Allocate supercompression global data and write its header.
    //
    //if (bfh.m_flags & cBASISHeaderFlagHasAlphaSlices
        slice_desc_size = sizeof(ktxBasisSliceDesc);
    //else
    //    slice_desc_size = sizeof(ktxBasisGlobalBaseSliceDesc);

    bgd_size = sizeof(ktxBasisGlobalHeader)
             + slice_desc_size * bfh.m_total_slices
             + bfh.m_endpoint_cb_file_size + bfh.m_selector_cb_file_size
             + bfh.m_tables_file_size;
    bgd = new ktx_uint8_t[bgd_size];
    ktxBasisGlobalHeader& bgdh = *reinterpret_cast<ktxBasisGlobalHeader*>(bgd);
    bgdh.globalFlags = bfh.m_flags;
    bgdh.endpointCount = bfh.m_total_endpoints;
    bgdh.endpointsByteLength = bfh.m_endpoint_cb_file_size;
    bgdh.selectorCount = bfh.m_total_selectors;
    bgdh.selectorsByteLength = bfh.m_selector_cb_file_size;
    bgdh.tablesByteLength = bfh.m_tables_file_size;

    //
    // Write the index of slice descriptions to the global data.
    //

    ktxTexture2_private& priv = *This->_private;
    uint32_t base_offset = bfh.m_slice_desc_file_ofs;
    const basis_slice_desc* slice
                = reinterpret_cast<const basis_slice_desc*>(&bf[base_offset]);
    ktxBasisSliceDesc* kslice = BGD_SLICE_DESCS(bgd);

    // 3 things to remember about offsets:
    //    1. levelIndex offsets at this point are relative to This->pData;
    //    2. In the ktx slice descriptors, offsets are relative to the start
    //       of the mip level;
    //    3. basis_slice_desc offsets are relative to the end of the basis
    //       header. Hence base_offset set above is used to rebase offsets
    //       relative to the start of the slice data.

    // Assumption here is that slices produced by the compressor are in the
    // same order as we passed them in above, i.e. ordered by mip level.
    // Note also that slice->m_level_index is always 0 unless the compressor
    // generated mip levels so essentially useless. Alpha slices are always
    // the odd numbered slices.
    std::vector<uint32_t> level_file_offsets(This->numLevels);
    uint32_t image_data_size = 0;
    for (uint32_t level = 0; level < This->numLevels; level++) {
        uint32_t depth = MAX(1, This->baseDepth >> level);
        uint64_t level_byte_length = 0;

        assert(!(slice->m_flags & cSliceDescFlagsIsAlphaData));

        level_file_offsets[level] = slice->m_file_ofs;
        for (uint32_t layer = 0; layer < This->numLayers; layer++) {
            uint32_t faceSlices = This->numFaces == 1 ? depth : This->numFaces;
            for (uint32_t faceSlice = 0; faceSlice < faceSlices; faceSlice++) {
                level_byte_length += slice->m_file_size;
                kslice->sliceByteOffset = slice->m_file_ofs
                                        - level_file_offsets[level];
                kslice->sliceByteLength = slice->m_file_size;
                kslice->sliceFlags = slice->m_flags;
                if (bfh.m_flags & cBASISHeaderFlagHasAlphaSlices) {
                    slice++;
                    level_byte_length += slice->m_file_size;
                    kslice->alphaSliceByteOffset = slice->m_file_ofs
                                                 - level_file_offsets[level];
                    kslice->alphaSliceByteLength = slice->m_file_size;
                } else {
                    kslice->alphaSliceByteOffset = 0;
                    kslice->alphaSliceByteLength = 0;
                }
                slice++;
                kslice++;
            }
        }
        priv._levelIndex[level].byteLength = level_byte_length;
        priv._levelIndex[level].uncompressedByteLength = 0;
        image_data_size += level_byte_length;
    }

    //
    // Copy the global code books & huffman tables to global data.
    //

    // Slightly sleazy but since kslice is now pointing at the first byte after
    // the last entry in the slice description index ...
    uint8_t* dstptr = reinterpret_cast<uint8_t*>(kslice);
    // Copy the endpoints ...
    bgdh.endpointsByteOffset = static_cast<uint32_t>(dstptr - bgd);
    memcpy(dstptr,
           &bf[bfh.m_endpoint_cb_file_ofs],
           bfh.m_endpoint_cb_file_size);
    dstptr += bgdh.endpointsByteLength;
    // selectors ...
    bgdh.selectorsByteOffset = static_cast<uint32_t>(dstptr - bgd);
    memcpy(dstptr,
           &bf[bfh.m_selector_cb_file_ofs],
           bfh.m_selector_cb_file_size);
    dstptr += bgdh.selectorsByteLength;
    // and the huffman tables.
    bgdh.tablesByteOffset = static_cast<uint32_t>(dstptr - bgd);
    memcpy(dstptr,
           &bf[bfh.m_tables_file_ofs],
           bfh.m_tables_file_size);

    assert((dstptr + bgdh.tablesByteLength - bgd) <= bgd_size);

    //
    // We have a complete global data package and compressed images.
    // Update This texture and copy compressed image data to it.
    //
    result = ktxTexture2_rewriteDfd(This);
    if (result != KTX_SUCCESS) {
        delete bgd;
        return result;
    }

    uint8_t* new_data = new uint8_t[image_data_size];
    if (!new_data)
        return KTX_OUT_OF_MEMORY;

    This->vkFormat = VK_FORMAT_UNDEFINED;
    // FIXME. Need to fix up _protected->_formatSize. How?
    This->supercompressionScheme = KTX_SUPERCOMPRESSION_BASIS;
    priv._supercompressionGlobalData = bgd;
    delete(This->pData); // FIXME: Move this to right after the images have been
                         // copied to the Basis compressor once things are working.
    This->pData = new_data;
    This->dataSize = image_data_size;

    // Copy in the compressed image data.
    // NOTA BENE: Mipmap levels are ordered from largest to smallest in .basis.
    // We have to reorder.

    uint32_t level_offset = 0;
    for (int32_t level = This->numLevels - 1; level >= 0; level--) {
        priv._levelIndex[level].byteOffset = level_offset;
        // byteLength was set in loop above
        memcpy(This->pData + level_offset,
               &bf[level_file_offsets[level]],
               priv._levelIndex[level].byteLength);
        level_offset += priv._levelIndex[level].byteLength;
    }
#ifdef DEBUG
    slice = reinterpret_cast<const basis_slice_desc*>(&bf[base_offset]);
    for (uint32_t level = 0; level < This->numLevels; level++) {
        const uint8_t* src = &bf[level_file_offsets[level]];
        uint8_t* dst = This->pData + priv._levelIndex[level].byteOffset;

        assert(slice->m_file_ofs == level_file_offsets[level]);
        assert(slice->m_image_index == level);
        assert(memcmp((void*)src, dst, priv._levelIndex[level].byteLength) == 0);
        // FIXME: This is not correct for any case with more than 1 image in a
        // level, e.g., an array texture or cubemap.
        if (bfh.m_flags & cBASISHeaderFlagHasAlphaSlices)
            slice += 2;
        else
            slice += 1;
    }
#endif

    return KTX_SUCCESS;
}

#if 0
    bool basisu_transcoder::transcode_slice(const void* pData, uint32_t data_size, uint32_t slice_index, void* pOutput_blocks, uint32_t output_blocks_buf_size_in_blocks, block_format fmt,
        uint32_t output_block_stride_in_bytes, uint32_t decode_flags, uint32_t output_row_pitch_in_blocks, basisu_transcoder_state* pState) const
    {
        if (!m_lowlevel_decoder.m_endpoints.size())
        {
            BASISU_DEVEL_ERROR("basisu_transcoder::transcode_slice: must call start_transcoding first\n");
            return false;
        }

        if (decode_flags & cDecodeFlagsPVRTCDecodeToNextPow2)
        {
            // TODO: Not yet supported
            BASISU_DEVEL_ERROR("basisu_transcoder::transcode_slice: cDecodeFlagsPVRTCDecodeToNextPow2 currently unsupported\n");
            return false;
        }

        if (!validate_header_quick(pData, data_size))
        {
            BASISU_DEVEL_ERROR("basisu_transcoder::transcode_slice: header validation failed\n");
            return false;
        }

        const basis_file_header* pHeader = reinterpret_cast<const basis_file_header*>(pData);

        const uint8_t* pDataU8 = static_cast<const uint8_t*>(pData);

        if (slice_index >= pHeader->m_total_slices)
        {
            BASISU_DEVEL_ERROR("basisu_transcoder::transcode_slice: slice_index >= pHeader->m_total_slices\n");
            return false;
        }

        const basis_slice_desc& slice_desc = reinterpret_cast<const basis_slice_desc*>(pDataU8 + pHeader->m_slice_desc_file_ofs)[slice_index];

        uint32_t total_blocks = slice_desc.m_num_blocks_x * slice_desc.m_num_blocks_y;
        if (output_blocks_buf_size_in_blocks < total_blocks)
        {
            BASISU_DEVEL_ERROR("basisu_transcoder::transcode_slice: output_blocks_buf_size_in_blocks < total_blocks\n");
            return false;
        }

        if (fmt != cETC1)
        {
            if (fmt == cPVRTC1_4_OPAQUE_ONLY)
            {
                if ((!basisu::is_pow2(slice_desc.m_num_blocks_x * 4)) || (!basisu::is_pow2(slice_desc.m_num_blocks_y * 4)))
                {
                    // PVRTC1 only supports power of 2 dimensions
                    BASISU_DEVEL_ERROR("basisu_transcoder::transcode_slice: PVRTC1 only supports power of 2 dimensions\n");
                    return false;
                }
            }
        }

        if (slice_desc.m_file_ofs > data_size)
        {
            BASISU_DEVEL_ERROR("basisu_transcoder::transcode_slice: invalid slice_desc.m_file_ofs, or passed in buffer too small\n");
            return false;
        }

        const uint32_t data_size_left = data_size - slice_desc.m_file_ofs;
        if (data_size_left < slice_desc.m_file_size)
        {
            BASISU_DEVEL_ERROR("basisu_transcoder::transcode_slice: invalid slice_desc.m_file_size, or passed in buffer too small\n");
            return false;
        }

        return m_lowlevel_decoder.transcode_slice(pOutput_blocks, slice_desc.m_num_blocks_x, slice_desc.m_num_blocks_y,
            pDataU8 + slice_desc.m_file_ofs, slice_desc.m_file_size,
            fmt, output_block_stride_in_bytes, (decode_flags & cDecodeFlagsPVRTCWrapAddressing) != 0, (decode_flags & cDecodeFlagsBC1ForbidThreeColorBlocks) == 0, *pHeader, slice_desc, output_row_pitch_in_blocks, pState);
    }
#endif


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

// Basis decode flags
#if 0
        enum
        {
            // PVRTC1: texture will use wrap addressing vs. clamp (most PVRTC viewer tools assume wrap addressing, so we default to wrap although that can cause edge artifacts)
            cDecodeFlagsPVRTCWrapAddressing = 1,

            // PVRTC1: decode non-pow2 ETC1S texture level to the next larger power of 2 (not implemented yet, but we're going to support it). Ignored if the slice's dimensions are already a power of 2.
            cDecodeFlagsPVRTCDecodeToNextPow2 = 2,

            // When decoding to an opaque texture format, if the basis file has alpha, decode the alpha slice instead of the color slice to the output texture format
            cDecodeFlagsTranscodeAlphaDataToOpaqueFormats = 4,

            // Forbid usage of BC1 3 color blocks (we don't support BC1 punchthrough alpha yet).
            cDecodeFlagsBC1ForbidThreeColorBlocks = 8
        };
#endif
// Don't want to support cDecodeFlagsTranscodeAlphaDataToOpaqueFormats because
// should only be needed by OpenGL ES 1-only devices.
KTX_error_code
ktxTexture2_TranscodeBasis(ktxTexture2* This, ktx_texture_fmt_e outputFormat,
                           ktx_uint32_t decodeFlags)
{
    if (This->supercompressionScheme != KTX_SUPERCOMPRESSION_BASIS)
        return KTX_INVALID_OPERATION;

    if (!This->_private->_supercompressionGlobalData)
        return KTX_INVALID_OPERATION;

    if (decodeFlags & KTX_DF_PVRTC_DECODE_TO_NEXT_POW2) {
        debug_printf("ktxTexture_TranscodeBasis: KTX_DF_PVRTC_DECODE_TO_NEXT_POW2 currently unsupported\n");
        return KTX_UNSUPPORTED_FEATURE;
    }

    uint8_t* bgd = This->_private->_supercompressionGlobalData;
    ktxBasisGlobalHeader& bgdh = *reinterpret_cast<ktxBasisGlobalHeader*>(bgd);
    if (!(bgdh.endpointsByteLength && bgdh.selectorsByteLength && bgdh.tablesByteLength)) {
            debug_printf("ktxTexture_TranscodeBasis: missing endpoints, selectors or tables");
            return KTX_FILE_DATA_ERROR;
    }

    static basist::etc1_global_selector_codebook *global_codebook
        = new basist::etc1_global_selector_codebook(g_global_selector_cb_size,
                                                    g_global_selector_cb);
    basisu_lowlevel_transcoder llt(global_codebook);

    llt.decode_palettes(bgdh.endpointCount, bgd + bgdh.endpointsByteOffset,
                        bgdh.endpointsByteLength,
                        bgdh.selectorCount, bgd + bgdh.selectorsByteOffset,
                        bgdh.selectorsByteLength);

    llt.decode_tables(bgd + bgdh.tablesByteOffset, bgdh.tablesByteLength);

    // basisu::transcode_image_level -> basisu::transcode_slice -> lowlevel_transcoder::transcode_slice
    // transcode_image_level is called by TranscodeImage in basis_wrappers.cpp which is called
    // by the js application.

   // transcode_level does not do what it says on the tin as it only does 1 or 2 slices. It does though
   // handle using the low level decoder to assemble things for the higher-level formats.

#if 0
    bool basisu_transcoder::transcode_image_level(
        const void* pData, uint32_t data_size,
        uint32_t image_index, uint32_t level_index,
        void* pOutput_blocks, uint32_t output_blocks_buf_size_in_blocks,
        transcoder_texture_format fmt,
        uint32_t decode_flags, uint32_t output_row_pitch_in_blocks, basisu_transcoder_state *pState) const
    {
#endif
    if (outputFormat == KTX_TF_PVRTC1_4_OPAQUE_ONLY) {
        if ((!basisu::is_pow2(This->baseWidth)) || (!basisu::is_pow2(This->baseHeight))) {
            debug_printf("ktxTexture_TranscodeBasis: PVRTC1 only supports power of 2 dimensions\n");
            return KTX_INVALID_OPERATION;
       }
    }

    const bool hasAlpha = (bgdh.globalFlags & cBASISHeaderFlagHasAlphaSlices) != 0;
    const bool transcodeAlphaToOpaqueFormats
     = (hasAlpha && (decodeFlags & KTX_DF_TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS));
    const bool isVideo = false; // FIXME when video is supported.

    uint32_t* BDB = This->pDfd + 1;
    const bool srgb = (KHR_DFDVAL(BDB, TRANSFER) == KHR_DF_TRANSFER_SRGB);

    // Set vkFormat so we can get the formatSize;
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
        vkFormat = srgb ? VK_FORMAT_BC3_SRGB_BLOCK
                        : VK_FORMAT_BC3_UNORM_BLOCK;
        break;
      case KTX_TF_BC4:
        // How is sRGB handled?
        vkFormat = VK_FORMAT_BC4_UNORM_BLOCK;
        break;
      case KTX_TF_BC5:
        // How is sRGB handled?
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
    This->vkFormat = vkFormat;
    vkGetFormatSize(vkFormat, &This->_protected->_formatSize);
    This->isCompressed = true;
    ktx_size_t transcodedDataSize = ktxTexture_calcDataSizeTexture(
                                            ktxTexture(This),
                                            KTX_FORMAT_VERSION_TWO);
    //ktx_size_t transcodedBlockSize = transcodedDataSize
    //                                 / This->_protected->_formatSize.blockSizeInBits
    //                                 / 8;

    ktx_uint8_t* basisData = This->pData;
    ktx_size_t basisDataSize = This->dataSize;
    This->pData = new uint8_t[transcodedDataSize];
    This->dataSize = transcodedDataSize;

    // Finally we're ready to transcode the slices.

    uint32_t image = 0;
    ktxLevelIndexEntry* levelIndex = This->_private->_levelIndex;
    uint64_t levelOffsetWrite = 0;
    for (int32_t level = This->numLevels - 1; level >= 0; level--) {
        uint64_t levelOffset = ktxTexture2_levelDataOffset(This, level);
        uint64_t writeOffset = levelOffsetWrite;
        const ktxBasisSliceDesc* sliceDescs = BGD_SLICE_DESCS(bgd);
        uint32_t width = MAX(1, This->baseWidth >> level);
        uint32_t height = MAX(1, This->baseHeight >> level);
        uint32_t depth = MAX(1, This->baseDepth >> level);
        uint32_t faceSlices = This->numFaces == 1 ? depth : This->numFaces;
        uint32_t numImages = This->numLayers * faceSlices;
        uint32_t endImage = image + numImages;

        if (sliceDescs[image].sliceFlags & cSliceDescFlagsIsAlphaData) {
            debug_printf("ktxTexture_TranscodeBasis: out of order alpha slice");
            // First slice of a level should not have alpha.
            return KTX_FILE_DATA_ERROR;
        }

        // 4x4 is the ETC1S block size.
        const uint32_t num_blocks_x = get_block_width(width, 4);
        const uint32_t num_blocks_y = get_block_height(height, 4);

        for (; image < endImage; image++) {
            ktx_uint8_t* writePtr = This->pData + writeOffset;

#if 0
        uint32_t total_slices = 1;
        switch (outputFormat)
        {
          case KTX_TF_ETC2:
          case KTX_TF_BC3:
          case KTX_TF_BC5:
            total_slices = 2;
            break;
          default:
            break;
        }
        (void)total_slices;
#endif

#if 0
        if (hasAlpha)
        {
            // The alpha data should immediately follow the color data, and have the same resolution.
            if ((slice_index + 1U) >= pHeader->m_total_slices)
            {
                BASISU_DEVEL_ERROR("basisu_transcoder::transcode_image_level: alpha basis file has missing alpha slice\n");
                // basis file is missing the alpha slice
                return false;
            }

            // Basic sanity checks
            if ((pSlice_descs[slice_index + 1].m_flags & cSliceDescFlagsIsAlphaData) == 0)
            {
                BASISU_DEVEL_ERROR("basisu_transcoder::transcode_image_level: alpha basis file has missing alpha slice (flag check)\n");
                // This slice should have alpha data
                return false;
            }

            if ((pSlice_descs[slice_index].m_num_blocks_x != pSlice_descs[slice_index + 1].m_num_blocks_x) || (pSlice_descs[slice_index].m_num_blocks_y != pSlice_descs[slice_index + 1].m_num_blocks_y))
            {
                BASISU_DEVEL_ERROR("basisu_transcoder::transcode_image_level: alpha basis file slice dimensions bad\n");
                // Alpha slice should have been the same res as the color slice
                return false;
            }
        }
#endif

        uint32_t bytes_per_block = 8;
        switch (outputFormat) {
          case KTX_TF_BC7_M6_OPAQUE_ONLY:
          case KTX_TF_ETC2:
          case KTX_TF_BC3:
          case KTX_TF_BC5:
            bytes_per_block = 16;
            break;
        default:
            break;
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
        //bool transcode_slice(void *pDst_blocks, uint32_t num_blocks_x, uint32_t num_blocks_y, const uint8_t *pImage_data, uint32_t image_data_size, block_format fmt,
         //   uint32_t output_stride, bool wrap_addressing, bool bc1_allow_threecolor_blocks, const basis_file_header &header, const basis_slice_desc& slice_desc, uint32_t output_row_pitch_in_blocks = 0, //basisu_transcoder_state *pState = nullptr);

            // No need to pass output_row_pitch_in_blocks. It defaults to num_blocks_x. No need to pass transcoder state. It will use default state.
            // the header is only used to check if tex_type is cBASISTexTypeVideoFrames.
            // slice_desc is used only to check the alpha flag and to get the level index and check it against the max levels in the transcoder state, which is 16. However level is always 0 for us.
            // Conclusion, make another version of transcode slice which takes these things as arguments.

            // Pass 0 for level_index. In Basis files, level_index is only
            // ever non-zero when the Basis encoder generated mipmaps, a
            // function we're not currently using. Also this method only uses
            // level_index for video textures.
            status = llt.transcode_slice(writePtr, num_blocks_x, num_blocks_y,
                    basisData + levelOffset + sliceByteOffset, sliceByteLength,
                    basist::cETC1, bytes_per_block,
                    (decodeFlags & KTX_DF_PVRTC_WRAP_ADDRESSING) != 0,
                    (decodeFlags & KTX_DF_BC1_FORBID_THREE_COLOR_BLOCKS) == 0,
                    isVideo, hasAlpha, 0/* level_index*/);
            //status = transcode_slice(pData, data_size, slice_index_to_decode, pOutput_blocks, output_blocks_buf_size_in_blocks, cETC1, bytes_per_block, decode_flags, output_row_pitch_in_blocks, pState);
            if (!status)
            {
                BASISU_DEVEL_ERROR("basisu_transcoder::transcode_image_level: transcode_slice() to ETC1 failed\n");
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

            //status = transcode_slice(pData, data_size, slice_index_to_decode, pOutput_blocks, output_blocks_buf_size_in_blocks, cBC1, bytes_per_block, decode_flags, output_row_pitch_in_blocks, pState);
            if (!status)
            {
                BASISU_DEVEL_ERROR("basisu_transcoder::transcode_image_level: transcode_slice() to BC1 failed\n");
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

            //status = transcode_slice(pData, data_size, slice_index_to_decode, pOutput_blocks, output_blocks_buf_size_in_blocks, cBC4, bytes_per_block, decode_flags, output_row_pitch_in_blocks, pState);
            if (!status)
            {
                BASISU_DEVEL_ERROR("basisu_transcoder::transcode_image_level: transcode_slice() to BC4 failed\n");
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

            //status = transcode_slice(pData, data_size, slice_index_to_decode, pOutput_blocks, output_blocks_buf_size_in_blocks, cPVRTC1_4_OPAQUE_ONLY, bytes_per_block, decode_flags, output_row_pitch_in_blocks, pState);
            if (!status)
            {
                BASISU_DEVEL_ERROR("basisu_transcoder::transcode_image_level: transcode_slice() to PVRTC1 4 opaque only failed\n");
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
            //status = transcode_slice(pData, data_size, slice_index_to_decode, pOutput_blocks, output_blocks_buf_size_in_blocks, cBC7_M6_OPAQUE_ONLY, bytes_per_block, decode_flags, output_row_pitch_in_blocks, pState);
            if (!status)
            {
                BASISU_DEVEL_ERROR("basisu_transcoder::transcode_image_level: transcode_slice() to BC7 m6 opaque only failed\n");
            }
            break;
        }
        case KTX_TF_ETC2:
        {
#if !BASISD_SUPPORT_ETC2_EAC_A8
            return KTX_UNSUPPORTED_FEATURE;
#endif
            if (hasAlpha)
            {
                // First decode the alpha data
                status = llt.transcode_slice(writePtr, num_blocks_x, num_blocks_y,
                        basisData + levelOffset + sliceDescs[image].alphaSliceByteOffset,
                        sliceDescs[image].alphaSliceByteLength,
                        basist::cETC2_EAC_A8, bytes_per_block,
                        (decodeFlags & KTX_DF_PVRTC_WRAP_ADDRESSING) != 0,
                        (decodeFlags & KTX_DF_BC1_FORBID_THREE_COLOR_BLOCKS) == 0,
                        isVideo, hasAlpha, 0/* level_index*/);
                //status = transcode_slice(pData, data_size, slice_index + 1, pOutput_blocks, output_blocks_buf_size_in_blocks, cETC2_EAC_A8, 16, decode_flags, output_row_pitch_in_blocks, pState);
            }
            else
            {
                basisu_transcoder::write_opaque_alpha_blocks(num_blocks_x, num_blocks_y, writePtr,
                             (uint32_t)((This->dataSize - writeOffset) / bytes_per_block),
                             cETC2_EAC_A8, bytes_per_block, 0);
                status = true;
            }

            if (status)
            {
                // Now decode the color data
                status = llt.transcode_slice(writePtr + 8, num_blocks_x, num_blocks_y,
                        basisData + levelOffset + sliceDescs[image].sliceByteOffset,
                        sliceDescs[image].sliceByteLength,
                        basist::cETC1, bytes_per_block,
                        (decodeFlags & KTX_DF_PVRTC_WRAP_ADDRESSING) != 0,
                        (decodeFlags & KTX_DF_BC1_FORBID_THREE_COLOR_BLOCKS) == 0,
                        isVideo, hasAlpha, 0/* level_index*/);
                //status = transcode_slice(pData, data_size, slice_index, (uint8_t*)pOutput_blocks + 8, output_blocks_buf_size_in_blocks, cETC1, 16, decode_flags, output_row_pitch_in_blocks, pState);
                if (!status)
                {
                    BASISU_DEVEL_ERROR("basisu_transcoder::transcode_image_level: transcode_slice() to ETC2 RGB failed\n");
                }
            }
            else
            {
                BASISU_DEVEL_ERROR("basisu_transcoder::transcode_image_level: transcode_slice() to ETC2 A failed\n");
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
#if 0
            if (hasAlpha)
            {
                status = llt.transcode_slice(writePtr, num_blocks_x, num_blocks_y,
                        basisData + levelOffset + sliceDescs[image].alphaSliceByteOffset,
                        sliceDescs[image].alphaSliceByteLength,
                        basist::cBC4, bytes_per_block,
                        (decodeFlags & KTX_DF_PVRTC_WRAP_ADDRESSING) != 0,
                        (decodeFlags & KTX_DF_BC1_FORBID_THREE_COLOR_BLOCKS) == 0,
                        isVideo, hasAlpha, 0/* level_index*/);
                //status = transcode_slice(pData, data_size, slice_index + 1, pOutput_blocks, output_blocks_buf_size_in_blocks, cBC4, 16, decode_flags, output_row_pitch_in_blocks, pState);
            }
            else
            {
                basisu_transcoder::write_opaque_alpha_blocks(num_blocks_x, num_blocks_y, writePtr,
                            (uint32_t)((This->dataSize - writeOffset) / bytes_per_block),
                            basist::cBC4, bytes_per_block, 0);
                status = true;
            }
#endif
            if (status)
            {
                // Now decode the color data. Forbid 3 color blocks, which aren't allowed in BC3.
                status = llt.transcode_slice(writePtr + 8, num_blocks_x, num_blocks_y,
                        basisData + levelOffset + sliceDescs[image].sliceByteOffset,
                        sliceDescs[image].sliceByteLength,
                        basist::cBC1, bytes_per_block,
                        (decodeFlags & KTX_DF_PVRTC_WRAP_ADDRESSING) != 0,
                        0, // Forbid 3 color blocks
                        isVideo, hasAlpha, 0/* level_index*/);
                //status = transcode_slice(pData, data_size, slice_index, (uint8_t*)pOutput_blocks + 8, output_blocks_buf_size_in_blocks, cBC1, 16, decode_flags | cDecodeFlagsBC1ForbidThreeColorBlocks, output_row_pitch_in_blocks, pState);
                if (!status)
                {
                    BASISU_DEVEL_ERROR("basisu_transcoder::transcode_image_level: transcode_slice() to BC3 RGB failed\n");
                }
            }
            else
            {
                BASISU_DEVEL_ERROR("basisu_transcoder::transcode_image_level: transcode_slice() to BC3 A failed\n");
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

            //status = transcode_slice(pData, data_size, slice_index, pOutput_blocks, output_blocks_buf_size_in_blocks, cBC4, 16, decode_flags, output_row_pitch_in_blocks, pState);
            if (status)
            {
                if (hasAlpha)
                {
                    // Decode the G data (actually the green channel of the alpha data slice in the basis file)
                    status = llt.transcode_slice(writePtr + 8, num_blocks_x, num_blocks_y,
                            basisData + levelOffset + sliceDescs[image].alphaSliceByteOffset,
                            sliceDescs[image].alphaSliceByteLength,
                            basist::cBC4, bytes_per_block,
                            (decodeFlags & KTX_DF_PVRTC_WRAP_ADDRESSING) != 0,
                            (decodeFlags & KTX_DF_BC1_FORBID_THREE_COLOR_BLOCKS) == 0,
                            isVideo, hasAlpha, 0/* level_index*/);
                    //status = transcode_slice(pData, data_size, slice_index + 1, (uint8_t*)pOutput_blocks + 8, output_blocks_buf_size_in_blocks, cBC4, 16, decode_flags, output_row_pitch_in_blocks, pState);
                    if (!status)
                    {
                        BASISU_DEVEL_ERROR("basisu_transcoder::transcode_image_level: transcode_slice() to BC5 1 failed\n");
                    }
                }
                else
                {
                    basisu_transcoder::write_opaque_alpha_blocks(num_blocks_x, num_blocks_y, writePtr + 8,
                            (uint32_t)((This->dataSize - writeOffset - 8) / bytes_per_block),
                            basist::cBC4, bytes_per_block, 0);
                    //write_opaque_alpha_blocks(pSlice_descs[slice_index].m_num_blocks_x, pSlice_descs[slice_index].m_num_blocks_y, (uint8_t*)pOutput_blocks + 8, output_blocks_buf_size_in_blocks, cBC4, 16, output_row_pitch_in_blocks);
                    status = true;
                }
            }
            else
            {
                BASISU_DEVEL_ERROR("basisu_transcoder::transcode_image_level: transcode_slice() to BC5 channel 0 failed\n");
            }
            break;
        }
        default:
        {
            assert(0);
            BASISU_DEVEL_ERROR("basisu_transcoder::transcode_image_level: Invalid fmt\n");
            break;
        }
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
    // XXX Don't forget to change _private->_firstLevelFileOffset;

    // FIXME fixup rest of ktxTexture including the DFD.
    return KTX_SUCCESS;
}
