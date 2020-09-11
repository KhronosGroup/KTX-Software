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

#include "ktx.h"
#include "ktxint.h"
#include "stream.h"
#include "filestream.h"
#include "memstream.h"
#include "texture1.h"

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
ktxTexture1_setImageFromStream(ktxTexture1* This, ktx_uint32_t level,
                               ktx_uint32_t layer, ktx_uint32_t faceSlice,
                               ktxStream* src, ktx_size_t srcSize)
{
    ktx_uint32_t packedRowBytes, rowBytes, rowPadding, numRows;
    ktx_size_t packedBytes, unpackedBytes;
    ktx_size_t imageOffset;
#if (KTX_GL_UNPACK_ALIGNMENT != 4)
    ktx_uint32_t faceLodPadding;
#endif

    if (!This || !src)
        return KTX_INVALID_VALUE;

    if (!This->pData)
        return KTX_INVALID_OPERATION;

    ktxTexture_GetImageOffset(ktxTexture(This), level, layer, faceSlice, &imageOffset);

    if (This->isCompressed) {
        packedBytes = ktxTexture_GetImageSize(ktxTexture(This), level);
        rowPadding = 0;
        // These 2 are not used when rowPadding == 0. Quiets compiler warning.
        packedRowBytes = 0;
        rowBytes = 0;
    } else {
        ktxTexture_rowInfo(ktxTexture(This), level, &numRows, &rowBytes, &rowPadding);
        unpackedBytes = rowBytes * numRows;
        if (rowPadding) {
            packedRowBytes = rowBytes - rowPadding;
            packedBytes = packedRowBytes * numRows;
        } else {
            packedRowBytes = rowBytes;
            packedBytes = unpackedBytes;
        }
    }

    if (srcSize != packedBytes)
        return KTX_INVALID_OPERATION;
    // The above will catch a flagrantly invalid srcSize. This is an
    // additional check of the internal calculations.
    assert (imageOffset + srcSize <= This->dataSize);

#if (KTX_GL_UNPACK_ALIGNMENT != 4)
    faceLodPadding = _KTX_PAD4_LEN(faceLodSize);
#endif

    if (rowPadding == 0) {
        /* Can copy whole image at once */
        src->read(src, This->pData + imageOffset, srcSize);
    } else {
        /* Copy the rows individually, padding each one */
        ktx_uint32_t row;
        ktx_uint8_t* dst = This->pData + imageOffset;
        ktx_uint8_t pad[4] = { 0, 0, 0, 0 };
        for (row = 0; row < numRows; row++) {
            ktx_uint32_t rowOffset = rowBytes * row;
            src->read(src, dst + rowOffset, packedRowBytes);
            memcpy(dst + rowOffset + packedRowBytes, pad, rowPadding);
        }
    }
#if (KTX_GL_UNPACK_ALIGNMENT != 4)
    /*
     * When KTX_GL_UNPACK_ALIGNMENT == 4, rows, and therefore everything else,
     * are always 4-byte aligned and faceLodPadding is always 0. It is always
     * 0 for compressed formats too because they all have multiple-of-4 block
     * sizes.
     */
    if (faceLodPadding)
        memcpy(This->pData + faceLodSize, pad, faceLodPadding);
#endif
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
ktxTexture1_SetImageFromStdioStream(ktxTexture1* This, ktx_uint32_t level,
                                    ktx_uint32_t layer, ktx_uint32_t faceSlice,
                                    FILE* src, ktx_size_t srcSize)
{
    ktxStream srcstr;
    KTX_error_code result;

    result = ktxFileStream_construct(&srcstr, src, KTX_FALSE);
    if (result != KTX_SUCCESS)
        return result;
    result = ktxTexture1_setImageFromStream(This, level, layer, faceSlice,
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
 * @warning Do not use @c memcpy for this as it will not pad when necessary.
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
ktxTexture1_SetImageFromMemory(ktxTexture1* This, ktx_uint32_t level,
                               ktx_uint32_t layer, ktx_uint32_t faceSlice,
                               const ktx_uint8_t* src, ktx_size_t srcSize)
{
    ktxStream srcstr;
    KTX_error_code result;

    result = ktxMemStream_construct_ro(&srcstr, src, srcSize);
    if (result != KTX_SUCCESS)
        return result;
    result = ktxTexture1_setImageFromStream(This, level, layer, faceSlice,
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
 * @exception KTX_FILE_OVERFLOW The file exceeded the maximum size supported by
 *                              the system.
 * @exception KTX_FILE_WRITE_ERROR
 *                              An error occurred while writing the file.
 */
static KTX_error_code
ktxTexture1_writeToStream(ktxTexture1* This, ktxStream* dststr)
{
    KTX_header header = KTX_IDENTIFIER_REF;
    KTX_error_code result = KTX_SUCCESS;
    ktx_uint8_t* pKvd;
    ktx_uint32_t level, levelOffset;

    if (!dststr) {
        return KTX_INVALID_VALUE;
    }

    if (This->pData == NULL)
        return KTX_INVALID_OPERATION;

    if (This->kvDataHead && This->kvData)
        return KTX_INVALID_OPERATION;

    //endianess int.. if this comes out reversed, all of the other ints will too.
    header.endianness = KTX_ENDIAN_REF;
    header.glInternalformat = This->glInternalformat;
    header.glFormat = This->glFormat;
    header.glBaseInternalformat = This->glBaseInternalformat;
    header.glType = This->glType;
    header.glTypeSize = ktxTexture1_glTypeSize(This);
    header.pixelWidth = This->baseWidth;
    header.pixelHeight = This->numDimensions > 1 ? This->baseHeight : 0;
    header.pixelDepth = This->numDimensions > 2 ? This->baseDepth : 0;
    header.numberOfArrayElements = This->isArray ? This->numLayers : 0;
    assert (This->isCubemap ? This->numFaces == 6 : This->numFaces == 1);
    header.numberOfFaces = This->numFaces;
    assert (This->generateMipmaps ? This->numLevels == 1 : This->numLevels >= 1);
    header.numberOfMipLevels = This->generateMipmaps ? 0 : This->numLevels;

    if (This->kvDataHead != NULL) {
        ktxHashList_Serialize(&This->kvDataHead,
                              &header.bytesOfKeyValueData, &pKvd);
    } else if (This->kvData) {
        pKvd = This->kvData;
        header.bytesOfKeyValueData = This->kvDataLen;
    } else {
        header.bytesOfKeyValueData = 0;
    }

    //write header
    result = dststr->write(dststr, &header, sizeof(KTX_header), 1);
    if (result != KTX_SUCCESS)
        return result;

    //write keyValueData
    if (header.bytesOfKeyValueData != 0) {
        assert(pKvd != NULL);

        result = dststr->write(dststr, pKvd, 1, header.bytesOfKeyValueData);
        if (This->kvDataHead != NULL)
            free(pKvd);
        if (result != KTX_SUCCESS)
            return result;
    }

    /* Write the image data */
    for (level = 0, levelOffset=0; level < This->numLevels; ++level)
    {
        ktx_uint32_t faceLodSize, layer, levelDepth, numImages;
        ktx_size_t imageSize;

        faceLodSize = (ktx_uint32_t)ktxTexture_doCalcFaceLodSize(ktxTexture(This),
                                                    level,
                                                    KTX_FORMAT_VERSION_ONE);
        imageSize = ktxTexture_GetImageSize(ktxTexture(This), level);
        levelDepth = MAX(1, This->baseDepth >> level);
        if (This->isCubemap && !This->isArray)
            numImages = This->numFaces;
        else
            numImages = This->isCubemap ? This->numFaces : levelDepth;

        result = dststr->write(dststr, &faceLodSize, sizeof(faceLodSize), 1);
        if (result != KTX_SUCCESS)
            goto cleanup;

        for (layer = 0; layer < This->numLayers; layer++) {
            ktx_uint32_t faceSlice;

            for (faceSlice = 0; faceSlice < numImages; faceSlice++) {
                result = dststr->write(dststr, This->pData + levelOffset,
                                       imageSize, 1);
                levelOffset += (ktx_uint32_t)imageSize;
            }
        }
    }

cleanup:
    return result;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Write a ktxTexture object to a stdio stream in KTX format.
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
ktxTexture1_WriteToStdioStream(ktxTexture1* This, FILE* dstsstr)
{
    ktxStream stream;
    KTX_error_code result = KTX_SUCCESS;

    if (!This)
        return KTX_INVALID_VALUE;

    result = ktxFileStream_construct(&stream, dstsstr, KTX_FALSE);
    if (result != KTX_SUCCESS)
        return result;

    return ktxTexture1_writeToStream(This, &stream);
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Write a ktxTexture object to a named file in KTX format.
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
ktxTexture1_WriteToNamedFile(ktxTexture1* This, const char* const dstname)
{
    KTX_error_code result;
    FILE* dst;

    if (!This)
        return KTX_INVALID_VALUE;

    dst = fopen(dstname, "wb");
    if (dst) {
        result = ktxTexture1_WriteToStdioStream(This, dst);
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
ktxTexture1_WriteToMemory(ktxTexture1* This,
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

    result = ktxTexture1_writeToStream(This, &dststr);
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

ktx_uint32_t lcm4(uint32_t a);
KTX_error_code appendLibId(ktxHashList* head,
                           ktxHashListEntry* writerEntry);

/**
 * @memberof ktxTexture @private
 * @~English
 * @brief Write a ktxTexture object to a ktxStream in KTX 2 format.
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
 *                              The ktxTexture contains unknownY KTX- or ktx-
 *                              prefixed metadata keys.
 * @exception KTX_FILE_OVERFLOW The file exceeded the maximum size supported by
 *                              the system.
 * @exception KTX_FILE_WRITE_ERROR
 *                              An error occurred while writing the file.
 */
static KTX_error_code
ktxTexture1_writeKTX2ToStream(ktxTexture1* This, ktxStream* dststr)
{
    KTX_header2 header = KTX2_IDENTIFIER_REF;
    KTX_error_code result;
    ktx_uint32_t kvdLen;
    ktx_uint8_t* pKvd;
    ktx_uint32_t initialLevelPadLen;
    ktxLevelIndexEntry* levelIndex;
    ktx_uint32_t levelIndexSize;
    ktx_uint32_t offset;
    ktx_uint32_t requiredLevelAlignment;

    if (!dststr) {
        return KTX_INVALID_VALUE;
    }

    if (This->pData == NULL)
        return KTX_INVALID_OPERATION;

    header.vkFormat
            = vkGetFormatFromOpenGLInternalFormat(This->glInternalformat);
    // The above function does not return any formats in the prohibited list.
    if (header.vkFormat == VK_FORMAT_UNDEFINED) {
        // XXX TODO. Handle ASTC HDR & 3D.
        return KTX_UNSUPPORTED_TEXTURE_TYPE;
    }
    header.typeSize = ktxTexture1_glTypeSize(This);
    header.pixelWidth = This->baseWidth;
    header.pixelHeight = This->numDimensions > 1 ? This->baseHeight : 0;
    header.pixelDepth = This->numDimensions > 2 ? This->baseDepth : 0;
    header.layerCount = This->isArray ? This->numLayers : 0;
    assert (This->isCubemap ? This->numFaces == 6 : This->numFaces == 1);
    header.faceCount = This->numFaces;
    assert (This->generateMipmaps? This->numLevels == 1 : This->numLevels >= 1);
    header.levelCount = This->generateMipmaps ? 0 : This->numLevels;

    levelIndexSize = sizeof(ktxLevelIndexEntry) * This->numLevels;
    levelIndex = (ktxLevelIndexEntry*) malloc(levelIndexSize);

    offset = sizeof(header) + levelIndexSize;

    ktx_uint32_t* dfd = vk2dfd(header.vkFormat);
    if (!dfd)
        return KTX_UNSUPPORTED_TEXTURE_TYPE;

    header.dataFormatDescriptor.byteOffset = offset;
    header.dataFormatDescriptor.byteLength = *dfd;
    offset += header.dataFormatDescriptor.byteLength;

    ktxHashListEntry* pEntry;
    // Check for invalid metadata.
    for (pEntry = This->kvDataHead; pEntry != NULL; pEntry = ktxHashList_Next(pEntry)) {
        unsigned int keyLen;
        char* key;

        ktxHashListEntry_GetKey(pEntry, &keyLen, &key);
        if (strncasecmp(key, "KTX", 3) == 0) {
            if (strcmp(key, KTX_ORIENTATION_KEY) && strcmp(key, KTX_WRITER_KEY)) {
                result = KTX_INVALID_OPERATION;
                goto cleanup;
            }
        }
    }

    result = ktxHashList_FindEntry(&This->kvDataHead, KTX_ORIENTATION_KEY,
                                   &pEntry);
    // Rewrite the orientation value in the KTX2 form.
    if (result == KTX_SUCCESS) {
        unsigned int count;
        char* orientation;
        ktx_uint32_t orientationLen;
        char newOrient[4] = {0, 0, 0, 0};

        result = ktxHashListEntry_GetValue(pEntry,
                                   &orientationLen, (void**)&orientation);
        count = sscanf(orientation, "S=%c,T=%c,R=%c",
                       &newOrient[0],
                       &newOrient[1],
                       &newOrient[2]);

        if (count < This->numDimensions) {
            // There needs to be an entry for each dimension of the texture.
            result = KTX_FILE_DATA_ERROR;
            goto cleanup;
        } else if (count > This->numDimensions) {
            // KTX 1 is less strict than KTX 2 so there is a chance of having
            // more dimensions than needed.
            count = This->numDimensions;
            newOrient[count] = '\0';
        }

        ktxHashList_DeleteEntry(&This->kvDataHead, pEntry);
        ktxHashList_AddKVPair(&This->kvDataHead, KTX_ORIENTATION_KEY,
                              count+1, newOrient);
    }
    pEntry = NULL;
    result = ktxHashList_FindEntry(&This->kvDataHead, KTX_WRITER_KEY,
                                   &pEntry);
    result = appendLibId(&This->kvDataHead, pEntry);
    if (result != KTX_SUCCESS)
        goto cleanup;

    ktxHashList_Sort(&This->kvDataHead); // KTX2 requires sorted metadata.
    ktxHashList_Serialize(&This->kvDataHead, &kvdLen, &pKvd);
    header.keyValueData.byteOffset = kvdLen != 0 ? offset : 0;
    header.keyValueData.byteLength = kvdLen;
    offset += kvdLen;

    header.supercompressionGlobalData.byteOffset = 0;
    header.supercompressionGlobalData.byteLength = 0;

    requiredLevelAlignment
                = lcm4(This->_protected->_formatSize.blockSizeInBits / 8);
    initialLevelPadLen = _KTX_PADN_LEN(requiredLevelAlignment, offset);
    offset += initialLevelPadLen;

    for (ktx_int32_t level = This->numLevels - 1; level >= 0; level--) {
        ktx_size_t levelSize =
            ktxTexture_calcLevelSize(ktxTexture(This), level,
                                     KTX_FORMAT_VERSION_TWO);

        levelIndex[level].uncompressedByteLength = levelSize;
        levelIndex[level].byteLength = levelSize;
        levelIndex[level].byteOffset = offset;
        offset += _KTX_PADN(requiredLevelAlignment, levelSize);
    }

    // write header and indices
    result = dststr->write(dststr, &header, sizeof(header), 1);
    if (result != KTX_SUCCESS)
        return result;

    // write level index
    result = dststr->write(dststr, levelIndex, levelIndexSize, 1);
    if (result != KTX_SUCCESS)
        return result;

   // write data format descriptor
   result = dststr->write(dststr, dfd, 1, *dfd);

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

    if (initialLevelPadLen) {
        result = dststr->write(dststr, padding, 1, initialLevelPadLen);
        if (result != KTX_SUCCESS) {
             return result;
        }
    }

    // Write the image data
    for (ktx_int32_t level = This->numLevels - 1;
         level >= 0 && result == KTX_SUCCESS; --level)
    {
        //ktx_uint64_t faceLodSize;
        ktx_uint32_t layer, levelDepth, numImages;
        ktx_uint32_t srcLevelOffset, srcOffset;
        ktx_size_t imageSize, dstLevelSize = 0;
#define DUMP_IMAGE 0
#if defined(DEBUG) || DUMP_IMAGE
        ktx_size_t pos;
#endif
        imageSize = ktxTexture_calcImageSize(ktxTexture(This), level,
                                             KTX_FORMAT_VERSION_TWO);
#if defined(DEBUG)
        result = dststr->getpos(dststr, (ktx_off_t*)&pos);
        // Could fail if stdout is a pipe
        if (result == KTX_SUCCESS)
            assert(pos == levelIndex[level].byteOffset);
        else
            assert(result == KTX_FILE_ISPIPE);
#endif

        levelDepth = MAX(1, This->baseDepth >> level);
        if (This->isCubemap && !This->isArray)
            numImages = This->numFaces;
        else
            numImages = This->isCubemap ? This->numFaces : levelDepth;

        ktx_uint32_t  numRows = 0, rowBytes = 0, rowPadding = 0;
        if (!This->isCompressed) {
            ktxTexture_rowInfo(ktxTexture(This), level, &numRows, &rowBytes,
                               &rowPadding);
        }
        srcLevelOffset = (ktx_uint32_t)ktxTexture_calcLevelOffset(
                                                    ktxTexture(This),
                                                    level);
        srcOffset = srcLevelOffset;
        for (layer = 0; layer < This->numLayers; layer++) {
            ktx_uint32_t faceSlice;

            for (faceSlice = 0; faceSlice < numImages; faceSlice++) {
#if DUMP_IMAGE
                dststr->getsize(dststr, &pos);
                fprintf(stdout, "Writing level %d, layer %d, faceSlice %d to offset %#zx\n",
                        level, layer, faceSlice, pos);
#endif
                if (rowPadding == 0) {
#if DUMP_IMAGE
                  if (!This->isCompressed)
                    for (uint32_t y = 0; y < (This->baseHeight >> level); y++) {
                        for (uint32_t x = 0; x < rowBytes; x++) {
                            fprintf(stdout, "%#x, ",
                                    *(This->pData + srcOffset + y * rowBytes + x));
                        }
                        fprintf(stdout, "\n");
                    }
#endif
                    // Write entire image.
                    result = dststr->write(dststr, This->pData + srcOffset,
                                           imageSize, 1);
                    dstLevelSize += imageSize;
                } else {
                    /* Copy the rows individually, removing padding. */
                    ktx_uint32_t row;
                    ktx_uint8_t* src = This->pData + srcOffset;
                    ktx_uint32_t packedRowBytes = rowBytes - rowPadding;
                    for (row = 0; row < numRows; row++) {
                        ktx_uint32_t rowOffset = rowBytes * row;
#if DUMP_IMAGE
                        for (uint32_t i = 0; i < packedRowBytes; i++)
                            fprintf(stdout, "%#x, ", *(src + rowOffset + i));
#endif
                        result = dststr->write(dststr, src + rowOffset,
                                               packedRowBytes, 1);
                        dstLevelSize += packedRowBytes;
                    }
                }
#if DUMP_IMAGE
                fprintf(stdout, "\n");
#endif
                srcOffset += (ktx_uint32_t)imageSize;
            }
        }
        if (result == KTX_SUCCESS && level != 0) {
            uint32_t levelPadLen = _KTX_PADN_LEN(requiredLevelAlignment,
                                                 dstLevelSize);
            if (levelPadLen)
                result = dststr->write(dststr, padding, 1, levelPadLen);
        }
    }

cleanup:
    free(dfd);
    free(levelIndex);
    return result;
}

/**
 * @memberof ktxTexture1
 * @~English
 * @brief Write a ktxTexture object to a stdio stream in KTX2 format.
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
 *                              The ktxTexture contains unknownY KTX- or ktx-
 *                              prefixed metadata keys.
 * @exception KTX_FILE_OVERFLOW The file exceeded the maximum size supported by
 *                              the system.
 * @exception KTX_FILE_WRITE_ERROR
 *                              An error occurred while writing the file.
 */
KTX_error_code
ktxTexture1_WriteKTX2ToStdioStream(ktxTexture1* This, FILE* dstsstr)
{
    ktxStream stream;
    KTX_error_code result = KTX_SUCCESS;

    if (!This)
        return KTX_INVALID_VALUE;

    result = ktxFileStream_construct(&stream, dstsstr, KTX_FALSE);
    if (result != KTX_SUCCESS)
        return result;

    return ktxTexture1_writeKTX2ToStream(This, &stream);
}

/**
 * @memberof ktxTexture1
 * @~English
 * @brief Write a ktxTexture object to a named file in KTX2 format.
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
 *                              The ktxTexture contains unknownY KTX- or ktx-
 *                              prefixed metadata keys.
 * @exception KTX_FILE_OVERFLOW The file exceeded the maximum size supported by
 *                              the system.
 * @exception KTX_FILE_WRITE_ERROR
 *                              An error occurred while writing the file.
 */
KTX_error_code
ktxTexture1_WriteKTX2ToNamedFile(ktxTexture1* This, const char* const dstname)
{
    KTX_error_code result;
    FILE* dst;

    if (!This)
        return KTX_INVALID_VALUE;

    dst = fopen(dstname, "wb");
    if (dst) {
        result = ktxTexture1_WriteKTX2ToStdioStream(This, dst);
        fclose(dst);
    } else
        result = KTX_FILE_OPEN_FAILED;

    return result;
}

/**
 * @memberof ktxTexture1
 * @~English
 * @brief Write a ktxTexture object to block of memory in KTX2 format.
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
 *                              The ktxTexture contains unknownY KTX- or ktx-
 *                              prefixed metadata keys.
 * @exception KTX_FILE_OVERFLOW The file exceeded the maximum size supported by
 *                              the system.
 * @exception KTX_FILE_WRITE_ERROR
 *                              An error occurred while writing the file.
 */
KTX_error_code
ktxTexture1_WriteKTX2ToMemory(ktxTexture1* This,
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

    result = ktxTexture1_writeKTX2ToStream(This, &dststr);
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

