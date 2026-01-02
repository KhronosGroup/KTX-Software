/*
 * Copyright (c) 2023, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ktx.h>
#include "ktx_texture2.h"
#include <stddef.h>

ktxTextureMixed PY_ktxTexture2_Create(ktx_uint32_t glInternalformat,
                                      ktx_uint32_t vkFormat,
                                      ktx_uint32_t *pDfd,
                                      ktx_uint32_t baseWidth,
                                      ktx_uint32_t baseHeight,
                                      ktx_uint32_t baseDepth,
                                      ktx_uint32_t numDimensions,
                                      ktx_uint32_t numLevels,
                                      ktx_uint32_t numLayers,
                                      ktx_uint32_t numFaces,
                                      ktx_bool_t isArray,
                                      ktx_bool_t generateMipmaps,
                                      ktxTextureCreateStorageEnum storageAllocation)
{
    ktxTextureCreateInfo createInfo = {
        glInternalformat,
        vkFormat,
        pDfd,
        baseWidth,
        baseHeight,
        baseDepth,
        numDimensions,
        numLevels,
        numLayers,
        numFaces,
        isArray,
        generateMipmaps
    };

    ktxTexture2* newTex = NULL;
    KTX_error_code err = ktxTexture2_Create(&createInfo,
                                            storageAllocation,
                                            &newTex);

    return (ktxTextureMixed) {
        err,
        (ktxTexture*) newTex
    };
}

KTX_error_code PY_ktxTexture2_CompressAstcEx(ktxTexture2 *texture,
                                             ktx_bool_t verbose,
                                             ktx_uint32_t threadCount,
                                             ktx_uint32_t blockDimension,
                                             ktx_uint32_t mode,
                                             ktx_uint32_t qualityLevel,
                                             ktx_bool_t normalMap,
                                             ktx_bool_t perceptual,
                                             char *inputSwizzle)
{
    ktxAstcParams params = {
        .structSize = sizeof(ktxAstcParams),
        .verbose = verbose,
        .threadCount = threadCount,
        .blockDimension = blockDimension,
        .mode = mode,
        .qualityLevel = qualityLevel,
        .normalMap = normalMap,
        .perceptual = perceptual,
    };

    params.inputSwizzle[0] = inputSwizzle[0];
    params.inputSwizzle[1] = inputSwizzle[1];
    params.inputSwizzle[2] = inputSwizzle[2];
    params.inputSwizzle[3] = inputSwizzle[3];

    KTX_error_code err = ktxTexture2_CompressAstcEx(texture, &params);

    return err;
}

KTX_error_code PY_ktxTexture2_CompressBasisEx(ktxTexture2 *texture,
                                              ktx_bool_t uastc,
                                              ktx_bool_t verbose,
                                              ktx_bool_t noSSE,
                                              ktx_uint32_t threadCount,
                                              ktx_uint32_t compressionLevel,
                                              ktx_uint32_t qualityLevel,
                                              ktx_uint32_t maxEndpoints,
                                              float endpointRDOThreshold,
                                              ktx_uint32_t maxSelectors,
                                              float selectorRDOThreshold,
                                              char *inputSwizzle,
                                              ktx_bool_t normalMap,
                                              ktx_bool_t separateRGToRGB_A,
                                              ktx_bool_t preSwizzle,
                                              ktx_bool_t noEndpointRDO,
                                              ktx_bool_t noSelectorRDO,
                                              int uastcFlags,
                                              ktx_bool_t uastcRDO,
                                              float uastcRDOQualityScalar,
                                              ktx_uint32_t uastcRDODictSize,
                                              float uastcRDOMaxSmoothBlockErrorScale,
                                              float uastcRDOMaxSmoothBlockStdDev,
                                              ktx_bool_t uastcRDODontFavorSimplerModes,
                                              ktx_bool_t uastcRDONoMultithreading)
{
    ktxBasisParams params = {
        .structSize = sizeof(ktxBasisParams),
        .uastc = uastc,
        .verbose = verbose,
        .noSSE = noSSE,
        .threadCount = threadCount,
        .compressionLevel = compressionLevel,
        .qualityLevel = qualityLevel,
        .maxEndpoints = maxEndpoints,
        .endpointRDOThreshold = endpointRDOThreshold,
        .maxSelectors = maxSelectors,
        .selectorRDOThreshold = selectorRDOThreshold,
        // inputSwizzle skipped here
        .normalMap = normalMap,
        .separateRGToRGB_A = separateRGToRGB_A,
        .preSwizzle = preSwizzle,
        .noEndpointRDO = noEndpointRDO,
        .noSelectorRDO = noSelectorRDO,
        .uastcFlags = uastcFlags,
        .uastcRDO = uastcRDO,
        .uastcRDOQualityScalar = uastcRDOQualityScalar,
        .uastcRDOMaxSmoothBlockErrorScale = uastcRDOMaxSmoothBlockErrorScale,
        .uastcRDOMaxSmoothBlockStdDev = uastcRDOMaxSmoothBlockStdDev,
        .uastcRDODontFavorSimplerModes = uastcRDODontFavorSimplerModes,
        .uastcRDONoMultithreading = uastcRDONoMultithreading
    };

    params.inputSwizzle[0] = inputSwizzle[0];
    params.inputSwizzle[1] = inputSwizzle[1];
    params.inputSwizzle[2] = inputSwizzle[2];
    params.inputSwizzle[3] = inputSwizzle[3];

    KTX_error_code err = ktxTexture2_CompressBasisEx(texture, &params);

    return err;
}

KTX2_IMPL(ktx_uint32_t, vkFormat)
KTX2_IMPL(ktx_uint32_t, supercompressionScheme)
