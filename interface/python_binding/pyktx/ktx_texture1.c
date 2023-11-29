/*
 * Copyright (c) 2023, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ktx.h>
#include "ktx_texture1.h"
#include <stddef.h>

ktxTextureMixed PY_ktxTexture1_Create(ktx_uint32_t glInternalformat,
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

    ktxTexture1* newTex = NULL;
    KTX_error_code err = ktxTexture1_Create(&createInfo,
                                            storageAllocation,
                                            &newTex);

    return (ktxTextureMixed) {
        err,
        (ktxTexture*) newTex
    };
}

KTX1_IMPL(ktx_uint32_t, glFormat)
KTX1_IMPL(ktx_uint32_t, glInternalformat)
KTX1_IMPL(ktx_uint32_t, glBaseInternalformat)
KTX1_IMPL(ktx_uint32_t, glType)
