/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/**
 * @internal
 * @file writer_v2.c
 * @~English
 *
 * @brief Functions for creating KTX2-format files.
 *
 * @author Mark Callow, Edgewise Consulting.
 */

/*
 * ©2019 Mark Callow.
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
#include "texture.h"

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
 * @internal
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
 *                              The ktxTexture does not contain KTXwriter
 *                              metadata.
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
    DECLARE_SUPER(ktxTexture);
    KTX_header2 header = KTX2_IDENTIFIER_REF;
    KTX_error_code result;
    ktx_uint32_t kvdLen;
    ktx_uint8_t* pKvd;
    ktx_uint32_t align8PadLen;
    ktx_uint32_t sgdLen;
    ktx_uint32_t sgdPadLen;
    ktx_uint32_t level, levelOffset;
    ktxLevelIndexEntry* levelIndex;
    ktx_uint32_t levelIndexSize;
    ktx_uint32_t offset;

    if (!dststr) {
        return KTX_INVALID_VALUE;
    }

    if (super->pData == NULL)
        return KTX_INVALID_OPERATION;

    header.vkFormat
            = vkGetFormatFromOpenGLInternalFormat(This->glInternalformat);
    // The above function does not return any formats in the prohibited list.
    if (header.vkFormat == VK_FORMAT_UNDEFINED) {
        // XXX TODO. Handle ASTC HDR & 3D.
        return KTX_UNSUPPORTED_TEXTURE_TYPE;
    }
    header.typeSize = ktxTexture1_glTypeSize(This);
    header.pixelWidth = super->baseWidth;
    header.pixelHeight = super->baseHeight;
    header.pixelDepth = super->baseDepth;
    header.arrayElementCount = super->isArray ? super->numLayers : 0;
    assert (super->isCubemap ? super->numFaces == 6 : super->numFaces == 1);
    header.faceCount = super->numFaces;
    assert (super->generateMipmaps? super->numLevels == 1 : super->numLevels >= 1);
    header.levelCount = super->generateMipmaps ? 0 : super->numLevels;

    levelIndexSize = sizeof(ktxLevelIndexEntry) * super->numLevels;
    levelIndex = (ktxLevelIndexEntry*) malloc(levelIndexSize);

    offset = sizeof(header) + levelIndexSize;

    ktx_uint32_t* dfd = createDFD4VkFormat(header.vkFormat);
    if (!dfd)
        return KTX_UNSUPPORTED_TEXTURE_TYPE;

    header.dataFormatDescriptor.offset = offset;
    header.dataFormatDescriptor.byteLength = *dfd;
    offset += header.dataFormatDescriptor.byteLength;

    ktxHashListEntry* pEntry;
    // Check for invalid metadata.
    for (pEntry = super->kvDataHead; pEntry != NULL; pEntry = ktxHashList_Next(pEntry)) {
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

    result = ktxHashList_FindEntry(&super->kvDataHead, KTX_ORIENTATION_KEY,
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

        if (count < super->numDimensions) {
            // There needs to be an entry for each dimension of the texture.
            result = KTX_FILE_DATA_ERROR;
            goto cleanup;
        } else if (count > super->numDimensions) {
            // KTX 1 is less strict than KTX2 so there is a chance of having
            // more dimensions than needed.
            count = super->numDimensions;
            newOrient[count] = '\0';
        }

        ktxHashList_DeleteEntry(&super->kvDataHead, pEntry);
        ktxHashList_AddKVPair(&super->kvDataHead, KTX_ORIENTATION_KEY,
                              count+1, newOrient);
    }
    result = ktxHashList_FindEntry(&super->kvDataHead, KTX_WRITER_KEY,
                                   &pEntry);
    if (result != KTX_SUCCESS) {
        // KTXwriter is required in KTX2. Caller must set it.
        result = KTX_INVALID_OPERATION;
        goto cleanup;
    }

    ktxHashList_Sort(&super->kvDataHead); // KTX2 requires sorted metadata.
    ktxHashList_Serialize(&super->kvDataHead, &kvdLen, &pKvd);
    header.keyValueData.offset = kvdLen != 0 ? offset : 0;
    header.keyValueData.byteLength = kvdLen;

    align8PadLen = _KTX_PAD8_LEN(offset + kvdLen);
    offset += kvdLen + align8PadLen;

    sgdLen = 0;
    header.supercompressionGlobalData.offset = sgdLen != 0 ? offset : 0;
    header.supercompressionGlobalData.byteLength = sgdLen;

    sgdPadLen = _KTX_PAD8_LEN(sgdLen);
    offset += sgdLen + sgdPadLen;

    for (ktx_uint32_t level = 0; level < super->numLevels; level++) {
        levelIndex[level].uncompressedByteLength =
            ktxTexture_calcLevelSize(super, level, KTX_FORMAT_VERSION_TWO);
        levelIndex[level].byteLength =
            levelIndex[level].uncompressedByteLength;
        levelIndex[level].offset = offset +
            ktxTexture_calcLevelOffset(super, level, KTX_FORMAT_VERSION_TWO);
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

    char padding[] = {0,0,0,0,0,0,0};
    if (align8PadLen) {
        result = dststr->write(dststr, padding, 1, align8PadLen);
        if (result != KTX_SUCCESS) {
             return result;
        }
    }

    // write supercompressionGlobalData & sgdPadding

    // Write the image data
    for (level = super->numLevels, levelOffset=0;
         level > 0 && result == KTX_SUCCESS; )
    {
        //ktx_uint64_t faceLodSize;
        ktx_uint32_t layer, levelDepth, numImages;
        ktx_uint32_t srcLevelOffset, srcOffset;
        ktx_size_t imageSize;
#define DUMP_IMAGE 0
#if defined(DEBUG) || DUMP_IMAGE
        ktx_off_t pos;
#endif

        --level; // Calc proper level number for below. Conveniently
                 // decrements loop variable as well.
        imageSize = ktxTexture_calcImageSize(super, level,
                                             KTX_FORMAT_VERSION_TWO);
#if defined(DEBUG)
        result = dststr->getpos(dststr, &pos);
        // Could fail if stdout is a pipe
        if (result == KTX_SUCCESS)
            assert(pos == levelIndex[level].offset);
        else
            assert(result == KTX_FILE_ISPIPE);
#endif

        levelDepth = MAX(1, super->baseDepth >> level);
        if (super->isCubemap && !super->isArray)
            numImages = super->numFaces;
        else
            numImages = super->isCubemap ? super->numFaces : levelDepth;

        ktx_uint32_t  numRows = 0, rowBytes = 0, rowPadding = 0;
        if (!super->isCompressed) {
            ktxTexture_rowInfo(super, level, &numRows, &rowBytes, &rowPadding);
        }
        srcLevelOffset = (ktx_uint32_t)ktxTexture_calcLevelOffset(super, level,
                                                    KTX_FORMAT_VERSION_ONE);
        srcOffset = srcLevelOffset;
        for (layer = 0; layer < super->numLayers; layer++) {
            ktx_uint32_t faceSlice;

            for (faceSlice = 0; faceSlice < numImages; faceSlice++) {
#if DUMP_IMAGE
                dststr->getsize(dststr, &pos);
                fprintf(stdout, "Writing level %d, layer %d, faceSlice %d to offset %#zx\n",
                        level, layer, faceSlice, pos);
#endif
                if (rowPadding == 0) {
#if DUMP_IMAGE
                    for (uint32_t i = 0; i < imageSize; i++)
                        fprintf(stdout, "%#x, ", *(super->pData + srcOffset + i));
#endif
                    // Write entire image.
                    result = dststr->write(dststr, super->pData + srcOffset,
                                           imageSize, 1);
                } else {
                    /* Copy the rows individually, removing padding. */
                    ktx_uint32_t row;
                    ktx_uint8_t* src = super->pData + srcOffset;
                    ktx_uint32_t packedRowBytes = rowBytes - rowPadding;
                    for (row = 0; row < numRows; row++) {
                        ktx_uint32_t rowOffset = rowBytes * row;
#if DUMP_IMAGE
                        for (uint32_t i = 0; i < packedRowBytes; i++)
                            fprintf(stdout, "%#x, ", *(src + rowOffset + i));
#endif
                        result = dststr->write(dststr, src + rowOffset,
                                               packedRowBytes, 1);
                    }
                }
#if DUMP_IMAGE
                fprintf(stdout, "\n");
#endif
                srcOffset += (ktx_uint32_t)imageSize;
            }
        }
        if (result == KTX_SUCCESS) {
            result = dststr->write(dststr, padding, 1,
                                   _KTX_PAD8_LEN(srcOffset - srcLevelOffset));
        }
    }

cleanup:
    free(dfd);
    free(levelIndex);
    return result;
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Write a ktxTexture object to a stdio stream in KTX2 format.
 *
 * @param[in] This      pointer to the target ktxTexture object.
 * @param[in] dstsstr   destination stdio stream.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This or @p dstsstr is NULL.
 * @exception KTX_INVALID_OPERATION
 *                              The ktxTexture does not contain any image data.
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
 * @memberof ktxTexture
 * @~English
 * @brief Write a ktxTexture object to a named file in KTX2 format.
 *
 * @param[in] This      pointer to the target ktxTexture object.
 * @param[in] dstname   destination file name.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This or @p dstname is NULL.
 * @exception KTX_INVALID_OPERATION
 *                              The ktxTexture does not contain any image data.
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
 * @memberof ktxTexture
 * @~English
 * @brief Write a ktxTexture object to block of memory in KTX2 format.
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
