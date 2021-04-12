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
 * Copyright 2018-2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
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
#include <zstd.h>
#include <zstd_errors.h>
#include <KHR/khr_df.h>

#include "ktx.h"
#include "ktxint.h"
#include "stream.h"
#include "filestream.h"
#include "memstream.h"
#include "texture2.h"

#include "dfdutils/dfd.h"
#include "vkformat_enum.h"
#include "vk_format.h"

#include "version.h"

#if defined(_MSC_VER)
#define strncasecmp _strnicmp
#endif

/**
 * @defgroup writer Writer
 * @brief Write KTX-formatted data.
 * @{
 */

/** @internal
 *  @~English
 *  @brief Append the library's id to existing writeId.
 */
KTX_error_code
appendLibId(ktxHashList* head, ktxHashListEntry* writerEntry)
{
    KTX_error_code result;
    const char* id;
    const char* libId;
    const char idIntro[] = " / libktx ";
    ktx_uint32_t idLen;
    if (writerEntry) {
        result = ktxHashListEntry_GetValue(writerEntry, &idLen, (void**)&id);
    } else {
        id = "Unidentified app";
        idLen = 17;
    }
    if (strstr(id, "__default__") != NULL) {
        libId = STR(LIBKTX_DEFAULT_VERSION);
    } else {
        libId = STR(LIBKTX_VERSION);
    }
    if (id[idLen-1] == '\0')
      idLen--;
    // sizeof(idIntro) includes the terminating NUL which we will overwrite
    // so no need for +1 after strlen.
    ktx_uint32_t fullIdLen = idLen + sizeof(idIntro)
                             + (ktx_uint32_t)strlen(libId);
    char* fullId = malloc(fullIdLen);
    strncpy(fullId, id, idLen);
    strncpy(&fullId[idLen], idIntro, sizeof(idIntro));
    strcpy(&fullId[idLen + sizeof(idIntro)-1], libId);

    ktxHashList_DeleteEntry(head, writerEntry);
    result = ktxHashList_AddKVPair(head, KTX_WRITER_KEY, fullIdLen, fullId);
    free(fullId);
    return result;
}

/**
 * @memberof ktxTexture2 @private
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
 * @memberof ktxTexture2
 * @~English
 * @brief Set image for level, layer, faceSlice from a stdio stream source.
 *
 * Uncompressed images read from the stream are expected to have their rows
 * tightly packed as is the norm for most image file formats. KTX 2 also requires
 * tight packing this function does not add any padding.
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
 * @memberof ktxTexture2
 * @~English
 * @brief Set image for level, layer, faceSlice from an image in memory.
 *
 * Uncompressed images in memory are expected to have their rows tightly packed
 * as is the norm for most image file formats.  KTX 2 also requires
 * tight packing this function does not add any padding.
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

#if defined(TestNoMetadata)
// Only so texturetests can test loading of files without any metadata.
ktx_bool_t __disableWriterMetadata__ = KTX_FALSE;
#endif

/**
 * @memberof ktxTexture2 @private
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
            ktx_uint32_t i;
            const char* knownKeys[] = {
              "KTXcubemapIncomplete",
              "KTXorientation",
              "KTXglFormat",
              "KTXdxgiFormat__",
              "KTXswizzle",
              "KTXwriter",
              "KTXwriterScParams",
              "KTXastcDecodeMode",
              "KTXanimData"
            };
            if (strncmp(key, "ktx", 3) == 0)
                return KTX_INVALID_OPERATION;
            // Check for unrecognized KTX keys.
            for (i = 0; i < sizeof(knownKeys)/sizeof(char*); i++) {
                if (strcmp(key, knownKeys[i]) == 0)
                    break;
            }
            if (i == sizeof(knownKeys)/sizeof(char*))
                return KTX_INVALID_OPERATION;
        }
    }

#if defined(TestNoMetadata)
    if (!__disableWriterMetadata__) {
#endif
        pEntry = NULL;
        result = ktxHashList_FindEntry(&This->kvDataHead, KTX_WRITER_KEY,
                                       &pEntry);
        result = appendLibId(&This->kvDataHead, pEntry);
        if (result != KTX_SUCCESS)
            return result;
#if defined(TestNoMetadata)
    }
#endif

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
    }

    if (initialLevelPadLen) {
        result = dststr->write(dststr, padding, 1, initialLevelPadLen);
        if (result != KTX_SUCCESS) {
             return result;
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
              result = dststr->write(dststr, padding, 1, levelPadLen);
        }
    }

    return result;
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Write a ktxTexture object to a stdio stream in KTX format.
 *
 * Callers are strongly urged to include a KTXwriter item in the texture's metadata.
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
 * @memberof ktxTexture2
 * @~English
 * @brief Write a ktxTexture object to a named file in KTX format.
 *
 * Callers are strongly urged to include a KTXwriter item in the texture's metadata.
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
 * @memberof ktxTexture2
 * @~English
 * @brief Write a ktxTexture object to block of memory in KTX format.
 *
 * Memory is allocated by the function and the caller is responsible for
 * freeing it.
 *
 * Callers are strongly urged to include a KTXwriter item in the texture's metadata.
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

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Deflate the data in a ktxTexture2 object using Zstandard.
 *
 * The texture's levelIndex, dataSize, DFD  and supercompressionScheme will
 * all be updated after successful deflation to reflect the deflated data.
 *
 * @param[in] This pointer to the ktxTexture2 object of interest.
 * @param[in] compressionLevel set speed vs compression ratio trade-off. Values
 *            between 1 and 22 are accepted. The lower the level the faster. Values
 *            above 20 should be used with caution as they require more memory.
 */
KTX_error_code
ktxTexture2_DeflateZstd(ktxTexture2* This, ktx_uint32_t compressionLevel)
{
    ktx_uint32_t levelIndexByteLength =
                            This->numLevels * sizeof(ktxLevelIndexEntry);
    // Allocate a temporary buffer the same size as the current data since
    // that will clearly be big enough.
    ktx_uint8_t* workBuf = malloc(This->dataSize + levelIndexByteLength);
    ktx_uint8_t* cmpData;
    ktx_size_t dstRemainingByteLength = This->dataSize;
    ktx_size_t byteLengthCmp = 0;
    ktx_size_t levelOffset = 0;
    ktxLevelIndexEntry* cindex = This->_private->_levelIndex;
    ktxLevelIndexEntry* nindex = (ktxLevelIndexEntry*)workBuf;
    ktx_uint8_t* pCmpDst = &workBuf[levelIndexByteLength];

    ZSTD_CCtx* cctx = ZSTD_createCCtx();

    if (workBuf == NULL)
        return KTX_OUT_OF_MEMORY;

    if (This->supercompressionScheme != KTX_SS_NONE)
        return KTX_INVALID_OPERATION;

    for (int32_t level = This->numLevels - 1; level >= 0; level--) {
        size_t levelByteLengthCmp =
            ZSTD_compressCCtx(cctx, pCmpDst + levelOffset,
                              dstRemainingByteLength,
                              &This->pData[cindex[level].byteOffset],
                              cindex[level].byteLength,
                              compressionLevel);
        if (ZSTD_isError(levelByteLengthCmp)) {
            free(workBuf);
            ZSTD_ErrorCode error = ZSTD_getErrorCode(levelByteLengthCmp);
            switch(error) {
              case ZSTD_error_parameter_outOfBound:
                return KTX_INVALID_VALUE;
              case ZSTD_error_dstSize_tooSmall:
              case ZSTD_error_workSpace_tooSmall:
#ifdef DEBUG
                assert(true); // inflatedDataCapacity too small.
#else
                return KTX_OUT_OF_MEMORY;
#endif
              case ZSTD_error_memory_allocation:
                return KTX_OUT_OF_MEMORY;
              default:
                // The remaining errors look they should only occur during
                // decompression but just in case.
#ifdef DEBUG
                assert(true);
#else
                return KTX_INVALID_OPERATION;
#endif
            }
        }
        nindex[level].byteOffset = levelOffset;
        nindex[level].uncompressedByteLength = cindex[level].byteLength;
        nindex[level].byteLength = levelByteLengthCmp;
        byteLengthCmp += levelByteLengthCmp;
        levelOffset += levelByteLengthCmp;
        dstRemainingByteLength -= levelByteLengthCmp;
    }
    ZSTD_freeCCtx(cctx);

    // Move the compressed data into a correctly sized buffer.
    cmpData = malloc(byteLengthCmp);
    if (cmpData == NULL) {
        free(workBuf);
        return KTX_OUT_OF_MEMORY;
    }
    // Now modify the texture.
    memcpy(cmpData, pCmpDst, byteLengthCmp); // Copy data to sized buffer.
    memcpy(cindex, nindex, levelIndexByteLength); // Update level index
    free(workBuf);
    free(This->pData);
    This->pData = cmpData;
    This->dataSize = byteLengthCmp;
    This->supercompressionScheme = KTX_SS_ZSTD;
    This->_private->_requiredLevelAlignment = 1;
    // Clear bytesPlane to indicate we're now unsized.
    uint32_t* bdb = This->pDfd + 1;
    bdb[KHR_DF_WORD_BYTESPLANE0] = 0; /* bytesPlane3..0 = 0 */

    return KTX_SUCCESS;
}

/** @} */

