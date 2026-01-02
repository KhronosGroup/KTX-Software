/*
 * Copyright (c) 2023, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ktx.h>
#include "ktx_texture.h"
#include <stddef.h>
#include <stdio.h>

ktxTextureMixed PY_ktxTexture_CreateFromNamedFile(const char* const filename, ktx_uint32_t createFlags)
{
    ktxTextureMixed mixed;
    mixed.error = ktxTexture_CreateFromNamedFile(filename, createFlags, &mixed.texture);
    return mixed;
}

ktxWriteToMemory PY_ktxTexture_WriteToMemory(ktxTexture *texture)
{
    ktx_uint8_t *ppDstBytes = NULL;
    ktx_size_t pSize = 0;
    KTX_error_code error = ktxTexture_WriteToMemory(texture, &ppDstBytes, &pSize);

    return (ktxWriteToMemory) {
        .bytes = ppDstBytes,
        .size = pSize,
        .error = error
    };
}

ktxImageOffset PY_ktxTexture_GetImageOffset(ktxTexture *texture,
                                            ktx_uint32_t level,
                                            ktx_uint32_t layer,
                                            ktx_uint32_t faceSlice)
{
    ktxImageOffset result;

    result.error = ktxTexture_GetImageOffset(texture, level, layer, faceSlice, &result.offset);

    return result;
}

ktxWriteToMemory PY_ktxHashList_FindValue(ktxHashList *list, const char *key)
{
    unsigned int valueLen = 0;
    void *pValue = NULL;
    KTX_error_code err = ktxHashList_FindValue(list, key, &valueLen, &pValue);

    return (ktxWriteToMemory) {
        .bytes = pValue,
        .size = valueLen,
        .error = err
    };
}

ktxWriteToMemory PY_ktxHashListEntry_GetKey(ktxHashListEntry *entry)
{
    unsigned int keyLen = 0;
    char *pKey = NULL;
    KTX_error_code err = ktxHashListEntry_GetKey(entry, &keyLen, &pKey);

    return (ktxWriteToMemory) {
        .bytes = pKey,
        .size = keyLen,
        .error = err
    };
}

ktxWriteToMemory PY_ktxHashListEntry_GetValue(ktxHashListEntry *entry)
{
    unsigned int valueLen = 0;
    void *pValue = NULL;
    KTX_error_code err = ktxHashListEntry_GetValue(entry, &valueLen, &pValue);

    return (ktxWriteToMemory) {
        .bytes = pValue,
        .size = valueLen,
        .error = err
    };
}

KTX_IMPL(class_id, classId);
KTX_IMPL(ktx_bool_t, isArray);
KTX_IMPL(ktx_bool_t, isCompressed);
KTX_IMPL(ktx_bool_t, isCubemap);
KTX_IMPL(ktx_bool_t, generateMipmaps);
KTX_IMPL(ktx_uint32_t, baseWidth);
KTX_IMPL(ktx_uint32_t, baseHeight);
KTX_IMPL(ktx_uint32_t, baseDepth);
KTX_IMPL(ktx_uint32_t, numDimensions);
KTX_IMPL(ktx_uint32_t, numLevels);
KTX_IMPL(ktx_uint32_t, numFaces);
KTX_IMPL(ktx_uint32_t, kvDataLen);
KTX_IMPL(ktx_uint8_t *, kvData);

ktxHashList *PY_ktxTexture_get_kvDataHead(ktxTexture *texture)
{
    return &texture->kvDataHead;
}

ktxHashListEntry *PY_ktxHashList_get_listHead(ktxHashList *list)
{
    return *list;
}
