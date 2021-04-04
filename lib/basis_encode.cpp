/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2019-2020 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file basis_encode.cpp
 * @~English
 *
 * @brief Functions for supercompressing a texture with Basis Universal.
 *
 * This is where two worlds collide. Ugly!
 *
 * @author Mark Callow, www.edgewise-consulting.com
 */

#include <inttypes.h>
#include <zstd.h>
#include <KHR/khr_df.h>

#include "ktx.h"
#include "ktxint.h"
#include "texture2.h"
#include "vkformat_enum.h"
#include "vk_format.h"
#include "basis_sgd.h"
#include "basisu/encoder/basisu_comp.h"
#include "basisu/transcoder/basisu_file_headers.h"
#include "basisu/transcoder/basisu_transcoder.h"
#include "dfdutils/dfd.h"

using namespace basisu;
using namespace basist;

typedef struct ktxBasisParamsV1 {
    ktx_uint32_t structSize;
    ktx_uint32_t threadCount;
    ktx_uint32_t compressionLevel;
    ktx_uint32_t qualityLevel;
    ktx_uint32_t maxEndpoints;
    float endpointRDOThreshold;
    ktx_uint32_t maxSelectors;
    float selectorRDOThreshold;
    ktx_bool_t normalMap;
    ktx_bool_t separateRGToRGB_A;
    ktx_bool_t preSwizzle;
    ktx_bool_t noEndpointRDO;
    ktx_bool_t noSelectorRDO;
} ktxBasisParamsV1;

enum swizzle_e {
    R = 1,
    G = 2,
    B = 3,
    A = 4,
    ZERO = 5,
    ONE = 6,
};

typedef void
(* PFNBUCOPYCB)(uint8_t* rgbadst, uint8_t* rgbasrc, uint32_t src_len,
                ktx_size_t image_size, swizzle_e swizzle[4]);

// All callbacks expect source images to have no row padding and expect
// component size to be 8 bits.

static void
copy_rgba_to_rgba(uint8_t* rgbadst, uint8_t* rgbasrc, uint32_t src_len,
                  ktx_size_t image_size, swizzle_e swizzle[4])
{
    memcpy(rgbadst, rgbasrc, image_size);
}

// Copy rgb to rgba. No swizzle.
static void
copy_rgb_to_rgba(uint8_t* rgbadst, uint8_t* rgbsrc, uint32_t src_len,
                 ktx_size_t image_size, swizzle_e swizzle[4])
{
    for (ktx_size_t i = 0; i < image_size; i += 3) {
        memcpy(rgbadst, rgbsrc, 3);
        rgbadst[3] = 0xff; // Convince Basis there is no alpha.
        rgbadst += 4; rgbsrc += 3;
    }
}

// This is not static only so the unit tests can access it.
void
swizzle_to_rgba(uint8_t* rgbadst, uint8_t* rgbasrc, uint32_t src_len,
                ktx_size_t image_size, swizzle_e swizzle[4])
{
    for (ktx_size_t i = 0; i < image_size; i += src_len) {
        for (uint32_t c = 0; c < 4; c++) {
            switch (swizzle[c]) {
              case R:
                rgbadst[c] = rgbasrc[0];
                break;
              case G:
                rgbadst[c] = rgbasrc[1];
                break;
              case B:
                rgbadst[c] = rgbasrc[2];
                break;
              case A:
                rgbadst[c] = rgbasrc[3];
                break;
              case ZERO:
                rgbadst[c] = 0x00;
                break;
              case ONE:
                rgbadst[c] = 0xff;
                break;
              default:
                assert(false);
            }
        }
        rgbadst +=4; rgbasrc += src_len;
    }
}

#if 0
static void
swizzle_rgba_to_rgba(uint8_t* rgbadst, uint8_t* rgbasrc, ktx_size_t image_size,
                     swizzle_e swizzle[4])
{
    for (ktx_size_t i = 0; i < image_size; i += 4) {
        for (uint32_t c = 0; c < 4; c++) {
            switch (swizzle[c]) {
              case 0:
                rgbadst[c] = rgbasrc[0];
                break;
              case 1:
                rgbadst[c] = rgbasrc[1];
                break;
              case 2:
                rgbadst[c] = rgbasrc[2];
                break;
              case 3:
                rgbadst[c] = rgbasrc[3];
                break;
              case 4:
                rgbadst[c] = 0x00;
                break;
              case 5:
                rgbadst[i+c] = 0xff;
                break;
              default:
                assert(false);
            }
        }
        rgbadst +=4; rgbasrc += 4;
    }
}

static void
swizzle_rgb_to_rgba(uint8_t* rgbadst, uint8_t* rgbsrc, ktx_size_t image_size,
                     swizzle_e swizzle[4])
{
    for (ktx_size_t i = 0; i < image_size; i += 3) {
        for (uint32_t c = 0; c < 3; c++) {
            switch (swizzle[c]) {
              case 0:
                rgbadst[c] = rgbsrc[0];
                break;
              case 1:
                rgbadst[c] = rgbsrc[i];
                break;
              case 2:
                rgbadst[c] = rgbsrc[2];
                break;
              case 3:
                assert(false); // Shouldn't happen for an RGB texture.
                break;
              case 4:
                rgbadst[c] = 0x00;
                break;
              case 5:
                rgbadst[c] = 0xff;
                break;
              default:
                assert(false);
            }
        }
        rgbadst +=4; rgbsrc += 3;
    }
}

static void
swizzle_rg_to_rgb_a(uint8_t* rgbadst, uint8_t* rgsrc, ktx_size_t image_size,
                    swizzle_e swizzle[4])
{
    for (ktx_size_t i = 0; i < image_size; i += 2) {
        for (uint32_t c = 0; c < 2; c++) {
          switch (swizzle[c]) {
              case 0:
                rgbadst[c] = rgsrc[0];
                break;
              case 1:
                rgbadst[c] = rgsrc[1];
                break;
              case 2:
                 assert(false); // Shouldn't happen for an RG texture.
                 break;
              case 3:
                assert(false); // Shouldn't happen for an RG texture.
                break;
              case 4:
                rgbadst[c] = 0x00;
                break;
              case 5:
                rgbadst[c] = 0xff;
                break;
              default:
                assert(false);
            }
        }
    }
}
#endif

// Rewrite DFD changing it to unsized. Account for the Basis compressor
// not including an all 1's alpha channel, which would have been removed before
// encoding and supercompression, by using hasAlpha.
static KTX_error_code
ktxTexture2_rewriteDfd4BasisLzETC1S(ktxTexture2* This,
                                    alpha_content_e alphaContent,
                                    bool isLuminance)
{
    uint32_t* cdfd = This->pDfd;
    uint32_t* cbdb = cdfd + 1;
    uint32_t newSampleCount = alphaContent != eNone ? 2 : 1;

    uint32_t ndbSize = KHR_DF_WORD_SAMPLESTART
                       + newSampleCount * KHR_DF_WORD_SAMPLEWORDS;
    ndbSize *= sizeof(uint32_t);
    uint32_t ndfdSize = ndbSize + 1 * sizeof(uint32_t);
    uint32_t* ndfd = (uint32_t *)malloc(ndfdSize);
    uint32_t* nbdb = ndfd + 1;

    if (!ndfd)
        return KTX_OUT_OF_MEMORY;

    *ndfd = ndfdSize;
    KHR_DFDSETVAL(nbdb, VENDORID, KHR_DF_VENDORID_KHRONOS);
    KHR_DFDSETVAL(nbdb, DESCRIPTORTYPE, KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT);
    KHR_DFDSETVAL(nbdb, VERSIONNUMBER, KHR_DF_VERSIONNUMBER_LATEST);
    KHR_DFDSETVAL(nbdb, DESCRIPTORBLOCKSIZE, ndbSize);
    KHR_DFDSETVAL(nbdb, MODEL, KHR_DF_MODEL_ETC1S);
    KHR_DFDSETVAL(nbdb, PRIMARIES, KHR_DFDVAL(cbdb, PRIMARIES));
    KHR_DFDSETVAL(nbdb, TRANSFER, KHR_DFDVAL(cbdb, TRANSFER));
    KHR_DFDSETVAL(nbdb, FLAGS, KHR_DFDVAL(cbdb, FLAGS));

    nbdb[KHR_DF_WORD_TEXELBLOCKDIMENSION0] =
                            3 | (3 << KHR_DF_SHIFT_TEXELBLOCKDIMENSION1);
    // Show it describes an unsized format.
    nbdb[KHR_DF_WORD_BYTESPLANE0] = 0; /* bytesPlane3..0 = 0 */
    nbdb[KHR_DF_WORD_BYTESPLANE4] = 0; /* bytesPlane7..5 = 0 */

    for (uint32_t sample = 0; sample < newSampleCount; sample++) {
        uint16_t channelId, bitOffset;
        if (sample == 0) {
            bitOffset = 0;
            if (getDFDNumComponents(cdfd) < 3 && !isLuminance)
                channelId = KHR_DF_CHANNEL_ETC1S_RRR;
            else
                channelId = KHR_DF_CHANNEL_ETC1S_RGB;
        } else {
            assert(sample == 1 && alphaContent != eNone);
            bitOffset = 64;
            if (alphaContent == eAlpha)
                channelId = KHR_DF_CHANNEL_ETC1S_AAA;
            else if (alphaContent == eGreen)
                channelId = KHR_DF_CHANNEL_ETC1S_GGG;
            else // This is just to quiet a compiler warning.
                channelId = KHR_DF_CHANNEL_ETC1S_RGB;
        }
        KHR_DFDSETSVAL(nbdb, sample, CHANNELID, channelId);
        KHR_DFDSETSVAL(nbdb, sample, QUALIFIERS, 0);
        KHR_DFDSETSVAL(nbdb, sample, SAMPLEPOSITION_ALL, 0);
        KHR_DFDSETSVAL(nbdb, sample, BITOFFSET, bitOffset);
        KHR_DFDSETSVAL(nbdb, sample, BITLENGTH, 63);
        KHR_DFDSETSVAL(nbdb, sample, SAMPLELOWER, 0);
        KHR_DFDSETSVAL(nbdb, sample, SAMPLEUPPER, UINT32_MAX);
    }

    This->pDfd = ndfd;
    free(cdfd);
    return KTX_SUCCESS;
}

static KTX_error_code
ktxTexture2_rewriteDfd4Uastc(ktxTexture2* This,
                             alpha_content_e alphaContent)
{
    uint32_t* cdfd = This->pDfd;
    uint32_t* cbdb = cdfd + 1;

    uint32_t ndbSize = KHR_DF_WORD_SAMPLESTART
                       + 1 * KHR_DF_WORD_SAMPLEWORDS;
    ndbSize *= sizeof(uint32_t);
    uint32_t ndfdSize = ndbSize + 1 * sizeof(uint32_t);
    uint32_t* ndfd = (uint32_t *)malloc(ndfdSize);
    uint32_t* nbdb = ndfd + 1;

    if (!ndfd)
        return KTX_OUT_OF_MEMORY;

    *ndfd = ndfdSize;
    KHR_DFDSETVAL(nbdb, VENDORID, KHR_DF_VENDORID_KHRONOS);
    KHR_DFDSETVAL(nbdb, DESCRIPTORTYPE, KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT);
    KHR_DFDSETVAL(nbdb, VERSIONNUMBER, KHR_DF_VERSIONNUMBER_LATEST);
    KHR_DFDSETVAL(nbdb, DESCRIPTORBLOCKSIZE, ndbSize);
    KHR_DFDSETVAL(nbdb, MODEL, KHR_DF_MODEL_UASTC);
    KHR_DFDSETVAL(nbdb, PRIMARIES, KHR_DFDVAL(cbdb, PRIMARIES));
    KHR_DFDSETVAL(nbdb, TRANSFER, KHR_DFDVAL(cbdb, TRANSFER));
    KHR_DFDSETVAL(nbdb, FLAGS, KHR_DFDVAL(cbdb, FLAGS));

    nbdb[KHR_DF_WORD_TEXELBLOCKDIMENSION0] =
                            3 | (3 << KHR_DF_SHIFT_TEXELBLOCKDIMENSION1);
    nbdb[KHR_DF_WORD_BYTESPLANE0] = 16; /* bytesPlane0 = 16, bytesPlane3..1 = 0 */
    nbdb[KHR_DF_WORD_BYTESPLANE4] = 0; /* bytesPlane7..5 = 0 */

    // Set the data for our single sample
    uint16_t channelId;
    if (alphaContent == eAlpha) {
        channelId = KHR_DF_CHANNEL_UASTC_RGBA;
    } else if (alphaContent == eGreen) {
        channelId = KHR_DF_CHANNEL_UASTC_RG;
    } else if (getDFDNumComponents(cdfd) == 1) {
        channelId = KHR_DF_CHANNEL_UASTC_RRR;
    } else {
        channelId = KHR_DF_CHANNEL_UASTC_RGB;
    }
    KHR_DFDSETSVAL(nbdb, 0, CHANNELID, channelId);
    KHR_DFDSETSVAL(nbdb, 0, QUALIFIERS, 0);
    KHR_DFDSETSVAL(nbdb, 0, SAMPLEPOSITION_ALL, 0);
    KHR_DFDSETSVAL(nbdb, 0, BITOFFSET, 0);
    KHR_DFDSETSVAL(nbdb, 0, BITLENGTH, 127);
    KHR_DFDSETSVAL(nbdb, 0, SAMPLELOWER, 0);
    KHR_DFDSETSVAL(nbdb, 0, SAMPLEUPPER, UINT32_MAX);

    This->pDfd = ndfd;
    free(cdfd);
    return KTX_SUCCESS;
}

static bool basisuEncoderInitialized = false;

/**
 * @memberof ktxTexture2
 * @ingroup writer
 * @~English
 * @brief Encode and possibly Supercompress a KTX2 texture with uncompressed images.
 *
 * The images are either encoded to ETC1S block-compressed format and supercompressed
 * with Basis LZ or they are encoded to UASTC block-compressed format.  UASTC format is
 * selected by setting the @c uastc field of @a params to @c KTX_TRUE. The encoded images
 * replace the original images and the texture's fields including the DFD are modified to reflect the new
 * state.
 *
 * Such textures must be transcoded to a desired target block compressed format
 * before they can be uploaded to a GPU via a graphics API.
 *
 * @sa ktxTexture2_TranscodeBasis().
 *
 * @param[in]   This   pointer to the ktxTexture2 object of interest.
 * @param[in]   params pointer to Basis params object.
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
 *                              @c separateRGToRGB_A is specified but the texture
 *                              is only 1D.
 * @exception KTX_INVALID_OPERATION
 *                              The texture's images are 1D. Only 2D images can
 *                              be supercompressed.
 * @exception KTX_INVALID_OPERATION
 *                              Both preSwizzle and and inputSwizzle are specified
 *                              in @a params.
 * @exception KTX_OUT_OF_MEMORY Not enough memory to carry out supercompression.
 */
extern "C" KTX_error_code
ktxTexture2_CompressBasisEx(ktxTexture2* This, ktxBasisParams* params)
{
    KTX_error_code result;

    if (!params)
        return KTX_INVALID_VALUE;

    if (params->structSize != sizeof(struct ktxBasisParams))
        return KTX_INVALID_VALUE;

    if (This->supercompressionScheme != KTX_SS_NONE)
        return KTX_INVALID_OPERATION; // Can't apply multiple schemes.

    if (This->isCompressed)
        return KTX_INVALID_OPERATION;  // Only non-block compressed formats
                                       // can be encoded into a Basis format.

    if (This->_protected->_formatSize.flags & KTX_FORMAT_SIZE_PACKED_BIT)
        return KTX_INVALID_OPERATION;

    // Basic descriptor block begins after the total size field.
    const uint32_t* BDB = This->pDfd+1;

    uint32_t num_components, component_size;
    getDFDComponentInfoUnpacked(This->pDfd, &num_components, &component_size);

    if (component_size != 1)
        return KTX_INVALID_OPERATION; // Basis must have 8-bit components.

    if (params->separateRGToRGB_A && num_components == 1)
        return KTX_INVALID_OPERATION;

    if (This->pData == NULL) {
        result = ktxTexture2_LoadImageData(This, NULL, 0);
        if (result != KTX_SUCCESS)
            return result;
    }

    if (!basisuEncoderInitialized) {
        basisu_encoder_init();
        basisuEncoderInitialized = true;
    }

    basis_compressor_params cparams;
    cparams.m_read_source_images = false; // Don't read from source files.
    cparams.m_write_output_basis_files = false; // Don't write output files.
    cparams.m_status_output = params->verbose;

    //
    // Calculate number of images
    //
    uint32_t layersFaces = This->numLayers * This->numFaces;
    uint32_t num_images = 0;
    for (uint32_t level = 1; level <= This->numLevels; level++) {
        // NOTA BENE: numFaces * depth is only reasonable because they can't
        // both be > 1. I.e there are no 3d cubemaps.
        num_images += layersFaces * MAX(This->baseDepth >> (level - 1), 1);
    }

    //
    // Copy images into compressor parameters.
    //
    // Darn it! m_source_images is a vector of an internal image class which
    // has its own array of RGBA-only pixels. Pending modifications to the
    // basisu code we'll have to copy in the images.
    cparams.m_source_images.resize(num_images);
    basisu::vector<image>::iterator iit = cparams.m_source_images.begin();

    swizzle_e meta_mapping[4] = {};
    // Since we have to copy the data into the vector image anyway do the
    // separation here to avoid another loop over the image inside
    // basis_compressor.
    swizzle_e rg_to_rgba_mapping_etc1s[4] = { R, R, R, G };
    swizzle_e rg_to_rgba_mapping_uastc[4] = { R, G, ZERO, ONE };
    swizzle_e r_to_rgba_mapping[4] = { R, R, R, ONE };
    swizzle_e* comp_mapping = 0;

    alpha_content_e alphaContent = eNone;
    bool isLuminance = false;
    if (num_components == 1) {
        comp_mapping = r_to_rgba_mapping;
    } else if (num_components == 2) {
        if (params->uastc)
            comp_mapping = rg_to_rgba_mapping_uastc;
        else {
            comp_mapping = rg_to_rgba_mapping_etc1s;
            alphaContent = eGreen;
        }
    } else if (num_components == 4) {
        alphaContent = eAlpha;
    }

    std::string swizzleString = params->inputSwizzle;
    if (params->preSwizzle) {
        if (swizzleString.size() > 0) {
            return KTX_INVALID_OPERATION;
        }

        ktxHashListEntry* swizzleEntry;
        result = ktxHashList_FindEntry(&This->kvDataHead, KTX_SWIZZLE_KEY,
                                       &swizzleEntry);
        if (result == KTX_SUCCESS) {
            ktx_uint32_t swizzleLen = 0;
            char* swizzleStr = nullptr;

            ktxHashListEntry_GetValue(swizzleEntry,
                                      &swizzleLen, (void**)&swizzleStr);
            // Remove the swizzle as it is no longer needed.
            ktxHashList_DeleteEntry(&This->kvDataHead, swizzleEntry);
            // Do it this way in case there is no NUL terminator.
            swizzleString.resize(swizzleLen);
            swizzleString.assign(swizzleStr, swizzleLen);
        }
    }

    if (swizzleString.size() > 0) {
        // Only set comp_mapping for cases we can't shortcut.
        // If num_components < 3 we always swizzle so no shortcut there.
        if (num_components < 3
          || (num_components == 3 && swizzleString.compare("rgb1"))
          || (num_components == 4 && swizzleString.compare("rgba"))) {
            for (int i = 0; i < 4; i++) {
                switch (swizzleString[i]) {
                  case 'r': meta_mapping[i] = R; break;
                  case 'g': meta_mapping[i] = G; break;
                  case 'b': meta_mapping[i] = B; break;
                  case 'a': meta_mapping[i] = A; break;
                  case '0': meta_mapping[i] = ZERO; break;
                  case '1': meta_mapping[i] = ONE; break;
                }
            }
            comp_mapping = meta_mapping;
        }

        // An incoming swizzle of RRR1 or RRRG is assumed to be for a
        // luminance texture. Set isLuminance to distinguish from
        // an identical swizzle generated internally for R & RG cases.
        int i;
        for (i = 0; i < 3; i++) {
            if (meta_mapping[i] != r_to_rgba_mapping[i])
                break;
        }
        if (i == 3) {
            isLuminance = true;
        }
        if (meta_mapping[3] != ONE) {
            alphaContent = eAlpha;
        }
    }

    PFNBUCOPYCB copycb;
    if (comp_mapping) {
        copycb = swizzle_to_rgba;
    } else {
        switch (num_components) {
          case 4: copycb = copy_rgba_to_rgba; break;
          case 3: copycb = copy_rgb_to_rgba; break;
          default: assert(false);
        }
    }

    // NOTA BENE: It is advantageous for Basis LZ compression to order
    // mipmap levels from largest to smallest.
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
                copycb((uint8_t*)iit->get_ptr(), This->pData + offset,
                        num_components, image_size,
                        comp_mapping);
                ++iit;
            }
        }
    }

    free(This->pData); // No longer needed. Reduce memory footprint.
    This->pData = NULL;
    This->dataSize = 0;

    //
    // Setup rest of compressor parameters
    //

    // Why's there no default for this? I have no idea.
    basist::etc1_global_selector_codebook sel_codebook(basist::g_global_selector_cb_size,
                                                       basist::g_global_selector_cb);

    ktx_uint32_t threadCount = params->threadCount;
    if (threadCount < 1)
        threadCount = 1;
    job_pool jpool(threadCount);
    cparams.m_pJob_pool = &jpool;

#if BASISU_SUPPORT_SSE
    bool prevSSESupport = g_cpu_supports_sse41;
    if (params->noSSE)
        g_cpu_supports_sse41 = false;
#endif

    cparams.m_uastc = params->uastc;
    if (params->uastc) {
        cparams.m_pack_uastc_flags = params->uastcFlags;
        if (params->uastcRDO) {
            cparams.m_rdo_uastc = true;
            if (params->uastcRDOQualityScalar > 0.0f) {
                cparams.m_rdo_uastc_quality_scalar =
                                params->uastcRDOQualityScalar;
            }
            if (params->uastcRDODictSize > 0) {
                cparams.m_rdo_uastc_dict_size = params->uastcRDODictSize;
            }
            if (params->uastcRDOMaxSmoothBlockErrorScale > 0) {
              cparams.m_rdo_uastc_max_smooth_block_error_scale =
                               params->uastcRDOMaxSmoothBlockErrorScale;
            }
            if (params->uastcRDOMaxSmoothBlockStdDev > 0) {
                cparams.m_rdo_uastc_smooth_block_max_std_dev =
                                params->uastcRDOMaxSmoothBlockStdDev;
            }
            cparams.m_rdo_uastc_favor_simpler_modes_in_rdo_mode =
                                    !params->uastcRDODontFavorSimplerModes;
            cparams.m_rdo_uastc_favor_simpler_modes_in_rdo_mode =
                                    !params->uastcRDONoMultithreading;
        }
    } else {
        // ETC1S-related params.
        ktx_uint32_t transfer = KHR_DFDVAL(BDB, TRANSFER);
        if (transfer == KHR_DF_TRANSFER_SRGB)
            cparams.m_perceptual = true;
        else
            cparams.m_perceptual = false;

        cparams.m_mip_gen = false; // We provide the mip levels.

        // Explicit specification is required as 0 is a valid value
        // in the basis_compressor leaving us without a good way to
        // indicate the parameter has not been set by the caller. (If we
        // leave m_compression_level unset it will default to 1. We don't
        // want the default to differ from `basisu` so 0 can't be the default.
        cparams.m_compression_level = params->compressionLevel;

        // There's no default for m_quality_level. `basisu` tool overrides
        // any explicit m_{endpoint,selector}_clusters settings with those
        // calculated from m_quality_level, if the user set that option. On
        // the other hand the basis_compressor overrides the values of
        // m_{endpoint,selector}_rdo_thresh calculated from m_quality_level
        // with explicit settings made by the user. Note that, unlike the
        // first pair where both have to be set, each of the second pair
        // independently override the value for it calculated from
        // m_quality_level.
        //
        // This is confusing for the user and tricky to document clearly.
        // Therefore we override qualityLevel if both of max{Endpoint,Selector}s
        // have been set so both sets of parameters are treated the same,
        // except that intentionally we require the caller to have set both
        // of max{Endpoint,Selector}s
        if (params->maxEndpoints && params->maxSelectors) {
            cparams.m_max_endpoint_clusters = params->maxEndpoints;
            cparams.m_max_selector_clusters = params->maxSelectors;
            // cparams.m_quality_level = -1; // Default setting.
        } else if (params->qualityLevel != 0) {
            cparams.m_max_endpoint_clusters = 0;
            cparams.m_max_selector_clusters = 0;
            cparams.m_quality_level = params->qualityLevel;
        } else {
            cparams.m_max_endpoint_clusters = 0;
            cparams.m_max_selector_clusters = 0;
            cparams.m_quality_level = 128;
        }

        if (params->endpointRDOThreshold > 0)
            cparams.m_endpoint_rdo_thresh = params->endpointRDOThreshold;
        if (params->selectorRDOThreshold > 0)
            cparams.m_selector_rdo_thresh = params->selectorRDOThreshold;

        if (params->normalMap) {
            cparams.m_no_endpoint_rdo = true;
            cparams.m_no_selector_rdo = true;
        } else {
            cparams.m_no_endpoint_rdo = params->noEndpointRDO;
            cparams.m_no_selector_rdo = params->noSelectorRDO;
        }

        cparams.m_pSel_codebook = &sel_codebook;
    }

    // Flip images across Y axis
    // cparams.m_y_flip = false; // Let tool, e.g. toktx do its own yflip so
    // ktxTexture is consistent.

    // Output debug information during compression
    //cparams.m_debug = true;

    // m_debug_images is pretty slow
    //cparams.m_debug_images = true;

    // Split the R channel to RGB and the G channel to alpha. We do the
    // seperation in this func (see above) so leave this at its default, false.
    //bool_param<false> m_seperate_rg_to_color_alpha;

    // m_userdata0, m_userdata1 go directly into the .basis file header.
    // No need to set.

    if (This->isVideo) {
        // Encoder uses this to decode whether to create p-frames.
        cparams.m_tex_type = cBASISTexTypeVideoFrames;
        // cparams.m_us_per_frame & m_framerate are not used by
        // the encoder, except to write the values into the .basis file header,
        // so no point in setting them.
    } else {
        // Set to cBASISTexType2D as any other setting is likely to cause
        // validity checks that the encoder performs on its results, to
        // fail. The checks only work properly when the encoder generates
        // mipmaps tself and are oriented to ensuring the .basis file is
        // sensible. Underlying compression works fine and we already know
        // what level, layer and face/slice each image belongs too.
        cparams.m_tex_type = cBASISTexType2D;
    }

#define DUMP_BASIS_FILE 0
#if DUMP_BASIS_FILE
    cparams.m_out_filename = "ktxtest.basis";
    cparams.m_write_output_basis_files = true;
#endif

#define DEBUG_ENCODER 0
#if DEBUG_ENCODER
    cparams.m_debug = true;
    g_debug_printf = true;
#endif

    basis_compressor c;

    // init() only returns false if told to read source image files and the
    // list of files is empty.
    (void)c.init(cparams);
    //enable_debug_printf(true);

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

#if DUMP_BASIS_FILE
    return KTX_UNSUPPORTED_FEATURE;
#endif

    //
    // Compression successful. Now we have to unpick the basis output and
    // copy the info and images to This texture.
    //

#if BASISU_SUPPORT_SSE
    g_cpu_supports_sse41 = prevSSESupport;
#endif

    const uint8_vec& bf = c.get_output_basis_file();
    const basis_file_header& bfh = *reinterpret_cast<const basis_file_header*>(bf.data());

    assert(bfh.m_total_images == num_images);

    uint8_t* bgd = nullptr;
    uint32_t bgd_size;
    uint32_t image_data_size = 0;
    ktxTexture2_private& priv = *This->_private;
    uint32_t base_offset = bfh.m_slice_desc_file_ofs;
    const basis_slice_desc* slice
            = reinterpret_cast<const basis_slice_desc*>(&bf[base_offset]);
    std::vector<uint32_t> level_file_offsets(This->numLevels);

    if (params->uastc) {
        for (uint32_t level = 0; level < This->numLevels; level++) {
            uint32_t depth = MAX(1, This->baseDepth >> level);
            uint32_t levelByteLength = 0;
            uint32_t levelImageCount = This->numLayers * This->numFaces * depth;

            level_file_offsets[level] = slice->m_file_ofs;
            for (uint32_t image = 0; image < levelImageCount; image++, slice++) {
                image_data_size += slice->m_file_size;
                levelByteLength += slice->m_file_size;
            }
            priv._levelIndex[level].byteLength = levelByteLength;
            priv._levelIndex[level].uncompressedByteLength = levelByteLength;
        }
    } else {
        //
        // Allocate supercompression global data and write its header.
        //
        uint32_t image_desc_size = sizeof(ktxBasisLzEtc1sImageDesc);

        bgd_size = sizeof(ktxBasisLzGlobalHeader)
                 + image_desc_size * num_images
                 + bfh.m_endpoint_cb_file_size + bfh.m_selector_cb_file_size
                 + bfh.m_tables_file_size;
        bgd = new ktx_uint8_t[bgd_size];
        ktxBasisLzGlobalHeader& bgdh = *reinterpret_cast<ktxBasisLzGlobalHeader*>(bgd);
        bgdh.endpointCount = bfh.m_total_endpoints;
        bgdh.endpointsByteLength = bfh.m_endpoint_cb_file_size;
        bgdh.selectorCount = bfh.m_total_selectors;
        bgdh.selectorsByteLength = bfh.m_selector_cb_file_size;
        bgdh.tablesByteLength = bfh.m_tables_file_size;
        bgdh.extendedByteLength = 0;

        //
        // Write the index of slice descriptions to the global data.
        //

        ktxBasisLzEtc1sImageDesc* kimages = BGD_ETC1S_IMAGE_DESCS(bgd);

        // 3 things to remember about offsets:
        //    1. levelIndex offsets at this point are relative to This->pData;
        //    2. In the ktx image descriptors, slice offsets are relative to the
        //       start of the mip level;
        //    3. basis_slice_desc offsets are relative to the end of the basis
        //       header. Hence base_offset set above is used to rebase offsets
        //       relative to the start of the slice data.

        // Assumption here is that slices produced by the compressor are in the
        // same order as we passed them in above, i.e. ordered by mip level.
        // Note also that slice->m_level_index is always 0, unless the compressor
        // generated mip levels, so essentially useless. Alpha slices are always
        // the odd numbered slices.
        uint32_t image = 0;
        for (uint32_t level = 0; level < This->numLevels; level++) {
            uint32_t depth = MAX(1, This->baseDepth >> level);
            uint32_t level_byte_length = 0;

            assert(!(slice->m_flags & cSliceDescFlagsHasAlpha));
            level_file_offsets[level] = slice->m_file_ofs;
            for (uint32_t layer = 0; layer < This->numLayers; layer++) {
                uint32_t faceSlices = This->numFaces == 1 ? depth
                                                          : This->numFaces;
                for (uint32_t faceSlice = 0; faceSlice < faceSlices; faceSlice++) {
                    level_byte_length += slice->m_file_size;
                    kimages[image].rgbSliceByteOffset = slice->m_file_ofs
                                                   - level_file_offsets[level];
                    kimages[image].rgbSliceByteLength = slice->m_file_size;
                    if (bfh.m_flags & cBASISHeaderFlagHasAlphaSlices) {
                        slice++;
                        level_byte_length += slice->m_file_size;
                        kimages[image].alphaSliceByteOffset =
                                slice->m_file_ofs - level_file_offsets[level];
                        kimages[image].alphaSliceByteLength =
                                slice->m_file_size;
                    } else {
                        kimages[image].alphaSliceByteOffset = 0;
                        kimages[image].alphaSliceByteLength = 0;
                    }
                    // Set the PFrame flag, inverse of the .basis IFrame flag.
                    if (This->isVideo) {
                        // Extract FrameIsIFrame
                        kimages[image].imageFlags =
                                    (slice->m_flags & ~cSliceDescFlagsHasAlpha);
                        // Set our flag to the inverse.
                        kimages[image].imageFlags ^=
                                    cSliceDescFlagsFrameIsIFrame;
                    } else {
                        kimages[image].imageFlags = 0;
                    }
                    slice++;
                    image++;
                }
            }
            priv._levelIndex[level].byteLength = level_byte_length;
            priv._levelIndex[level].uncompressedByteLength = 0;
            image_data_size += level_byte_length;
        }

        //
        // Copy the global code books & huffman tables to global data.
        //

        // Slightly sleazy but as image is now the last valid index in the
        // slice description array plus 1, &kimages[image] points at the first
        // byte where the endpoints, etc. must be written.
        uint8_t* dstptr = reinterpret_cast<uint8_t*>(&kimages[image]);
        // Copy the endpoints ...
        memcpy(dstptr,
               &bf[bfh.m_endpoint_cb_file_ofs],
               bfh.m_endpoint_cb_file_size);
        dstptr += bgdh.endpointsByteLength;
        // selectors ...
        memcpy(dstptr,
               &bf[bfh.m_selector_cb_file_ofs],
               bfh.m_selector_cb_file_size);
        dstptr += bgdh.selectorsByteLength;
        // and the huffman tables.
        memcpy(dstptr,
               &bf[bfh.m_tables_file_ofs],
               bfh.m_tables_file_size);

        assert((dstptr + bgdh.tablesByteLength - bgd) <= bgd_size);

        //
        // We have a complete global data package.
        //

        priv._supercompressionGlobalData = bgd;
        priv._sgdByteLength = bgd_size;
    }

    //
    // Update This texture and copy compressed image data to it.
    //

    // Declare here so can use goto cleanup.
    uint8_t* new_data = 0;
    ktxFormatSize& formatSize = This->_protected->_formatSize;
    uint64_t level_offset = 0;

    // Since we've left m_check_for_alpha set and m_force_alpha unset in
    // the compressor parameters, the basis encoder will have removed an input
    // alpha channel, if every alpha pixel in every image is 255 prior to
    // encoding and supercompression. The DFD needs to reflect the encoded data
    // not the input texture. Override the alphacontent setting, if this has
    // happened.
    if ((bfh.m_flags & cBASISHeaderFlagHasAlphaSlices) == 0) {
        alphaContent = eNone;
    }

    new_data = (uint8_t*) malloc(image_data_size);
    if (!new_data) {
        result = KTX_OUT_OF_MEMORY;
        goto cleanup;
    }

    // Delayed modifying texture until here so it's after points of
    // possible failure.
    if (params->uastc) {
        result = ktxTexture2_rewriteDfd4Uastc(This, alphaContent);
        if (result != KTX_SUCCESS) goto cleanup;

        // Reflect this in the formatSize
        ktxFormatSize_initFromDfd(&formatSize, This->pDfd);
        // and the requiredLevelAlignment.
        priv._requiredLevelAlignment = 4 * 4;
    } else {
        result = ktxTexture2_rewriteDfd4BasisLzETC1S(This, alphaContent,
                                                     isLuminance);
        if (result != KTX_SUCCESS) goto cleanup;

        This->supercompressionScheme = KTX_SS_BASIS_LZ;
        // Reflect this in the formatSize
        ktxFormatSize_initFromDfd(&formatSize, This->pDfd);
        // and the requiredLevelAlignment.
        priv._requiredLevelAlignment = 1;
    }
    This->vkFormat = VK_FORMAT_UNDEFINED;

    // Since we only allow 8-bit components to be compressed ...
    assert(This->_protected->_typeSize == 1);

    // Copy in the compressed image data.

    This->pData = new_data;

    This->dataSize = image_data_size;

    for (int32_t level = This->numLevels - 1; level >= 0; level--) {
        priv._levelIndex[level].byteOffset = level_offset;
        // byteLength was set in loop above
        memcpy(This->pData + level_offset,
               &bf[level_file_offsets[level]],
               priv._levelIndex[level].byteLength);
        level_offset += _KTX_PADN(priv._requiredLevelAlignment,
                                  priv._levelIndex[level].byteLength);
    }

    return KTX_SUCCESS;

cleanup:
    if (bgd) {
        delete bgd;
        priv._supercompressionGlobalData = 0;
        priv._sgdByteLength = 0;
    }
    if (new_data) delete new_data;
    return result;
}

extern "C" KTX_API const ktx_uint32_t KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL
                                      = BASISU_DEFAULT_COMPRESSION_LEVEL;

/**
 * @memberof ktxTexture2
 * @ingroup writer
 * @~English
 * @brief Supercompress a KTX2 texture with uncompressed images.
 *
 * The images are encoded to ETC1S block-compressed format and supercompressed
 * with Basis Universal. The encoded images replace the original images and the
 * texture's fields including the DFD are modified to reflect the new state.
 *
 * Such textures must be transcoded to a desired target block compressed format
 * before they can be uploaded to a GPU via a graphics API.
 *
 * @sa ktxTexture2_CompressBasisEx().
 *
 * @param[in]   This    pointer to the ktxTexture2 object of interest.
 * @param[in]   quality Compression quality, a value from 1 - 255. Default is
                        128 which is selected if @p quality is 0. Lower=better
                        compression/lower quality/faster. Higher=less
                        compression/higher quality/slower.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_OPERATION
 *                              The texture is already supercompressed.
 * @exception KTX_INVALID_OPERATION
 *                              The texture's image are in a block compressed
 *                              format.
 * @exception KTX_INVALID_OPERATION
 *                              The texture's images are 1D. Only 2D images can
 *                              be supercompressed.
 * @exception KTX_OUT_OF_MEMORY Not enough memory to carry out supercompression.
 */
extern "C" KTX_error_code
ktxTexture2_CompressBasis(ktxTexture2* This, ktx_uint32_t quality)
{
    ktxBasisParams params = {};
    params.structSize = sizeof(params);
    params.threadCount = 1;
    params.compressionLevel = KTX_ETC1S_DEFAULT_COMPRESSION_LEVEL;
    params.qualityLevel = quality;

    return ktxTexture2_CompressBasisEx(This, &params);
}
