/*
 * Copyright (c) 2023, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KTX_TEXTURE2_H_9E005417467F4F98A33ACF592FE1D6FE
#define KTX_TEXTURE2_H_9E005417467F4F98A33ACF592FE1D6FE

#include <ktx.h>
#include "ktx_texture.h"

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
                                      ktxTextureCreateStorageEnum storageAllocation);

KTX_error_code PY_ktxTexture2_CompressAstcEx(ktxTexture2 *texture,
                                             ktx_bool_t verbose,
                                             ktx_uint32_t threadCount,
                                             ktx_uint32_t blockDimension,
                                             ktx_uint32_t mode,
                                             ktx_uint32_t quality,
                                             ktx_bool_t normalMap,
                                             ktx_bool_t perceptual,
                                             char *inputSwizzle);

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
                                              ktx_bool_t uastcRDONoMultithreading);

#define KTX2_GETTER(type, prop) \
    type PY_ktxTexture2_get_##prop(ktxTexture2 *texture)

KTX2_GETTER(ktx_uint32_t, vkFormat);
KTX2_GETTER(ktx_uint32_t, supercompressionScheme);

#define KTX2_IMPL(type, prop)       \
KTX2_GETTER(type, prop)             \
{                                   \
    return texture->prop;           \
}

#endif
