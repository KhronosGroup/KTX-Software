/*
 * Copyright (c) 2023, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KTX_TEXTURE_H_9E005417467F4F98A33ACF592FE1D6FE
#define KTX_TEXTURE_H_9E005417467F4F98A33ACF592FE1D6FE

#include <ktx.h>

typedef struct {
    KTX_error_code error;
    ktxTexture *texture;
} ktxTextureMixed;

typedef struct {
    void *bytes;
    ktx_size_t size;
    KTX_error_code error;
} ktxWriteToMemory;

typedef struct {
    size_t offset;
    int error;
} ktxImageOffset;

ktxTextureMixed PY_ktxTexture_CreateFromNamedFile(const char* const filename, ktx_uint32_t create_flags);
ktxWriteToMemory PY_ktxTexture_WriteToMemory(ktxTexture *);
ktxImageOffset PY_ktxTexture_GetImageOffset(ktxTexture *,
                                            ktx_uint32_t level,
                                            ktx_uint32_t layer,
                                            ktx_uint32_t faceSlice);
ktxWriteToMemory PY_ktxHashList_FindValue(ktxHashList *, const char *key);
ktxWriteToMemory PY_ktxHashListEntry_GetKey(ktxHashListEntry *);
ktxWriteToMemory PY_ktxHashListEntry_GetValue(ktxHashListEntry *);

#define KTX_GETTER(type, prop) \
    type PY_ktxTexture_get_##prop(ktxTexture *texture)

KTX_GETTER(class_id, classId);
KTX_GETTER(ktx_bool_t, isArray);
KTX_GETTER(ktx_bool_t, isCompressed);
KTX_GETTER(ktx_bool_t, isCubemap);
KTX_GETTER(ktx_bool_t, generateMipmaps);
KTX_GETTER(ktx_uint32_t, baseWidth);
KTX_GETTER(ktx_uint32_t, baseHeight);
KTX_GETTER(ktx_uint32_t, baseDepth);
KTX_GETTER(ktx_uint32_t, numDimensions);
KTX_GETTER(ktx_uint32_t, numLevels);
KTX_GETTER(ktx_uint32_t, numFaces);
KTX_GETTER(ktx_uint32_t, kvDataLen);
KTX_GETTER(ktx_uint8_t *, kvData);
KTX_GETTER(ktxHashList *, kvDataHead);
ktxHashListEntry *PY_ktxHashList_get_listHead(ktxHashList *list);

#define KTX_IMPL(type, prop)        \
KTX_GETTER(type, prop)              \
{                                   \
    return texture->prop;           \
}

#endif
