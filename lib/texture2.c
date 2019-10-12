/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2019 Khronos Group, Inc.
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

/**
 * @internal
 * @file texture2.c
 * @~English
 *
 * @brief ktxTexture2 implementation. Support for KTX2 format.
 *
 * @author Mark Callow, www.edgewise-consulting.com
 */

#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h>
#include <KHR/khr_df.h>

#include "dfdutils/dfd.h"
#include "ktx.h"
#include "ktxint.h"
#include "stream.h"
#include "filestream.h"
#include "memstream.h"
#include "texture2.h"
#include "uthash.h"
#include "vk_format.h"

// FIXME Test this #define and put it in a header somewhere.
//#define IS_BIG_ENDIAN (1 == *(unsigned char *)&(const int){0x01000000ul})
#define IS_BIG_ENDIAN 0

struct ktxTexture_vtbl ktxTexture2_vtbl;
struct ktxTexture_vtblInt ktxTexture2_vtblInt;
extern struct ktxTexture_vvtbl* pKtxTexture2_vvtbl;

/**
 * @memberof ktxTexture2 @private
 * @~English
 * @brief Do the part of ktxTexture2 construction that is common to
 *        new textures and those constructed from a stream.
 *
 * @param[in] This      pointer to a ktxTexture2-sized block of memory to
 *                      initialize.
 * @param[in] numLevels the number of levels the texture must have.
 *
 * @return    KTX_SUCCESS on success, other KTX_* enum values on error.
 * @exception KTX_OUT_OF_MEMORY Not enough memory for the texture data.
 */
static KTX_error_code
ktxTexture2_constructCommon(ktxTexture2* This, ktx_uint32_t numLevels)
{
    assert(This != NULL);
    ktx_size_t privateSize;

    This->classId = ktxTexture2_c;
    This->vtbl = &ktxTexture2_vtbl;
    This->_protected->_vtbl = ktxTexture2_vtblInt;
#if !KTX_OMIT_VULKAN
    This->vvtbl = pKtxTexture2_vvtbl;
#else
    This->vvtbl = NULL;
#endif
    privateSize = sizeof(ktxTexture2_private)
                + sizeof(ktxLevelIndexEntry) * (numLevels - 1);
    This->_private = (ktxTexture2_private*)malloc(privateSize);
    if (This->_private == NULL) {
        return KTX_OUT_OF_MEMORY;
    }
    memset(This->_private, 0, privateSize);
    return KTX_SUCCESS;
}


/**
 * @memberof ktxTexture2 @private
 * @~English
 * @brief Construct a new, empty, ktxTexture2.
 *
 * @param[in] This       pointer to a ktxTexture2-sized block of memory to
 *                       initialize.
 * @param[in] createInfo pointer to a ktxTextureCreateInfo struct with
 *                       information describing the texture.
 * @param[in] storageAllocation
 *                       enum indicating whether or not to allocate storage
 *                       for the texture images.
 * @return    KTX_SUCCESS on success, other KTX_* enum values on error.
 * @exception KTX_OUT_OF_MEMORY Not enough memory for the texture data.
 * @exception KTX_UNSUPPORTED_TEXTURE_TYPE
 *                              The request VkFormat is one of the
 *                              prohibited formats.
 */
static KTX_error_code
ktxTexture2_construct(ktxTexture2* This, ktxTextureCreateInfo* createInfo,
                        ktxTextureCreateStorageEnum storageAllocation)
{
    ktxFormatSize formatSize;
    KTX_error_code result;

    memset(This, 0, sizeof(*This));

    if (createInfo->vkFormat != VK_FORMAT_UNDEFINED) {
        vkGetFormatSize(createInfo->vkFormat, &formatSize);
        if (formatSize.blockSizeInBits == 0) {
            return KTX_INVALID_VALUE; // TODO Return a more reasonable error?
        } else {
            This->pDfd = vk2dfd(createInfo->vkFormat);
        }
    } else {
        // TODO Validate createInfo->pDfd and create formatSize from it.
    }
    if (!This->pDfd)
        return KTX_UNSUPPORTED_TEXTURE_TYPE;

    result =  ktxTexture_construct(ktxTexture(This), createInfo, &formatSize,
                                   storageAllocation);
    if (result != KTX_SUCCESS)
        return result;
    result = ktxTexture2_constructCommon(This, createInfo->numLevels);
    if (result != KTX_SUCCESS)
        goto cleanup;;

    This->vkFormat = createInfo->vkFormat;

    if (This->isCompressed)
        This->_protected->_typeSize = 1;
    else if (formatSize.flags & KTX_FORMAT_SIZE_PACKED_BIT)
        This->_protected->_typeSize = formatSize.blockSizeInBits / 8;
    else if (formatSize.flags & (KTX_FORMAT_SIZE_DEPTH_BIT | KTX_FORMAT_SIZE_STENCIL_BIT)) {
        if (createInfo->vkFormat == VK_FORMAT_D16_UNORM_S8_UINT)
            This->_protected->_typeSize = 2;
        else
            This->_protected->_typeSize = 4;
    } else {
        // Unpacked and uncompressed
        uint32_t numComponents;
        getDFDComponentInfoUnpacked(This->pDfd, &numComponents,
                                    &This->_protected->_typeSize);
    }

    This->supercompressionScheme = KTX_SUPERCOMPRESSION_NONE;

    // Create levelIndex. Offsets are from start of the KTX2 stream.
    ktxLevelIndexEntry* levelIndex = This->_private->_levelIndex;
    ktx_uint32_t levelIndexSize;

    This->_private->_firstLevelFileOffset = 0;
    levelIndexSize = sizeof(ktxLevelIndexEntry) * This->numLevels;

    for (ktx_uint32_t level = 0; level < This->numLevels; level++) {
        levelIndex[level].uncompressedByteLength =
            ktxTexture_calcLevelSize(ktxTexture(This), level,
                                     KTX_FORMAT_VERSION_TWO);
        levelIndex[level].byteLength =
            levelIndex[level].uncompressedByteLength;
        levelIndex[level].byteOffset =
            ktxTexture_calcLevelOffset(ktxTexture(This), level,
                                       KTX_FORMAT_VERSION_TWO);
    }

    // Allocate storage, if requested.
    if (storageAllocation == KTX_TEXTURE_CREATE_ALLOC_STORAGE) {
        This->dataSize
                = ktxTexture_calcDataSizeTexture(ktxTexture(This),
                                                 KTX_FORMAT_VERSION_TWO);
        This->pData = malloc(This->dataSize);
        if (This->pData == NULL) {
            result = KTX_OUT_OF_MEMORY;
            goto cleanup;
        }
    }
    return result;

cleanup:
    ktxTexture2_destruct(This);
    return result;
}

/**
 * @memberof ktxTexture2 @private
 * @~English
 * @brief Construct a ktxTexture by copying a source ktxTexture.
 *
 * @param[in] This pointer to a ktxTexture2-sized block of memory to
 *                 initialize.
 * @param[in] orig pointer to the source texture to copy.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_OUT_OF_MEMORY Not enough memory for the texture data.
 */
static KTX_error_code
ktxTexture2_constructCopy(ktxTexture2* This, ktxTexture2* orig)
{
    KTX_error_code result;

    memcpy(This, orig, sizeof(ktxTexture2));
    // Zero all the pointers to make error handling easier
    This->_protected = NULL;
    This->_private = NULL;
    This->pDfd = NULL;
    This->kvData = NULL;
    This->kvDataHead = NULL;
    This->pData = NULL;

    This->_protected =
                    (ktxTexture_protected*)malloc(sizeof(ktxTexture_protected));
    if (!This->_protected)
        return KTX_OUT_OF_MEMORY;
    // Must come before memcpy of _protected so as to close an active stream.
    if (!orig->pData && ktxTexture_isActiveStream((ktxTexture*)orig))
        ktxTexture2_LoadImageData(orig, NULL, 0);
    memcpy(This->_protected, orig->_protected, sizeof(ktxTexture_protected));

    ktx_size_t privateSize = sizeof(ktxTexture2_private)
                           + sizeof(ktxLevelIndexEntry) * (orig->numLevels - 1);
    This->_private = (ktxTexture2_private*)malloc(privateSize);
    if (This->_private == NULL) {
        result = KTX_OUT_OF_MEMORY;
        goto cleanup;
    }
    memcpy(This->_private, orig->_private, privateSize);
    if (orig->_private->_sgdByteLength > 0) {
        This->_private->_supercompressionGlobalData
                        = (ktx_uint8_t*)malloc(orig->_private->_sgdByteLength);
        if (!This->_private->_supercompressionGlobalData) {
            result = KTX_OUT_OF_MEMORY;
            goto cleanup;
        }
        memcpy(This->_private->_supercompressionGlobalData,
               orig->_private->_supercompressionGlobalData,
               orig->_private->_sgdByteLength);
    }

    This->pDfd = (ktx_uint32_t*)malloc(*orig->pDfd);
    if (!This->pDfd) {
        result = KTX_OUT_OF_MEMORY;
        goto cleanup;
    }
    memcpy(This->pDfd, orig->pDfd, *orig->pDfd);

    if (orig->kvDataHead) {
        ktxHashList_ConstructCopy(&This->kvDataHead, orig->kvDataHead);
    } else if (orig->kvData) {
        This->kvData = (ktx_uint8_t*)malloc(orig->kvDataLen);
        if (!This->kvData) {
            result = KTX_OUT_OF_MEMORY;
            goto cleanup;
        }
        memcpy(This->kvData, orig->kvData, orig->kvDataLen);
    }

    // Can't share the image data as the data pointer is exposed in the
    // ktxTexture2 structure. Changing it to a ref-counted pointer would
    // break code. Maybe that's okay as we're still pre-release. But,
    // since this constructor will be mostly be used when transcoding
    // supercompressed images, it is probably not too big a deal to make
    // a copy of the data.
    This->pData = (ktx_uint8_t*)malloc(This->dataSize);
    if (This->pData == NULL) {
        result = KTX_OUT_OF_MEMORY;
        goto cleanup;
    }
    memcpy(This->pData, orig->pData, orig->dataSize);
    return KTX_SUCCESS;

cleanup:
    if (This->_protected) free(This->_protected);
    if (This->_private) {
        if (This->_private->_supercompressionGlobalData)
            free(This->_private->_supercompressionGlobalData);
        free(This->_private);
    }
    if (This->pDfd) free (This->pDfd);
    if (This->kvDataHead) ktxHashList_Destruct(&This->kvDataHead);

    return result;
}

/**
 * @memberof ktxTexture2 @private
 * @~English
 * @brief Construct a ktxTexture from a ktxStream reading from a KTX source.
 *
 * The KTX header, which must have been read prior to calling this, is passed
 * to the function.
 *
 * The stream object is copied into the constructed ktxTexture2.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture.
 *
 * If either KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT or
 * KTX_TEXTURE_CREATE_RAW_KVDATA_BIT is set then the ktxTexture's orientation
 * fields will be set to defaults even if the KTX source contains
 * KTXorientation metadata.
 *
 * @param[in] This pointer to a ktxTexture2-sized block of memory to
 *                 initialize.
 * @param[in] pStream pointer to the stream to read.
 * @param[in] pHeader pointer to a KTX header that has already been read from
 *            the stream.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_DATA_ERROR
 *                              Source data is inconsistent with the KTX
 *                              specification.
 * @exception KTX_FILE_READ_ERROR
 *                              An error occurred while reading the source.
 * @exception KTX_FILE_UNEXPECTED_EOF
 *                              Not enough data in the source.
 * @exception KTX_OUT_OF_MEMORY Not enough memory to load either the images or
 *                              the key-value data.
 * @exception KTX_UNKNOWN_FILE_FORMAT
 *                              The source is not in KTX format.
 * @exception KTX_UNSUPPORTED_TEXTURE_TYPE
 *                              The source describes a texture type not
 *                              supported by OpenGL or Vulkan, e.g, a 3D array.
 */
KTX_error_code
ktxTexture2_constructFromStreamAndHeader(ktxTexture2* This, ktxStream* pStream,
                                        KTX_header2* pHeader,
                                        ktxTextureCreateFlags createFlags)
{
    ktxTexture2_private* private;
    KTX_error_code result;
    KTX_supplemental_info suppInfo;
    ktxStream* stream;
    ktx_size_t levelIndexSize;

    assert(pHeader != NULL && pStream != NULL);

    memset(This, 0, sizeof(*This));
    result = ktxTexture_constructFromStream(ktxTexture(This), pStream,
                                            createFlags);
    if (result != KTX_SUCCESS)
        return result;

    result = ktxCheckHeader2_(pHeader, &suppInfo);
    if (result != KTX_SUCCESS)
        goto cleanup;
    // ktxCheckHeader2_ has done the max(1, levelCount) on pHeader->levelCount.
    result = ktxTexture2_constructCommon(This, pHeader->levelCount);
    if (result != KTX_SUCCESS)
        goto cleanup;
    private = This->_private;

    stream = ktxTexture2_getStream(This);

    /*
     * Initialize from pHeader->info.
     */
    This->vkFormat = pHeader->vkFormat;
    This->supercompressionScheme = pHeader->supercompressionScheme;
    if (This->vkFormat != VK_FORMAT_UNDEFINED) {
        vkGetFormatSize(This->vkFormat, &This->_protected->_formatSize);
        if (This->_protected->_formatSize.blockSizeInBits == 0) {
            return KTX_INVALID_VALUE; // TODO Return a more reasonable error?
        }
    }

    This->_protected->_typeSize = pHeader->typeSize;
    // Can these be done by a ktxTexture_constructFromStream?
    This->numDimensions = suppInfo.textureDimension;
    This->baseWidth = pHeader->pixelWidth;
    assert(suppInfo.textureDimension > 0 && suppInfo.textureDimension < 4);
    switch (suppInfo.textureDimension) {
      case 1:
        This->baseHeight = This->baseDepth = 1;
        break;
      case 2:
        This->baseHeight = pHeader->pixelHeight;
        This->baseDepth = 1;
        break;
      case 3:
        This->baseHeight = pHeader->pixelHeight;
        This->baseDepth = pHeader->pixelDepth;
        break;
    }
    if (pHeader->layerCount > 0) {
        This->numLayers = pHeader->layerCount;
        This->isArray = KTX_TRUE;
    } else {
        This->numLayers = 1;
        This->isArray = KTX_FALSE;
    }
    This->numFaces = pHeader->faceCount;
    if (pHeader->faceCount == 6)
        This->isCubemap = KTX_TRUE;
    else
        This->isCubemap = KTX_FALSE;
    // ktxCheckHeader2_ does the max(1, levelCount) and sets
    // suppInfo.generateMipmaps when it was originally 0.
    This->numLevels = pHeader->levelCount;
    This->isCompressed = (This->_protected->_formatSize.flags & KTX_FORMAT_SIZE_COMPRESSED_BIT);
    This->generateMipmaps = suppInfo.generateMipmaps;

    // Read level index
    levelIndexSize = sizeof(ktxLevelIndexEntry) * This->numLevels;
    result = stream->read(stream, &private->_levelIndex, levelIndexSize);
    if (result != KTX_SUCCESS)
        goto cleanup;
    // Rebase index to start of data and save file offset.
    private->_firstLevelFileOffset
                    = private->_levelIndex[This->numLevels-1].byteOffset;
    for (ktx_uint32_t level = 0; level < This->numLevels; level++) {
        private->_levelIndex[level].byteOffset
                                        -= private->_firstLevelFileOffset;
    }

    // Read DFD
    This->pDfd =
            (ktx_uint32_t*)malloc(pHeader->dataFormatDescriptor.byteLength);
    result = stream->read(stream, This->pDfd,
                          pHeader->dataFormatDescriptor.byteLength);
    if (result != KTX_SUCCESS)
        goto cleanup;

    // Make an empty hash list.
    ktxHashList_Construct(&This->kvDataHead);
    // Load KVData.
    if (pHeader->keyValueData.byteLength > 0) {
        if (!(createFlags & KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT)) {
            ktx_uint32_t kvdLen = pHeader->keyValueData.byteLength;
            ktx_uint8_t* pKvd;

            pKvd = malloc(kvdLen);
            if (pKvd == NULL) {
                result = KTX_OUT_OF_MEMORY;
                goto cleanup;
            }

            result = stream->read(stream, pKvd, kvdLen);
            if (result != KTX_SUCCESS)
                goto cleanup;

            if (IS_BIG_ENDIAN) {
                /* Swap the counts inside the key & value data. */
                ktx_uint8_t* src = pKvd;
                ktx_uint8_t* end = pKvd + kvdLen;
                while (src < end) {
                    ktx_uint32_t keyAndValueByteSize = *((ktx_uint32_t*)src);
                    _ktxSwapEndian32(&keyAndValueByteSize, 1);
                    src += _KTX_PAD4(keyAndValueByteSize);
                }
            }

            if (!(createFlags & KTX_TEXTURE_CREATE_RAW_KVDATA_BIT)) {
                char* orientationStr;
                ktx_uint32_t orientationLen;

                result = ktxHashList_Deserialize(&This->kvDataHead,
                                                 kvdLen, pKvd);
                if (result != KTX_SUCCESS) {
                    free(pKvd);
                    goto cleanup;
                }

                result = ktxHashList_FindValue(&This->kvDataHead,
                                               KTX_ORIENTATION_KEY,
                                               &orientationLen,
                                               (void**)&orientationStr);
                assert(result != KTX_INVALID_VALUE);
                if (result == KTX_SUCCESS) {
                    // Length includes the terminating NUL.
                    if (orientationLen != This->numDimensions + 1) {
                        // There needs to be an entry for each dimension of
                        // the texture.
                        result = KTX_FILE_DATA_ERROR;
                        goto cleanup;
                    } else {
                        switch (This->numDimensions) {
                          case 3:
                            This->orientation.z = orientationStr[2];
                          case 2:
                            This->orientation.y = orientationStr[1];
                          case 1:
                            This->orientation.x = orientationStr[0];
                        }
                    }
                } else {
                    result = KTX_SUCCESS; // Not finding orientation is okay.
                }
            } else {
                This->kvDataLen = kvdLen;
                This->kvData = pKvd;
            }
        } else {
            stream->skip(stream, pHeader->keyValueData.byteLength);
        }
    }

    if (pHeader->supercompressionGlobalData.byteLength > 0) {
        // There could be padding here so seek to the next item.
        (void)stream->setpos(stream,
                             pHeader->supercompressionGlobalData.byteOffset);

        // Read supercompressionGlobalData
        private->_supercompressionGlobalData =
          (ktx_uint8_t*)malloc(pHeader->supercompressionGlobalData.byteLength);
        if (!private->_supercompressionGlobalData) {
            result = KTX_OUT_OF_MEMORY;
            goto cleanup;
        }
        private->_sgdByteLength
                            = pHeader->supercompressionGlobalData.byteLength;
        result = stream->read(stream, private->_supercompressionGlobalData,
                              private->_sgdByteLength);

        if (result != KTX_SUCCESS)
            goto cleanup;
    }

    // Calculate size of the image data.
    This->dataSize = 0;
    for (ktx_uint32_t i = 0; i < This->numLevels; i++) {
        This->dataSize += _KTX_PAD8(private->_levelIndex[i].byteLength);
    }

    /*
     * Load the images, if requested.
     */
    if (createFlags & KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT) {
        result = ktxTexture2_LoadImageData(This, NULL, 0);
    }
    if (result != KTX_SUCCESS)
        goto cleanup;

    return result;

cleanup:
    ktxTexture2_destruct(This);
    return result;
}

/**
 * @memberof ktxTexture2 @private
 * @~English
 * @brief Construct a ktxTexture from a ktxStream reading from a KTX source.
 *
 * The stream object is copied into the constructed ktxTexture2.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture.
 *
 * @param[in] This pointer to a ktxTexture2-sized block of memory to
 *            initialize.
 * @param[in] pStream pointer to the stream to read.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 *
 * @return    KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_READ_ERROR
 *                              An error occurred while reading the source.
 *
 * For other exceptions see ktxTexture2_constructFromStreamAndHeader().
 */
static KTX_error_code
ktxTexture2_constructFromStream(ktxTexture2* This, ktxStream* pStream,
                                ktxTextureCreateFlags createFlags)
{
    KTX_header2 header;
    KTX_error_code result;

    // Read header.
    result = pStream->read(pStream, &header, KTX2_HEADER_SIZE);
    if (result != KTX_SUCCESS)
        return result;

#if IS_BIG_ENDIAN
    // byte swap the header
#endif
    return ktxTexture2_constructFromStreamAndHeader(This, pStream,
                                                    &header, createFlags);
}

/**
 * @memberof ktxTexture2 @private
 * @~English
 * @brief Construct a ktxTexture from a stdio stream reading from a KTX source.
 *
 * See ktxTextureInt_constructFromStream for details.
 *
 * @note Do not close the stdio stream until you are finished with the texture
 *       object.
 *
 * @param[in] This pointer to a ktxTextureInt-sized block of memory to
 *                 initialize.
 * @param[in] stdioStream a stdio FILE pointer opened on the source.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE Either @p stdiostream or @p This is null.
 *
 * For other exceptions, see ktxTexture_constructFromStream().
 */
static KTX_error_code
ktxTexture2_constructFromStdioStream(ktxTexture2* This, FILE* stdioStream,
                                     ktxTextureCreateFlags createFlags)
{
    KTX_error_code result;
    ktxStream stream;

    if (stdioStream == NULL || This == NULL)
        return KTX_INVALID_VALUE;

    result = ktxFileStream_construct(&stream, stdioStream, KTX_FALSE);
    if (result == KTX_SUCCESS)
        result = ktxTexture2_constructFromStream(This, &stream, createFlags);
    return result;
}

/**
 * @memberof ktxTexture2 @private
 * @~English
 * @brief Construct a ktxTexture from a named KTX file.
 *
 * See ktxTextureInt_constructFromStream for details.
 *
 * @param[in] This pointer to a ktxTextureInt-sized block of memory to
 *                 initialize.
 * @param[in] filename    pointer to a char array containing the file name.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_FILE_OPEN_FAILED The file could not be opened.
 * @exception KTX_INVALID_VALUE @p filename is @c NULL.
 *
 * For other exceptions, see ktxTexture_constructFromStream().
 */
static KTX_error_code
ktxTexture2_constructFromNamedFile(ktxTexture2* This,
                                   const char* const filename,
                                   ktxTextureCreateFlags createFlags)
{
    KTX_error_code result;
    ktxStream stream;
    FILE* file;

    if (This == NULL || filename == NULL)
        return KTX_INVALID_VALUE;

    file = fopen(filename, "rb");
    if (!file)
       return KTX_FILE_OPEN_FAILED;

    result = ktxFileStream_construct(&stream, file, KTX_TRUE);
    if (result == KTX_SUCCESS)
        result = ktxTexture2_constructFromStream(This, &stream, createFlags);

    return result;
}

/**
 * @memberof ktxTexture2 @private
 * @~English
 * @brief Construct a ktxTexture from KTX-formatted data in memory.
 *
 * See ktxTextureInt_constructFromStream for details.
 *
 * @param[in] This  pointer to a ktxTextureInt-sized block of memory to
 *                  initialize.
 * @param[in] bytes pointer to the memory containing the serialized KTX data.
 * @param[in] size  length of the KTX data in bytes.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE Either @p bytes is NULL or @p size is 0.
 *
 * For other exceptions, see ktxTexture_constructFromStream().
 */
static KTX_error_code
ktxTexture2_constructFromMemory(ktxTexture2* This,
                                  const ktx_uint8_t* bytes, ktx_size_t size,
                                  ktxTextureCreateFlags createFlags)
{
    KTX_error_code result;
    ktxStream stream;

    if (bytes == NULL || size == 0)
        return KTX_INVALID_VALUE;

    result = ktxMemStream_construct_ro(&stream, bytes, size);
    if (result == KTX_SUCCESS)
        result = ktxTexture2_constructFromStream(This, &stream, createFlags);

    return result;
}

/**
 * @memberof ktxTexture2 @private
 * @~English
 * @brief Destruct a ktxTexture2, freeing and internal memory.
 *
 * @param[in] This pointer to a ktxTexture2-sized block of memory to
 *                 initialize.
 */
void
ktxTexture2_destruct(ktxTexture2* This)
{
    if (This->pDfd) free(This->pDfd);
    if (This->_private) {
      ktx_uint8_t* sgd = This->_private->_supercompressionGlobalData;
      if (sgd) free(sgd);
      free(This->_private);
    }
    ktxTexture_destruct(ktxTexture(This));
}

/**
 * @memberof ktxTexture2
 * @ingroup writer
 * @~English
 * @brief Create a new empty ktxTexture2.
 *
 * The address of the newly created ktxTexture2 is written to the location
 * pointed at by @p newTex.
 *
 * @param[in] createInfo pointer to a ktxTextureCreateInfo struct with
 *                       information describing the texture.
 * @param[in] storageAllocation
 *                       enum indicating whether or not to allocate storage
 *                       for the texture images.
 * @param[in,out] newTex pointer to a location in which store the address of
 *                       the newly created texture.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @c glInternalFormat in @p createInfo is not a
 *                              valid OpenGL internal format value.
 * @exception KTX_INVALID_VALUE @c numDimensions in @p createInfo is not 1, 2
 *                              or 3.
 * @exception KTX_INVALID_VALUE One of <tt>base{Width,Height,Depth}</tt> in
 *                              @p createInfo is 0.
 * @exception KTX_INVALID_VALUE @c numFaces in @p createInfo is not 1 or 6.
 * @exception KTX_INVALID_VALUE @c numLevels in @p createInfo is 0.
 * @exception KTX_INVALID_OPERATION
 *                              The <tt>base{Width,Height,Depth}</tt> specified
 *                              in @p createInfo are inconsistent with
 *                              @c numDimensions.
 * @exception KTX_INVALID_OPERATION
 *                              @p createInfo is requesting a 3D array or
 *                              3D cubemap texture.
 * @exception KTX_INVALID_OPERATION
 *                              @p createInfo is requesting a cubemap with
 *                              non-square or non-2D images.
 * @exception KTX_INVALID_OPERATION
 *                              @p createInfo is requesting more mip levels
 *                              than needed for the specified
 *                              <tt>base{Width,Height,Depth}</tt>.
 * @exception KTX_OUT_OF_MEMORY Not enough memory for the texture's images.
 */
KTX_error_code
ktxTexture2_Create(ktxTextureCreateInfo* createInfo,
                  ktxTextureCreateStorageEnum storageAllocation,
                  ktxTexture2** newTex)
{
    KTX_error_code result;

    if (newTex == NULL)
        return KTX_INVALID_VALUE;

    ktxTexture2* tex = (ktxTexture2*)malloc(sizeof(ktxTexture2));
    if (tex == NULL)
        return KTX_OUT_OF_MEMORY;

    result = ktxTexture2_construct(tex, createInfo, storageAllocation);
    if (result != KTX_SUCCESS) {
        free(tex);
    } else {
        *newTex = tex;
    }
    return result;
}

/**
 * @memberof ktxTexture2
 * @ingroup writer
 * @~English
 * @brief Create a ktxTexture2 by making a copy of a ktxTexture2.
 *
 * The address of the newly created ktxTexture2 is written to the location
 * pointed at by @p newTex.
 *
 * @param[in]     orig   pointer to the texture to copy.
 * @param[in,out] newTex pointer to a location in which store the address of
 *                       the newly created texture.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_OUT_OF_MEMORY Not enough memory for the texture data.
 */
 KTX_error_code
 ktxTexture2_CreateCopy(ktxTexture2* orig, ktxTexture2** newTex)
 {
    KTX_error_code result;

    if (newTex == NULL)
        return KTX_INVALID_VALUE;

    ktxTexture2* tex = (ktxTexture2*)malloc(sizeof(ktxTexture2));
    if (tex == NULL)
        return KTX_OUT_OF_MEMORY;

    result = ktxTexture2_constructCopy(tex, orig);
    if (result != KTX_SUCCESS) {
        free(tex);
    } else {
        *newTex = tex;
    }
    return result;

 }

/**
 * @defgroup reader Reader
 * @brief Read KTX-formatted data.
 * @{
 */

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Create a ktxTexture2 from a stdio stream reading from a KTX source.
 *
 * The address of a newly created ktxTexture2 reflecting the contents of the
 * stdio stream is written to the location pointed at by @p newTex.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture.
 *
 * @param[in] stdioStream stdio FILE pointer created from the desired file.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 * @param[in,out] newTex  pointer to a location in which store the address of
 *                        the newly created texture.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p newTex is @c NULL.
 * @exception KTX_FILE_DATA_ERROR
 *                              Source data is inconsistent with the KTX
 *                              specification.
 * @exception KTX_FILE_READ_ERROR
 *                              An error occurred while reading the source.
 * @exception KTX_FILE_UNEXPECTED_EOF
 *                              Not enough data in the source.
 * @exception KTX_OUT_OF_MEMORY Not enough memory to create the texture object,
 *                              load the images or load the key-value data.
 * @exception KTX_UNKNOWN_FILE_FORMAT
 *                              The source is not in KTX format.
 * @exception KTX_UNSUPPORTED_TEXTURE_TYPE
 *                              The source describes a texture type not
 *                              supported by OpenGL or Vulkan, e.g, a 3D array.
 */
KTX_error_code
ktxTexture2_CreateFromStdioStream(FILE* stdioStream,
                                  ktxTextureCreateFlags createFlags,
                                  ktxTexture2** newTex)
{
    KTX_error_code result;
    if (newTex == NULL)
        return KTX_INVALID_VALUE;

    ktxTexture2* tex = (ktxTexture2*)malloc(sizeof(ktxTexture2));
    if (tex == NULL)
        return KTX_OUT_OF_MEMORY;

    result = ktxTexture2_constructFromStdioStream(tex, stdioStream,
                                                  createFlags);
    if (result == KTX_SUCCESS)
        *newTex = (ktxTexture2*)tex;
    else {
        free(tex);
        *newTex = NULL;
    }
    return result;
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Create a ktxTexture2 from a named KTX file.
 *
 * The address of a newly created ktxTexture2 reflecting the contents of the
 * file is written to the location pointed at by @p newTex.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture.
 *
 * @param[in] filename    pointer to a char array containing the file name.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 * @param[in,out] newTex  pointer to a location in which store the address of
 *                        the newly created texture.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.

 * @exception KTX_FILE_OPEN_FAILED The file could not be opened.
 * @exception KTX_INVALID_VALUE @p filename is @c NULL.
 *
 * For other exceptions, see ktxTexture_CreateFromStdioStream().
 */
KTX_error_code
ktxTexture2_CreateFromNamedFile(const char* const filename,
                                ktxTextureCreateFlags createFlags,
                                ktxTexture2** newTex)
{
    KTX_error_code result;

    if (newTex == NULL)
        return KTX_INVALID_VALUE;

    ktxTexture2* tex = (ktxTexture2*)malloc(sizeof(ktxTexture2));
    if (tex == NULL)
        return KTX_OUT_OF_MEMORY;

    result = ktxTexture2_constructFromNamedFile(tex, filename, createFlags);
    if (result == KTX_SUCCESS)
        *newTex = (ktxTexture2*)tex;
    else {
        free(tex);
        *newTex = NULL;
    }
    return result;
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Create a ktxTexture2 from KTX-formatted data in memory.
 *
 * The address of a newly created ktxTexture2 reflecting the contents of the
 * serialized KTX data is written to the location pointed at by @p newTex.
 *
 * The create flag KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT should not be set,
 * if the ktxTexture is ultimately to be uploaded to OpenGL or Vulkan. This
 * will minimize memory usage by allowing, for example, loading the images
 * directly from the source into a Vulkan staging buffer.
 *
 * The create flag KTX_TEXTURE_CREATE_RAW_KVDATA_BIT should not be used. It is
 * provided solely to enable implementation of the @e libktx v1 API on top of
 * ktxTexture.
 *
 * @param[in] bytes pointer to the memory containing the serialized KTX data.
 * @param[in] size  length of the KTX data in bytes.
 * @param[in] createFlags bitmask requesting specific actions during creation.
 * @param[in,out] newTex  pointer to a location in which store the address of
 *                        the newly created texture.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE Either @p bytes is NULL or @p size is 0.
 *
 * For other exceptions, see ktxTexture_CreateFromStdioStream().
 */
KTX_error_code
ktxTexture2_CreateFromMemory(const ktx_uint8_t* bytes, ktx_size_t size,
                             ktxTextureCreateFlags createFlags,
                             ktxTexture2** newTex)
{
    KTX_error_code result;
    if (newTex == NULL)
        return KTX_INVALID_VALUE;

    ktxTexture2* tex = (ktxTexture2*)malloc(sizeof(ktxTexture2));
    if (tex == NULL)
        return KTX_OUT_OF_MEMORY;

    result = ktxTexture2_constructFromMemory(tex, bytes, size,
                                             createFlags);
    if (result == KTX_SUCCESS)
        *newTex = (ktxTexture2*)tex;
    else {
        free(tex);
        *newTex = NULL;
    }
    return result;
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Destroy a ktxTexture2 object.
 *
 * This frees the memory associated with the texture contents and the memory
 * of the ktxTexture2 object. This does @e not delete any OpenGL or Vulkan
 * texture objects created by ktxTexture2_GLUpload or ktxTexture2_VkUpload.
 *
 * @param[in] This pointer to the ktxTexture2 object to destroy
 */
void
ktxTexture2_Destroy(ktxTexture2* This)
{
    ktxTexture2_destruct(This);
    free(This);
}

/**
 * @memberof ktxTexture2 @private
 * @~English
 *
 * @copydoc ktxTexture::ktxTexture_doCalcFaceLodSize
 */
ktx_size_t
ktxTexture2_calcFaceLodSize(ktxTexture2* This, ktx_uint32_t level)
{
    /*
     * For non-array cubemaps this is the size of a face. For everything
     * else it is the size of the level.
     */
    if (This->isCubemap && !This->isArray)
        return ktxTexture_calcImageSize(ktxTexture(This), level,
                                        KTX_FORMAT_VERSION_TWO);
    else
        return This->_private->_levelIndex[level].uncompressedByteLength;
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Return information about the components of an image format.
 *
 * @param[in]     This           pointer to the ktxTexture object of interest.
 * @param[in,out] pNumComponents pointer to location in which to write the
 *                               number of ocmponents in the textures images.
 * @param[in,out] pComponentByteLength
 *                               pointer to the location in which to write
 *                               byte length of a component.
 */
 void
 ktxTexture2_GetComponentInfo(ktxTexture2* This, uint32_t* pNumComponents,
                              uint32_t* pComponentByteLength)
{
    // FIXME Need to handle packed case.
    getDFDComponentInfoUnpacked(This->pDfd, pNumComponents,
                                pComponentByteLength);
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Find the offset of an image within a ktxTexture's image data.
 *
 * As there is no such thing as a 3D cubemap we make the 3rd location parameter
 * do double duty.
 *
 * @param[in]     This      pointer to the ktxTexture object of interest.
 * @param[in]     level     mip level of the image.
 * @param[in]     layer     array layer of the image.
 * @param[in]     faceSlice cube map face or depth slice of the image.
 * @param[in,out] pOffset   pointer to location to store the offset.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_OPERATION
 *                         @p level, @p layer or @p faceSlice exceed the
 *                         dimensions of the texture.
 * @exception KTX_INVALID_VALID @p This is NULL.
 */
KTX_error_code
ktxTexture2_GetImageOffset(ktxTexture2* This, ktx_uint32_t level,
                          ktx_uint32_t layer, ktx_uint32_t faceSlice,
                          ktx_size_t* pOffset)
{
    if (This == NULL)
        return KTX_INVALID_VALUE;

    if (level >= This->numLevels || layer >= This->numLayers)
        return KTX_INVALID_OPERATION;

    if (This->isCubemap) {
        if (faceSlice >= This->numFaces)
            return KTX_INVALID_OPERATION;
    } else {
        ktx_uint32_t maxSlice = MAX(1, This->baseDepth >> level);
        if (faceSlice >= maxSlice)
            return KTX_INVALID_OPERATION;
    }

    // Get the offset of the start of the level.
    *pOffset = ktxTexture2_levelDataOffset(This, level);

    if (This->supercompressionScheme == KTX_SUPERCOMPRESSION_NONE) {
        // All layers, faces & slices within a level are the same size.
        if (layer != 0) {
            ktx_size_t layerSize;
            layerSize = ktxTexture_layerSize(ktxTexture(This), level,
                                             KTX_FORMAT_VERSION_TWO);
            *pOffset += layer * layerSize;
        }
        if (faceSlice != 0) {
            ktx_size_t imageSize;
            imageSize = ktxTexture2_GetImageSize(This, level);
            *pOffset += faceSlice * imageSize;
        }
    } else {
        // TODO
    }
    return KTX_SUCCESS;
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Retrieve the opto-electrical transfer function of the images.
 *
 * @param[in]     This      pointer to the ktxTexture object of interest.
 *
 * @return A KHR_DF enum value specifying the OETF.
 */
ktx_uint32_t
ktxTexture2_GetOETF(ktxTexture2* This)
{
    return KHR_DFDVAL(This->pDfd+1, TRANSFER);
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Calculate & return the size in bytes of an image at the specified
 *        mip level.
 *
 * For arrays, this is the size of a layer, for cubemaps, the size of a face
 * and for 3D textures, the size of a depth slice.
 *
 * The size reflects the padding of each row to KTX_GL_UNPACK_ALIGNMENT.
 *
 * @param[in]     This     pointer to the ktxTexture object of interest.
 * @param[in]     level    level of interest. *
 */
ktx_size_t
ktxTexture2_GetImageSize(ktxTexture2* This, ktx_uint32_t level)
{
    return ktxTexture_calcImageSize(ktxTexture(This), level,
                                    KTX_FORMAT_VERSION_TWO);
}

/**
 * @memberof ktxTexture
 * @~English
 * @brief Iterate over the mip levels in a ktxTexture2 object.
 *
 * This is almost identical to ktxTexture_IterateLevelFaces(). The difference is
 * that the blocks of image data for non-array cube maps include all faces of
 * a mip level.
 *
 * This function works even if @p This->pData == 0 so it can be used to
 * obtain offsets and sizes for each level by callers who have loaded the data
 * externally.
 *
 * @param[in]     This     handle of the ktxTexture opened on the data.
 * @param[in,out] iterCb   the address of a callback function which is called
 *                         with the data for each image block.
 * @param[in,out] userdata the address of application-specific data which is
 *                         passed to the callback along with the image data.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error. The
 *          following are returned directly by this function. @p iterCb may
 *          return these for other causes or may return additional errors.
 *
 * @exception KTX_FILE_DATA_ERROR   Mip level sizes are increasing not
 *                                  decreasing
 * @exception KTX_INVALID_VALUE     @p This is @c NULL or @p iterCb is @c NULL.
 *
 */
KTX_error_code
ktxTexture2_IterateLevels(ktxTexture2* This, PFNKTXITERCB iterCb, void* userdata)
{
    KTX_error_code  result = KTX_SUCCESS;

    if (This == NULL)
        return KTX_INVALID_VALUE;

    if (iterCb == NULL)
        return KTX_INVALID_VALUE;

    for (ktx_int32_t level = This->numLevels - 1; level >= 0; level--)
    {
        ktx_uint32_t width, height, depth;
        ktx_uint64_t levelSize;
        ktx_uint64_t offset;

        /* Array textures have the same number of layers at each mip level. */
        width = MAX(1, This->baseWidth  >> level);
        height = MAX(1, This->baseHeight >> level);
        depth = MAX(1, This->baseDepth  >> level);

        levelSize = This->_private->_levelIndex[level].uncompressedByteLength;

        /* All array layers are passed in a group because that is how
         * GL & Vulkan need them. Hence no
         *    for (layer = 0; layer < This->numLayers)
         */
        offset = ktxTexture2_levelDataOffset(This, level);
        result = iterCb(level, 0, width, height, depth,
                         levelSize, This->pData + offset, userdata);
        if (result != KTX_SUCCESS)
            break;
    }

    return result;
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Iterate over the images in a ktxTexture2 object while loading the
 *        image data.
 *
 * This operates similarly to ktxTexture_IterateLevelFaces() except that it
 * loads the images from the ktxTexture2's source to a temporary buffer
 * while iterating. The callback function must copy the image data if it
 * wishes to preserve it as the temporary buffer is reused for each level and
 * is freed when this function exits.
 *
 * This function is helpful for reducing memory usage when uploading the data
 * to a graphics API.
 *
 * @param[in]     This     pointer to the ktxTexture2 object of interest.
 * @param[in,out] iterCb   the address of a callback function which is called
 *                         with the data for each image.
 * @param[in,out] userdata the address of application-specific data which is
 *                         passed to the callback along with the image data.
 *
 * @return  KTX_SUCCESS on success, other KTX_* enum values on error. The
 *          following are returned directly by this function. @p iterCb may
 *          return these for other causes or may return additional errors.
 *
 * @exception KTX_FILE_DATA_ERROR   mip level sizes are increasing not
 *                                  decreasing
 * @exception KTX_INVALID_OPERATION the ktxTexture2 was not created from a
 *                                  stream, i.e there is no data to load, or
 *                                  this ktxTexture2's images have already
 *                                  been loaded.
 * @exception KTX_INVALID_VALUE     @p This is @c NULL or @p iterCb is @c NULL.
 * @exception KTX_OUT_OF_MEMORY     not enough memory to allocate a block to
 *                                  hold the base level image.
 */
KTX_error_code
ktxTexture2_IterateLoadLevelFaces(ktxTexture2* This, PFNKTXITERCB iterCb,
                                  void* userdata)
{
    struct ktxTexture_protected* prtctd = This->_protected;
    ktxStream* stream = (ktxStream *)&prtctd->_stream;
    ktxLevelIndexEntry* levelIndex;
    ktx_size_t    dataSize = 0;
    KTX_error_code  result = KTX_SUCCESS;
    void*           data = NULL;

    if (This == NULL)
        return KTX_INVALID_VALUE;

    if (This->classId != ktxTexture2_c)
        return KTX_INVALID_OPERATION;

    if (iterCb == NULL)
        return KTX_INVALID_VALUE;

    if (prtctd->_stream.data.file == NULL)
        // This Texture not created from a stream or images are already loaded.
        return KTX_INVALID_OPERATION;

    levelIndex = This->_private->_levelIndex;

    // Allocate memory sufficient for the base level
    dataSize = levelIndex[0].byteLength;
    data = malloc(dataSize);
    if (!data)
        return KTX_OUT_OF_MEMORY;

    for (ktx_int32_t level = This->numLevels - 1; level >= 0; --level)
    {
        ktx_size_t   levelSize;
        GLsizei      width, height, depth;

        // Array textures have the same number of layers at each mip level.
        width = MAX(1, This->baseWidth  >> level);
        height = MAX(1, This->baseHeight >> level);
        depth = MAX(1, This->baseDepth  >> level);

        levelSize = levelIndex[level].byteLength;
        if (dataSize < levelSize) {
            // Levels cannot be larger than the base level
            result = KTX_FILE_DATA_ERROR;
            goto cleanup;
        }

        // Use setpos so we skip any padding.
        result = stream->setpos(stream,
                                ktxTexture2_levelFileOffset(This, level));
        if (result != KTX_SUCCESS)
            goto cleanup;

        result = stream->read(stream, data, levelSize);
        if (result != KTX_SUCCESS)
            goto cleanup;

#if IS_BIG_ENDIAN
        /* FIXME Perform endianness conversion on texture data */
#endif

        // With the exception of non-array cubemaps the entire level
        // is passed at once because that is how OpenGL and Vulkan need them.
        // Vulkan could take all the faces at once too but we iterate
        // them separately or OpenGL.
        if (This->isCubemap && !This->isArray) {
            ktx_uint8_t* pFace = data;
            ktx_size_t faceSize = ktxTexture_calcImageSize(ktxTexture(This),
                                                        level,
                                                        KTX_FORMAT_VERSION_TWO);
            for (ktx_uint32_t face = 0; face < This->numFaces; ++face) {
                result = iterCb(level, face,
                                width, height, depth,
                                (ktx_uint32_t)faceSize, pFace, userdata);
                pFace += faceSize;
                if (result != KTX_SUCCESS)
                    goto cleanup;
            }
        } else {
            result = iterCb(level, 0,
                             width, height, depth,
                             (ktx_uint32_t)levelSize, data, userdata);
            if (result != KTX_SUCCESS)
                goto cleanup;
       }
    }

    // No further need for this.
    stream->destruct(stream);
    This->_private->_firstLevelFileOffset = 0;
cleanup:
    free(data);

    return result;
}

/**
 * @memberof ktxTexture2
 * @~English
 * @brief Load all the image data from the ktxTexture2's source.
 *
 * The data is loaded into the provided buffer or to an internally allocated
 * buffer, if @p pBuffer is @c NULL.
 *
 * @param[in] This pointer to the ktxTexture object of interest.
 * @param[in] pBuffer pointer to the buffer in which to load the image data.
 * @param[in] bufSize size of the buffer pointed at by @p pBuffer.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p This is NULL.
 * @exception KTX_INVALID_VALUE @p bufSize is less than the the image data size.
 * @exception KTX_INVALID_OPERATION
 *                              The data has already been loaded or the
 *                              ktxTexture was not created from a KTX source.
 * @exception KTX_OUT_OF_MEMORY Insufficient memory for the image data.
 */
KTX_error_code
ktxTexture2_LoadImageData(ktxTexture2* This,
                          ktx_uint8_t* pBuffer, ktx_size_t bufSize)
{
    DECLARE_PROTECTED(ktxTexture);
    DECLARE_PRIVATE(ktxTexture2);
    ktx_uint8_t*    pDest;
    KTX_error_code  result = KTX_SUCCESS;

    if (This == NULL)
        return KTX_INVALID_VALUE;

    if (prtctd->_stream.data.file == NULL)
        // This Texture not created from a stream or images already loaded;
        return KTX_INVALID_OPERATION;

    if (pBuffer == NULL) {
        This->pData = malloc(This->dataSize);
        if (This->pData == NULL)
            return KTX_OUT_OF_MEMORY;
        pDest = This->pData;
    } else if (bufSize < This->dataSize) {
        return KTX_INVALID_VALUE;
    } else {
        pDest = pBuffer;
    }

    // Seek to data for first level as there may be padding between the
    // metadata/sgd and the image data.

    result = prtctd->_stream.setpos(&prtctd->_stream,
                                    private->_firstLevelFileOffset);
    if (result != KTX_SUCCESS)
        return result;

    result = prtctd->_stream.read(&prtctd->_stream, pDest,
                                  This->dataSize);
    if (result != KTX_SUCCESS)
        return result;

    if (IS_BIG_ENDIAN) {
        // Perform endianness conversion on texture data.
        // To avoid mip padding, need to convert each level individually.
        for (ktx_uint32_t level = 0; level < This->numLevels; ++level)
        {
            ktx_size_t levelOffset;
            ktx_size_t levelByteLength;

            levelByteLength = private->_levelIndex[level].byteLength;
            levelOffset = ktxTexture2_levelDataOffset(This, level);
            pDest = This->pData + levelOffset;
            switch (prtctd->_typeSize) {
              case 2:
                _ktxSwapEndian16((ktx_uint16_t*)pDest, levelByteLength / 2);
                break;
              case 4:
                _ktxSwapEndian32((ktx_uint32_t*)pDest, levelByteLength / 4);
                break;
              case 8:
                _ktxSwapEndian64((ktx_uint64_t*)pDest, levelByteLength / 8);
                break;
            }
        }
    }

    // No further need for stream or file offset.
    prtctd->_stream.destruct(&prtctd->_stream);
    private->_firstLevelFileOffset = 0;
    return result;
}

/**
 * @memberof ktxTexture2 @private
 * @~English
 * @brief Retrieve the offset of a level's first image within the KTX2 file.
 *
 * @param[in] This pointer to the ktxTexture object of interest.
 */
ktx_uint64_t ktxTexture2_levelFileOffset(ktxTexture2* This, ktx_uint32_t level)
{
    assert(This->_private->_firstLevelFileOffset != 0);
    return This->_private->_levelIndex[level].byteOffset
           + This->_private->_firstLevelFileOffset;
}

/**
 * @memberof ktxTexture2 @private
 * @~English
 * @brief Retrieve the offset of a level's first image within the ktxTexture2's
 *        image data.
 *
 * @param[in] This pointer to the ktxTexture object of interest.
 */
ktx_uint64_t ktxTexture2_levelDataOffset(ktxTexture2* This, ktx_uint32_t level)
{
    return This->_private->_levelIndex[level].byteOffset;
}

/*
 * Initialized here at the end to avoid the need for multiple declarations of
 * the virtual functions.
 */

struct ktxTexture_vtblInt ktxTexture2_vtblInt = {
    (PFNCALCFACELODSIZE)ktxTexture2_calcFaceLodSize
};

#ifdef KTX_FEATURE_UPLOAD

struct ktxTexture_vtbl ktxTexture2_vtbl = {
    (PFNKTEXDESTROY)ktxTexture2_Destroy,
    (PFNKTEXGETIMAGEOFFSET)ktxTexture2_GetImageOffset,
    (PFNKTEXGETIMAGESIZE)ktxTexture2_GetImageSize,
    (PFNKTEXGLUPLOAD)ktxTexture2_GLUpload,
    (PFNKTEXITERATELEVELS)ktxTexture2_IterateLevels,
    (PFNKTEXITERATELOADLEVELFACES)ktxTexture2_IterateLoadLevelFaces,
    (PFNKTEXLOADIMAGEDATA)ktxTexture2_LoadImageData,
    (PFNKTEXSETIMAGEFROMMEMORY)ktxTexture2_SetImageFromMemory,
    (PFNKTEXSETIMAGEFROMSTDIOSTREAM)ktxTexture2_SetImageFromStdioStream,
    (PFNKTEXWRITETOSTDIOSTREAM)ktxTexture2_WriteToStdioStream,
    (PFNKTEXWRITETONAMEDFILE)ktxTexture2_WriteToNamedFile,
    (PFNKTEXWRITETOMEMORY)ktxTexture2_WriteToMemory,
};

#endif

/** @} */

