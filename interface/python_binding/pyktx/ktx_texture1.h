/*
 * Copyright (c) 2023, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KTX_TEXTURE1_H_9E005417467F4F98A33ACF592FE1D6FE
#define KTX_TEXTURE1_H_9E005417467F4F98A33ACF592FE1D6FE

#include <ktx.h>
#include "ktx_texture.h"

ktxTextureMixed PY_ktxTexture1_Create(ktx_uint32_t glInternalFormat,
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

#define KTX1_GETTER(type, prop) \
    type PY_ktxTexture1_get_##prop(ktxTexture1 *texture)

KTX1_GETTER(ktx_uint32_t, glFormat);
KTX1_GETTER(ktx_uint32_t, glInternalformat);
KTX1_GETTER(ktx_uint32_t, glBaseInternalformat);
KTX1_GETTER(ktx_uint32_t, glType);

#define KTX1_IMPL(type, prop)       \
KTX1_GETTER(type, prop)             \
{                                   \
    return texture->prop;           \
}

#endif
