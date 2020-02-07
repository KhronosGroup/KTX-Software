/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/**
 * @internal
 * @file writer.c
 * @~English
 *
 * @brief Functions for creating KTX-format files from a set of images.
 *
 * @author Mark Callow, HI Corporation
 */

/*
 * ©2018 Mark Callow.
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

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__GNUC__)
#include <strings.h>  // For strncasecmp on GNU/Linux
#endif

#include "ktx.h"
#include "ktxint.h"
#include "stream.h"
#include "filestream.h"
#include "memstream.h"
#include "texture2.h"

#include "dfdutils/dfd.h"
#include "vkformat_enum.h"
#include "vk_format.h"

#if defined(_MSC_VER)
#define strncasecmp _strnicmp
#endif

/**
 * @defgroup writer Writer
 * @brief Write KTX-formatted data.
 * @{
 */

/**
 * @memberof ktxTexture @private
 * @~English
 * @brief Set image for level, layer, faceSlice from a ktxStream source.
 *
 * @param[in] This      pointer to the target ktxTexture object.
 * @param[in] level     mip level of the image to set.
 * @param[in] layer     array layer of the image to set.
 * @param[in] faceSlice cube map face or depth slice of the image to set.
 * @param[in] src       ktxStream pointer to the source.
 * @param[in] srcSize   size of the source image in bytes.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This or @p src is NULL.
 * @exception KTX_INVALID_VALUE @p srcSize != the expected image size for the
 *                              specified level, layer & faceSlice.
 * @exception KTX_INVALID_OPERATION
 *                              No storage was allocated when the texture was
 *                              created.
 */
KTX_error_code
ktxTexture2_setImageFromStream(ktxTexture2* This, ktx_uint32_t level,
                               ktx_uint32_t layer, ktx_uint32_t faceSlice,
                               ktxStream* src, ktx_size_t srcSize)
{
    ktx_size_t imageByteLength;
    ktx_size_t imageByteOffset;

    if (!This || !src)
        return KTX_INVALID_VALUE;

    if (!This->pData)
        return KTX_INVALID_OPERATION;

    ktxTexture_GetImageOffset(ktxTexture(This), level, layer, faceSlice,
                                         &imageByteOffset);
    imageByteLength = ktxTexture_GetImageSize(ktxTexture(This), level);

    if (srcSize != imageByteLength)
        return KTX_INVALID_OPERATION;
    // The above will catch a flagrantly invalid srcSize. This is an
    // additional check of the internal calculations.
    assert (imageByteOffset + srcSize <= This->dataSize);

    /* Can copy whole image at once */
    src->read(src, This->pData + imageByteOffset, srcSize);
    return KTX_SUCCESS;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Set image for level, layer, faceSlice from a stdio stream source.
 *
 * Uncompressed images read from the stream are expected to have their rows
 * tightly packed as is the norm for most image file formats. The copied image
 * is padded as necessary to achieve the KTX-specified row alignment. No
 * padding is done if the ktxTexture's @c isCompressed field is @c KTX_TRUE.
 *
 * Level, layer, faceSlice rather than offset are specified to enable some
 * validation.
 *
 * @param[in] This      pointer to the target ktxTexture object.
 * @param[in] level     mip level of the image to set.
 * @param[in] layer     array layer of the image to set.
 * @param[in] faceSlice cube map face or depth slice of the image to set.
 * @param[in] src       stdio stream pointer to the source.
 * @param[in] srcSize   size of the source image in bytes.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This or @p src is NULL.
 * @exception KTX_INVALID_VALUE @p srcSize != the expected image size for the
 *                              specified level, layer & faceSlice.
 * @exception KTX_INVALID_OPERATION
 *                              No storage was allocated when the texture was
 *                              created.
 */
KTX_error_code
ktxTexture2_SetImageFromStdioStream(ktxTexture2* This, ktx_uint32_t level,
                                    ktx_uint32_t layer, ktx_uint32_t faceSlice,
                                    FILE* src, ktx_size_t srcSize)
{
    ktxStream srcstr;
    KTX_error_code result;

    result = ktxFileStream_construct(&srcstr, src, KTX_FALSE);
    if (result != KTX_SUCCESS)
        return result;
    result = ktxTexture2_setImageFromStream(This, level, layer, faceSlice,
                                            &srcstr, srcSize);
    ktxFileStream_destruct(&srcstr);
    return result;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Set image for level, layer, faceSlice from an image in memory.
 *
 * Uncompressed images in memory are expected to have their rows tightly packed
 * as is the norm for most image file formats. The copied image is padded as
 * necessary to achieve the KTX-specified row alignment. No padding is done if
 * the ktxTexture's @c isCompressed field is @c KTX_TRUE.
 *
 * Level, layer, faceSlice rather than offset are specified to enable some
 * validation.
 *
 * @param[in] This      pointer to the target ktxTexture object.
 * @param[in] level     mip level of the image to set.
 * @param[in] layer     array layer of the image to set.
 * @param[in] faceSlice cube map face or depth slice of the image to set.
 * @param[in] src       pointer to the image source in memory.
 * @param[in] srcSize   size of the source image in bytes.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This or @p src is NULL.
 * @exception KTX_INVALID_VALUE @p srcSize != the expected image size for the
 *                              specified level, layer & faceSlice.
 * @exception KTX_INVALID_OPERATION
 *                              No storage was allocated when the texture was
 *                              created.
 */
KTX_error_code
ktxTexture2_SetImageFromMemory(ktxTexture2* This, ktx_uint32_t level,
                               ktx_uint32_t layer, ktx_uint32_t faceSlice,
                               const ktx_uint8_t* src, ktx_size_t srcSize)
{
    ktxStream srcstr;
    KTX_error_code result;

    result = ktxMemStream_construct_ro(&srcstr, src, srcSize);
    if (result != KTX_SUCCESS)
        return result;
    result = ktxTexture2_setImageFromStream(This, level, layer, faceSlice,
                                            &srcstr, srcSize);
    ktxMemStream_destruct(&srcstr);
    return result;
}

/**
 * @memberof ktxTexture @private
 * @~English
 * @brief Write a ktxTexture object to a ktxStream in KTX format.
 *
 * @param[in] This      pointer to the target ktxTexture object.
 * @param[in] dststr    destination ktxStream.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This or @p dststr is NULL.
 * @exception KTX_INVALID_OPERATION
 *                              The ktxTexture does not contain any image data.
 * @exception KTX_INVALID_OPERATION
 *                              Both kvDataHead and kvData are set in the
 *                              ktxTexture
 * @exception KTX_INVALID_OPERATION
 *                              The ktxTexture does not contain KTXwriter
 *                              metadata.
 * @exception KTX_FILE_OVERFLOW The file exceeded the maximum size supported by
 *                              the system.
 * @exception KTX_FILE_WRITE_ERROR
 *                              An error occurred while writing the file.
 */
static KTX_error_code
ktxTexture2_writeToStream(ktxTexture2* This, ktxStream* dststr)
{
    DECLARE_PRIVATE(ktxTexture2);
    KTX_header2 header = KTX2_IDENTIFIER_REF;
    KTX_error_code result;
    ktx_uint32_t kvdLen;
    ktx_uint8_t* pKvd;
    ktx_uint32_t align8PadLen;
    ktx_uint64_t sgdLen;
    ktx_uint32_t initialLevelPadLen;
    ktx_uint32_t levelIndexSize;
    ktx_uint64_t baseOffset;

    if (!dststr) {
        return KTX_INVALID_VALUE;
    }

    if (This->pData == NULL)
        return KTX_INVALID_OPERATION;

    header.vkFormat = This->vkFormat;
    header.typeSize = This->_protected->_typeSize;
    header.pixelWidth = This->baseWidth;
    header.pixelHeight = This->numDimensions > 1 ? This->baseHeight : 0;
    header.pixelDepth = This->numDimensions > 2 ? This->baseDepth : 0;
    header.layerCount = This->isArray ? This->numLayers : 0;
    assert (This->isCubemap ? This->numFaces == 6 : This->numFaces == 1);
    header.faceCount = This->numFaces;
    assert (This->generateMipmaps? This->numLevels == 1 : This->numLevels >= 1);
    header.levelCount = This->generateMipmaps ? 0 : This->numLevels;
    header.supercompressionScheme = This->supercompressionScheme;

    levelIndexSize = sizeof(ktxLevelIndexEntry) * This->numLevels;

    baseOffset = sizeof(header) + levelIndexSize;

    header.dataFormatDescriptor.byteOffset = (uint32_t)baseOffset;
    header.dataFormatDescriptor.byteLength = *This->pDfd;
    baseOffset += header.dataFormatDescriptor.byteLength;

    ktxHashListEntry* pEntry;
    // Check for invalid metadata.
    for (pEntry = This->kvDataHead; pEntry != NULL; pEntry = ktxHashList_Next(pEntry)) {
        unsigned int keyLen;
        char* key;

        ktxHashListEntry_GetKey(pEntry, &keyLen, &key);
        if (strncasecmp(key, "KTX", 3) == 0) {
            // FIXME. Check for an undefined key.
            //return KTX_INVALID_OPERATION;
        }
    }

    result = ktxHashList_FindEntry(&This->kvDataHead, KTX_WRITER_KEY,
                                   &pEntry);
    if (result != KTX_SUCCESS) {
        // KTXwriter is required in KTX2. Caller must set it.
        return KTX_INVALID_OPERATION;
    }

    ktxHashList_Sort(&This->kvDataHead); // KTX2 requires sorted metadata.
    ktxHashList_Serialize(&This->kvDataHead, &kvdLen, &pKvd);
    header.keyValueData.byteOffset = kvdLen != 0 ? (uint32_t)baseOffset : 0;
    header.keyValueData.byteLength = kvdLen;
    baseOffset += kvdLen;

    sgdLen = private->_sgdByteLength;
    if (sgdLen) {
        align8PadLen = _KTX_PAD8_LEN(baseOffset);
        baseOffset += align8PadLen;
    }

    header.supercompressionGlobalData.byteOffset = sgdLen != 0 ? baseOffset : 0;
    header.supercompressionGlobalData.byteLength = sgdLen;
    baseOffset += sgdLen;

    initialLevelPadLen = _KTX_PADN_LEN(This->_private->_requiredLevelAlignment,
                                       baseOffset);
    baseOffset += initialLevelPadLen;

    // write header and indices
    result = dststr->write(dststr, &header, sizeof(header), 1);
    if (result != KTX_SUCCESS)
        return result;

    // Create a copy of the level index with file-adjusted offsets and write it.
    ktxLevelIndexEntry* levelIndex
                            = (ktxLevelIndexEntry*)malloc(levelIndexSize);
    if (!levelIndex)
        return KTX_OUT_OF_MEMORY;
    for (ktx_uint32_t level = 0; level < This->numLevels; level++) {
        levelIndex[level].byteLength = private->_levelIndex[level].byteLength;
        levelIndex[level].uncompressedByteLength
                         = private->_levelIndex[level].uncompressedByteLength;
        levelIndex[level].byteOffset = private->_levelIndex[level].byteOffset;
        levelIndex[level].byteOffset += baseOffset;
    }
    result = dststr->write(dststr, levelIndex, levelIndexSize, 1);
    free(levelIndex);
    if (result != KTX_SUCCESS)
        return result;

   // write data format descriptor
   result = dststr->write(dststr, This->pDfd, 1, *This->pDfd);

   // write keyValueData
    if (kvdLen != 0) {
        assert(pKvd != NULL);

        result = dststr->write(dststr, pKvd, 1, kvdLen);
        free(pKvd);
        if (result != KTX_SUCCESS) {
             return result;
        }
    }

    char padding[32] = { 0 };
    // write supercompressionGlobalData & sgdPadding
    if (private->_sgdByteLength != 0) {
        if (align8PadLen) {
            result = dststr->write(dststr, padding, 1, align8PadLen);
            if (result != KTX_SUCCESS) {
                 return result;
            }
        }

        result = dststr->write(dststr, private->_supercompressionGlobalData,
                               1, private->_sgdByteLength);
        if (result != KTX_SUCCESS) {
            return result;
        }

        if (initialLevelPadLen) {
            result = dststr->write(dststr, padding, 1, initialLevelPadLen);
            if (result != KTX_SUCCESS) {
                 return result;
            }
        }
    }

    // write the image data
    for (ktx_int32_t level = This->numLevels-1; level >= 0 && result == KTX_SUCCESS; --level)
    {
        ktx_uint64_t srcLevelOffset, levelSize;
#define DUMP_IMAGE 0
#if defined(DEBUG) || DUMP_IMAGE
        ktx_size_t pos;
#endif

#if defined(DEBUG)
        result = dststr->getpos(dststr, (ktx_off_t*)&pos);
        // Could fail if stdout is a pipe
        if (result == KTX_SUCCESS)
            assert(pos == private->_levelIndex[level].byteOffset + baseOffset);
        else
            assert(result == KTX_FILE_ISPIPE);
#endif

        srcLevelOffset = ktxTexture2_levelDataOffset(This, level);
        levelSize = private->_levelIndex[level].byteLength;

#if DUMP_IMAGE
        if (!This->isCompressed) {
            for (layer = 0; layer < This->numLayers; layer++) {
                ktx_uint32_t faceSlice;
                for (faceSlice = 0; faceSlice < numImages; faceSlice++) {
                    dststr->getsize(dststr, &pos);
                    fprintf(stdout, "Writing level %d, layer %d, faceSlice %d to baseOffset %#zx\n",
                            level, layer, faceSlice, pos);
                    for (uint32_t y = 0; y < (This->baseHeight >> level); y++) {
                        for (uint32_t x = 0; x < rowBytes; x++) {
                            fprintf(stdout, "%#x, ",
                                    *(This->pData + srcOffset + y * rowBytes + x));
                        }
                        fprintf(stdout, "\n");
                    }
                }
            }
        }
        fprintf(stdout, "\n");
#endif
        // Write entire level.
        result = dststr->write(dststr, This->pData + srcLevelOffset,
                               levelSize, 1);
        if (result == KTX_SUCCESS && level > 0) { // No padding at end.
            ktx_uint32_t levelPadLen
                       = _KTX_PADN_LEN(This->_private->_requiredLevelAlignment,
                                       levelSize);
            if (levelPadLen != 0)
              result = dststr->write(dststr, padding, 1, align8PadLen);
        }
    }

    return result;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Write a ktxTexture object to a stdio stream in KTX format.
 *
 * If there is no KTXwriter item in the texture's metadata, the function
 * returns @c KTX_INVALID_OPERATION. KTXwriter is required by the specification.
 * It can be added by code, similar to the following, prior to calling this
 * function.
 * @code
 *     char writer[100];
 *     snprintf(writer, sizeof(writer), "%s version %s", appName, appVer);
 *     ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
 *                           (ktx_uint32_t)strlen(writer) + 1,
 *                           writer);
 * @endcode
 *
 * @param[in] This      pointer to the target ktxTexture object.
 * @param[in] dstsstr   destination stdio stream.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This or @p dstsstr is NULL.
 * @exception KTX_INVALID_OPERATION
 *                              The ktxTexture does not contain any image data.
 * @exception KTX_INVALID_OPERATION
 *                              Both kvDataHead and kvData are set in the
 *                              ktxTexture
 * @exception KTX_INVALID_OPERATION
 *                              The ktxTexture does not contain KTXwriter
 *                              metadata.
 * @exception KTX_FILE_OVERFLOW The file exceeded the maximum size supported by
 *                              the system.
 * @exception KTX_FILE_WRITE_ERROR
 *                              An error occurred while writing the file.
 */
KTX_error_code
ktxTexture2_WriteToStdioStream(ktxTexture2* This, FILE* dstsstr)
{
    ktxStream stream;
    KTX_error_code result = KTX_SUCCESS;

    if (!This)
        return KTX_INVALID_VALUE;

    result = ktxFileStream_construct(&stream, dstsstr, KTX_FALSE);
    if (result != KTX_SUCCESS)
        return result;

    return ktxTexture2_writeToStream(This, &stream);
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Write a ktxTexture object to a named file in KTX format.
 *
 * If there is no KTXwriter item in the texture's metadata, the function
 * returns @c KTX_INVALID_OPERATION. KTXwriter is required by the specification.
 * It can be added by code, similar to the following, prior to calling this
 * function.
 * @code
 *     char writer[100];
 *     snprintf(writer, sizeof(writer), "%s version %s", appName, appVer);
 *     ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
 *                           (ktx_uint32_t)strlen(writer) + 1,
 *                           writer);
 * @endcode
 *
 * @param[in] This      pointer to the target ktxTexture object.
 * @param[in] dstname   destination file name.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This or @p dstname is NULL.
 * @exception KTX_INVALID_OPERATION
 *                              The ktxTexture does not contain any image data.
 * @exception KTX_INVALID_OPERATION
 *                              Both kvDataHead and kvData are set in the
 *                              ktxTexture
 * @exception KTX_INVALID_OPERATION
 *                              The ktxTexture does not contain KTXwriter
 *                              metadata.
 * @exception KTX_FILE_OVERFLOW The file exceeded the maximum size supported by
 *                              the system.
 * @exception KTX_FILE_WRITE_ERROR
 *                              An error occurred while writing the file.
 */
KTX_error_code
ktxTexture2_WriteToNamedFile(ktxTexture2* This, const char* const dstname)
{
    KTX_error_code result;
    FILE* dst;

    if (!This)
        return KTX_INVALID_VALUE;

    dst = fopen(dstname, "wb");
    if (dst) {
        result = ktxTexture2_WriteToStdioStream(This, dst);
        fclose(dst);
    } else
        result = KTX_FILE_OPEN_FAILED;

    return result;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Write a ktxTexture object to block of memory in KTX format.
 *
 * Memory is allocated by the function and the caller is responsible for
 * freeing it.
 *
 * If there is no KTXwriter item in the texture's metadata, the function
 * returns @c KTX_INVALID_OPERATION. KTXwriter is required by the specification.
 * It can be added by code, similar to the following, prior to calling this
 * function.
 * @code
 *     char writer[100];
 *     snprintf(writer, sizeof(writer), "%s version %s", appName, appVer);
 *     ktxHashList_AddKVPair(&texture->kvDataHead, KTX_WRITER_KEY,
 *                           (ktx_uint32_t)strlen(writer) + 1,
 *                           writer);
 * @endcode
 *
 * @param[in]     This       pointer to the target ktxTexture object.
 * @param[in,out] ppDstBytes pointer to location to write the address of
 *                           the destination memory. The Application is
 *                           responsible for freeing this memory.
 * @param[in,out] pSize      pointer to location to write the size in bytes of
 *                           the KTX data.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This, @p ppDstBytes or @p pSize is NULL.
 * @exception KTX_INVALID_OPERATION
 *                              The ktxTexture does not contain any image data.
 * @exception KTX_INVALID_OPERATION
 *                              Both kvDataHead and kvData are set in the
 *                              ktxTexture
 * @exception KTX_INVALID_OPERATION
 *                              The ktxTexture does not contain KTXwriter
 *                              metadata.
 * @exception KTX_FILE_OVERFLOW The file exceeded the maximum size supported by
 *                              the system.
 * @exception KTX_FILE_WRITE_ERROR
 *                              An error occurred while writing the file.
 */
KTX_error_code
ktxTexture2_WriteToMemory(ktxTexture2* This,
                          ktx_uint8_t** ppDstBytes, ktx_size_t* pSize)
{
    struct ktxStream dststr;
    KTX_error_code result;
    ktx_size_t strSize;

    if (!This || !ppDstBytes || !pSize)
        return KTX_INVALID_VALUE;

    *ppDstBytes = NULL;

    result = ktxMemStream_construct(&dststr, KTX_FALSE);
    if (result != KTX_SUCCESS)
        return result;

    result = ktxTexture2_writeToStream(This, &dststr);
    if(result != KTX_SUCCESS)
    {
        ktxMemStream_destruct(&dststr);
        return result;
    }

    ktxMemStream_getdata(&dststr, ppDstBytes);
    dststr.getsize(&dststr, &strSize);
    *pSize = (GLsizei)strSize;
    /* This function does not free the memory pointed at by the
     * value obtained from ktxMemStream_getdata() thanks to the
     * KTX_FALSE passed to the constructor above.
     */
    ktxMemStream_destruct(&dststr);
    return KTX_SUCCESS;

}

/** @} */

